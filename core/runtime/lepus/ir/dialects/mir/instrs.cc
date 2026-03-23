// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"

#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/Hashing.h"
#include "core/runtime/lepus/ir/type_op.h"

namespace lynx {
namespace lepus {
namespace ir {

BranchInst::BranchInst(Block* parent, OpBuilder* builder, int64_t location,
                       Block* dest)
    : TerminatorInst(ValueKind::BranchInstKind, parent, builder, location) {
  PushOperand(dest);
}

CondBranchInst::CondBranchInst(Block* parent, OpBuilder* builder,
                               int64_t location, Value* cond, Block* true_block,
                               Block* false_block)
    : TerminatorInst(ValueKind::CondBranchInstKind, parent, builder, location) {
  RegisterAttr(SpecificAttr::SA_SmallJmp,
               Attributes::GetBoolAttr(BoolAttrEntry, false));
  PushOperand(cond);
  PushOperand(true_block);
  PushOperand(false_block);
}

EqCondBranchInst::EqCondBranchInst(Block* parent, OpBuilder* builder,
                                   int64_t location, Value* left, Value* right,
                                   Block* true_block, Block* false_block)
    : TerminatorInst(ValueKind::EqCondBranchInstKind, parent, builder,
                     location) {
  PushOperand(left);
  PushOperand(right);
  PushOperand(true_block);
  PushOperand(false_block);
}

NeqCondBranchInst::NeqCondBranchInst(Block* parent, OpBuilder* builder,
                                     int64_t location, Value* left,
                                     Value* right, Block* true_block,
                                     Block* false_block)
    : TerminatorInst(ValueKind::NeqCondBranchInstKind, parent, builder,
                     location) {
  PushOperand(left);
  PushOperand(right);
  PushOperand(true_block);
  PushOperand(false_block);
}

BinaryOperatorInst::BinaryOperatorInst(Block* parent, OpBuilder* builder,
                                       int64_t location, Value* left,
                                       Value* right, ValueKind kind,
                                       TypeOp* ret_type)
    : Instruction(kind, parent, builder, location) {
  PushOperand(left);
  PushOperand(right);
  if (ret_type->IsAnyType()) {
    SelectType(left->GetType(), right->GetType(), kind);
  } else {
    SetType(ret_type);
  }
}

void BinaryOperatorInst::SelectType(TypeOp* left_ty, TypeOp* right_ty,
                                    ValueKind kind) {
  auto* builder = GetBuilder();
  // for addInst, if one of the operands is string type, the result type is
  // string type.
  if (kind == ValueKind::BinaryAddInstKind &&
      (left_ty->IsStringType() || right_ty->IsStringType())) {
    SetType(TypeOp::CreateString(builder));
    return;
  }

  switch (kind) {
    case ValueKind::BinaryStrictlyEqualInstKind:
    case ValueKind::BinaryStrictlyNotEqualInstKind:
    case ValueKind::BinaryGreaterThanInstKind:
    case ValueKind::BinaryLessThanInstKind:
    case ValueKind::BinaryGreaterThanOrEqualInstKind:
    case ValueKind::BinaryLessThanOrEqualInstKind:
      SetType(TypeOp::CreateBoolean(builder));
      return;
    default:
      break;
  }

  // If any side is still generic, keep the result generic.
  // (Except the string-add fast path above, which is still safe because isel
  // uses a dedicated AddStringAny opcode for string + any.)
  if (left_ty->IsAnyType() || right_ty->IsAnyType()) {
    SetType(TypeOp::CreateAnyType(builder));
    return;
  }

  switch (kind) {
    case ValueKind::BinaryAddInstKind: {
      if (left_ty->IsStringType()) {
        SetType(TypeOp::CreateString(builder));
      } else if (left_ty->IsInt64Type()) {
        if (right_ty->IsInt64Type()) {
          SetType(TypeOp::CreateInt64(builder));
        } else if (right_ty->IsFloat64Type()) {
          SetType(TypeOp::CreateFloat64(builder));
        } else {
          SetType(TypeOp::CreateString(builder));
        }
      } else if (left_ty->IsFloat64Type()) {
        if (right_ty->IsFloat64Type()) {
          SetType(TypeOp::CreateFloat64(builder));
        } else {
          SetType(TypeOp::CreateString(builder));
        }
      } else {
        SetType(TypeOp::CreateString(builder));
      }
    } break;
    case ValueKind::BinarySubInstKind:
    case ValueKind::BinaryMulInstKind: {
      if (left_ty->IsInt64Type() && right_ty->IsInt64Type()) {
        SetType(TypeOp::CreateInt64(builder));
      } else {
        SetType(TypeOp::CreateFloat64(builder));
      }
    } break;
    case ValueKind::BinaryPowInstKind: {
      if (left_ty->IsInt64Type() && right_ty->IsInt64Type()) {
        SetType(TypeOp::CreateInt64(builder));
      } else if (left_ty->IsNumberType() || right_ty->IsNumberType()) {
        SetType(TypeOp::CreateNumber(builder));
      } else {
        SetType(TypeOp::CreateAnyType(builder));
      }
    } break;
    case ValueKind::BinaryDivInstKind: {
      SetType(TypeOp::CreateAnyType(builder));
    } break;
    case ValueKind::BinaryBitOrInstKind:
    case ValueKind::BinaryBitXorInstKind:
    case ValueKind::BinaryBitAndInstKind: {
      if (left_ty->IsInt64Type() && right_ty->IsInt64Type()) {
        SetType(TypeOp::CreateInt64(builder));
      } else if (left_ty->IsNumberType() || right_ty->IsNumberType()) {
        SetType(TypeOp::CreateInt64(builder));
      } else {
        SetType(TypeOp::CreateAnyType(builder));
      }
    } break;
    case ValueKind::BinaryAndInstKind:
    case ValueKind::BinaryOrInstKind: {
      SetType(TypeOp::CreateAnyType(builder));
    } break;
    case ValueKind::BinaryModInstKind: {
      SetType(TypeOp::CreateNumber(builder));
      break;
    }
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BinaryOperatorInst::SelectType encountered "
          "unsupported ValueKind");
  }
}

SideEffect BinaryOperatorInst::GetBinarySideEffect(TypeOp* left_ty,
                                                   TypeOp* right_ty,
                                                   ValueKind op) {
  switch (op) {
    case ValueKind::BinaryStrictlyEqualInstKind:
    case ValueKind::BinaryStrictlyNotEqualInstKind:
      return SideEffect{}.SetIdempotent();
    case ValueKind::BinaryLessThanInstKind:
    case ValueKind::BinaryLessThanOrEqualInstKind:
    case ValueKind::BinaryGreaterThanInstKind:
    case ValueKind::BinaryGreaterThanOrEqualInstKind:
      return {};
    case ValueKind::BinaryDivInstKind:
    case ValueKind::BinaryModInstKind:
      return SideEffect{}.SetThrow();
    case ValueKind::BinaryAddInstKind:
    case ValueKind::BinarySubInstKind:
    case ValueKind::BinaryMulInstKind:
    case ValueKind::BinaryPowInstKind:
    case ValueKind::BinaryBitAndInstKind:
    case ValueKind::BinaryBitOrInstKind:
    case ValueKind::BinaryBitXorInstKind:
    case ValueKind::BinaryAndInstKind:
    case ValueKind::BinaryOrInstKind:
      return SideEffect{}.SetIdempotent();
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BinaryOperatorInst::GetBinarySideEffect encountered "
          "unsupported ValueKind");
  }

