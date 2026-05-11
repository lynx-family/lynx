// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/instruction_selection.h"

#include <algorithm>
#include <limits>
#include <string>

#include "base/include/value/base_string.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallPtrSet.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/op_code.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

static inline bool TryGetSignedInt16Immediate(const lepus::Value& v,
                                              int16_t* out) {
  if (!out) return false;

  int64_t as_i64 = 0;
  if (v.IsInt64()) {
    as_i64 = v.Int64();
  } else if (v.IsInt32()) {
    as_i64 = static_cast<int64_t>(v.Int32());
  } else if (v.IsUInt32()) {
    as_i64 = static_cast<int64_t>(v.UInt32());
  } else if (v.IsUInt64()) {
    auto u = v.UInt64();
    if (u > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
      return false;
    }
    as_i64 = static_cast<int64_t>(u);
  } else {
    return false;
  }

  if (as_i64 < std::numeric_limits<int16_t>::min() ||
      as_i64 > std::numeric_limits<int16_t>::max()) {
    return false;
  }
  *out = static_cast<int16_t>(as_i64);
  return true;
}

static inline bool CanSpecializeTableStoreReceiver(Value* receiver) {
  if (!receiver) {
    return false;
  }
  if (receiver->GetType() && receiver->GetType()->IsTableType()) {
    return true;
  }

  llvh::DenseMap<Value*, bool> memo;
  llvh::SmallPtrSet<Value*, 16> visiting;
  auto resolve_fresh_table = [&](auto&& self, Value* value) -> bool {
    if (!value) {
      return false;
    }

    auto it = memo.find(value);
    if (it != memo.end()) {
      return it->second;
    }

    if (!visiting.insert(value).second) {
      memo[value] = false;
      return false;
    }

    bool result = false;
    if (llvh::isa<NewTableInst>(value)) {
      result = true;
    } else if (auto* phi = llvh::dyn_cast<PhiInst>(value)) {
      const unsigned n = phi->GetNumEntries();
      if (n > 0) {
        result = true;
        for (unsigned i = 0; i < n; ++i) {
          auto entry = phi->GetEntry(i);
          if (!self(self, entry.first)) {
            result = false;
            break;
          }
        }
      }
    }

    visiting.erase(value);
    memo[value] = result;
    return result;
  };

  return resolve_fresh_table(resolve_fresh_table, receiver);
}

InstructionSelectionPass::InstructionSelectionPass(IRContext* ir_ctx)
    : FunctionPass(ir_ctx, "instruction-selection") {}

uint32_t InstructionSelectionPass::GetCurrentOffset() const {
  return static_cast<uint32_t>(op_codes_.size());
}

void InstructionSelectionPass::Generate(Instruction* ii, Block* next_bb) {
  // Generate the debug info.
  if (ii->HasLocation()) {
    relocations_.push_back(
        {GetCurrentOffset(), Relocation::RelocationType::DebugInfo, ii});
  }

  switch (ii->GetKind()) {
#define DEF_VALUE(CLASS, PARENT) \
  case ValueKind::CLASS##Kind:   \
    return Generate##CLASS(llvh::cast<CLASS>(ii), next_bb);
#define DEF_TAG(NAME, PARENT) \
  case ValueKind::NAME##Kind: \
    return Generate##PARENT(llvh::cast<PARENT>(ii), next_bb);
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/instrs.def"
#undef ENABLE_MIR_INSTR
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: InstructionSelectionPass::Generate encountered "
          "invalid ValueKind");
  }
}

void InstructionSelectionPass::Generate(Block* bb, Block* next) {
  auto begin_loc = GetCurrentOffset();

  relocations_.push_back(
      {begin_loc, Relocation::RelocationType::BasicBlockType, bb});

  std::for_each(bb->InstBegin(), bb->InstEnd(),
                [&](Instruction* inst) { Generate(inst, next); });

  basic_block_map_[bb] = std::make_pair(begin_loc, next);

  auto end_loc = GetCurrentOffset();
  if (!next) {
    // When next is nullptr, we are hitting the last bb.
    // We should also register that null bb with it's location.
    basic_block_map_[nullptr] = std::make_pair(end_loc, nullptr);
  }
}

#define ADD_INSTRUCTION(inst, instruction)          \
  do {                                              \
    op_codes_.push_back(instruction);               \
    debug_line_col_.push_back(inst->GetLocation()); \
  } while (0)

#define BinaryOperator(inst, type, dst, src1, src2)                            \
  do {                                                                         \
    ADD_INSTRUCTION(inst,                                                      \
                    lynx::lepus::Instruction::ABCCode(type, dst, src1, src2)); \
  } while (0);

#define UnaryOperator(inst, type, dst)                                 \
  do {                                                                 \
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ACode(type, dst)); \
  } while (0);

#define UnaryOperator2(inst, type, dst, src)                                 \
  do {                                                                       \
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(type, dst, src)); \
  } while (0);

lynx::lepus::Instruction* InstructionSelectionPass::GetInstruction(
    uint32_t offset) {
  return &op_codes_[offset];
}

void InstructionSelectionPass::ResolveRelocations() {
  ResolveRelocationsInternal();
}

static bool IsJmpInstruction(lynx::lepus::Instruction& i) {
  auto opcode = lynx::lepus::Instruction::GetOpCode(i);
  return opcode == TypeOp_Jmp || opcode == TypeOp_JmpTrue ||
         opcode == TypeOp_JmpFalse || opcode == TypeOp_BoolJmpFalse ||
         opcode == TypeOp_BoolJmpTrue;
}

static bool IsCmpJmpInstruction(lynx::lepus::Instruction& i) {
  auto opcode = lynx::lepus::Instruction::GetOpCode(i);
  return opcode == TypeOp_EqualJmpTrue || opcode == TypeOp_EqualJmpFalse ||
         opcode == TypeOp_UnEqualJmpTrue || opcode == TypeOp_UnEqualJmpFalse;
}

static bool IsNoOpJmpInstruction(lynx::lepus::Instruction& i) {
  return lynx::lepus::Instruction::GetOpCode(i) == TypeOp_Jmp &&
         lynx::lepus::Instruction::GetParamsBx(i) == 1;
}

bool InstructionSelectionPass::ResolveRelocationsInternal() {
  for (auto relocation : relocations_) {
    auto loc = relocation.loc;
    auto pointer = relocation.pointer;
    auto type = relocation.type;

    switch (type) {
      case Relocation::Jmp: {
        auto* target_bb = llvh::cast<Block>(pointer);
        auto it = basic_block_map_.find(target_bb);
        if (it == basic_block_map_.end()) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: InstructionSelectionPass relocation target "
              "basic block missing");
        }
        auto target_location = it->second.first;
        int diff = static_cast<int>(target_location) - static_cast<int>(loc);
        if (LEPUS_UNLIKELY(diff == 0 || diff < -32768 || diff > 32767)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: isel relocation produced out-of-range jmp "
              "offset");
        }
        short jmp_offset = static_cast<short>(diff);
        auto& jmp_inst = *GetInstruction(loc);
        if (LEPUS_UNLIKELY(!IsJmpInstruction(jmp_inst))) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: isel relocation expected a jmp instruction at "
              "relocation site");
        }
        jmp_inst.RefillsBx(jmp_offset);
      } break;
      case Relocation::CmpJmp: {
        auto* target_bb = llvh::cast<Block>(pointer);
        auto it = basic_block_map_.find(target_bb);
        if (it == basic_block_map_.end()) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: InstructionSelectionPass relocation target "
              "basic block missing");
        }
        auto target_location = it->second.first;
        int diff = static_cast<int>(target_location) - static_cast<int>(loc);
        if (LEPUS_UNLIKELY(diff == 0 || diff < -128 || diff > 127)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: isel relocation produced out-of-range cmp-jmp "
              "offset");
        }
        short jmp_offset = static_cast<short>(diff);
        auto& jmp_inst = *GetInstruction(loc);
        if (LEPUS_UNLIKELY(!IsCmpJmpInstruction(jmp_inst))) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: isel relocation expected a cmp-jmp instruction "
              "at relocation site");
        }
        jmp_inst.RefillsB(jmp_offset);
      } break;
      case Relocation::BasicBlockType: {
        basic_block_map_[llvh::cast<Block>(pointer)].first = loc;
      } break;
      case Relocation::DebugInfo:
        break;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: InstructionSelectionPass encountered unsupported "
            "relocation type");
    }
  }
  return true;
}

