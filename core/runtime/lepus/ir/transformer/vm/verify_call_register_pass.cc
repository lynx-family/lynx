// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/verify_call_register_pass.h"

#include <algorithm>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

bool VerifyCallRegisterPass::RunOnFunction(FuncOp* func) {
  auto* ra = ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (!ra) return false;

  for (auto& block : *func) {
    for (auto& inst : block) {
      if (!llvh::isa<CallInst>(&inst)) continue;

      auto* call_inst = llvh::dyn_cast<CallInst>(&inst);
      Value* func_val = call_inst->GetFunction();
      if (!func_val || !ra->IsAllocated(func_val)) continue;

      unsigned func_reg = ra->GetRegister(func_val).GetIndex();
      const unsigned argc = call_inst->GetNumArguments();
      if (argc == 0) {
        continue;
      }

      // VM call fast-paths materialize arguments into registers
      //   [func_reg + 1, func_reg + argc]
      // Only live values that fall into this clobber range are unsafe.
      // This is strictly weaker (and more accurate) than requiring
      // func_reg to be greater than ALL live registers.
      const unsigned clobber_lo = func_reg + 1;
      const unsigned clobber_hi = func_reg + argc;
      unsigned max_conflict_reg = 0;

      // If an argument is already located in its destination register
      // (func_reg + i), then the materialization for that slot is a no-op
      // (self-copy) and does not clobber the value.
      llvh::SmallDenseSet<unsigned, 8> allowed_dest_regs;
      for (unsigned i = 0; i < argc; ++i) {
        Value* arg = call_inst->GetArgument(i);
        if (!arg || !ra->IsAllocated(arg)) continue;
        unsigned arg_reg = ra->GetRegister(arg).GetIndex();
        unsigned dest_reg = func_reg + 1 + i;
        if (arg_reg == dest_reg) {
          allowed_dest_regs.insert(dest_reg);
        }
      }

      // Calculate max live register at call site
      // 1. Check all allocated instructions that are live at this point
      if (!ra->HasInstructionNumber(call_inst)) continue;
      unsigned call_idx = ra->GetInstructionNumber(call_inst);
      Segment call_point(call_idx + 1, call_idx + 2);

      // Iterate over all instructions in the function to check liveness
      for (auto& bb : *func) {
        for (auto& i : bb) {
          if (&i == call_inst) continue;
          if (&i == func_val) continue;
          if (!ra->IsAllocated(&i)) continue;

          if (auto* i_inst = llvh::dyn_cast<Instruction>(&i)) {
            if (ra->HasInstructionNumber(i_inst)) {
              if (ra->GetInstructionInterval(i_inst).Intersects(call_point)) {
                unsigned r = ra->GetRegister(i_inst).GetIndex();
                if (r >= clobber_lo && r <= clobber_hi &&
                    !allowed_dest_regs.count(r)) {
                  max_conflict_reg = std::max(max_conflict_reg, r);
                  throw ::lynx::lepus::CompileException(
                      "Lepus IR error: VerifyCallRegisterPass detected call "
                      "clobbers a live register");
                }
              }
            }
          }
        }
      }

      // 2. Toplevel variables protection for toplevel functions
      if (func->IsToplevelFunc() && call_inst->GetNumArguments() > 0) {
        auto& toplevel_vars = ir_ctx_->GetToplevelVariables();
        for (auto& kv : toplevel_vars) {
          if (ra->IsAllocated(kv.second)) {
            unsigned r = ra->GetRegister(kv.second).GetIndex();
            if (r >= clobber_lo && r <= clobber_hi &&
                !allowed_dest_regs.count(r)) {
              max_conflict_reg = std::max(max_conflict_reg, r);
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: VerifyCallRegisterPass detected call "
                  "clobbers a toplevel variable register");
            }
          }
        }
      }

      // 3. Parameter protection.
      //
      // Parameters are preallocated in the prefix region and are Values
      // (not Instructions), so they are not covered by instruction intervals.
      // If the VM writes arguments into `func_reg + 1 .. func_reg + argc`,
      // it must not clobber parameter registers.
      for (auto* param : func->GetParams()) {
        if (!param || !ra->IsAllocated(param)) continue;
        unsigned r = ra->GetRegister(param).GetIndex();
        if (r >= clobber_lo && r <= clobber_hi && !allowed_dest_regs.count(r)) {
          max_conflict_reg = std::max(max_conflict_reg, r);
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: VerifyCallRegisterPass detected call clobbers a "
              "parameter register");
        }
      }
    }
  }
  return false;
}

Pass* CreateVerifyCallRegisterPass(IRContext* ir_ctx) {
  return new VerifyCallRegisterPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
