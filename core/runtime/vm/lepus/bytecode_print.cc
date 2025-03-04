// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/bytecode_print.h"

#include <chrono>

#include "base/include/log/logging.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/op_code.h"
#include "core/runtime/vm/lepus/vm_context.h"
namespace lynx {
namespace lepus {

#ifdef LEPUS_TEST
void Dumper::Dump() {
  functions_.emplace_back(root);
  DumpFunction();
}

void Dumper::DumpFunction() {
  Instruction* ins;
  for (int j = 0; j < functions_.size(); j++) {
    Function* func_ptr = functions_[j];
    std::cout << "######## BEGIN #########: "
              << ", function name:" << func_ptr->GetFunctionName() << std::endl;
    func_ptr->DumpScope();
    for (size_t i = 0; i < func_ptr->OpCodeSize(); i++) {
      ins = func_ptr->GetInstruction(i);
      PrintOpCode(*ins, func_ptr, i);
    }
    std::cout << "######## END #########" << std::endl;
  }
}
#endif

void Dumper::PrintOpCode(Instruction i, Function* func_ptr, int index) {
  long offsets[3];
  OffsetScope id[3];
  id[0] = Normal;
  int32_t line = 0;
  int32_t col = 0;
  func_ptr->GetLineCol(index, line, col);
  std::cout << line << " : " << col << "    ";
  switch (Instruction::GetOpCode(i)) {
    case TypeOp_LoadNil:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("LoadNil", 1, offsets, id);
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
      PrintDetail("GetUpvalue", 2, offsets, id);
      break;
    case TypeOp_GetContextSlot: {
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamB(i);
      offsets[2] = Instruction::GetParamC(i);
      PrintDetail("GetContextSlot", 3, offsets, id);
      break;
    }
    case TypeOp_SetContextSlot: {
      offsets[0] = Instruction::GetParamA(i);
      ;
      PrintDetail("SetContextSlot", 1, offsets, id);
      break;
    }
    case TypeOp_SetUpvalue:
      offsets[0] = Instruction::GetParamB(i);
      offsets[1] = Instruction::GetParamA(i);
      id[1] = Clo;
      PrintDetail("SetUpvalue", 2, offsets, id);
      break;
    case TypeOp_GetGlobal:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamBx(i);
      id[1] = Global;
      PrintDetail("GetGlobal", 2, offsets, id);
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
    case TypeOp_PushContext: {
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("PushContext", 1, offsets, id);
      break;
    }
    case TypeOp_PopContext: {
      std::cout << "Pop Context" << std::endl;
      break;
    }
    case TypeOp_Call: {
      long argc = Instruction::GetParamB(i);
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = Instruction::GetParamC(i);
      offsets[2] = argc;
      id[1] = Normal;
      PrintDetail("Call", 3, offsets, id);
    } break;
    case TypeOp_Ret:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Ret", 1, offsets, id);
      return;
    case TypeOp_JmpFalse:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = -1 + Instruction::GetParamsBx(i);
      id[1] = Normal;
      PrintDetail("JmpFalse", 2, offsets, id);
      break;
    case TypeOp_JmpTrue:
      offsets[0] = Instruction::GetParamA(i);
      offsets[1] = -1 + Instruction::GetParamsBx(i);
      id[1] = Normal;
      PrintDetail("JmpTrue", 2, offsets, id);
      break;
    case TypeOp_Jmp:
      std::cout << "Jmp [" << -1 + Instruction::GetParamsBx(i) << "]"
                << std::endl;
      break;
    case TypeLabel_Catch:
      std::cout << "Catch Label" << std::endl;
      break;
    case TypeLabel_Throw:
      std::cout << "Throw Label" << std::endl;
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
    case TypeOp_Not:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Not", 1, offsets, id);
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
      PrintDetail("NewArray", 1, offsets, id);
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
    case TypeOp_Dec:
      offsets[0] = Instruction::GetParamA(i);
      PrintDetail("Dec", 1, offsets, id);
      break;
    case TypeOp_Noop:
      PrintDetail("Noop", 0, offsets, id);
      break;
    default:
      break;
  }
}

void Dumper::PrintDetail(const char* oper, int nums, long offsets[],
                         OffsetScope id[]) {
  std::cout << oper << " ";
  const char* scopeId;
  for (size_t i = 0; i < static_cast<size_t>(nums); i++) {
    switch (id[i]) {
      case Global:
        scopeId = "G";
        break;
      case Constant:
        scopeId = "C";
        break;
      case Clo:  // Closure
        scopeId = "CL";
        break;
      default:
        scopeId = "";
        break;
    }
    std::cout << scopeId << "[" << offsets[i] << "] ";
  }
  std::cout << std::endl;
}
}  // namespace lepus
}  // namespace lynx