void InstructionSelectionPass::CompactConstPoolAndRewriteConstIndices() {
  // After instruction selection we may lower some const-pool loads to
  // immediates (e.g. TypeOp_LoadSmallInt). Compact the constant pool and
  // rewrite any remaining const indices so we can drop now-unused entries.
  const auto& old_consts = lepus_function_->GetConstValue();
  const size_t old_size = old_consts.size();
  if (old_size == 0) {
    return;
  }

  std::vector<uint8_t> used(old_size, 0);

  for (size_t pc = 0; pc < op_codes_.size(); ++pc) {
    const auto& inst = op_codes_[pc];
    const long op = lynx::lepus::Instruction::GetOpCode(inst);
    if (op == TypeOp_LoadConst || op == TypeOp_LoadConstAndClone) {
      const auto idx =
          static_cast<size_t>(lynx::lepus::Instruction::GetParamBx(inst));
      if (idx < old_size) used[idx] = 1;
    } else if (op == TypeOp_SetObjectConstString) {
      const auto idx =
          static_cast<size_t>(lynx::lepus::Instruction::GetParamB(inst));
      if (idx < old_size) used[idx] = 1;
    } else if (op == TypeOp_GetTableConstString) {
      const auto idx =
          static_cast<size_t>(lynx::lepus::Instruction::GetParamC(inst));
      if (idx < old_size) used[idx] = 1;
    }
  }

  bool all_used = true;
  for (auto u : used) {
    if (!u) {
      all_used = false;
      break;
    }
  }

  if (all_used) {
    return;
  }

  // Build old->new remap.
  std::vector<uint16_t> remap(old_size, static_cast<uint16_t>(0xFFFFu));
  base::InlineVector<lepus::Value, 8> new_consts;
  for (size_t i = 0; i < old_size; ++i) {
    if (!used[i]) continue;
    const auto new_idx = static_cast<uint16_t>(new_consts.size());
    remap[i] = new_idx;
    new_consts.push_back(old_consts[i]);
  }

  // Rewrite bytecode operands.
  for (size_t pc = 0; pc < op_codes_.size(); ++pc) {
    auto& inst = op_codes_[pc];
    const long op = lynx::lepus::Instruction::GetOpCode(inst);
    if (op == TypeOp_LoadConst || op == TypeOp_LoadConstAndClone) {
      const auto old_idx =
          static_cast<size_t>(lynx::lepus::Instruction::GetParamBx(inst));
      if (old_idx < old_size) {
        const auto new_idx = remap[old_idx];
        if (LEPUS_UNLIKELY(new_idx == 0xFFFFu)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: const pool remap missing for const load");
        }
        inst.RefillsBx(static_cast<short>(new_idx));
      }
    } else if (op == TypeOp_SetObjectConstString) {
      const auto old_idx =
          static_cast<size_t>(lynx::lepus::Instruction::GetParamB(inst));
      if (old_idx < old_size) {
        const auto new_idx = remap[old_idx];
        if (LEPUS_UNLIKELY(new_idx == 0xFFFFu || new_idx > 0xFFu)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: const pool remap out of range for "
              "SetObjectConstString");
        }
        inst.RefillsB(static_cast<long>(new_idx));
      }
    } else if (op == TypeOp_GetTableConstString) {
      const auto old_idx =
          static_cast<size_t>(lynx::lepus::Instruction::GetParamC(inst));
      if (old_idx < old_size) {
        const auto new_idx = remap[old_idx];
        if (LEPUS_UNLIKELY(new_idx == 0xFFFFu || new_idx > 0xFFu)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: const pool remap out of range for "
              "GetTableConstString");
        }
        inst = lynx::lepus::Instruction::ABCCode(
            static_cast<lynx::lepus::TypeOpCode>(op),
            lynx::lepus::Instruction::GetParamA(inst),
            lynx::lepus::Instruction::GetParamB(inst),
            static_cast<long>(new_idx));
      }
    }
  }

  lepus_function_->ResetConstValues(std::move(new_consts));
}

void InstructionSelectionPass::CompactNoOpJumpsAndRewriteOffsets() {
  const size_t old_size = op_codes_.size();
  if (old_size == 0) {
    return;
  }

  std::vector<uint8_t> relocated_jmp(old_size, 0);
  std::vector<uint8_t> relocated_cmp_jmp(old_size, 0);
  std::vector<uint8_t> removable_no_op_jmp(old_size, 0);
  for (const auto& relocation : relocations_) {
    if (relocation.loc >= old_size) {
      continue;
    }
    if (relocation.type == Relocation::RelocationType::Jmp) {
      relocated_jmp[relocation.loc] = 1;
      if (IsNoOpJmpInstruction(op_codes_[relocation.loc])) {
        removable_no_op_jmp[relocation.loc] = 1;
      }
    } else if (relocation.type == Relocation::RelocationType::CmpJmp) {
      relocated_cmp_jmp[relocation.loc] = 1;
    }
  }

  std::vector<uint32_t> old_to_new(old_size + 1, 0);
  uint32_t removed_count = 0;
  for (size_t pc = 0; pc < old_size; ++pc) {
    old_to_new[pc] = static_cast<uint32_t>(pc) - removed_count;
    if (removable_no_op_jmp[pc]) {
      ++removed_count;
    }
  }
  old_to_new[old_size] = static_cast<uint32_t>(old_size) - removed_count;

  if (removed_count == 0) {
    return;
  }

  auto map_target = [&](int old_target) -> uint32_t {
    if (LEPUS_UNLIKELY(old_target < 0 ||
                       old_target > static_cast<int>(old_size))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: no-op jmp compaction found invalid target");
    }
    while (old_target < static_cast<int>(old_size) &&
           removable_no_op_jmp[static_cast<size_t>(old_target)]) {
      old_target += lynx::lepus::Instruction::GetParamsBx(
          op_codes_[static_cast<size_t>(old_target)]);
      if (LEPUS_UNLIKELY(old_target < 0 ||
                         old_target > static_cast<int>(old_size))) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: no-op jmp compaction found invalid no-op jmp "
            "target");
      }
    }
    return old_to_new[static_cast<size_t>(old_target)];
  };

  base::Vector<lynx::lepus::Instruction> new_op_codes;
  std::vector<int64_t> new_debug_line_col;
  new_op_codes.reserve(old_size - removed_count);
  new_debug_line_col.reserve(old_size - removed_count);

  for (size_t old_pc = 0; old_pc < old_size; ++old_pc) {
    auto inst = op_codes_[old_pc];
    if (removable_no_op_jmp[old_pc]) {
      continue;
    }

    const uint32_t new_pc = old_to_new[old_pc];
    if (relocated_jmp[old_pc]) {
      const auto old_offset = lynx::lepus::Instruction::GetParamsBx(inst);
      if (old_offset == 0) {
        new_op_codes.push_back(inst);
        new_debug_line_col.push_back(debug_line_col_[old_pc]);
        continue;
      }
      const int old_target = static_cast<int>(old_pc) + old_offset;
      const int new_offset =
          static_cast<int>(map_target(old_target)) - static_cast<int>(new_pc);
      if (LEPUS_UNLIKELY(new_offset == 0 || new_offset < -32768 ||
                         new_offset > 32767)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: no-op jmp compaction produced out-of-range jmp "
            "offset");
      }
      inst.RefillsBx(static_cast<short>(new_offset));
    } else if (relocated_cmp_jmp[old_pc]) {
      const auto old_offset =
          static_cast<int8_t>(lynx::lepus::Instruction::GetParamB(inst));
      const int old_target = static_cast<int>(old_pc) + old_offset;
      const int new_offset =
          static_cast<int>(map_target(old_target)) - static_cast<int>(new_pc);
      if (LEPUS_UNLIKELY(new_offset == 0 || new_offset < -128 ||
                         new_offset > 127)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: no-op jmp compaction produced out-of-range "
            "cmp-jmp offset");
      }
      inst.RefillsB(static_cast<long>(new_offset));
    }

    new_op_codes.push_back(inst);
    new_debug_line_col.push_back(debug_line_col_[old_pc]);
  }

  op_codes_ = std::move(new_op_codes);
  debug_line_col_ = std::move(new_debug_line_col);
}

