// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/instruction_selection.h"

#include <algorithm>
#include <limits>

#include "base/include/value/base_string.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
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

void InstructionSelectionPass::BytecodeGenerateComplete() {
  CompactConstPoolAndRewriteConstIndices();

  lepus_function_->ResetOpcodes(op_codes_, debug_line_col_);
}

void InstructionSelectionPass::Reset(FuncOp* func) {
  relocations_.clear();
  basic_block_map_.clear();
  op_codes_.clear();
  debug_line_col_.clear();
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
  // after ir optimization, upvalues are not used any more.
  lepus_function_->ClearUpvalues();
}

bool InstructionSelectionPass::RunOnFunction(FuncOp* func) {
  Reset(func);
  if (func->GetName() == constants::kDeepCloneName) {
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

    // Replace cmp-jmp at `loc` with a compare writing into reg `a`.
    op_codes_[loc] = lynx::lepus::Instruction::ABCCode(cmp_op, a, a, c);

    // Insert a wide BoolJmp instruction right after.
    op_codes_.insert(op_codes_.begin() + static_cast<long>(loc + 1),
                     lynx::lepus::Instruction::ABxCode(bool_jmp_op, a, 0));
    debug_line_col_.insert(debug_line_col_.begin() + static_cast<long>(loc + 1),
                           debug_line_col_[loc]);

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

void InstructionSelectionPass::GenerateEqCondBranchInst(EqCondBranchInst* inst,
                                                        Block* next_bb) {
  auto left = EncodeValue(inst->GetLeftHandSide());
  auto right = EncodeValue(inst->GetRightHandSide());

  Block* true_bb = inst->GetTrueDest();
  Block* false_bb = inst->GetFalseDest();

  auto loc = GetCurrentOffset();
  if (next_bb == true_bb) {
    // Emit a conditional jump to the 'False' destination and a fall-through to
    // the 'True' side.
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_EqualJmpFalse, left, 0, right));
    RegisterCmpJmp(loc, false_bb);
    return;
  }

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_EqualJmpTrue,
                                                          left, 0, right));
  RegisterCmpJmp(loc, true_bb);

  if (next_bb == false_bb) {
    return;
  }

  loc = GetCurrentOffset();
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(TypeOp_Jmp, 0, 0));
  RegisterJmp(loc, false_bb);
}

void InstructionSelectionPass::GenerateNeqCondBranchInst(
    NeqCondBranchInst* inst, Block* next_bb) {
  auto left = EncodeValue(inst->GetLeftHandSide());
  auto right = EncodeValue(inst->GetRightHandSide());

  Block* true_bb = inst->GetTrueDest();
  Block* false_bb = inst->GetFalseDest();

  auto loc = GetCurrentOffset();
  if (next_bb == true_bb) {
    // Emit a conditional jump to the 'False' destination and a fall-through to
    // the 'True' side.
    ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(
                              TypeOp_UnEqualJmpFalse, left, 0, right));
    RegisterCmpJmp(loc, false_bb);
    return;
  }

  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCCode(TypeOp_UnEqualJmpTrue,
                                                          left, 0, right));
  RegisterCmpJmp(loc, true_bb);

  if (next_bb == false_bb) {
    return;
  }

  loc = GetCurrentOffset();
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABxCode(TypeOp_Jmp, 0, 0));
  RegisterJmp(loc, false_bb);
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

void InstructionSelectionPass::RegisterCmpJmp(uint32_t loc, Block* target_bb) {
  relocations_.push_back({loc, Relocation::RelocationType::CmpJmp, target_bb});
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
    if (res >= Register::kMaxRegistersLimit) {
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
  } else if ((obj_val->GetType()->IsTableType() &&
              key_val->GetType()->IsStringType()) ||
             (obj_val->GetType()->IsAnyType() &&
              key_val->GetType()->IsStringType())) {
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

void InstructionSelectionPass::GenerateGetUpvalueInst(GetUpvalueInst* inst,
                                                      Block* next_bb) {
  auto dst_reg = EncodeValue(inst);
  auto current_func = inst->GetFunc();
  auto index_literal = inst->GetIndex();
  if (!llvh::isa<LiteralUint8>(index_literal)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass::GenerateGetUpvalueInst "
        "expects LiteralUint8 index");
  }
  auto index = llvh::cast<LiteralUint8>(index_literal)->GetValue();

  auto toplevel_reg = current_func->GetClosureVarToplevelReg(index);

  // All upvalues in this context should be toplevel closure variables
  if (toplevel_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass::GenerateGetUpvalueInst "
        "found invalid toplevel_reg");
  }

  ADD_INSTRUCTION(inst,
                  lynx::lepus::Instruction::ABCode(TypeOp_GetToplevelClosureVar,
                                                   dst_reg, toplevel_reg));
}

