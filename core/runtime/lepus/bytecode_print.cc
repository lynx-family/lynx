// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/lepus/bytecode_print.h"

#include <chrono>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "core/runtime/lepus/op_code.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {

#ifdef LEPUS_TEST
void Dumper::Dump() {
  functions_.emplace_back(root_);
  DumpFunction();
}

void Dumper::ProcessSpecialInstructions(Instruction* ins, size_t& i) {
  if (Instruction::GetOpCode(*ins) == TypeOp_CallRandom ||
      Instruction::GetOpCode(*ins) == TypeOp_CallRandom1 ||
      Instruction::GetOpCode(*ins) == TypeOp_CallRandom2 ||
      Instruction::GetOpCode(*ins) == TypeOp_CallRandom3) {
    long argc = Instruction::GetParamB(*ins);
    auto new_instrs_size = (argc + 3) / 4;
    i += new_instrs_size;
    for (auto index = 0; index < new_instrs_size; index++) {
      os_ << 0 << " : " << 0 << "    ";
      os_ << "call random args"
          << "\n";
    }
  } else if (Instruction::GetOpCode(*ins) == TypeOp_NewArrayRandom ||
             Instruction::GetOpCode(*ins) == TypeOp_NewArrayRandom1 ||
             Instruction::GetOpCode(*ins) == TypeOp_NewArrayRandom2 ||
             Instruction::GetOpCode(*ins) == TypeOp_NewArrayRandom3) {
    long argc = Instruction::GetParamB(*ins);
    auto new_instrs_size = (argc + 3) / 4;
    i += new_instrs_size;
    for (auto index = 0; index < new_instrs_size; index++) {
      os_ << 0 << " : " << 0 << "    ";
      os_ << "new array random args"
          << "\n";
    }
  }
}

void Dumper::DumpFunction() {
  Instruction* ins;
  for (int j = 0; j < functions_.size(); j++) {
    Function* func_ptr = functions_[j];
    os_ << "######## BEGIN DUMP FUNCTION #########: "
        << ", function name:" << func_ptr->GetFunctionName() << "\n";
    DumpScope(func_ptr);
    for (size_t i = 0; i < func_ptr->OpCodeSize(); i++) {
      ins = func_ptr->GetInstruction(i);
      PrintOpCode(*ins, func_ptr, i);
      // there are multiple insts for callRandom & newArrayRandom
      ProcessSpecialInstructions(ins, i);
    }
    os_ << "######## END DUMP FUNCTION #########"
        << "\n";
  }
}

void Dumper::DumpFunction(fml::RefPtr<Function>& func_ptr) {
  os_ << "######## BEGIN DUMP FUNCTION #########: "
      << ", function name:" << func_ptr->GetFunctionName() << "\n";
  DumpScope(func_ptr.get());
  for (size_t i = 0; i < func_ptr->OpCodeSize(); i++) {
    auto ins = func_ptr->GetInstruction(i);
    PrintOpCode(*ins, func_ptr.get(), i);
    // there are multiple insts for callRandom & newArrayRandom
    ProcessSpecialInstructions(ins, i);
  }
  os_ << "######## NEW DUMP FUNCTION #########"
      << "\n";
}

static void DumpEmptySpaces(std::ostream& os, int32_t intend) {
  for (size_t i = 0; i < intend; i++) {
    os << " ";
  }
}

