// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_OP_CODE_H_
#define CORE_RUNTIME_LEPUS_OP_CODE_H_

#include <cstdint>

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

#if defined(LEPUS_ENABLE_CODEGEN)
  Instruction() : op_code_(0) {}

  // Bytecode building helpers.
  //
  // Definitions live in `op_code_builder.h` so VM-only compilation units that
  // include `op_code.h` don't pull in bytecode-generation-only helpers.
  Instruction(TypeOpCode op_code, long a, long b, long c);
  Instruction(TypeOpCode op_code, long a, short b);
  Instruction(TypeOpCode op_code, long a, unsigned short b);
  Instruction(long op_code, long a, long b, long c);

  void RefillsA(long a);
  void RefillsB(long b);
  void RefillsBx(short b);

  static Instruction OpABCCode(long d, long a, long b, long c);
  static Instruction ABCCode(TypeOpCode op, long a, long b, long c);
  static Instruction ABCode(TypeOpCode op, long a, long b);
  static Instruction ACode(TypeOpCode op, long a);
  static Instruction Code(TypeOpCode op);
  static Instruction ABxCode(TypeOpCode op, long a, long b);
#endif

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

#if defined(LEPUS_ENABLE_CODEGEN)
// Bring in the inline definitions for bytecode-generation helpers.
#include "core/runtime/lepus/op_code_builder.h"
#endif
#endif  // CORE_RUNTIME_LEPUS_OP_CODE_H_
