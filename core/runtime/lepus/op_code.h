// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_OP_CODE_H_
#define CORE_RUNTIME_LEPUS_OP_CODE_H_
namespace lynx {
namespace lepus {
enum TypeOpCode {
  OP_PLACEHOLDER = 0,
#define DEF_OPCODE(x) x,
#define DEF_NEW_OPCODE(x) x,
#include "core/runtime/lepus/lepus_bytecode_def.h"
  TypeOpCount
#undef DEF_OPCODE
#undef DEF_NEW_OPCODE
};

struct Instruction {
  uint32_t op_code_;

  Instruction() : op_code_(0) {}

  Instruction(TypeOpCode op_code, long a, long b, long c) : op_code_(op_code) {
    op_code_ = (op_code_ << 24) | ((static_cast<uint32_t>(a) & 0xFF) << 16) |
               ((static_cast<uint32_t>(b) & 0xFF) << 8) |
               (static_cast<uint32_t>(c) & 0xFF);
  }

  Instruction(TypeOpCode op_code, long a, short b) : op_code_(op_code) {
    op_code_ = (op_code_ << 24) | ((static_cast<uint32_t>(a) & 0xFF) << 16) |
               (static_cast<uint32_t>(b) & 0xFFFF);
  }

  Instruction(TypeOpCode op_code, long a, unsigned short b)
      : op_code_(op_code) {
    op_code_ = (op_code_ << 24) | ((static_cast<uint32_t>(a) & 0xFF) << 16) |
               (static_cast<uint32_t>(b) & 0xFFFF);
  }

  Instruction(long op_code, long a, long b, long c)
      : op_code_(static_cast<uint32_t>(op_code)) {
    op_code_ = (op_code_ << 24) | ((static_cast<uint32_t>(a) & 0xFF) << 16) |
               ((static_cast<uint32_t>(b) & 0xFF) << 8) |
               (static_cast<uint32_t>(c) & 0xFF);
  }

  void RefillsA(long a) {
    op_code_ =
        (op_code_ & 0xFF00FFFF) | ((static_cast<uint32_t>(a) & 0xFF) << 16);
  }

  void RefillsB(long a) {
    op_code_ =
        (op_code_ & 0xFFFF00FF) | ((static_cast<uint32_t>(a) & 0xFF) << 8);
  }

  static Instruction OpABCCode(long d, long a, long b, long c) {
    assert(d <= (long)UINT32_MAX && "for ABCD code, d must be uint32");
    return Instruction(d, a, b, c);
  }

  void RefillsBx(short b) {
    op_code_ = (op_code_ & 0xFFFF0000) | (static_cast<int>(b) & 0xFFFF);
  }

  static Instruction ABCCode(TypeOpCode op, long a, long b, long c) {
    return Instruction(op, a, b, c);
  }

  static Instruction ABCode(TypeOpCode op, long a, long b) {
    return Instruction(op, a, b, 0);
  }

  static Instruction ACode(TypeOpCode op, long a) {
    return Instruction(op, a, 0, 0);
  }

  static Instruction Code(TypeOpCode op) { return Instruction(op, 0, 0, 0); }

  static Instruction ABxCode(TypeOpCode op, long a, long b) {
    return Instruction(op, a, static_cast<unsigned short>(b));
  }

  inline static long GetOpCode(Instruction i) {
    return (i.op_code_ >> 24) & 0xFF;
  }

  inline static long GetParamA(Instruction i) {
    return (i.op_code_ >> 16) & 0xFF;
  }

  inline static long GetParamB(Instruction i) {
    return (i.op_code_ >> 8) & 0xFF;
  }

  inline static long GetParamC(Instruction i) { return i.op_code_ & 0xFF; }

  inline static long GetParamsBx(Instruction i) {
    return static_cast<short>(i.op_code_ & 0xFFFF);
  }

  inline static long GetParamBx(Instruction i) {
    return static_cast<unsigned short>(i.op_code_ & 0xFFFF);
  }
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_OP_CODE_H_