  return {};
}

PhiInst::PhiInst(Block* parent, OpBuilder* builder, int64_t location,
                 const ValueListType& values, const BlockListType& blocks)
    : Instruction(ValueKind::PhiInstKind, parent, builder, location) {
  if (LEPUS_UNLIKELY(values.size() != blocks.size())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: PhiInst constructed with mismatched block/value "
        "pairs");
  }
  // Push the incoming values.
  for (int i = 0, e = values.size(); i < e; ++i) {
    PushOperand(values[i]);
    PushOperand(blocks[i]);
  }
  SetType(TypeOp::CreateAnyType(builder));
  RecalculateResultType();
}

unsigned PhiInst::GetNumEntries() const {
  // The PHI operands are just pairs of values and basic blocks.
  return GetNumOperands() / 2;
}

/// Returns the index of the first operand for phi entry pair.
static unsigned IndexOfPhiEntry(unsigned index) { return index * 2; }

std::pair<Value*, Block*> PhiInst::GetEntry(unsigned i) const {
  return std::make_pair(GetOperand(IndexOfPhiEntry(i)),
                        llvh::cast<Block>(GetOperand(IndexOfPhiEntry(i) + 1)));
}

void PhiInst::UpdateEntry(unsigned i, Value* val, Block* bb) {
  SetOperand(val, IndexOfPhiEntry(i));
  SetOperand(bb, IndexOfPhiEntry(i) + 1);
  RecalculateResultType();
}

void PhiInst::ForceUpdateEntry(unsigned i, Value* val, Block* bb) {
  SetOperand(val, IndexOfPhiEntry(i));
  SetOperand(bb, IndexOfPhiEntry(i) + 1);
}

void PhiInst::AddEntry(Value* val, Block* bb) {
  PushOperand(val);
  PushOperand(bb);
}

void PhiInst::RemoveEntry(unsigned index) {
  RemoveEntryHelper(index);
  RecalculateResultType();
}

