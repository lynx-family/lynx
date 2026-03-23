// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_ITERATOR_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_ITERATOR_H_

#include <cstdint>

#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ArrayRef.h"
#include "core/runtime/lepus/op_code.h"

namespace lynx {
namespace lepus {

struct Instruction;

namespace ir {

enum class LepusOpcode : uint8_t {
  OP_PLACEHOLDER = 0,
#define DEF_OPCODE(name) OP_##name,
#define DEF_NEW_OPCODE(name) OP_##name,
#include "core/runtime/lepus/lepus_bytecode_def.h"
  OP_COUNT = 0xFF
#undef DEF_OPCODE
#undef DEF_NEW_OPCODE
};

enum class LepusOperandType : uint8_t {
#define DEF_OPERAND(name, type) name,
#include "core/runtime/lepus/lepus_bytecode_def.h"
  lepus_last_operand
#undef DEF_OPERAND
};

enum class LepusCmpKind : uint8_t {
  Eq = 0,
  Neq,
  Lt,
  Lte,
  Gt,
  Gte,
};

enum class LepusBinaryOpKind : uint8_t {
  Add = 0,
  Sub,
  Mul,
  Div,
  Pow,
  Mod,
  BitAnd,
  BitOr,
  BitXor,
  And,
  Or,
};

enum class LepusUnaryOpKind : uint8_t {
  Not = 0,
  Pos,
  Neg,
  Inc,
  Dec,
  Typeof,
  BitNot,
};

typedef int64_t LepusOperandValue;

class Bytecode {
 public:
  static bool IsSwitch(LepusOpcode op_code);
  static bool IsJump(LepusOpcode op_code);
  static bool IsTerminate(LepusOpcode op_code);
  static bool IsJumpImm(LepusOpcode op_code);
  static bool IsCallRange(LepusOpcode op_code);
  static bool IsNewArray(LepusOpcode op_code);
};

class BytecodeIterator {
 public:
  BytecodeIterator() : start_(nullptr), end_(nullptr), ptr_(nullptr) {}

  BytecodeIterator(lynx::lepus::Instruction* start,
                   lynx::lepus::Instruction* end)
      : start_(start), end_(end), ptr_(start) {}

  void Reset() { ptr_ = start_; }

  void Reset(lynx::lepus::Instruction* start, lynx::lepus::Instruction* end,
             llvh::ArrayRef<int64_t> line_cols) {
    this->start_ = start;
    this->end_ = end;
    this->ptr_ = start;
    this->line_cols_ = line_cols;
  }

  void SkipTo(int offset) { ptr_ = start_ + offset; }

  void Next() { ptr_++; }

  inline bool Done() const { return ptr_ >= end_; }

  inline int GetCurrentOffset() const {
    return static_cast<int>(ptr_ - start_);
  }

  inline int64_t GetCurrentLineCol() const {
    if (LEPUS_UNLIKELY(line_cols_.empty())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BytecodeIterator::GetCurrentLineCol requires "
          "non-null line_cols");
    }
    return line_cols_[GetCurrentOffset()];
  }

  inline LepusOpcode GetOpcode() const {
    return static_cast<LepusOpcode>(lynx::lepus::Instruction::GetOpCode(*ptr_));
  }

  lynx::lepus::Instruction* Current() const { return ptr_; }

  int NumOperands() const;

  LepusOperandType GetOperandType(int index) const;

  int JumpOffset() const;
  long GetOperand0() const;
  long GetOperand1() const;
  long GetOperand2() const;
  long GetOperand1x() const;
  long GetOperandReg(int index) const;

 private:
  lynx::lepus::Instruction* start_;
  lynx::lepus::Instruction* end_;
  lynx::lepus::Instruction* ptr_;

  llvh::ArrayRef<int64_t> line_cols_{};
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_ITERATOR_H_
