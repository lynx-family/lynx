// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/instruction.h"

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {
Instruction::Instruction(Block* parent, OpBuilder* builder,
                         const Instruction* src,
                         llvh::ArrayRef<Value*> operands)
    : Instruction(src->GetKind(), parent, builder, src->GetLocation()) {
  SetType(src->GetType());
  for (auto val : operands) PushOperand(val);
}

void Instruction::PushOperand(Value* val) {
  operands_.push_back({nullptr, 0});
  SetOperand(val, GetNumOperands() - 1);
}

void Instruction::RegisterRequiredAttr() {
  RegisterAttr(SpecificAttr::SA_ClosureVarReg,
               Attributes::GetUnsignedAttr(UnsignedAttrEntry,
                                           constants::kInvalidSignedValue));
  RegisterAttr(SpecificAttr::SA_ChildrenIndex,
               Attributes::GetUnsignedAttr(UnsignedAttrEntry,
                                           constants::kInvalidSignedValue));
  RegisterAttr(SpecificAttr::SA_ToplevelVarReg,
               Attributes::GetUnsignedAttr(UnsignedAttrEntry,
                                           constants::kInvalidSignedValue));
  RegisterAttr(SpecificAttr::SA_FixReg,
               Attributes::GetBoolAttr(BoolAttrEntry, false));
}

void Instruction::SetOperand(Value* val, unsigned index) {
  if (LEPUS_UNLIKELY(index >= operands_.size())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Instruction::SetOperand index out of range (operands "
        "not fully pushed)");
  }

  Value* current_value = operands_[index].first;

  // If the operand is already set then there is nothing to do. The instruction
  // is already registered in the use-list of the value.
  if (current_value == val) {
    return;
  }

  // Remove the current instruction from the old value that we are removing.
  if (current_value) {
    current_value->RemoveUse(operands_[index]);
  }

  // Register this instruction as a user of the new value and set the operand.
  if (val) {
    operands_[index] = val->AddUser(this);
  } else {
    operands_[index] = {nullptr, 0};
  }
}

Value* Instruction::GetOperand(unsigned index) const {
  return operands_[index].first;
}

unsigned Instruction::GetNumOperands() const { return operands_.size(); }

void Instruction::RemoveOperand(unsigned index) {
  // We call to SetOperand before deleting the operand because SetOperand
  // un-registers the user from the user list.
  SetOperand(nullptr, index);
  operands_.erase(operands_.begin() + index);
}

void Instruction::ReplaceFirstOperandWith(Value* old_value, Value* new_value) {
  for (int i = 0, e = GetNumOperands(); i < e; i++) {
    if (old_value == GetOperand(i)) {
      SetOperand(new_value, i);
      return;
    }
  }
  throw ::lynx::lepus::CompileException(
      "Lepus IR error: Instruction::ReplaceFirstOperandWith can't find old "
      "operand in use-def chain");
}

void Instruction::MoveBefore(Instruction* later) {
  if (this == later) return;

  RemoveFromParent();
  later->GetParent()->Insert(Block::iterator(later), this);
}

std::string Instruction::GetName() const { return GetBaseName(); }

SideEffect Instruction::GetSideEffect() const {
  switch (GetKind()) {
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Instruction::GetSideEffect encountered invalid "
          "ValueKind");
#define DEF_VALUE(XX, PARENT) \
  case ValueKind::XX##Kind:   \
    return llvh::cast<XX>(this)->GetSideEffectImpl();
#define DEF_TAG(XX, PARENT) \
  case ValueKind::XX##Kind: \
    return llvh::cast<PARENT>(this)->GetSideEffectImpl();
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/instrs.def"
#undef ENABLE_MIR_INSTR
  }

  return SideEffect{};
}

bool Instruction::HasOutput() const {
  switch (GetKind()) {
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Instruction::HasOutput encountered invalid "
          "ValueKind");
#define DEF_VALUE(XX, PARENT) \
  case ValueKind::XX##Kind:   \
    return XX::HasOutput();
#define DEF_TAG(XX, PARENT) \
  case ValueKind::XX##Kind: \
    return PARENT::HasOutput();

#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/instrs.def"
#undef ENABLE_MIR_INSTR
  }

  return false;
}

}  // namespace ir

}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::Instruction>::deleteNode(
    ::lynx::lepus::ir::Instruction* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