void Dumper::DumpBlockScope(Function* func_ptr, const Value& scopes_,
                            int32_t intend) {
  if (!scopes_.IsTable()) return;
  int32_t line;
  int32_t col;
  Value start = scopes_.GetProperty(Function::kStartLine);
  Value end = scopes_.GetProperty(Function::kEndLine);

  Function::DecodeLineCol(start.Number(), line, col);
  DumpEmptySpaces(os_, intend);
  os_ << "ScopeLine: (" << line << ":" << col << ") => ";
  Function::DecodeLineCol(end.Number(), line, col);
  os_ << "(" << line << ":" << col << ")"
      << "\n";

  for (const auto& it : *scopes_.Table()) {
    int32_t type = -1;
    int32_t reg_index = -1;
    int32_t array_index = -1;
    int32_t offset = -1;
    if (!it.second.IsUInt32()) continue;
    Function::DecodeVariableInfo(it.second.UInt32(), type, reg_index,
                                 array_index, offset);
    if (type == 0) {
      DumpEmptySpaces(os_, intend);
      os_ << it.first.c_str() << "  : " << reg_index << " : NORMAL"
          << "\n";
    } else if (type == 1) {
      DumpEmptySpaces(os_, intend);
      os_ << it.first.c_str() << " :array_index(" << array_index << ") :offset("
          << offset << ")"
          << " :Closure"
          << "\n";
    } else if (type == 2) {
      DumpEmptySpaces(os_, intend);
      os_ << it.first.c_str() << " :array_index(" << array_index
          << ") :current_context(" << offset << ")"
          << " :Closure_Outside"
          << "\n";
    } else {
      os_ << "wrong decode type, please check";
    }
  }
  os_ << "\n";

  Value childs = scopes_.GetProperty(Function::kChilds);
  if (!childs.Array()) return;

  size_t size = childs.Array()->size();
  for (size_t i = 0; i < size; i++) {
    DumpBlockScope(func_ptr, childs.Array()->get(i), intend + 1);
  }
}
void Dumper::DumpScope(Function* func_ptr) {
  os_ << "----ScopeInfo:-----"
      << "\n";
  DumpBlockScope(func_ptr, func_ptr->GetScope(), 0);
}

