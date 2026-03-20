// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/bytecode_analysis.h"

#include <algorithm>
#include <utility>

#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/op_code.h"

namespace lynx {
namespace lepus {
namespace ir {

static bool UnionIsChanged(llvh::BitVector& lhs, const llvh::BitVector& rhs) {
  if (rhs.test(lhs)) {
    lhs |= rhs;
    return true;
  }
  return false;
}

class BytecodeAnalysisInternal {
 public:
  BytecodeIterator bytecode_iterator_;
  BytecodeAnalysis* analysis_;
  llvh::DenseMap<int, LoopInfo> header_to_info_;
  llvh::SmallVector<std::pair<int, LoopInfo*>, 8> loop_stack_;
  llvh::DenseMap<int, int> end_to_header_;
  llvh::DenseMap<int, std::shared_ptr<BlockItem>> bb_items_;
  llvh::SmallVector<int, 64> work_list_;
  llvh::SmallVector<int, 64> post_order_;

  int register_count_ = 0;

  BytecodeIterator& iterator() { return bytecode_iterator_; }

  BytecodeAnalysisInternal(BytecodeAnalysis* analysis)
      : analysis_(analysis), register_count_(analysis->GetRegisterCount()) {}

  std::shared_ptr<BlockItem> GetOrCreateBlock(int offset, bool end = false) {
    auto it = bb_items_.find(offset);
    if (it == bb_items_.end()) {
      auto item = std::make_shared<BlockItem>(offset);

      item->live_in_ = std::make_shared<llvh::BitVector>(register_count_);
      item->live_out_ = std::make_shared<llvh::BitVector>(register_count_);
      bb_items_.insert({offset, item});
      return item;
    } else {
      return it->second;
    }
  }

  void insertEdge(int offset, int to, bool split_bb,
                  bool fall_through = false) {
    auto item = GetOrCreateBlock(offset);
    auto to_item = GetOrCreateBlock(to);
    item->AddSuccessor(to_item);
    to_item->AddPredecessor(item);
    to_item->new_bb_ = split_bb;
    if (fall_through) {
      to_item->fall_through_pred_ = offset;
    }
  }

  void InsertFallThrough(int offset, int to, bool split_bb) {
    insertEdge(offset, to, split_bb, true);
  }

  void VisitBytecode(lynx::lepus::Instruction* ptr,
                     lynx::lepus::Instruction* end,
                     llvh::ArrayRef<int64_t> line_cols) {
    // Pre-scan to collect all catch label offsets for quick "next catch"
    // lookup.
    llvh::SmallVector<int, 8> catch_offsets;
    {
      BytecodeIterator scan;
      scan.Reset(ptr, end, line_cols);
      while (!scan.Done()) {
        if (scan.GetOpcode() == LepusOpcode::OP_TypeLabel_Catch) {
          catch_offsets.push_back(scan.GetCurrentOffset());
        }
        scan.Next();
      }
    }

    iterator().Reset(ptr, end, line_cols);
    while (!iterator().Done()) {
      auto opcode = iterator().GetOpcode();
      auto offset = iterator().GetCurrentOffset();
      auto bb = GetOrCreateBlock(offset);
      bb->SetOpcode(opcode);

      if (Bytecode::IsJump(opcode)) {
        auto target_offset =
            iterator().GetCurrentOffset() + iterator().JumpOffset();
        insertEdge(offset, target_offset, true);
        if (target_offset <= offset) {
          end_to_header_[offset] = target_offset;
        }
        iterator().Next();
        auto next_offset = iterator().GetCurrentOffset();
        if (!Bytecode::IsJumpImm(opcode)) {
          InsertFallThrough(offset, next_offset, true);
        }
      } else if (Bytecode::IsSwitch(opcode)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: Switch instructions are not supported in bytecode "
            "analysis");
      } else {
        if (opcode == LepusOpcode::OP_TypeLabel_Throw) {
          // Model an exceptional edge to the nearest catch label after this
          // throw. The VM handles exceptions by scanning forward for
          // TypeLabel_Catch.
          auto it = std::upper_bound(catch_offsets.begin(), catch_offsets.end(),
                                     offset);
          if (it != catch_offsets.end()) {
            insertEdge(offset, *it, true);
          }
        }
        iterator().Next();
        auto next_offset = iterator().GetCurrentOffset();
        // fall through
        if (!Bytecode::IsTerminate(opcode)) {
          InsertFallThrough(offset, next_offset, false);
        }
      }
    }
    GetOrCreateBlock(end - ptr, true);
  }

