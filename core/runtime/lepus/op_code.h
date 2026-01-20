// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_OP_CODE_H_
#define CORE_RUNTIME_LEPUS_OP_CODE_H_
namespace lynx {
namespace lepus {
#define OPCODE_LIST(V)                                                         \
  V(TypeOp_LoadNil)    /* A    A: register*/                                   \
  V(TypeOp_LoadConst)  /* ABx  A: register Bx: const index*/                   \
  V(TypeOp_Move)       /* AB   A: dst register B: src register*/               \
  V(TypeOp_GetUpvalue) /* AB   A: register B: upvalue index*/                  \
  V(TypeOp_SetUpvalue) /* AB   A: register B: upvalue index*/                  \
  V(TypeOp_GetGlobal)  /* ABx  A: value register Bx: const index*/             \
  V(TypeOp_SetGlobal)  /* ABx  A: value register Bx: const index*/             \
  V(TypeOp_Closure)    /* ABx  A: register Bx: proto index*/                   \
  V(TypeOp_Call) /* ABC  A: register B: arg value count + 1 C: expected result \
                    count + 1*/                                                \
  V(TypeOp_Ret)  /* AsBx A: return value start register sBx: return value      \
                    count*/                                                    \
  V(TypeOp_JmpFalse)  /* AsBx A: register sBx: diff of instruction index*/     \
  V(TypeOp_Jmp)       /* sBx  sBx: diff of instruction index*/                 \
  V(TypeOp_Neg)       /* A    A: operand register and dst register*/           \
  V(TypeOp_Not)       /* A    A: operand register and dst register*/           \
  V(TypeOp_Len)       /* A    A: operand register and dst register*/           \
  V(TypeOp_Add)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Sub)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Mul)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Div)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Pow)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Mod)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_And)       /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Or)        /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Less)      /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Greater)   /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_Equal)     /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_UnEqual)   /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_LessEqual) /* ABC  A: dst register B: operand1 register C: operand2 \
                         register*/                                            \
  V(TypeOp_GreaterEqual) /* ABC  A: dst register B: operand1 register C:       \
                            operand2 register*/                                \
  V(TypeOp_NewTable)     /* A    A: register of table*/                        \
  V(TypeOp_SetTable)     /* ABC  A: register of table B: key register C: value \
                            register*/                                         \
  V(TypeOp_GetTable)     /* ABC  A: register of table B: key register C: value \
                            register*/                                         \
  V(TypeOp_Switch)                                                             \
  V(TypeOp_Inc)                                                                \
  V(TypeOp_Dec)                                                                \
  V(TypeOp_Noop)                                                               \
  V(TypeOp_NewArray)   /* ABC  A: register B: value count + 1*/                \
  V(TypeOp_GetBuiltin) /* ABx  A: value register Bx: const index*/             \
  V(TypeOp_Typeof)                                                             \
  V(TypeOp_SetCatchId)                                                         \
  V(TypeLabel_Throw)                                                           \
  V(TypeLabel_Catch)                                                           \
  V(TypeOp_BitOr)                                                              \
  V(TypeOp_BitAnd)                                                             \
  V(TypeOp_BitXor)                                                             \
  V(TypeOp_BitNot)                                                             \
  V(TypeOp_Pos)                                                                \
  V(TypeOp_CreateContext)                                                      \
  V(TypeOp_SetContextSlotMove)                                                 \
  V(TypeOp_GetContextSlotMove)                                                 \
  V(TypeOp_PushContext)                                                        \
  V(TypeOp_PopContext)                                                         \
  V(TypeOp_GetContextSlot)                                                     \
  V(TypeOp_SetContextSlot)                                                     \
  V(TypeOp_AbsUnEqual)                                                         \
  V(TypeOp_AbsEqual)                                                           \
  V(TypeOp_JmpTrue)                                                            \
  V(TypeLabel_EnterBlock)                                                      \
  V(TypeLabel_LeaveBlock)                                                      \
  V(TypeOp_CreateBlockContext)

enum TypeOpCode {
  OP_PLACEHOLDER = 0,
#define DECLARE(x) x,
  OPCODE_LIST(DECLARE) TypeOpCount
#undef DECLARE
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

  void RefillsA(long a) {
    op_code_ =
        (op_code_ & 0xFF00FFFF) | ((static_cast<uint32_t>(a) & 0xFF) << 16);
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