void InstructionSelectionPass::BytecodeGenerateComplete() {
  CompactNoOpJumpsAndRewriteOffsets();

  CompactConstPoolAndRewriteConstIndices();

  // `Function::register_count_` stores the highest register index, not the
  // number of registers. IR O1 may renumber registers after the legacy
  // bytecode generator already populated this metadata. If we only replace
  // opcodes here and keep the stale max register index, the VM can allocate a
  // stack frame that is too small for the newly selected bytecode registers,
  // which then corrupts later register reads/writes at runtime.
  long max_allocated_reg_index = -1;
  if (ra_ != nullptr) {
    const unsigned max_usage = ra_->GetMaxRegisterUsage();
    if (max_usage > 0) {
      max_allocated_reg_index = static_cast<long>(max_usage - 1);
    }
  }
  max_allocated_reg_index =
      std::max(max_allocated_reg_index, extra_temp_max_reg_index_);
  if (lepus_function_->GetRegisterCount() < max_allocated_reg_index) {
    lepus_function_->SetRegisterCount(max_allocated_reg_index);
  }

  lepus_function_->ResetOpcodes(op_codes_, debug_line_col_);
}

void InstructionSelectionPass::Reset(FuncOp* func) {
  relocations_.clear();
  basic_block_map_.clear();
  op_codes_.clear();
  debug_line_col_.clear();
  extra_temp_max_reg_index_ = -1;
  func_op_ = func;
  ra_ = ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (LEPUS_UNLIKELY(!ra_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass requires RegisterAllocator "
        "analysis");
  }
  lepus_function_ = func->GetLepusFunction();
  if (LEPUS_UNLIKELY(!lepus_function_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass requires non-null "
        "LepusFunction");
  }
  lepus_function_->ClearOpCodes();
  root_func_deopt_ = ir_ctx_->GetTargetContext()->IsRootFuncDeopt();
  // UpdateToplevelClosureVarPass resolves child-function upvalue indices to the
  // final root shared slot and records that mapping on FuncOp. Instruction
  // selection then lowers those accesses to explicit
  // Get/SetToplevelClosureVar bytecode. This invariant is required for both
  // normal root layout and root-function deopt fallback, so serialized upvalue
  // metadata is no longer needed after lowering.
  lepus_function_->ClearUpvalues();
}

bool InstructionSelectionPass::RunOnFunction(FuncOp* func) {
  Reset(func);
  if (func->GetName() == constants::kDeepCloneName &&
      func->CanSkipDeepCloneLowering() && !root_func_deopt_) {
    // since DeepClone is only used in builtin and we don't want to generate
    // bytecode for it, we can skip it in instruction selection pass.
    return true;
  }

  PostOrderAnalysis2 PO(func);

  /// The order of the blocks is reverse-post-order, which is a simply
  /// topological sort.
  llvh::SmallVector<Block*, 16> order(PO.rbegin(), PO.rend());

  for (int i = 0, e = order.size(); i < e; ++i) {
    Block* bb = order[i];
    Block* next_bb = ((i + 1) == e) ? nullptr : order[i + 1];
    Generate(bb, next_bb);
  }

  FixOutOfRangeCmpJmpRelocations();

  ResolveRelocations();

  BytecodeGenerateComplete();

  return true;
}

static bool IsEqNeqCmpJmpInstruction(lynx::lepus::Instruction& i,
                                     lynx::lepus::TypeOpCode& out_cmp_op,
                                     lynx::lepus::TypeOpCode& out_bool_jmp_op) {
  const long opcode = lynx::lepus::Instruction::GetOpCode(i);
  switch (opcode) {
    case TypeOp_EqualJmpTrue:
      out_cmp_op = TypeOp_Equal;
      out_bool_jmp_op = TypeOp_BoolJmpTrue;
      return true;
    case TypeOp_EqualJmpFalse:
      out_cmp_op = TypeOp_Equal;
      out_bool_jmp_op = TypeOp_BoolJmpFalse;
      return true;
    case TypeOp_UnEqualJmpTrue:
      out_cmp_op = TypeOp_UnEqual;
      out_bool_jmp_op = TypeOp_BoolJmpTrue;
      return true;
    case TypeOp_UnEqualJmpFalse:
      out_cmp_op = TypeOp_UnEqual;
      out_bool_jmp_op = TypeOp_BoolJmpFalse;
      return true;
    default:
      return false;
  }
}

int InstructionSelectionPass::FindCmpJmpFallbackRegister(
    Instruction* branch_inst, uint8_t lhs_reg, uint8_t rhs_reg) const {
  if (!ra_ || !branch_inst || !ra_->HasInstructionNumber(branch_inst)) {
    return static_cast<int>(lhs_reg);
  }

  const unsigned branch_idx = ra_->GetInstructionNumber(branch_inst);
  Segment branch_point(branch_idx + 1, branch_idx + 2);

  // Single O(N) pass: collect all registers live at the branch point.
  llvh::SmallDenseSet<unsigned, 16> live_regs;
  for (auto& bb : *func_op_) {
    for (auto& op : bb) {
      auto* inst = llvh::dyn_cast<Instruction>(&op);
      if (!inst || inst == branch_inst || !ra_->IsAllocated(inst) ||
          !ra_->HasInstructionNumber(inst)) {
        continue;
      }
      if (ra_->GetInstructionInterval(inst).Intersects(branch_point)) {
        live_regs.insert(ra_->GetRegister(inst).GetIndex());
      }
    }
  }
  // Parameters are conservatively treated as always-live.
  for (auto* param : func_op_->GetParams()) {
    if (!param || !ra_->IsAllocated(param)) continue;
    live_regs.insert(ra_->GetRegister(param).GetIndex());
  }

  // Prefer reusing lhs_reg if it is dead at the branch point.
  if (live_regs.count(lhs_reg) == 0) {
    return static_cast<int>(lhs_reg);
  }

  // Otherwise, find the first available register not conflicting with lhs/rhs.
  for (unsigned reg = 0; reg < Register::kMaxRegistersLimit; ++reg) {
    if (reg == lhs_reg || reg == rhs_reg) continue;
    if (live_regs.count(reg) == 0) {
      return static_cast<int>(reg);
    }
  }
  return -1;
}