void PhiInst::RemoveEntry(Block* bb) {
  bool need_recalc = false;
  unsigned i = 0;
  // For each one of the entries:
  while (i < GetNumEntries()) {
    // If this entry is from the bb we want to remove, then remove it.
    if (GetEntry(i).second == bb) {
      RemoveEntryHelper(i);
      need_recalc = true;
      // keep the current iteration index.
      continue;
    }
    // Else, move to the next entry.
    i++;
  }
  if (need_recalc) RecalculateResultType();
}

void PhiInst::RemoveEntryHelper(unsigned index) {
  // Remove the pair at the right offset. See calculation of GetEntry above.
  unsigned start_idx = IndexOfPhiEntry(index);
  // Remove the value:
  RemoveOperand(start_idx);
  // Remove the basic block. Notice that we use the same index because the
  // list is shifted.
  RemoveOperand(start_idx);
}

void PhiInst::RecalculateResultType() {
  if (LEPUS_UNLIKELY(GetNumEntries() == 0)) return;

  auto* type = GetType();
  if (LEPUS_UNLIKELY(!type->IsValidType())) type = GetEntry(0).first->GetType();

  if (LEPUS_UNLIKELY(!type->IsValidType())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: PhiInst::RecalculateResultType failed to infer a "
        "valid type");
  }

  for (unsigned i = 0, e = GetNumEntries(); i != e; ++i) {
    auto* entry_ty = GetEntry(i).first->GetType();
    if (*type != *entry_ty) {
      type = TypeOp::CreateAnyType(GetBuilder());
      break;
    }
  }
  SetType(type);
}

UnaryOperatorInst::UnaryOperatorInst(Block* parent, OpBuilder* builder,
                                     int64_t location, Value* value,
                                     ValueKind kind)
    : SingleOperandInst(kind, parent, builder, location, value) {
  if (LEPUS_UNLIKELY(!LEPUS_IR_KIND_IN_CLASS(kind, UnaryOperatorInst))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UnaryOperatorInst constructed with non-unary "
        "ValueKind");
  }
  SelectType(value->GetType(), kind);
}

void UnaryOperatorInst::SelectType(TypeOp* value_ty, ValueKind kind) {
  auto* builder = GetBuilder();

  switch (kind) {
    case ValueKind::UnaryNotInstKind:
      SetType(TypeOp::CreateBoolean(builder));
      break;
    case ValueKind::UnaryPosInstKind: {
      // value type is StringType
      if (value_ty->IsStringType()) {
        // nan or int64Type
        SetType(TypeOp::CreateAnyType(builder));
      } else {
        SetType(value_ty);
      }
      break;
    }
    case ValueKind::UnaryNegInstKind:
      if (value_ty->IsInt64Type()) {
        SetType(TypeOp::CreateInt64(builder));
      } else if (value_ty->IsNumberType()) {
        SetType(TypeOp::CreateFloat64(builder));
      } else {
        SetType(TypeOp::CreateAnyType(builder));
      }
      break;
    case ValueKind::UnaryIncInstKind:
    case ValueKind::UnaryDecInstKind: {
      if (value_ty->IsInt64Type()) {
        SetType(TypeOp::CreateInt64(builder));
      } else if (value_ty->IsFloat64Type()) {
        SetType(TypeOp::CreateFloat64(builder));
      } else {
        SetType(TypeOp::CreateAnyType(builder));
      }
    } break;
    case ValueKind::UnaryTypeofInstKind:
      SetType(TypeOp::CreateString(builder));
      break;
    case ValueKind::UnaryBitNotInstKind:
      SetType(TypeOp::CreateInt64(builder));
      break;
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: UnaryOperatorInst::SelectType encountered "
          "unsupported ValueKind");
  }
}

llvh::hash_code Instruction::GetHashCode() const {
  llvh::hash_code hc =
      llvh::hash_combine((unsigned)GetKind(), GetNumOperands());

  // Check operands.
  for (unsigned i = 0, e = GetNumOperands(); i != e; ++i)
    hc = llvh::hash_combine(hc, GetOperand(i));

  // Hash in any special attributes for an instruction.
  return hc;
}

bool Instruction::IsIdenticalTo(const Instruction* rhs) const {
  // Check if both instructions have the same kind and number of operands.
  // This should filter out most cases.
  if (GetKind() != rhs->GetKind() || GetNumOperands() != rhs->GetNumOperands())
    return false;

  // Check operands.
  for (unsigned i = 0, e = GetNumOperands(); i != e; ++i)
    if (GetOperand(i) != rhs->GetOperand(i)) return false;

  return true;
}

void SetToplevelVarInst::SetToplevelReg(Literal* reg) {
  SetOperand(reg, ToplevelRegIdx);
}

void GetToplevelVarInst::SetToplevelReg(Literal* reg) { SetOperand(reg, 0); }
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
