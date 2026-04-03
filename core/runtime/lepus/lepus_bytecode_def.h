// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEF_OPCODE
#define DEF_OPCODE(...)
#endif
#ifndef DEF_NEW_OPCODE
#define DEF_NEW_OPCODE(...)
#endif
#ifndef DEF_OPERAND
#define DEF_OPERAND(...)
#endif

#ifndef DEF_OPCODE_0
#define DEF_OPCODE_0(name) DEF_OPCODE(name)
#endif
#ifndef DEF_OPCODE_1
#define DEF_OPCODE_1(name, ...) DEF_OPCODE(name)
#endif
#ifndef DEF_OPCODE_2
#define DEF_OPCODE_2(name, ...) DEF_OPCODE(name)
#endif
#ifndef DEF_OPCODE_3
#define DEF_OPCODE_3(name, ...) DEF_OPCODE(name)
#endif

#ifndef DEF_NEW_OPCODE_0
#define DEF_NEW_OPCODE_0(name) DEF_NEW_OPCODE(name)
#endif
#ifndef DEF_NEW_OPCODE_1
#define DEF_NEW_OPCODE_1(name, ...) DEF_NEW_OPCODE(name)
#endif
#ifndef DEF_NEW_OPCODE_2
#define DEF_NEW_OPCODE_2(name, ...) DEF_NEW_OPCODE(name)
#endif
#ifndef DEF_NEW_OPCODE_3
#define DEF_NEW_OPCODE_3(name, ...) DEF_NEW_OPCODE(name)
#endif

DEF_OPERAND(SrcReg, uint8_t)
DEF_OPERAND(DstReg, uint8_t)
DEF_OPERAND(SrcDstReg, uint8_t)
DEF_OPERAND(UInt8, uint8_t)
DEF_OPERAND(DstUInt8, uint8_t)
DEF_OPERAND(UInt16, uint16_t)

// A    A: register
DEF_OPCODE_2(TypeOp_LoadNil, DstReg, UInt8)
// ABx  A: register Bx: const index
DEF_OPCODE_2(TypeOp_LoadConst, DstReg, UInt16)
// AB   A: dst register B: src register
DEF_OPCODE_2(TypeOp_Move, DstReg, SrcReg)
// AB   A: register B: upvalue index
DEF_OPCODE_2(TypeOp_GetUpvalue, DstReg, UInt8)
// AB   A: src register B: upvalue index
DEF_OPCODE_2(TypeOp_SetUpvalue, SrcReg, UInt8)
// ABx  A: value register Bx: const index
DEF_OPCODE_2(TypeOp_GetGlobal, DstReg, UInt16)
DEF_OPCODE_0(TypeOp_SetGlobal)
// ABx  A: register Bx: proto index
DEF_OPCODE_2(TypeOp_Closure, DstReg, UInt16)
// ABC  A: register B: arg value count + 1 C: expected result count + 1
DEF_OPCODE_3(TypeOp_Call, SrcReg, UInt8, DstReg)
// AsBx A: return value start register sBx: return value count
DEF_OPCODE_1(TypeOp_Ret, SrcReg)
// AsBx A: register sBx: diff of instruction index
DEF_OPCODE_2(TypeOp_JmpFalse, SrcReg, UInt16)
// sBx  sBx: diff of instruction index
DEF_OPCODE_1(TypeOp_Jmp, UInt16)
// A    A: operand register and dst register
DEF_OPCODE_1(TypeOp_Neg, SrcDstReg)
// A    A: operand register and dst register
DEF_OPCODE_1(TypeOp_Not, SrcDstReg)
// A    A: operand register and dst register
DEF_OPCODE_0(TypeOp_Len)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Add, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Sub, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Mul, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Div, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Pow, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Mod, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_And, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Or, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Less, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Greater, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_Equal, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_UnEqual, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_LessEqual, DstReg, SrcReg, SrcReg)
// ABC  A: dst register B: operand1 register C: operand2 register
DEF_OPCODE_3(TypeOp_GreaterEqual, DstReg, SrcReg, SrcReg)
// A    A: register of table
DEF_OPCODE_1(TypeOp_NewTable, DstReg)
// ABC  A: register of table B: key register C: value register
DEF_OPCODE_3(TypeOp_SetTable, SrcReg, SrcReg, SrcReg)
// ABC  A: register of table B: key register C: value register
DEF_OPCODE_3(TypeOp_GetTable, DstReg, SrcReg, SrcReg)
DEF_OPCODE_2(TypeOp_Switch, SrcReg, UInt16)
DEF_OPCODE_1(TypeOp_Inc, SrcDstReg)
DEF_OPCODE_1(TypeOp_Dec, SrcDstReg)
DEF_OPCODE_0(TypeOp_Noop)
// ABC  A: register B: value count + 1
DEF_OPCODE_2(TypeOp_NewArray, DstReg, UInt8)
// ABx  A: value register Bx: const index
DEF_OPCODE_2(TypeOp_GetBuiltin, DstReg, UInt16)
DEF_OPCODE_1(TypeOp_Typeof, SrcDstReg)
DEF_OPCODE_1(TypeOp_SetCatchId, DstReg)
DEF_OPCODE_1(TypeLabel_Throw, SrcReg)
DEF_OPCODE_0(TypeLabel_Catch)
DEF_OPCODE_3(TypeOp_BitOr, DstReg, SrcReg, SrcReg)
DEF_OPCODE_3(TypeOp_BitAnd, DstReg, SrcReg, SrcReg)
DEF_OPCODE_3(TypeOp_BitXor, DstReg, SrcReg, SrcReg)
DEF_OPCODE_1(TypeOp_BitNot, SrcDstReg)
DEF_OPCODE_1(TypeOp_Pos, SrcDstReg)
DEF_OPCODE_2(TypeOp_CreateContext, DstReg, UInt8)
DEF_OPCODE_3(TypeOp_SetContextSlotMove, SrcReg, UInt8, SrcReg)
DEF_OPCODE_3(TypeOp_GetContextSlotMove, DstReg, UInt8, SrcReg)
DEF_OPCODE_1(TypeOp_PushContext, SrcReg)
DEF_OPCODE_0(TypeOp_PopContext)
DEF_OPCODE_3(TypeOp_GetContextSlot, DstReg, UInt8, UInt8)
DEF_OPCODE_3(TypeOp_SetContextSlot, SrcReg, UInt8, UInt8)
DEF_OPCODE_3(TypeOp_AbsUnEqual, DstReg, SrcReg, SrcReg)
DEF_OPCODE_3(TypeOp_AbsEqual, DstReg, SrcReg, SrcReg)
DEF_OPCODE_2(TypeOp_JmpTrue, SrcReg, UInt16)
DEF_OPCODE_0(TypeLabel_EnterBlock)
DEF_OPCODE_0(TypeLabel_LeaveBlock)
DEF_OPCODE_2(TypeOp_CreateBlockContext, DstReg, UInt8)  // 60