void InstructionSelectionPass::FixOutOfRangeCmpJmpRelocations() {
  // Expand out-of-range 8-bit cmp-jmp relocations into:
  //   cmp (writes bool into regA) + BoolJmp{True/False} (16-bit)
  // This is done BEFORE ResolveRelocations so the remaining relocations are
  // guaranteed to be in-range.

  // Collect indices of cmp-jmp relocations.
  llvh::SmallVector<size_t, 16> indices;
  indices.reserve(relocations_.size());
  for (size_t i = 0; i < relocations_.size(); i++) {
    if (relocations_[i].type == Relocation::RelocationType::CmpJmp) {
      indices.push_back(i);
    }
  }
  if (indices.empty()) return;

  // Sort by loc ascending to apply shifts deterministically.
  std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
    return relocations_[a].loc < relocations_[b].loc;
  });

  auto shift_after = [&](uint32_t loc, uint32_t delta) {
    // Update cached offsets for future relocation calculations.
    for (auto& kv : basic_block_map_) {
      if (kv.second.first > loc) {
        kv.second.first += delta;
      }
    }
    for (auto& r : relocations_) {
      if (r.loc > loc) {
        r.loc += delta;
      }
    }
  };

  bool changed = false;
  for (size_t r_index : indices) {
    if (r_index >= relocations_.size()) continue;
    auto& r = relocations_[r_index];
    if (r.type != Relocation::RelocationType::CmpJmp) continue;

    const uint32_t loc = r.loc;
    auto* target_bb = llvh::cast<Block>(r.pointer);
    auto it = basic_block_map_.find(target_bb);
    if (it == basic_block_map_.end()) continue;
    const uint32_t target = it->second.first;
    const int diff = static_cast<int>(target) - static_cast<int>(loc);
    if (diff != 0 && diff >= -128 && diff <= 127) {
      continue;
    }

    if (loc >= op_codes_.size()) continue;
    auto inst = op_codes_[loc];
    lynx::lepus::TypeOpCode cmp_op = TypeOp_Equal;
    lynx::lepus::TypeOpCode bool_jmp_op = TypeOp_BoolJmpTrue;
    if (!IsEqNeqCmpJmpInstruction(inst, cmp_op, bool_jmp_op)) {
      continue;
    }

    const long a = lynx::lepus::Instruction::GetParamA(inst);
    const long c = lynx::lepus::Instruction::GetParamC(inst);

    // Re-check after previous transformations since they may have shifted
    // block offsets.
    auto it2 = basic_block_map_.find(target_bb);
    if (it2 == basic_block_map_.end()) continue;
    const uint32_t target2 = it2->second.first;
    const int diff2 = static_cast<int>(target2) - static_cast<int>(loc);
    if (diff2 != 0 && diff2 >= -128 && diff2 <= 127) {
      continue;
    }

    const int temp_reg =
        FindCmpJmpFallbackRegister(r.source_inst, r.lhs_reg, r.rhs_reg);
    if (LEPUS_UNLIKELY(temp_reg < 0)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: out-of-range cmp-jmp widening cannot find a "
          "scratch register");
    }

    // Replace cmp-jmp at `loc` with a compare writing into a register that is
    // dead after the branch site, so widening preserves the original lhs value
    // when it stays live across successors.
    op_codes_[loc] = lynx::lepus::Instruction::ABCCode(cmp_op, temp_reg, a, c);

    // Insert a wide BoolJmp instruction right after.
    op_codes_.insert(
        op_codes_.begin() + static_cast<long>(loc + 1),
        lynx::lepus::Instruction::ABxCode(bool_jmp_op, temp_reg, 0));
    debug_line_col_.insert(debug_line_col_.begin() + static_cast<long>(loc + 1),
                           debug_line_col_[loc]);
    extra_temp_max_reg_index_ =
        std::max(extra_temp_max_reg_index_, static_cast<long>(temp_reg));

    // All subsequent offsets and relocation sites move by +1.
    shift_after(loc, 1);

    // Retarget this relocation to the inserted BoolJmp and switch to wide jmp.
    r.type = Relocation::RelocationType::Jmp;
    r.loc = loc + 1;
    changed = true;
  }

  // There may be multiple cmp-jmps and their diffs can change after each
  // insertion. Iterate to a fixed point.
  if (changed) {
    FixOutOfRangeCmpJmpRelocations();
  }
}

bool InstructionSelectionPass::IsInCmpJmpRange(int32_t diff) {
  return diff != 0 && diff >= -128 && diff <= 127;
}

void InstructionSelectionPass::GenerateNopInst(NopInst* inst, Block* next_bb) {}

void InstructionSelectionPass::GeneratePhiInst(PhiInst* inst, Block* next_bb) {
  // PhiInst has been translated into a sequence of MOVs in RegAlloc
  // Nothing to do here.
}

void InstructionSelectionPass::GenerateEqNeqCondBranch(
    Instruction* inst, Value* lhs, Value* rhs, Block* true_bb, Block* false_bb,
    Block* next_bb, TypeOpCode true_opcode, TypeOpCode false_opcode) {
  auto left = EncodeValue(lhs);
  auto right = EncodeValue(rhs);

  auto loc = GetCurrentOffset();
  if (next_bb == true_bb) {
    // Emit a conditional jump to the 'False' destination and a fall-through to
    // the 'True' side.
    ADD_INSTRUCTION(
        inst, lynx::lepus::Instruction::ABCCode(false_opcode, left, 0, right));
    RegisterCmpJmp(loc, false_bb, inst, left, right);
    return;
  }

  ADD_INSTRUCTION(
      inst, lynx::lepus::Instruction::ABCCode(true_opcode, left, 0, right));
  RegisterCmpJmp(loc, true_bb, inst, left, right);

  if (next_bb == false_bb) {
    return;
  }

  loc = GetCurrentOffset();
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(TypeOp_Jmp, 0, 0));
  RegisterJmp(loc, false_bb);
}

void InstructionSelectionPass::GenerateEqCondBranchInst(EqCondBranchInst* inst,
                                                        Block* next_bb) {
  GenerateEqNeqCondBranch(inst, inst->GetLeftHandSide(),
                          inst->GetRightHandSide(), inst->GetTrueDest(),
                          inst->GetFalseDest(), next_bb, TypeOp_EqualJmpTrue,
                          TypeOp_EqualJmpFalse);
}

void InstructionSelectionPass::GenerateNeqCondBranchInst(
    NeqCondBranchInst* inst, Block* next_bb) {
  GenerateEqNeqCondBranch(inst, inst->GetLeftHandSide(),
                          inst->GetRightHandSide(), inst->GetTrueDest(),
                          inst->GetFalseDest(), next_bb, TypeOp_UnEqualJmpTrue,
                          TypeOp_UnEqualJmpFalse);
}

void InstructionSelectionPass::GenerateCondBranchInst(CondBranchInst* inst,
                                                      Block* next_bb) {
  auto cond_val = inst->GetCondition();
  if (llvh::isa<Literal>(cond_val)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass expected inst-combine to "
        "fold literal CondBranchInst");
  }

  auto cond_reg = EncodeValue(cond_val);

  Block* true_bb = inst->GetTrueDest();
  Block* false_bb = inst->GetFalseDest();

  auto loc = GetCurrentOffset();
  if (next_bb == true_bb) {
    auto opcode = TypeOp_JmpFalse;
    if (cond_val->GetType()->IsBooleanType()) {
      opcode = TypeOp_BoolJmpFalse;
    }

    // Emit a conditional jump to the 'False' destination and a fall-through to
    // the 'True' side.
    ADD_INSTRUCTION(inst,
                    lynx::lepus::Instruction::ABxCode(opcode, cond_reg, 0));
    RegisterJmp(loc, false_bb);
    return;
  }

  auto opcode = TypeOp_JmpTrue;
  if (cond_val->GetType()->IsBooleanType()) {
    opcode = TypeOp_BoolJmpTrue;
  }
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(opcode, cond_reg, 0));
  RegisterJmp(loc, true_bb);

  if (next_bb == false_bb) {
    return;
  }

  loc = GetCurrentOffset();
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(TypeOp_Jmp, 0, 0));
  RegisterJmp(loc, false_bb);
}

void InstructionSelectionPass::RegisterJmp(uint32_t loc, Block* target_bb) {
  relocations_.push_back({loc, Relocation::RelocationType::Jmp, target_bb});
}

void InstructionSelectionPass::RegisterCmpJmp(uint32_t loc, Block* target_bb,
                                              Instruction* source_inst,
                                              uint8_t lhs_reg,
                                              uint8_t rhs_reg) {
  relocations_.push_back({loc, Relocation::RelocationType::CmpJmp, target_bb,
                          source_inst, lhs_reg, rhs_reg});
}

void InstructionSelectionPass::GenerateBranchInst(BranchInst* inst,
                                                  Block* next_bb) {
  auto* dst_bb = inst->GetBranchDest();

  if (dst_bb == next_bb) return;

  auto location = GetCurrentOffset();
  // fill the jump offset later
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(TypeOp_Jmp, 0, 0));

  RegisterJmp(location, dst_bb);
}

void InstructionSelectionPass::GenerateReturnInst(ReturnInst* inst,
                                                  Block* next_bb) {
  auto val = EncodeValue(inst->GetValue());
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ACode(TypeOp_Ret, val));
}

void InstructionSelectionPass::GenerateThrowInst(ThrowInst* inst,
                                                 Block* next_bb) {
  auto src = EncodeValue(inst->GetThrownValue());
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ACode(TypeLabel_Throw, src));
}

