// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_ANALYSIS_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_ANALYSIS_H_

#include <memory>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ArrayRef.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/BitVector.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_iterator.h"

namespace lynx {
namespace lepus {
namespace ir {

class FuncOp;

enum BBVisitState : uint8_t {
  kUnvisited = 0,
  kPending,
  kOnStack,
  kVisited,
};

class BlockItem {
 public:
  class NextBlock {
   public:
    std::shared_ptr<BlockItem> to_;
    NextBlock* next_ = nullptr;

    explicit NextBlock(std::shared_ptr<BlockItem> to) : to_(to) {}
  };
  int start_offset_;
  int bb_predecessors_ = 0;
  int fall_through_pred_ = -1;
  bool new_bb_ = false;
  LepusOpcode lepus_opcode_;
  BBVisitState state_{BBVisitState::kUnvisited};
  NextBlock first_successor_;
  NextBlock first_predecessor_;
  std::shared_ptr<llvh::BitVector> live_in_;
  std::shared_ptr<llvh::BitVector> live_out_;

  using value_type = NextBlock*;

  explicit BlockItem(int offset)
      : start_offset_(offset),
        first_successor_(nullptr),
        first_predecessor_(nullptr) {}

  bool IsBBHeader() {
    return ((fall_through_pred_ == -1) || (bb_predecessors_ >= 2) || new_bb_);
  }

  bool MultipleIncoming() { return bb_predecessors_ >= 2; }

  bool HasSuccessors() const { return first_successor_.to_ != nullptr; }

  std::shared_ptr<llvh::BitVector> GetLiveIn() const { return live_in_; }

  std::shared_ptr<llvh::BitVector> GetLiveOut() const { return live_out_; }

  int GetStartOffset() const { return start_offset_; }

  void SetOpcode(LepusOpcode opcode) { lepus_opcode_ = opcode; }

  void AddSuccessor(std::shared_ptr<BlockItem> to);
  void AddPredecessor(std::shared_ptr<BlockItem> to);

  class ConstIterator {
   public:
    NextBlock* current_;

    explicit ConstIterator(NextBlock* current)
        : current_(current->to_ == nullptr ? nullptr : current) {}
    ConstIterator() : current_(nullptr) {}

    bool operator==(const ConstIterator& other) const {
      return current_ == other.current_;
    }

    bool operator!=(const ConstIterator& other) const {
      return current_ != other.current_;
    }

    std::shared_ptr<BlockItem> operator*() { return current_->to_; }

    ConstIterator& operator++();
  };

  class BlockList {
   public:
    using value_type = BlockItem*;
    explicit BlockList(NextBlock* item) : item_(item) {}

    inline ConstIterator begin() const {
      return ConstIterator(const_cast<NextBlock*>(item_));
    }
    inline ConstIterator end() const { return ConstIterator(); }

   private:
    NextBlock* item_;
  };

  BlockList Predecessors() { return BlockList(&first_predecessor_); }
  BlockList Successors() { return BlockList(&first_successor_); }
};

class LoopInfoBase {
 public:
  int parent_;
  int header_;
  int end_;

  LoopInfoBase(int parent, int header, int end)
      : parent_(parent), header_(header), end_(end) {}

  int Header() const { return header_; }
  int End() const { return end_; }
};

class LoopInfo : public LoopInfoBase {
 private:
  std::shared_ptr<llvh::BitVector> assignments_;

 public:
  LoopInfo(int parent, int header, int end, int count)
      : LoopInfoBase(parent, header, end) {
    assignments_ = std::make_shared<llvh::BitVector>(count);
  }

  std::shared_ptr<llvh::BitVector> GetAssignments() const {
    return assignments_;
  }
};

class BytecodeAnalysis {
 public:
  BytecodeAnalysis(FuncOp* func, int register_count)
      : register_count_(register_count) {}

  ~BytecodeAnalysis() = default;

  void Run(lynx::lepus::Instruction* start, lynx::lepus::Instruction* end,
           llvh::ArrayRef<int64_t> line_cols);

  bool IsBBHeader(int32_t offset);
  bool MultipleIncoming(int32_t offset);
  bool HasLoop() const { return !header_to_info_.empty(); }

  const llvh::DenseMap<int, LoopInfo>& GetHeaderToInfo() {
    return header_to_info_;
  }

  bool IsLoopHeader(int offset) {
    return header_to_info_.find(offset) != header_to_info_.end();
  }

  const LoopInfo& GetLoopInfo(int offset);
  std::shared_ptr<BlockItem> GetBlockItem(int offset);
  int GetBBPredecessors(int offset);
  int GetRegisterCount() const { return register_count_; }
  std::shared_ptr<llvh::BitVector> GetLiveIn(int offset) {
    return bb_items_[offset]->GetLiveIn();
  }

  /// Returns all basic-block header offsets in ascending order.
  ///
  /// Note: the analysis may contain a synthetic end block at offset ==
  /// bytecode_size. Callers should filter it out if not needed.
  llvh::SmallVector<int, 32> GetBBHeaderOffsets() const;

 private:
  int32_t register_count_;
  llvh::DenseMap<int, std::shared_ptr<BlockItem>> bb_items_;
  llvh::DenseMap<int, LoopInfo> header_to_info_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_ANALYSIS_H_