DEF_NEW_OPCODE_2(TypeOp_GetToplevelClosureVar, DstReg, UInt8)
DEF_NEW_OPCODE_2(TypeOp_SetToplevelClosureVar, SrcReg, UInt8)
DEF_NEW_OPCODE_3(TypeOp_SetObjectString, SrcReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_SetObjectNumber, SrcReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_SetTableNumber, SrcReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetObjectString, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetTableString, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetObjectNumber, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetTableNumber, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetArrayInt64, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_EqualJmpFalse, SrcReg, UInt8, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_EqualJmpTrue, SrcReg, UInt8, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_UnEqualJmpFalse, SrcReg, UInt8, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_UnEqualJmpTrue, SrcReg, UInt8, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_BoolJmpFalse, SrcReg, UInt16)
DEF_NEW_OPCODE_2(TypeOp_BoolJmpTrue, SrcReg, UInt16)
DEF_NEW_OPCODE_3(TypeOp_AddStringString, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_AddStringAny, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_AddAnyString, DstReg, SrcReg, SrcReg)
// array element in consecutive regs
DEF_NEW_OPCODE_3(TypeOp_NewArrayConsecutive, DstReg, UInt8, UInt8)
// array element % 4 == 0
DEF_NEW_OPCODE_3(TypeOp_NewArrayRandom, DstReg, UInt8, UInt8)
// array element % 4 == 1
DEF_NEW_OPCODE_3(TypeOp_NewArrayRandom1, DstReg, UInt8, UInt8)
// array element % 4 == 2
DEF_NEW_OPCODE_3(TypeOp_NewArrayRandom2, DstReg, UInt8, UInt8)
// array element % 4 == 3
DEF_NEW_OPCODE_3(TypeOp_NewArrayRandom3, DstReg, UInt8, UInt8)
// argc = 1
DEF_NEW_OPCODE_3(TypeOp_Call1, SrcReg, SrcReg, DstReg)
// argc % 4 == 0
DEF_NEW_OPCODE_3(TypeOp_CallRandom, SrcReg, UInt8, DstReg)
// argc % 4 == 1
DEF_NEW_OPCODE_3(TypeOp_CallRandom1, SrcReg, UInt8, DstReg)
// argc % 4 == 2
DEF_NEW_OPCODE_3(TypeOp_CallRandom2, SrcReg, UInt8, DstReg)
// argc % 3 == 3
DEF_NEW_OPCODE_3(TypeOp_CallRandom3, SrcReg, UInt8, DstReg)
DEF_NEW_OPCODE_3(TypeOp_EqualString, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_UnEqualString, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_GetStringLength, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_ToString, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_DeepClone, DstReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetBuiltinFunc, DstReg, SrcReg, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_SetObjectConstString, SrcReg, UInt8, SrcReg)
DEF_NEW_OPCODE_3(TypeOp_GetTableConstString, DstReg, SrcReg, UInt8)
DEF_NEW_OPCODE_2(TypeOp_Not2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_Neg2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_Pos2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_BitNot2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_Inc2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_Dec2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_Typeof2, DstReg, SrcReg)
DEF_NEW_OPCODE_2(TypeOp_LoadSmallInt, DstReg, UInt16)
DEF_NEW_OPCODE_2(TypeOp_LoadConstAndClone, DstReg, UInt16)

#undef DEF_OPERAND
#undef DEF_OPCODE
#undef DEF_OPCODE_0
#undef DEF_OPCODE_1
#undef DEF_OPCODE_2
#undef DEF_OPCODE_3
#undef DEF_NEW_OPCODE_0
#undef DEF_NEW_OPCODE_1
#undef DEF_NEW_OPCODE_2
#undef DEF_NEW_OPCODE_3
#undef DEF_NEW_OPCODE