void InstructionSelectionPass::GenerateBinaryOperatorInst(
    BinaryOperatorInst* inst, Block* next_bb) {
  auto dst = EncodeValue(inst);
  auto left = EncodeValue(inst->GetLeftHandSide());
  auto right = EncodeValue(inst->GetRightHandSide());

  auto* left_ty = inst->GetLeftHandSide()->GetType();
  auto* right_ty = inst->GetRightHandSide()->GetType();

  if (llvh::isa<Literal>(left_ty) && llvh::isa<Literal>(right_ty)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass expected inst-combine to "
        "fold constant binary operator");
  }

  switch (inst->GetKind()) {
    case ValueKind::BinaryLessThanInstKind: {
      BinaryOperator(inst, TypeOp_Less, dst, left, right);
    } break;
    case ValueKind::BinaryLessThanOrEqualInstKind: {
      BinaryOperator(inst, TypeOp_LessEqual, dst, left, right);
    } break;
    case ValueKind::BinaryGreaterThanInstKind: {
      BinaryOperator(inst, TypeOp_Greater, dst, left, right);
    } break;
    case ValueKind::BinaryGreaterThanOrEqualInstKind: {
      BinaryOperator(inst, TypeOp_GreaterEqual, dst, left, right);
    } break;
    case ValueKind::BinaryStrictlyEqualInstKind: {
      if (left_ty->IsStringType() && right_ty->IsStringType()) {
        BinaryOperator(inst, TypeOp_EqualString, dst, left, right);
      } else {
        BinaryOperator(inst, TypeOp_Equal, dst, left, right);
      }
    } break;
    case ValueKind::BinaryStrictlyNotEqualInstKind: {
      if (left_ty->IsStringType() && right_ty->IsStringType()) {
        BinaryOperator(inst, TypeOp_UnEqualString, dst, left, right);
      } else {
        BinaryOperator(inst, TypeOp_UnEqual, dst, left, right);
      }
    } break;
    case ValueKind::BinaryAddInstKind: {
      if (left_ty->IsStringType() && right_ty->IsStringType()) {
        BinaryOperator(inst, TypeOp_AddStringString, dst, left, right);
      } else if (left_ty->IsStringType()) {
        BinaryOperator(inst, TypeOp_AddStringAny, dst, left, right);
      } else if (right_ty->IsStringType()) {
        BinaryOperator(inst, TypeOp_AddAnyString, dst, left, right);
      } else {
        BinaryOperator(inst, TypeOp_Add, dst, left, right);
      }
    } break;
    case ValueKind::BinarySubInstKind:
      BinaryOperator(inst, TypeOp_Sub, dst, left, right);
      break;
    case ValueKind::BinaryMulInstKind:
      BinaryOperator(inst, TypeOp_Mul, dst, left, right);
      break;
    case ValueKind::BinaryDivInstKind:
      BinaryOperator(inst, TypeOp_Div, dst, left, right);
      break;
    case ValueKind::BinaryModInstKind:
      BinaryOperator(inst, TypeOp_Mod, dst, left, right);
      break;
    case ValueKind::BinaryBitOrInstKind:
      BinaryOperator(inst, TypeOp_BitOr, dst, left, right);
      break;
    case ValueKind::BinaryBitAndInstKind:
      BinaryOperator(inst, TypeOp_BitAnd, dst, left, right);
      break;
    case ValueKind::BinaryBitXorInstKind:
      BinaryOperator(inst, TypeOp_BitXor, dst, left, right);
      break;
    case ValueKind::BinaryPowInstKind:
      BinaryOperator(inst, TypeOp_Pow, dst, left, right);
      break;
    case ValueKind::BinaryOrInstKind:
      BinaryOperator(inst, TypeOp_Or, dst, left, right);
      break;
    case ValueKind::BinaryAndInstKind:
      BinaryOperator(inst, TypeOp_And, dst, left, right);
      break;
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: InstructionSelectionPass does not support this "
          "BinaryOperatorInst kind");
  }
}

void InstructionSelectionPass::GenerateUnaryOperatorInst(
    UnaryOperatorInst* inst, Block* next_bb) {
  [[maybe_unused]] auto src = EncodeValue(inst->GetSingleOperand());
  auto dst = EncodeValue(inst);

  if (src != dst) {
    switch (inst->GetKind()) {
      case ValueKind::UnaryPosInstKind:
        UnaryOperator2(inst, TypeOp_Pos2, dst, src);
        break;
      case ValueKind::UnaryNotInstKind:
        UnaryOperator2(inst, TypeOp_Not2, dst, src);
        break;
      case ValueKind::UnaryNegInstKind:
        UnaryOperator2(inst, TypeOp_Neg2, dst, src);
        break;
      case ValueKind::UnaryIncInstKind:
        UnaryOperator2(inst, TypeOp_Inc2, dst, src);
        break;
      case ValueKind::UnaryDecInstKind:
        UnaryOperator2(inst, TypeOp_Dec2, dst, src);
        break;
      case ValueKind::UnaryBitNotInstKind:
        UnaryOperator2(inst, TypeOp_BitNot2, dst, src);
        break;
      case ValueKind::UnaryTypeofInstKind:
        UnaryOperator2(inst, TypeOp_Typeof2, dst, src);
        break;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: InstructionSelectionPass expected unary "
            "src==dst");
        break;
    }
  } else {
    switch (inst->GetKind()) {
      case ValueKind::UnaryPosInstKind:
        UnaryOperator(inst, TypeOp_Pos, dst);
        break;
      case ValueKind::UnaryNotInstKind:
        UnaryOperator(inst, TypeOp_Not, dst);
        break;
      case ValueKind::UnaryNegInstKind:
        UnaryOperator(inst, TypeOp_Neg, dst);
        break;
      case ValueKind::UnaryIncInstKind:
        UnaryOperator(inst, TypeOp_Inc, dst);
        break;
      case ValueKind::UnaryDecInstKind:
        UnaryOperator(inst, TypeOp_Dec, dst);
        break;
      case ValueKind::UnaryBitNotInstKind:
        UnaryOperator(inst, TypeOp_BitNot, dst);
        break;
      case ValueKind::UnaryTypeofInstKind:
        UnaryOperator(inst, TypeOp_Typeof, dst);
        break;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: InstructionSelectionPass does not support this "
            "UnaryOperatorInst kind");
        break;
    }
  }
}

uint8_t InstructionSelectionPass::EncodeValue(Value* value) {
  if (llvh::isa<Instruction>(value) || llvh::isa<Parameter>(value)) {
    auto res = ra_->GetRegister(value).GetIndex();
    if (LEPUS_UNLIKELY(res >= Register::kMaxRegistersLimit)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: InstructionSelectionPass register index exceeds "
          "255");
    }
    return res;
  }
  throw ::lynx::lepus::CompileException(
      "Lepus IR error: InstructionSelectionPass::EncodeValue does not support "
      "this Value type");
}

void InstructionSelectionPass::GenerateLoadConstInst(LoadConstInst* inst,
                                                     Block* next_bb) {
  if (LEPUS_UNLIKELY(!llvh::isa<LiteralUint32>(inst->GetConst()))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: GenerateLoadConstInst expects const index to be "
        "LiteralUint32");
  }
  auto const_index = llvh::cast<LiteralUint32>(inst->GetConst())->GetValue();
  auto res = EncodeValue(inst);

  // If the const pool entry is a small integer that fits in 2 bytes, lower
  // it to an immediate-form opcode to avoid const-pool traffic.
  if (lepus_function_) {
    auto* cval = lepus_function_->GetConstValue(const_index);
    if (cval != nullptr) {
      int16_t imm16 = 0;
      if (TryGetSignedInt16Immediate(*cval, &imm16)) {
        ADD_INSTRUCTION(
            inst, lynx::lepus::Instruction::ABxCode(
                      TypeOp_LoadSmallInt, res,
                      static_cast<uint16_t>(static_cast<uint16_t>(imm16))));
        return;
      }
    }
  }

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(TypeOp_LoadConst, res,
                                                          const_index));
}

void InstructionSelectionPass::GenerateLoadConstMaterializeInst(
    LoadConstMaterializeInst* inst, Block* next_bb) {
  if (LEPUS_UNLIKELY(!llvh::isa<LiteralUint32>(inst->GetConst()))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: GenerateLoadConstMaterializeInst expects const index "
        "to be LiteralUint32");
  }
  const auto const_index =
      llvh::cast<LiteralUint32>(inst->GetConst())->GetValue();
  const auto res = EncodeValue(inst);
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(
                            TypeOp_LoadConstAndClone, res, const_index));
}