void Dumper::PrintOpCode(Instruction i, Function* func_ptr, int index) {
  long offsets[3];
  OffsetScope id[3];
  id[0] = Normal;
  int32_t line = 0;
  int32_t col = 0;
  func_ptr->GetLineCol(index, line, col);
  os_ << line << " : " << col << "    ";
  switch (Instruction::GetOpCode(i)) {
    case TypeOp_LoadNil:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Constant;
      PrintDetail("LoadNil", 2, offsets, id);
      break;
    case TypeOp_SetCatchId:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("SetCatchId", 1, offsets, id);
      break;
    case TypeOp_LoadConst:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamBx(i);
      id[1] = Constant;
      PrintDetail("LoadConst", 2, offsets, id);
      break;
    case TypeOp_LoadSmallInt:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamsBx(i);
      id[1] = Constant;
      PrintDetail("LoadSmallInt", 2, offsets, id);
      break;
    case TypeOp_LoadConstAndClone:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamsBx(i);
      id[1] = Constant;
      PrintDetail("LoadConstAndClone", 2, offsets, id);
      break;
    case TypeOp_Move:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Move", 2, offsets, id);
      break;
    case TypeOp_GetUpvalue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[0] = Normal;
      id[1] = Clo;
      PrintDetail("GetUpvalue", 2, offsets, id);
      break;
    case TypeOp_GetContextSlot: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Constant;
      PrintDetail("GetContextSlot", 3, offsets, id);
      break;
    }
    case TypeOp_SetContextSlot: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Constant;
      PrintDetail("SetContextSlot", 3, offsets, id);
      break;
    }
    case TypeOp_SetUpvalue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Clo;
      PrintDetail("SetUpvalue", 2, offsets, id);
      break;
    case TypeOp_GetGlobal:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamBx(i);
      id[1] = Global;
      PrintDetail("GetGlobal", 2, offsets, id);
      break;
    case TypeOp_GetBuiltin:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamBx(i);
      id[1] = Constant;
      PrintDetail("GetBuiltin", 2, offsets, id);
      break;
    case TypeOp_SetGlobal:
      PrintDetail("SetGlobal", 0, offsets, id);
      break;
    case TypeOp_Closure: {
      long idx = Instruction::GetParamBx(i);
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Closure", 1, offsets, id);
      functions_.push_back(func_ptr->GetChildFunction(idx).get());
    } break;
    case TypeOp_CreateContext: {
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("CreateContext", 1, offsets, id);
    } break;
    case TypeOp_CreateBlockContext: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Constant;
      PrintDetail("CreateBlockContext", 2, offsets, id);
    } break;
    case TypeOp_PushContext: {
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("PushContext", 1, offsets, id);
      break;
    }
    case TypeOp_PopContext: {
      os_ << "Pop Context"
          << "\n";
      break;
    }
    case TypeLabel_EnterBlock: {
      os_ << "Enter Block"
          << "\n";
      break;
    }
    case TypeLabel_LeaveBlock: {
      os_ << "Leave Block"
          << "\n";
      break;
    }
    case TypeOp_Call: {
      long argc = Instruction::GetParamB(i);
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = argc;
      id[1] = Normal;
      id[2] = Constant;
      PrintDetail("Call", 3, offsets, id);
    } break;
    case TypeOp_Ret:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Ret", 1, offsets, id);
      return;
    case TypeOp_JmpFalse:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamsBx(i);
      id[0] = Normal;
      id[1] = Constant;
      PrintDetail("JmpFalse", 2, offsets, id);
      break;
    case TypeOp_JmpTrue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamsBx(i);
      id[0] = Normal;
      id[1] = Constant;
      PrintDetail("JmpTrue", 2, offsets, id);
      break;
    case TypeOp_Jmp:
      os_ << "Jmp C[" << Instruction::GetParamsBx(i) << "]"
          << "\n";
      break;
    case TypeLabel_Catch:
      os_ << "Catch Label"
          << "\n";
      break;
    case TypeLabel_Throw:
      os_ << "Throw Label"
          << "\n";
      break;
    case TypeOp_SetContextSlotMove:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      PrintDetail("SetContextSlotMove", 3, offsets, id);
      break;
    case TypeOp_GetContextSlotMove:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      PrintDetail("GetContextSlotMove", 3, offsets, id);
      break;
    case TypeOp_Neg:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Neg", 1, offsets, id);
      break;
    case TypeOp_Pos:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Pos", 1, offsets, id);
      break;
    case TypeOp_Not:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Not", 1, offsets, id);
      break;
    case TypeOp_Not2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Not2", 2, offsets, id);
      break;
    case TypeOp_Neg2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Neg2", 2, offsets, id);
      break;
    case TypeOp_Pos2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Pos2", 2, offsets, id);
      break;
    case TypeOp_BitNot2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("BitNot2", 2, offsets, id);
      break;
    case TypeOp_BitNot:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("BitNot", 1, offsets, id);
      break;
    case TypeOp_BitOr:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("BitOr", 3, offsets, id);
      break;
    case TypeOp_BitAnd:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("BitAnd", 3, offsets, id);
      break;
    case TypeOp_BitXor:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("BitXor", 3, offsets, id);
      break;
    case TypeOp_Typeof:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Typeof", 1, offsets, id);
      break;
    case TypeOp_Typeof2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Typeof2", 2, offsets, id);
      break;
    case TypeOp_Len:
      PrintDetail("Len", 0, offsets, id);
      break;
    case TypeOp_Add:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Add", 3, offsets, id);
      break;
    case TypeOp_Sub:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Sub", 3, offsets, id);
      break;
    case TypeOp_Mul:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Mul", 3, offsets, id);
      break;
    case TypeOp_Div:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Div", 3, offsets, id);
      break;
    case TypeOp_Pow:
      PrintDetail("Pow", 0, offsets, id);
      break;
    case TypeOp_Mod:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Mod", 3, offsets, id);
      break;
    case TypeOp_And:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("And", 3, offsets, id);
      break;
    case TypeOp_Or:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Or", 3, offsets, id);
      break;
    case TypeOp_Less:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Less", 3, offsets, id);
      break;
    case TypeOp_Greater:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Greater", 3, offsets, id);
      break;
    case TypeOp_Equal:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Equal", 3, offsets, id);
      break;
    case TypeOp_AbsEqual:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("AbsEqual", 3, offsets, id);
      break;
    case TypeOp_UnEqual:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("UnEqual", 3, offsets, id);
      break;
    case TypeOp_AbsUnEqual:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("AbsUnEqual", 3, offsets, id);
      break;
    case TypeOp_LessEqual:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("LessEqual", 3, offsets, id);
      break;
    case TypeOp_GreaterEqual:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GreaterEqual", 3, offsets, id);
      break;
    case TypeOp_NewArray: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Constant;
      PrintDetail("NewArray", 2, offsets, id);
    } break;
    case TypeOp_NewTable:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("NewTable", 1, offsets, id);
      break;
    case TypeOp_SetTable:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("SetTable", 3, offsets, id);
      break;
    case TypeOp_GetTable:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetTable", 3, offsets, id);
      break;
    case TypeOp_Switch: {
      // TODO: open this when needed

      //        a = GET_REGISTER_A(i);
      //        long index = Instruction::GetParamBx(i);
      //        long jmp = function->GetSwitch(index)->Switch(a);
      //        frame->instruction_ += -1 + jmp;
      PrintDetail("Switch", 0, offsets, id);
    } break;
    case TypeOp_Inc:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Inc", 1, offsets, id);
      break;
    case TypeOp_Inc2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Inc2", 2, offsets, id);
      break;
    case TypeOp_Dec:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Dec", 1, offsets, id);
      break;
    case TypeOp_Dec2:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      PrintDetail("Dec2", 2, offsets, id);
      break;
    case TypeOp_Noop:
      PrintDetail("Noop", 0, offsets, id);
      break;
    case TypeOp_SetObjectString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("SetObjectString", 3, offsets, id);
      break;
    case TypeOp_SetObjectConstString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Normal;
      PrintDetail("SetObjectConstString", 3, offsets, id);
      break;
    case TypeOp_DeepClone:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      PrintDetail("CallDeepClone", 2, offsets, id);
      break;
    case TypeOp_SetObjectNumber:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("SetObjectNumber", 3, offsets, id);
      break;
    case TypeOp_GetObjectString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetObjectString", 3, offsets, id);
      break;
    case TypeOp_GetTableString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetTableString", 3, offsets, id);
      break;
    case TypeOp_GetTableConstString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Constant;
      PrintDetail("GetTableConstString", 3, offsets, id);
      break;
    case TypeOp_GetTableNumber:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetTableNumber", 3, offsets, id);
      break;
    case TypeOp_GetArrayInt64:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetArrayInt64", 3, offsets, id);
      break;
    case TypeOp_GetObjectNumber:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetObjectNumber", 3, offsets, id);
      break;
    case TypeOp_BoolJmpFalse:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamsBx(i);
      id[0] = Normal;
      id[1] = Constant;
      PrintDetail("BoolJmpFalse", 2, offsets, id);
      break;
    case TypeOp_BoolJmpTrue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamsBx(i);
      id[0] = Normal;
      id[1] = Constant;
      PrintDetail("BoolJmpTrue", 2, offsets, id);
      break;
    case TypeOp_AddStringString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("AddStringString", 3, offsets, id);
      break;
    case TypeOp_AddStringAny:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("AddStringAny", 3, offsets, id);
      break;
    case TypeOp_AddAnyString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("AddAnyString", 3, offsets, id);
      break;
    case TypeOp_GetToplevelClosureVar:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[0] = Normal;
      id[1] = Constant;
      PrintDetail("GetToplevelClosureVar", 2, offsets, id);
      break;
    case TypeOp_SetToplevelClosureVar:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Constant;
      PrintDetail("SetToplevelClosureVar", 2, offsets, id);
      break;
    case TypeOp_EqualJmpFalse:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = static_cast<int8_t>(Instruction::GetParamB(i));
      offsets[2] = Instruction::GetParamC(i);
      id[0] = Normal;
      id[1] = Constant;
      id[2] = Normal;
      PrintDetail("EqualJmpFalse", 3, offsets, id);
      break;
    case TypeOp_EqualJmpTrue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = static_cast<int8_t>(Instruction::GetParamB(i));
      offsets[2] = Instruction::GetParamC(i);
      id[0] = Normal;
      id[1] = Constant;
      id[2] = Normal;
      PrintDetail("EqualJmpTrue", 3, offsets, id);
      break;
    case TypeOp_UnEqualJmpFalse:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = static_cast<int8_t>(Instruction::GetParamB(i));
      offsets[2] = Instruction::GetParamC(i);
      id[0] = Normal;
      id[1] = Constant;
      id[2] = Normal;
      PrintDetail("UnEqualJmpFalse", 3, offsets, id);
      break;
    case TypeOp_UnEqualJmpTrue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = static_cast<int8_t>(Instruction::GetParamB(i));
      offsets[2] = Instruction::GetParamC(i);
      id[0] = Normal;
      id[1] = Constant;
      id[2] = Normal;
      PrintDetail("UnEqualJmpTrue", 3, offsets, id);
      break;
    case TypeOp_SetTableNumber:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("SetTableNumber", 3, offsets, id);
      break;
    case TypeOp_NewArrayConsecutive: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      PrintDetail("NewArrayConsecutive", 3, offsets, id);
    } break;
    case TypeOp_NewArrayRandom: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Constant;
      PrintDetail("NewArrayRandom", 3, offsets, id);
    } break;
    case TypeOp_NewArrayRandom1: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Constant;
      PrintDetail("NewArrayRandom1", 3, offsets, id);
    } break;
    case TypeOp_NewArrayRandom2: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Constant;
      PrintDetail("NewArrayRandom2", 3, offsets, id);
    } break;
    case TypeOp_NewArrayRandom3: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Constant;
      id[2] = Constant;
      PrintDetail("NewArrayRandom3", 3, offsets, id);
    } break;
    case TypeOp_CallRandom: {
      long argc = Instruction::GetParamB(i);
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = argc;
      id[1] = Normal;
      id[2] = Constant;
      PrintDetail("CallRandom", 3, offsets, id);
    } break;
    case TypeOp_CallRandom1: {
      long argc = Instruction::GetParamB(i);
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = argc;
      id[1] = Normal;
      id[2] = Constant;
      PrintDetail("CallRandom1", 3, offsets, id);
    } break;
    case TypeOp_Call1: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = Instruction::GetParamB(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("Call1", 3, offsets, id);
    } break;
    case TypeOp_CallRandom2: {
      long argc = Instruction::GetParamB(i);
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = argc;
      id[1] = Normal;
      id[2] = Constant;
      PrintDetail("CallRandom2", 3, offsets, id);
    } break;
    case TypeOp_CallRandom3: {
      long argc = Instruction::GetParamB(i);
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = argc;
      id[1] = Normal;
      id[2] = Constant;
      PrintDetail("CallRandom3", 3, offsets, id);
    } break;
    case TypeOp_EqualString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("EqualString", 3, offsets, id);
      break;
    case TypeOp_UnEqualString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("UnEqualString", 3, offsets, id);
      break;
    case TypeOp_GetStringLength:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetStringLength", 2, offsets, id);
      break;
    case TypeOp_ToString:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("ToString", 2, offsets, id);
      break;
    case TypeOp_GetBuiltinFunc:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      id[1] = Normal;
      id[2] = Normal;
      PrintDetail("GetBuiltinFunc", 3, offsets, id);
      break;
    default:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      PrintDetail("Unknown", 3, offsets, id);
      break;
  }
}

void Dumper::PrintDetail(const char* oper, int nums, long offsets[],
                         OffsetScope id[]) {
  os_ << oper << " ";
  const char* scope_id;
  for (size_t i = 0; i < static_cast<size_t>(nums); i++) {
    switch (id[i]) {
      case Global:
        scope_id = "G";
        break;
      case Constant:
        scope_id = "C";
        break;
      case Clo:  // Closure
        scope_id = "CL";
        break;
      default:
        scope_id = "";
        break;
    }
    os_ << scope_id << "[" << offsets[i] << "] ";
  }
  os_ << "\n";
}
#endif
}  // namespace lepus
}  // namespace lynx
