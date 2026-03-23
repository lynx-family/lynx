// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/bytecode_iterator.h"

#include "core/runtime/lepus/ir/ir_base.h"

namespace lynx {
namespace lepus {
namespace ir {

static const struct {
  /// Number of operands.
  uint8_t num_operands;
  /// The type of each operand.
  LepusOperandType LepusOperandType[constants::kInstMaxOperands];
} meta[] = {
    {0, {}},  // placeholder
#define DEF_OPCODE_0(name) {0, {}},
#define DEF_OPCODE_1(name, t1) {1, {LepusOperandType::t1}},
#define DEF_OPCODE_2(name, t1, t2) \
  {2, {LepusOperandType::t1, LepusOperandType::t2}},
#define DEF_OPCODE_3(name, t1, t2, t3) \
  {3, {LepusOperandType::t1, LepusOperandType::t2, LepusOperandType::t3}},

// New opcodes share the same operand encoding scheme.
#define DEF_NEW_OPCODE_0(name) {0, {}},
#define DEF_NEW_OPCODE_1(name, t1) {1, {LepusOperandType::t1}},
#define DEF_NEW_OPCODE_2(name, t1, t2) \
  {2, {LepusOperandType::t1, LepusOperandType::t2}},
#define DEF_NEW_OPCODE_3(name, t1, t2, t3) \
  {3, {LepusOperandType::t1, LepusOperandType::t2, LepusOperandType::t3}},

#include "core/runtime/lepus/lepus_bytecode_def.h"

#undef DEF_NEW_OPCODE_3
#undef DEF_NEW_OPCODE_2
#undef DEF_NEW_OPCODE_1
#undef DEF_NEW_OPCODE_0
};

int BytecodeIterator::NumOperands() const {
  return meta[(unsigned)GetOpcode()].num_operands;
}

LepusOperandType BytecodeIterator::GetOperandType(int index) const {
  if (LEPUS_UNLIKELY(index < 0 || index >= NumOperands())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: BytecodeIterator::GetOperandType index out of range");
  }
  return meta[(unsigned)GetOpcode()].LepusOperandType[index];
}

long BytecodeIterator::GetOperandReg(int index) const {
  if (index == 0) {
    return lynx::lepus::Instruction::GetParamA(*Current());
  } else if (index == 1) {
    return lynx::lepus::Instruction::GetParamB(*Current());
  } else if (index == 2) {
    return lynx::lepus::Instruction::GetParamC(*Current());
  }
  throw ::lynx::lepus::CompileException(
      "Lepus IR error: BytecodeIterator::GetOperandReg only supports index "
      "0..2");
}

long BytecodeIterator::GetOperand0() const {
  return lynx::lepus::Instruction::GetParamA(*Current());
}
long BytecodeIterator::GetOperand1() const {
  return lynx::lepus::Instruction::GetParamB(*Current());
}
long BytecodeIterator::GetOperand2() const {
  return lynx::lepus::Instruction::GetParamC(*Current());
}
long BytecodeIterator::GetOperand1x() const {
  return lynx::lepus::Instruction::GetParamsBx(*Current());
}

int BytecodeIterator::JumpOffset() const {
  LepusOpcode opcode = GetOpcode();
  switch (opcode) {
    case LepusOpcode::OP_TypeOp_Jmp:
    case LepusOpcode::OP_TypeOp_JmpFalse:
    case LepusOpcode::OP_TypeOp_JmpTrue:
    case LepusOpcode::OP_TypeOp_BoolJmpFalse:
    case LepusOpcode::OP_TypeOp_BoolJmpTrue:
      return GetOperand1x();
    case LepusOpcode::OP_TypeOp_EqualJmpFalse:
    case LepusOpcode::OP_TypeOp_EqualJmpTrue:
    case LepusOpcode::OP_TypeOp_UnEqualJmpFalse:
    case LepusOpcode::OP_TypeOp_UnEqualJmpTrue:
      return static_cast<int8_t>(GetOperand1());
    default:
      break;
  }
  return -1;
}

bool Bytecode::IsCallRange(LepusOpcode opcode) {
  return opcode == LepusOpcode::OP_TypeOp_Call;
}

bool Bytecode::IsNewArray(LepusOpcode opcode) {
  return opcode == LepusOpcode::OP_TypeOp_NewArray;
}

bool Bytecode::IsTerminate(LepusOpcode op_code) {
  switch (op_code) {
    case LepusOpcode::OP_TypeOp_Ret:
    case LepusOpcode::OP_TypeLabel_Throw:
    case LepusOpcode::OP_PLACEHOLDER:
      return true;
    default:
      break;
  }
  return false;
}

bool Bytecode::IsSwitch(LepusOpcode op_code) {
  return op_code == LepusOpcode::OP_TypeOp_Switch;
}

bool Bytecode::IsJump(LepusOpcode op_code) {
  switch (op_code) {
    case LepusOpcode::OP_TypeOp_Jmp:
    case LepusOpcode::OP_TypeOp_JmpFalse:
    case LepusOpcode::OP_TypeOp_JmpTrue:
    case LepusOpcode::OP_TypeOp_BoolJmpFalse:
    case LepusOpcode::OP_TypeOp_BoolJmpTrue:
    case LepusOpcode::OP_TypeOp_EqualJmpFalse:
    case LepusOpcode::OP_TypeOp_EqualJmpTrue:
    case LepusOpcode::OP_TypeOp_UnEqualJmpFalse:
    case LepusOpcode::OP_TypeOp_UnEqualJmpTrue:
      return true;
    default:
      break;
  }
  return false;
}

bool Bytecode::IsJumpImm(LepusOpcode op_code) {
  switch (op_code) {
    case LepusOpcode::OP_TypeOp_Jmp:
      return true;
    default:
      break;
  }
  return false;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