void InstructionSelectionPass::GenerateGetTableConstStringKeyInst(
    GetTableConstStringKeyInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto obj = EncodeValue(inst->GetObject());
  auto key_const_index =
      llvh::cast<LiteralUint32>(inst->GetConstIndex())->GetValue();
  if (LEPUS_UNLIKELY(key_const_index > 0xFF)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: GetTableConstStringKey const index must fit in UInt8");
  }
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                            TypeOp_GetTableConstString, dst_reg, obj,
                            static_cast<uint8_t>(key_const_index)));
}

void InstructionSelectionPass::GenerateGetTableInst(GetTableInst* inst,
                                                    Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto obj_val = inst->GetObject();
  auto key_val = inst->GetProp();
  auto obj = EncodeValue(obj_val);
  auto key = EncodeValue(key_val);
  if (obj_val->GetType()->IsTableType() && key_val->GetType()->IsStringType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_GetObjectString, dst_reg, obj, key));
  } else if (obj_val->GetType()->IsTableType() &&
             key_val->GetType()->IsNumberType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_GetObjectNumber, dst_reg, obj, key));
  } else if (obj_val->GetType()->IsArrayType() &&
             key_val->GetType()->IsInt64Type()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_GetArrayInt64, dst_reg, obj, key));
  } else if (obj_val->GetType()->IsBuiltinFuncTableType()) {
    if (LEPUS_UNLIKELY(!key_val->GetType()->IsStringType())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: GetBuiltinFunc requires string key");
    }
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_GetBuiltinFunc, dst_reg, obj, key));
  } else if (obj_val->GetType()->IsAnyType() &&
             key_val->GetType()->IsStringType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_GetTableString, dst_reg, obj, key));
  } else if (obj_val->GetType()->IsAnyType() &&
             key_val->GetType()->IsNumberType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_GetTableNumber, dst_reg, obj, key));
  } else {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_GetTable,
                                                            dst_reg, obj, key));
  }
}

void InstructionSelectionPass::GenerateSetTableConstStringKeyInst(
    SetTableConstStringKeyInst* inst, Block* next_bb) {
  auto obj_val = inst->GetObject();
  if (LEPUS_UNLIKELY(!CanSpecializeTableStoreReceiver(obj_val))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SetTableConstStringKey requires table receiver");
  }
  auto key_const_index =
      llvh::cast<LiteralUint32>(inst->GetConstIndex())->GetValue();
  if (LEPUS_UNLIKELY(key_const_index > 0xFF)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SetTableConstStringKey const index must fit in UInt8");
  }
  auto obj = EncodeValue(obj_val);
  auto val = EncodeValue(inst->GetStoreVal());
  ADD_INSTRUCTION(
      inst, lynx::lepus::Instruction::ABCCode(TypeOp_SetObjectConstString, obj,
                                              key_const_index, val));
}

void InstructionSelectionPass::GenerateSetTableInst(SetTableInst* inst,
                                                    Block* next_bb) {
  auto obj_val = inst->GetObject();
  auto key_val = inst->GetProp();
  auto obj = EncodeValue(obj_val);
  auto key = EncodeValue(key_val);
  auto val = EncodeValue(inst->GetStoreVal());

  if (obj_val->GetType()->IsArrayType() && key_val->GetType()->IsStringType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_SetTable,
                                                            obj, key, val));
  } else if (CanSpecializeTableStoreReceiver(obj_val) &&
             key_val->GetType()->IsStringType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_SetObjectString, obj, key, val));
  } else if (obj_val->GetType()->IsTableType() &&
             key_val->GetType()->IsNumberType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_SetObjectNumber, obj, key, val));
  } else if (obj_val->GetType()->IsAnyType() &&
             key_val->GetType()->IsNumberType()) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_SetTableNumber, obj, key, val));
  } else {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_SetTable,
                                                            obj, key, val));
  }
}

void InstructionSelectionPass::GenerateGetStringLengthInst(
    GetStringLengthInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto str_reg = EncodeValue(inst->GetSingleOperand());
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(TypeOp_GetStringLength,
                                                         dst_reg, str_reg));
}

void InstructionSelectionPass::GenerateToStringInst(ToStringInst* inst,
                                                    Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto src_reg = EncodeValue(inst->GetSrc());
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(TypeOp_ToString,
                                                         dst_reg, src_reg));
}

void InstructionSelectionPass::GenerateNewTableInst(NewTableInst* inst,
                                                    Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ACode(TypeOp_NewTable, dst_reg));
}

void InstructionSelectionPass::GenerateNewArrayInst(NewArrayInst* inst,
                                                    Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto array_size = inst->GetArraySize();

  llvh::SmallVector<unsigned, 4> item_regs;
  for (auto i = 0; i < array_size; i++) {
    item_regs.push_back(EncodeValue(inst->GetOperand(i)));
  }

  bool is_consecutive_after_array = true;
  for (auto i = 0; i < array_size; i++) {
    [[maybe_unused]] auto item_reg = item_regs[i];
    if (item_reg != dst_reg + i + 1) {
      is_consecutive_after_array = false;
      break;
    }
  }

  bool is_consecutive = true;
  for (auto i = 0; i < array_size; i++) {
    [[maybe_unused]] auto item_reg = item_regs[i];
    if (i > 0) {
      if (item_reg != item_regs[i - 1] + 1) {
        is_consecutive = false;
        break;
      }
    }
  }

  if (is_consecutive_after_array) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(
                              TypeOp_NewArray, dst_reg, array_size));
  } else if (is_consecutive) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_NewArrayConsecutive, dst_reg, array_size,
                              item_regs[0]));
  } else {
    auto new_insts_size = (array_size + 3) / 4;
    auto opcode = TypeOp_NewArrayRandom;
    switch (array_size % 4) {
      case 1:
        opcode = TypeOp_NewArrayRandom1;
        break;
      case 2:
        opcode = TypeOp_NewArrayRandom2;
        break;
      case 3:
        opcode = TypeOp_NewArrayRandom3;
        break;
      default:
        break;
    }

    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              opcode, dst_reg, array_size, new_insts_size));

    auto op_count = 0;
    for (auto i = 0; i < new_insts_size; i++) {
      long a, b, c, d;
      d = item_regs[op_count++];

      if (op_count >= array_size) {
        a = b = c = 0;
        ADD_INSTRUCTION(inst, lynx::lepus::Instruction::OpABCCode(d, a, b, c));
        break;
      }
      a = item_regs[op_count++];
      if (op_count >= array_size) {
        b = c = 0;
        ADD_INSTRUCTION(inst, lynx::lepus::Instruction::OpABCCode(d, a, b, c));
        break;
      }
      b = item_regs[op_count++];
      if (op_count >= array_size) {
        c = 0;
        ADD_INSTRUCTION(inst, lynx::lepus::Instruction::OpABCCode(d, a, b, c));
        break;
      }
      c = item_regs[op_count++];
      ADD_INSTRUCTION(inst, lynx::lepus::Instruction::OpABCCode(d, a, b, c));
    }
  }
}

void InstructionSelectionPass::GenerateGetGlobalInst(GetGlobalInst* inst,
                                                     Block* next_bb) {
  auto op = inst->GetSingleOperand();
  if (LEPUS_UNLIKELY(!llvh::isa<LiteralUint32>(op))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: GenerateGetGlobalInst expects operand to be "
        "LiteralUint32");
  }
  auto global_index = llvh::cast<LiteralUint32>(op)->GetValue();
  auto dst_reg = EncodeValue(inst);
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(
                            TypeOp_GetGlobal, dst_reg, global_index));
}

void InstructionSelectionPass::GenerateGetBuiltinInst(GetBuiltinInst* inst,
                                                      Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto global_index = inst->GetBuiltinIndex()->GetValue();

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(
                            TypeOp_GetBuiltin, dst_reg, global_index));
}

void InstructionSelectionPass::GenerateSetCatchIdInst(SetCatchIdInst* inst,
                                                      Block* next_bb) {
  auto dst = EncodeValue(inst);
  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ACode(TypeOp_SetCatchId, dst));
}