  void UpdateInLivenessBC(std::shared_ptr<BlockItem>& bb) {
    iterator().SkipTo(bb->GetStartOffset());
    auto num_operands = iterator().NumOperands();
    auto opcode = iterator().GetOpcode();
    auto live_in = bb->GetLiveIn();

    for (int i = 0; i < num_operands; i++) {
      auto type = iterator().GetOperandType(i);
      switch (type) {
        case LepusOperandType::DstReg: {
          auto dst = iterator().GetOperandReg(i);
          live_in->reset(dst);
          break;
        }
        case LepusOperandType::SrcReg:
        case LepusOperandType::SrcDstReg: {
          auto val = iterator().GetOperandReg(i);
          live_in->set(val);
          break;
        }
        default:
          break;
      }
    }

    if (Bytecode::IsCallRange(opcode)) {
      auto func = iterator().GetOperand0();
      auto param_size = iterator().GetOperand1();
      for (int i = 0; i < param_size; i++) {
        live_in->set(func + i + 1);
      }
      return;
    } else if (Bytecode::IsNewArray(opcode)) {
      auto dst = iterator().GetOperand0();
      auto array_size = iterator().GetOperand1();
      for (int i = 0; i < array_size; i++) {
        live_in->set(dst + i + 1);
      }
      return;
    }
  }

  bool UpdateLiveness(std::shared_ptr<BlockItem>& bb) {
    bool changed = false;
    auto live_out = bb->GetLiveOut();
    auto live_in = bb->GetLiveIn();

    for (auto succ : bb->Successors()) {
      changed |= UnionIsChanged(*live_out, *succ->GetLiveIn());
    }
    *live_in = *live_out;
    UpdateInLivenessBC(bb);
    return changed;
  }

  bool UpdateLiveness() {
    bool changed = false;
    for (auto it : post_order_) {
      auto bb = bb_items_[it];
      changed |= UpdateLiveness(bb);
    }
    return changed;
  }

  void ComputeLiveOut() {
    if (!header_to_info_.empty()) {
      InitVisitState();
      llvh::SmallVector<int, 64> work_list;
      for (auto it : post_order_) {
        work_list.push_back(it);
        bb_items_[it]->state_ = BBVisitState::kOnStack;
      }

      while (!work_list.empty()) {
        int offset = work_list.back();
        work_list.pop_back();
        auto bb = bb_items_[offset];
        bb->state_ = BBVisitState::kVisited;

        if (UpdateLiveness(bb)) {
          for (auto pred : bb->Predecessors()) {
            int pred_offset = pred->GetStartOffset();
            if (bb_items_[pred_offset]->state_ != BBVisitState::kOnStack) {
              work_list.push_back(pred_offset);
              bb_items_[pred_offset]->state_ = BBVisitState::kOnStack;
            }
          }
        }
      }
    }
  }

  void InitVisitState() {
    // init visit state
    work_list_.clear();
    for (auto& it : bb_items_) {
      it.second->state_ = BBVisitState::kUnvisited;
    }
  }

  void VisitPostOrder() {
    InitVisitState();
    work_list_.emplace_back(0);

    while (!work_list_.empty()) {
      auto current_offset = work_list_.back();
      auto bb = bb_items_[current_offset];
      bb->state_ = BBVisitState::kPending;
      bool visit_all = true;

      for (auto succ : bb->Successors()) {
        auto succ_offset = succ->GetStartOffset();
        if (succ->state_ == BBVisitState::kUnvisited) {
          work_list_.emplace_back(succ_offset);
          succ->state_ = BBVisitState::kOnStack;
          visit_all = false;
          continue;
        }
      }
      if (visit_all) {
        bb->state_ = BBVisitState::kVisited;
        post_order_.emplace_back(current_offset);
        work_list_.pop_back();
      }
    }
  }

  void UpdateAssignments(std::shared_ptr<BlockItem> bb,
                         std::shared_ptr<llvh::BitVector> assignments) {
    iterator().SkipTo(bb->GetStartOffset());
    auto num_operands = iterator().NumOperands();
    if (num_operands == 0) return;
    for (int i = 0; i < num_operands; i++) {
      auto type = iterator().GetOperandType(i);
      switch (type) {
        case LepusOperandType::DstReg:
        case LepusOperandType::SrcDstReg: {
          auto dst = iterator().GetOperandReg(i);
          assignments->set(dst);
          break;
        }
        default:
          break;
      }
    }
  }

  void PushLoop(int loop_end, int loop_header) {
    end_to_header_.insert({loop_end, loop_header});
    if (header_to_info_.find(loop_header) != header_to_info_.end()) {
      return;
    }
    int parent_offset = loop_stack_.back().first;
    auto it = header_to_info_.insert(
        {loop_header,
         LoopInfo(parent_offset, loop_header, loop_end, register_count_)});
    LoopInfo* loop_info = &it.first->second;
    loop_stack_.push_back(std::make_pair(loop_header, loop_info));
  }

