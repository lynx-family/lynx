// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_OP_CODE_BUILDER_H_
#define CORE_RUNTIME_LEPUS_OP_CODE_BUILDER_H_

#include <cassert>
#include <cstdint>

#include "core/runtime/lepus/op_code.h"

namespace lynx {
namespace lepus {

// Helpers for bytecode generation / compilation.
//
// Keep these APIs out of `op_code.h` so that VM-only compilation units that
// include `vm_context.h` (and transitively `function.h`) don't pull in
// bytecode-generation-only helpers.
struct InstructionBuilder {
  static inline Instruction OpABCCode(long d, long a, long b, long c) {
    assert(d >= 0 && d <= static_cast<long>(UINT32_MAX) &&
           "for ABCD code, d must be uint32");
    Instruction inst;
    inst.op_code_ = (static_cast<uint32_t>(d) << 24) |
                    ((static_cast<uint32_t>(a) & 0xFF) << 16) |
                    ((static_cast<uint32_t>(b) & 0xFF) << 8) |
                    (static_cast<uint32_t>(c) & 0xFF);
    return inst;
  }

  static inline Instruction ABCCode(TypeOpCode op, long a, long b, long c) {
    return OpABCCode(static_cast<long>(op), a, b, c);
  }

  static inline Instruction ABCode(TypeOpCode op, long a, long b) {
    return ABCCode(op, a, b, 0);
  }

  static inline Instruction ACode(TypeOpCode op, long a) {
    return ABCCode(op, a, 0, 0);
  }

  static inline Instruction Code(TypeOpCode op) { return ABCCode(op, 0, 0, 0); }

  static inline Instruction ABxCode(TypeOpCode op, long a, long b) {
    Instruction inst;
    inst.op_code_ =
        (static_cast<uint32_t>(op) << 24) |
        ((static_cast<uint32_t>(a) & 0xFF) << 16) |
        (static_cast<uint32_t>(static_cast<unsigned short>(b)) & 0xFFFF);
    return inst;
  }

  static inline void RefillsA(Instruction* inst, long a) {
    inst->op_code_ = (inst->op_code_ & 0xFF00FFFF) |
                     ((static_cast<uint32_t>(a) & 0xFF) << 16);
  }

  static inline void RefillsB(Instruction* inst, long b) {
    inst->op_code_ = (inst->op_code_ & 0xFFFF00FF) |
                     ((static_cast<uint32_t>(b) & 0xFF) << 8);
  }

  static inline void RefillsBx(Instruction* inst, short bx) {
    inst->op_code_ =
        (inst->op_code_ & 0xFFFF0000) |
        (static_cast<uint32_t>(static_cast<uint16_t>(bx)) & 0xFFFF);
  }
};

// Definitions for `Instruction` bytecode-building APIs.
//
// Note: these are intentionally defined here (instead of in `op_code.h`) so
// VM-only code won't pick up the inline bodies.
inline Instruction::Instruction(TypeOpCode op_code, long a, long b, long c)
    : op_code_(0) {
  op_code_ = (static_cast<uint32_t>(op_code) << 24) |
             ((static_cast<uint32_t>(a) & 0xFF) << 16) |
             ((static_cast<uint32_t>(b) & 0xFF) << 8) |
             (static_cast<uint32_t>(c) & 0xFF);
}

inline Instruction::Instruction(TypeOpCode op_code, long a, short b)
    : op_code_(0) {
  op_code_ = (static_cast<uint32_t>(op_code) << 24) |
             ((static_cast<uint32_t>(a) & 0xFF) << 16) |
             (static_cast<uint32_t>(static_cast<uint16_t>(b)) & 0xFFFF);
}

inline Instruction::Instruction(TypeOpCode op_code, long a, unsigned short b)
    : op_code_(0) {
  op_code_ = (static_cast<uint32_t>(op_code) << 24) |
             ((static_cast<uint32_t>(a) & 0xFF) << 16) |
             (static_cast<uint32_t>(b) & 0xFFFF);
}

inline Instruction::Instruction(long op_code, long a, long b, long c)
    : op_code_(0) {
  op_code_ = (static_cast<uint32_t>(op_code) << 24) |
             ((static_cast<uint32_t>(a) & 0xFF) << 16) |
             ((static_cast<uint32_t>(b) & 0xFF) << 8) |
             (static_cast<uint32_t>(c) & 0xFF);
}

inline void Instruction::RefillsA(long a) {
  InstructionBuilder::RefillsA(this, a);
}

inline void Instruction::RefillsB(long b) {
  InstructionBuilder::RefillsB(this, b);
}

inline void Instruction::RefillsBx(short bx) {
  InstructionBuilder::RefillsBx(this, bx);
}

inline Instruction Instruction::OpABCCode(long d, long a, long b, long c) {
  return InstructionBuilder::OpABCCode(d, a, b, c);
}

inline Instruction Instruction::ABCCode(TypeOpCode op, long a, long b, long c) {
  return InstructionBuilder::ABCCode(op, a, b, c);
}

inline Instruction Instruction::ABCode(TypeOpCode op, long a, long b) {
  return InstructionBuilder::ABCode(op, a, b);
}

inline Instruction Instruction::ACode(TypeOpCode op, long a) {
  return InstructionBuilder::ACode(op, a);
}

inline Instruction Instruction::Code(TypeOpCode op) {
  return InstructionBuilder::Code(op);
}

inline Instruction Instruction::ABxCode(TypeOpCode op, long a, long b) {
  return InstructionBuilder::ABxCode(op, a, b);
}

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_OP_CODE_BUILDER_H_