void InstructionSelectionPass::GenerateCatchInst(CatchInst* inst,
                                                 Block* next_bb) {
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::Code(TypeLabel_Catch));
}

void InstructionSelectionPass::GenerateMovInst(MovInst* inst, Block* next_bb) {
  auto dst = EncodeValue(inst);
  auto src = EncodeValue(inst->GetSingleOperand());
  if (LEPUS_UNLIKELY(dst == src)) {
    return;
  }
  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ABCode(TypeOp_Move, dst, src));
}

void InstructionSelectionPass::GenerateGetGlobalLynxInst(
    GetGlobalLynxInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ABCode(TypeOp_LoadNil, dst_reg, 3));
}

void InstructionSelectionPass::GenerateLoadNullOrUndefinedInst(
    LoadNullOrUndefinedInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  ADD_INSTRUCTION(
      inst, lynx::lepus::Instruction::ABCode(
                TypeOp_LoadNil, dst_reg, inst->GetLoadNilType()->GetValue()));
}

void InstructionSelectionPass::GenerateLoadToplevelVarsInst(
    LoadToplevelVarsInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);

  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ABCode(TypeOp_LoadNil, dst_reg, 2));
}

void InstructionSelectionPass::GeneratePushContextInst(PushContextInst* inst,
                                                       Block* next_bb) {
  auto src = EncodeValue(inst->GetSingleOperand());

  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ACode(TypeOp_PushContext, src));
}

void InstructionSelectionPass::GeneratePopContextInst(PopContextInst* inst,
                                                      Block* next_bb) {
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::Code(TypeOp_PopContext));
}

void InstructionSelectionPass::GenerateGetContextSlotInst(
    GetContextSlotInst* inst, Block* next_bb) {
  auto index = llvh::cast<LiteralUint8>(inst->GetIndex())->GetValue();
  auto depth = llvh::cast<LiteralUint8>(inst->GetDepth())->GetValue();
  auto dst = EncodeValue(inst);

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_GetContextSlot,
                                                          dst, index, depth));
}

void InstructionSelectionPass::GenerateSetContextSlotInst(
    SetContextSlotInst* inst, Block* next_bb) {
  auto index = llvh::cast<LiteralUint8>(inst->GetIndex())->GetValue();
  auto depth = llvh::cast<LiteralUint8>(inst->GetDepth())->GetValue();
  auto src = EncodeValue(inst->GetToStore());

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_SetContextSlot,
                                                          src, index, depth));
}

void InstructionSelectionPass::GenerateGetContextSlotMovInst(
    GetContextSlotMovInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto context = EncodeValue(inst->GetContext());
  auto index = llvh::cast<LiteralUint8>(inst->GetIndex())->GetValue();
  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ABCCode(TypeOp_GetContextSlotMove,
                                                    dst_reg, index, context));
}

void InstructionSelectionPass::GenerateSetContextSlotMovInst(
    SetContextSlotMovInst* inst, Block* next_bb) {
  auto context = EncodeValue(inst->GetContext());
  auto index = llvh::cast<LiteralUint8>(inst->GetIndex())->GetValue();
  auto val = EncodeValue(inst->GetToStore());
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                            TypeOp_SetContextSlotMove, context, index, val));
}

void InstructionSelectionPass::GeneratePushBlockContextInst(
    PushBlockContextInst* inst, Block* next_bb) {
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::Code(TypeLabel_EnterBlock));
}

void InstructionSelectionPass::GeneratePopBlockContextInst(
    PopBlockContextInst* inst, Block* next_bb) {
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::Code(TypeLabel_LeaveBlock));
}

void InstructionSelectionPass::GenerateCreateBlockContextInst(
    CreateBlockContextInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);

  auto array_size = inst->GetContextArraySize()->GetValue();

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(
                            TypeOp_CreateBlockContext, dst_reg, array_size));
}

void InstructionSelectionPass::GenerateCreateFunctionContextInst(
    CreateFunctionContextInst* inst, Block* next_bb) {
  if (!llvh::isa<LiteralUint8>(inst->GetSingleOperand())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateCreateFunctionContextInst expects "
        "LiteralUint8 operand");
  }

  auto array_size = inst->GetContextArraySize()->GetValue();

  auto dst_reg = EncodeValue(inst);
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(TypeOp_CreateContext,
                                                         dst_reg, array_size));
}

void InstructionSelectionPass::GenerateCreateClosureInst(
    CreateClosureInst* inst, Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto child_func_index = inst->GetChildrenIndex();
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(
                            TypeOp_Closure, dst_reg, child_func_index));
}

uint8_t InstructionSelectionPass::GetLiteralUint8Value(
    Value* value, const char* inst_name) const {
  if (!llvh::isa<LiteralUint8>(value)) {
    throw ::lynx::lepus::CompileException((std::string("Lepus IR error: ") +
                                           inst_name +
                                           " expects LiteralUint8 index")
                                              .c_str());
  }
  return llvh::cast<LiteralUint8>(value)->GetValue();
}

uint32_t InstructionSelectionPass::GetLiteralUint32Value(
    Value* value, const char* inst_name, const char* value_desc) const {
  if (!llvh::isa<LiteralUint32>(value)) {
    throw ::lynx::lepus::CompileException(
        (std::string("Lepus IR error: ") + inst_name +
         " expects LiteralUint32 " + value_desc)
            .c_str());
  }
  return llvh::cast<LiteralUint32>(value)->GetValue();
}

uint8_t InstructionSelectionPass::RequireSpecialRegisterIndex(long reg) const {
  if (LEPUS_UNLIKELY(reg >= Register::kMaxRegistersLimit)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass special register index "
        "exceeds 255");
  }
  return static_cast<uint8_t>(reg);
}

long InstructionSelectionPass::ResolveUpvalueToplevelRegister(
    FuncOp* current_func, uint8_t index, const char* inst_name) const {
  auto toplevel_reg = current_func->GetClosureVarToplevelReg(index);
  if (toplevel_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        (std::string("Lepus IR error: ") + inst_name +
         " requires resolved toplevel reg mapping")
            .c_str());
  }
  return toplevel_reg;
}

long InstructionSelectionPass::ResolveToplevelClosurePhysicalRegister(
    uint32_t original_closure_reg, const char* inst_name) const {
  auto* mod = ir_ctx_->GetMainMod();
  auto* toplevel_func_op = mod->GetRootFunction();
  if (!toplevel_func_op) {
    throw ::lynx::lepus::CompileException((std::string("Lepus IR error: ") +
                                           inst_name +
                                           " found nullptr toplevel function")
                                              .c_str());
  }

  long physical_reg = original_closure_reg;
  if (!root_func_deopt_) {
    auto closure_value =
        toplevel_func_op->GetClosureVarGivenReg(original_closure_reg);
    if (!closure_value) {
      throw ::lynx::lepus::CompileException((std::string("Lepus IR error: ") +
                                             inst_name +
                                             " found nullptr closure value")
                                                .c_str());
    }

    auto* target_ctx = ir_ctx_->GetTargetContext();
    auto* toplevel_ra = target_ctx->GetRegisterAllocAnalysis(toplevel_func_op);
    if (!toplevel_ra) {
      throw ::lynx::lepus::CompileException(
          (std::string("Lepus IR error: ") + inst_name +
           " found nullptr toplevel register allocator")
              .c_str());
    }
    physical_reg =
        static_cast<long>(toplevel_ra->GetRegister(closure_value).GetIndex());
  }

  RequireSpecialRegisterIndex(physical_reg);
  return physical_reg;
}

void InstructionSelectionPass::GenerateGetUpvalueInst(GetUpvalueInst* inst,
                                                      Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto current_func = inst->GetFunc();
  auto index = GetLiteralUint8Value(
      inst->GetIndex(), "InstructionSelectionPass::GenerateGetUpvalueInst");
  auto toplevel_reg = ResolveUpvalueToplevelRegister(
      current_func, index, "InstructionSelectionPass::GenerateGetUpvalueInst");

  // GetUpvalueInst should only reach VM lowering after
  // UpdateToplevelClosureVarPass has resolved its upvalue index to a concrete
  // root shared slot. In root-function deopt fallback that recorded register is
  // the root-function-deopt slot; otherwise it is the optimized physical
  // register.
  auto encoded_toplevel_reg = RequireSpecialRegisterIndex(toplevel_reg);

  ADD_INSTRUCTION(
      inst, lynx::lepus::Instruction::ABCode(TypeOp_GetToplevelClosureVar,
                                             dst_reg, encoded_toplevel_reg));
}

