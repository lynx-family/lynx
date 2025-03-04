// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_OP_CODE_H_
#define CORE_RUNTIME_VM_LEPUS_OP_CODE_H_
namespace lynx {
namespace lepus {
enum TypeOpCode {
  TypeOp_LoadNil = 1,  // A    A: register
  TypeOp_LoadConst,    // ABx  A: register Bx: const index
  TypeOp_Move,         // AB   A: dst register B: src register
  TypeOp_GetUpvalue,   // AB   A: register B: upvalue index
  TypeOp_SetUpvalue,   // AB   A: register B: upvalue index
  TypeOp_GetGlobal,    // ABx  A: value register Bx: const index
  TypeOp_SetGlobal,    // ABx  A: value register Bx: const index

  TypeOp_Closure,  // ABx  A: register Bx: proto index
  TypeOp_Call,     // ABC  A: register B: arg value count + 1 C: expected result
                   // count + 1
  TypeOp_Ret,  // AsBx A: return value start register sBx: return value count
  TypeOp_JmpFalse,  // AsBx A: register sBx: diff of instruction index
  TypeOp_Jmp,       // sBx  sBx: diff of instruction index
  TypeOp_Neg,       // A    A: operand register and dst register
  TypeOp_Not,       // A    A: operand register and dst register
  TypeOp_Len,       // A    A: operand register and dst register
  TypeOp_Add,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Sub,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Mul,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Div,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Pow,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Mod,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_And,  // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Or,   // ABC  A: dst register B: operand1 register C: operand2 register
  TypeOp_Less,          // ABC  A: dst register B: operand1 register C: operand2
                        // register
  TypeOp_Greater,       // ABC  A: dst register B: operand1 register C: operand2
                        // register
  TypeOp_Equal,         // ABC  A: dst register B: operand1 register C: operand2
                        // register
  TypeOp_UnEqual,       // ABC  A: dst register B: operand1 register C: operand2
                        // register
  TypeOp_LessEqual,     // ABC  A: dst register B: operand1 register C: operand2
                        // register
  TypeOp_GreaterEqual,  // ABC  A: dst register B: operand1 register C: operand2
                        // register
  TypeOp_NewTable,      // A    A: register of table
  TypeOp_SetTable,      // ABC  A: register of table B: key register C: value
                        // register
  TypeOp_GetTable,      // ABC  A: register of table B: key register C: value
                        // register
  TypeOp_Switch,
  TypeOp_Inc,
  TypeOp_Dec,
  TypeOp_Noop,
  TypeOp_NewArray,    // ABC  A: register B: value count + 1
  TypeOp_GetBuiltin,  // ABx  A: value register Bx: const index
  TypeOp_Typeof,
  TypeOp_SetCatchId,
  TypeLabel_Throw,
  TypeLabel_Catch,
  TypeOp_BitOr,
  TypeOp_BitAnd,
  TypeOp_BitXor,
  TypeOp_BitNot,
  TypeOp_Pos,
  TypeOp_CreateContext,
  TypeOp_SetContextSlotMove,
  TypeOp_GetContextSlotMove,
  TypeOp_PushContext,
  TypeOp_PopContext,
  TypeOp_GetContextSlot,
  TypeOp_SetContextSlot,
  TypeOp_AbsUnEqual,
  TypeOp_AbsEqual,
  TypeOp_JmpTrue,
  TypeLabel_EnterBlock,
  TypeLabel_LeaveBlock,
  TypeOp_CreateBlockContext,
};

struct Instruction {
  unsigned long op_code_;

  Instruction() : op_code_(0) {}

  Instruction(TypeOpCode op_code, long a, long b, long c) : op_code_(op_code) {
    op_code_ =
        (op_code_ << 24) | ((a & 0xFF) << 16) | ((b & 0xFF) << 8) | (c & 0xFF);
  }

  Instruction(TypeOpCode op_code, long a, short b) : op_code_(op_code) {
    op_code_ =
        (op_code_ << 24) | ((a & 0xFF) << 16) | (static_cast<int>(b) & 0xFFFF);
  }

  Instruction(TypeOpCode op_code, long a, unsigned short b)
      : op_code_(op_code) {
    op_code_ =
        (op_code_ << 24) | ((a & 0xFF) << 16) | (static_cast<int>(b) & 0xFFFF);
  }

  void RefillsA(long a) {
    op_code_ = (op_code_ & 0xFF00FFFF) | ((a & 0xFF) << 16);
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

#endif  // CORE_RUNTIME_VM_LEPUS_OP_CODE_H_