  void VisitLoopBody() {
    loop_stack_.emplace_back(std::make_pair(0, nullptr));

    for (auto it : post_order_) {
      auto bb = bb_items_[it];
      auto current_offset = bb->GetStartOffset();

      bool in_loop = false;

      if (end_to_header_.find(current_offset) != end_to_header_.end()) {
        int loop_header = end_to_header_[current_offset];

        PushLoop(bb->GetStartOffset(), loop_header);

        if (bb->GetStartOffset() == loop_header) {
          in_loop = true;
        }
      } else {
        in_loop = loop_stack_.size() > 1;
      }

      if (in_loop) {
        auto loop_info = loop_stack_.back().second;
        UpdateAssignments(bb, loop_info->GetAssignments());
        if (current_offset == loop_info->Header()) {
          loop_stack_.pop_back();
          if (loop_stack_.size() > 1) {
            auto parent_loop_info = loop_stack_.back().second;
            *parent_loop_info->GetAssignments() |= *loop_info->GetAssignments();
          }
        }
      }
      UpdateLiveness(bb);
    }

    for (auto it : post_order_) {
      auto bb = bb_items_[it];
      UpdateLiveness(bb);
    }
  }

  void Run(lynx::lepus::Instruction* ptr, lynx::lepus::Instruction* end,
           llvh::ArrayRef<int64_t> line_cols) {
    VisitBytecode(ptr, end, line_cols);
    VisitPostOrder();
    VisitLoopBody();
    ComputeLiveOut();
  }
};

void BytecodeAnalysis::Run(lynx::lepus::Instruction* ptr,
                           lynx::lepus::Instruction* end,
                           llvh::ArrayRef<int64_t> line_cols) {
  BytecodeAnalysisInternal internal(this);
  internal.Run(ptr, end, line_cols);
  bb_items_ = std::move(internal.bb_items_);
  header_to_info_ = std::move(internal.header_to_info_);
}

llvh::SmallVector<int, 32> BytecodeAnalysis::GetBBHeaderOffsets() const {
  llvh::SmallVector<int, 32> headers;
  headers.reserve(bb_items_.size());
  for (const auto& it : bb_items_) {
    const auto& item = it.second;
    if (item && item->IsBBHeader()) {
      headers.push_back(it.first);
    }
  }
  std::sort(headers.begin(), headers.end());
  return headers;
}

bool BytecodeAnalysis::IsBBHeader(int32_t offset) {
  if (LEPUS_UNLIKELY(bb_items_.find(offset) == bb_items_.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeAnalysis::IsBBHeader offset not found");
  }
  return bb_items_[offset]->IsBBHeader();
}

bool BytecodeAnalysis::MultipleIncoming(int32_t offset) {
  if (LEPUS_UNLIKELY(bb_items_.find(offset) == bb_items_.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeAnalysis::MultipleIncoming offset not "
        "found");
  }
  return bb_items_[offset]->MultipleIncoming();
}

const LoopInfo& BytecodeAnalysis::GetLoopInfo(int offset) {
  if (LEPUS_UNLIKELY(!IsLoopHeader(offset))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeAnalysis::GetLoopInfo called on "
        "non-loop-header offset");
  }
  return header_to_info_.find(offset)->second;
}

std::shared_ptr<BlockItem> BytecodeAnalysis::GetBlockItem(int offset) {
  if (LEPUS_UNLIKELY(bb_items_.find(offset) == bb_items_.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeAnalysis::GetBlockItem offset not found");
  }
  return bb_items_[offset];
}

int BytecodeAnalysis::GetBBPredecessors(int offset) {
  if (LEPUS_UNLIKELY(!IsBBHeader(offset))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeAnalysis::GetBBPredecessors called on "
        "non-basic-block-header offset");
  }
  return bb_items_[offset]->bb_predecessors_;
}

void BlockItem::AddPredecessor(std::shared_ptr<BlockItem> to) {
  bb_predecessors_++;
  if (first_predecessor_.to_ == nullptr) {
    first_predecessor_.to_ = to;
  } else {
    auto* new_next_block = new NextBlock(to);
    new_next_block->next_ = first_predecessor_.next_;
    first_predecessor_.next_ = new_next_block;
  }
}

void BlockItem::AddSuccessor(std::shared_ptr<BlockItem> to) {
  if (first_successor_.to_ == nullptr) {
    first_successor_.to_ = to;
  } else {
    auto* new_next_block = new NextBlock(to);
    new_next_block->next_ = first_successor_.next_;
    first_successor_.next_ = new_next_block;
  }
}

BlockItem::ConstIterator& BlockItem::ConstIterator::operator++() {
  if (LEPUS_UNLIKELY(current_ == nullptr)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeAnalysis iterator incremented past end");
  }
  current_ = current_->next_;
  return *this;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