void InstructionSelectionPass::GenerateSetUpvalueInst(SetUpvalueInst* inst,
                                                      Block* next_bb) {
  auto current_func = inst->GetFunc();
  auto index = GetLiteralUint8Value(
      inst->GetIndex(), "InstructionSelectionPass::GenerateSetUpvalueInst");
  auto src = EncodeValue(inst->GetSrc());
  auto toplevel_reg = ResolveUpvalueToplevelRegister(
      current_func, index, "InstructionSelectionPass::GenerateSetUpvalueInst");
  auto encoded_toplevel_reg = RequireSpecialRegisterIndex(toplevel_reg);

  auto* toplevel_func_op = ir_ctx_->GetMainMod()->GetRootFunction();
  // Only the root function can prove that writing a shared closure slot back
  // to the exact same root register is a no-op.
  if (encoded_toplevel_reg == src && func_op_ == toplevel_func_op) {
    return;
  }
  ADD_INSTRUCTION(
      inst, lynx::lepus::Instruction::ABCode(TypeOp_SetToplevelClosureVar, src,
                                             encoded_toplevel_reg));
}

bool InstructionSelectionPass::TryLowerDeepCloneCall(CallInst* inst,
                                                     uint8_t ret_reg) {
  if (inst->GetNumArguments() != 1 || !ir_ctx_ || !ir_ctx_->GetVMContext()) {
    return false;
  }

  // Preferred path: an earlier pass marks eligible calls.
  if (inst->IsDeepCloneCall()) {
    auto src_reg = EncodeValue(inst->GetArgument(0));
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(TypeOp_DeepClone,
                                                           ret_reg, src_reg));
    return true;
  }

  return false;
}

void InstructionSelectionPass::GenerateGetToplevelClosureVarInst(
    GetToplevelClosureVarInst* inst, Block* next_bb) {
  auto dst = EncodeValue(inst);
  auto original_closure_reg = GetLiteralUint32Value(
      inst->GetClosureReg(),
      "InstructionSelectionPass::GenerateGetToplevelClosureVarInst",
      "original closure reg");
  if (original_closure_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateGetToplevelClosureVarInst found "
        "invalid original closure reg");
  }
  // In the normal path this is a root physical register; in root-function
  // deopt mode it is already the resolved shared slot recorded earlier.
  auto physical_reg = ResolveToplevelClosurePhysicalRegister(
      original_closure_reg,
      "InstructionSelectionPass::GenerateGetToplevelClosureVarInst");
  // Generate GetToplevelClosureVar instruction to read from the physical
  // register
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(
                            TypeOp_GetToplevelClosureVar, dst, physical_reg));
}

void InstructionSelectionPass::GenerateSetToplevelVarInst(
    SetToplevelVarInst* inst, Block* next_bb) {
  auto src = EncodeValue(inst->GetSrc());
  auto toplevel_reg = RequireSpecialRegisterIndex(GetLiteralUint32Value(
      inst->GetToplevelReg(),
      "InstructionSelectionPass::GenerateSetToplevelVarInst", "toplevel reg"));
  if (src != toplevel_reg)
    ADD_INSTRUCTION(
        inst, lynx::lepus::Instruction::ABCode(TypeOp_Move, toplevel_reg, src));
}

void InstructionSelectionPass::GenerateGetToplevelVarInst(
    GetToplevelVarInst* inst, Block* next_bb) {
  auto dst = EncodeValue(inst);
  auto toplevel_reg = RequireSpecialRegisterIndex(GetLiteralUint32Value(
      inst->GetToplevelReg(),
      "InstructionSelectionPass::GenerateGetToplevelVarInst", "toplevel reg"));
  if (toplevel_reg != dst) {
    ADD_INSTRUCTION(
        inst, lynx::lepus::Instruction::ABCode(TypeOp_Move, dst, toplevel_reg));
  }
}

void InstructionSelectionPass::GenerateSetToplevelClosureVarInst(
    SetToplevelClosureVarInst* inst, Block* next_bb) {
  auto src = EncodeValue(inst->GetSrc());
  auto original_closure_reg = GetLiteralUint32Value(
      inst->GetClosureReg(),
      "InstructionSelectionPass::GenerateSetToplevelClosureVarInst",
      "original closure reg");
  if (original_closure_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateSetToplevelClosureVarInst found "
        "invalid original closure reg");
  }
  auto* toplevel_func_op = ir_ctx_->GetMainMod()->GetRootFunction();
  auto physical_reg = ResolveToplevelClosurePhysicalRegister(
      original_closure_reg,
      "InstructionSelectionPass::GenerateSetToplevelClosureVarInst");
  if (physical_reg == src && toplevel_func_op == func_op_) {
    return;
  }
  // Then set the toplevel closure variable to the value in dst
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(
                            TypeOp_SetToplevelClosureVar, src, physical_reg));
}

void InstructionSelectionPass::GenerateCallInst(CallInst* inst,
                                                Block* next_bb) {
  auto ret_reg = EncodeValue(inst);
  auto param_size = inst->GetNumArguments();

  // Special-case: lower `$deepClone(x)` to a dedicated opcode.
  // This avoids a generic function call and lets runtime do a fast shallow
  // clone for array/table, or return `x` for other types.
  if (TryLowerDeepCloneCall(inst, ret_reg)) {
    return;
  }

  auto func_reg = EncodeValue(inst->GetFunction());
  llvh::SmallVector<unsigned, 4> param_regs;

  bool consecutive_params = true;
  for (auto i = 0; i < param_size; i++) {
    [[maybe_unused]] auto param_reg = EncodeValue(inst->GetArgument(i));
    param_regs.push_back(param_reg);
    if (param_reg != func_reg + i + 1) {
      consecutive_params = false;
    }
  }

  if (consecutive_params) {
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_Call, func_reg, param_size, ret_reg));
  } else {
    auto new_instrs_size = (param_size + 3) / 4;
    auto opcode = TypeOp_CallRandom;
    switch (param_size % 4) {
      case 1:
        opcode = TypeOp_CallRandom1;
        break;
      case 2:
        opcode = TypeOp_CallRandom2;
        break;
      case 3:
        opcode = TypeOp_CallRandom3;
        break;
      default:
        break;
    }

    if (param_size == 1) {
      // opt: if there are only one parameter, use Call1 opcode
      opcode = TypeOp_Call1;
      if (param_regs.size() != 1) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: InstructionSelectionPass::GenerateCallInst "
            "expects exactly one param_reg for Call1 lowering");
      }
      ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                                opcode, func_reg, param_regs[0], ret_reg));
    } else {
      ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                                opcode, func_reg, param_size, ret_reg));
      uint32_t op = 0;
      for (auto i = 0; i < new_instrs_size; i++) {
        long a, b, c, d;
        d = param_regs[op++];
        if (op >= param_size) {
          a = b = c = 0;
          ADD_INSTRUCTION(inst,
                          lynx::lepus::Instruction::OpABCCode(d, a, b, c));
          break;
        }
        a = param_regs[op++];
        if (op >= param_size) {
          b = c = 0;
          ADD_INSTRUCTION(inst,
                          lynx::lepus::Instruction::OpABCCode(d, a, b, c));
          break;
        }
        b = param_regs[op++];
        if (op >= param_size) {
          c = 0;
          ADD_INSTRUCTION(inst,
                          lynx::lepus::Instruction::OpABCCode(d, a, b, c));
          break;
        }
        c = param_regs[op++];
        ADD_INSTRUCTION(inst, lynx::lepus::Instruction::OpABCCode(d, a, b, c));
      }
    }
  }
}

void InstructionSelectionPass::GenerateFunctionEndInst(FunctionEndInst* inst,
                                                       Block* next_bb) {
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::Code(OP_PLACEHOLDER));
};

Pass* CreateInstructionSelectionPass(IRContext* ir_ctx) {
  return new InstructionSelectionPass(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