void InstructionSelectionPass::GenerateSetUpvalueInst(SetUpvalueInst* inst,
                                                      Block* next_bb) {
  auto current_func = inst->GetFunc();
  auto index_literal = inst->GetIndex();
  if (!llvh::isa<LiteralUint8>(index_literal)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass::GenerateSetUpvalueInst "
        "expects LiteralUint8 index");
  }
  auto index = llvh::cast<LiteralUint8>(index_literal)->GetValue();
  auto src = EncodeValue(inst->GetSrc());

  auto toplevel_reg = current_func->GetClosureVarToplevelReg(index);

  // All upvalues in this context should be toplevel closure variables
  if (toplevel_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass::GenerateSetUpvalueInst "
        "found invalid toplevel_reg");
  }

  auto* toplevel_func_op = ir_ctx_->GetMainMod()->GetRootFunction();
  if (toplevel_reg == src && func_op_ == toplevel_func_op) {
    return;
  }
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(
                            TypeOp_SetToplevelClosureVar, src, toplevel_reg));
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
  auto original_closure_reg =
      llvh::cast<LiteralUint32>(inst->GetClosureReg())->GetValue();
  if (original_closure_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateGetToplevelClosureVarInst found "
        "invalid original closure reg");
  }

  // Get the toplevel function to find the closure variable
  auto* mod = ir_ctx_->GetMainMod();
  auto* toplevel_func_op = mod->GetRootFunction();
  if (!toplevel_func_op) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateGetToplevelClosureVarInst found "
        "nullptr toplevel function");
  }

  // Find the IR Value corresponding to the original closure register
  auto closure_value =
      toplevel_func_op->GetClosureVarGivenReg(original_closure_reg);
  if (!closure_value) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateGetToplevelClosureVarInst found "
        "nullptr closure value");
  }

  // Get the register allocator for toplevel function
  auto toplevel_ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(toplevel_func_op);
  if (!toplevel_ra) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateGetToplevelClosureVarInst found "
        "nullptr toplevel register allocator");
  }

  // Get the physical register assigned to the closure variable after
  // optimization
  auto physical_reg = toplevel_ra->GetRegister(closure_value).GetIndex();

  // Generate GetToplevelClosureVar instruction to read from the physical
  // register
  ADD_INSTRUCTION(inst, lynx::lepus::Instruction::ABCode(
                            TypeOp_GetToplevelClosureVar, dst, physical_reg));
}

void InstructionSelectionPass::GenerateSetToplevelVarInst(
    SetToplevelVarInst* inst, Block* next_bb) {
  auto src = EncodeValue(inst->GetSrc());
  if (!llvh::isa<LiteralUint32>(inst->GetToplevelReg())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass::GenerateSetToplevelVarInst "
        "expects LiteralUint32 toplevel reg");
  }
  auto toplevel_reg =
      llvh::cast<LiteralUint32>(inst->GetToplevelReg())->GetValue();
  if (src != toplevel_reg)
    ADD_INSTRUCTION(
        inst, lynx::lepus::Instruction::ABCode(TypeOp_Move, toplevel_reg, src));
}

void InstructionSelectionPass::GenerateGetToplevelVarInst(
    GetToplevelVarInst* inst, Block* next_bb) {
  auto dst = EncodeValue(inst);
  if (!llvh::isa<LiteralUint32>(inst->GetToplevelReg())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InstructionSelectionPass::GenerateGetToplevelVarInst "
        "expects LiteralUint32 toplevel reg");
  }
  auto toplevel_reg =
      llvh::cast<LiteralUint32>(inst->GetToplevelReg())->GetValue();
  if (toplevel_reg != dst) {
    ADD_INSTRUCTION(
        inst, lynx::lepus::Instruction::ABCode(TypeOp_Move, dst, toplevel_reg));
  }
}

void InstructionSelectionPass::GenerateSetToplevelClosureVarInst(
    SetToplevelClosureVarInst* inst, Block* next_bb) {
  auto src = EncodeValue(inst->GetSrc());

  auto original_closure_reg_val = inst->GetClosureReg();
  if (!llvh::isa<LiteralUint32>(original_closure_reg_val)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateSetToplevelClosureVarInst expects "
        "LiteralUint32 original closure reg");
  }
  auto original_closure_reg =
      llvh::cast<LiteralUint32>(original_closure_reg_val)->GetValue();
  if (original_closure_reg == constants::kInvalidSignedValue) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateSetToplevelClosureVarInst found "
        "invalid original closure reg");
  }

  // Get the toplevel function to find the closure variable
  auto* mod = ir_ctx_->GetMainMod();
  auto* toplevel_func_op = mod->GetRootFunction();
  if (!toplevel_func_op) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateSetToplevelClosureVarInst found "
        "nullptr toplevel function");
  }

  // Find the IR Value corresponding to the original closure register
  auto closure_value =
      toplevel_func_op->GetClosureVarGivenReg(original_closure_reg);
  if (!closure_value) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateSetToplevelClosureVarInst found "
        "nullptr closure value");
  }

  // Get the register allocator for toplevel function
  auto toplevel_ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(toplevel_func_op);
  if (!toplevel_ra) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: "
        "InstructionSelectionPass::GenerateSetToplevelClosureVarInst found "
        "nullptr toplevel register allocator");
  }

  // Get the physical register assigned to the closure variable after
  // optimization
  auto physical_reg = toplevel_ra->GetRegister(closure_value).GetIndex();

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
