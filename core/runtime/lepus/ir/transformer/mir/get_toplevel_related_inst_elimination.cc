// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/get_toplevel_related_inst_elimination.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"

namespace lynx {
namespace lepus {
namespace ir {

bool GetToplevelRelatedInstEliminationPass::PerformFunctionRemove(
    FuncOp* func) {
  bool changed = false;
  for (auto& block : *func) {
    for (auto it = block.InstBegin(); it != block.InstEnd();) {
      Instruction* inst = *it;
      ++it;
      if (auto* get_toplevel_var = llvh::dyn_cast<GetToplevelVarInst>(inst)) {
        // the toplevelVarReg & closureVarReg attribute in GetToplevelVarInst is
        // no longer needed.
        get_toplevel_var->SetToplevelVarReg(constants::kInvalidSignedValue);
        get_toplevel_var->SetClosureVarReg(constants::kInvalidSignedValue);

        // delete if no use
        if (get_toplevel_var->GetNumUsers() == 0) {
          inst->EraseFromParent();
          changed = true;
        }
      }

      if (auto* get_toplevel_closure_var =
              llvh::dyn_cast<GetToplevelClosureVarInst>(inst)) {
        // the toplevelVarReg & closureVarReg attribute in
        // GetToplevelClosureVarInst is no longer needed.
        get_toplevel_closure_var->SetToplevelVarReg(
            constants::kInvalidSignedValue);
        get_toplevel_closure_var->SetClosureVarReg(
            constants::kInvalidSignedValue);

        // delete if no use
        if (get_toplevel_closure_var->GetNumUsers() == 0) {
          inst->EraseFromParent();
          changed = true;
        }
      }

      // combine GetToplevelVarInst for this pattern: SetTopelvelVarInst,
      // GetToplevelVarInst
      if (auto* set_toplevel_var = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
        Instruction* next_inst = nullptr;
        if (it != block.InstEnd()) {
          next_inst = *it;
        }
        auto* reg = set_toplevel_var->GetToplevelReg();

        if (next_inst && llvh::isa<GetToplevelVarInst>(next_inst)) {
          auto* get_reg =
              llvh::cast<GetToplevelVarInst>(next_inst)->GetToplevelReg();
          if (reg == get_reg) {
            ++it;
            next_inst->ReplaceAllUsesWith(set_toplevel_var->GetSrc());
            next_inst->EraseFromParent();
            changed = true;
          }
        }
      }

      // combine GetToplevelClosureVarInst for this pattern:
      // SetTopelvelClosureVarInst, GetToplevelClosureVarInst
      if (auto* set_toplevel_closure_var =
              llvh::dyn_cast<SetToplevelClosureVarInst>(inst)) {
        Instruction* next_inst = nullptr;
        if (it != block.InstEnd()) {
          next_inst = *it;
        }
        auto* reg = set_toplevel_closure_var->GetClosureReg();

        if (next_inst && llvh::isa<GetToplevelClosureVarInst>(next_inst)) {
          auto* get_reg =
              llvh::cast<GetToplevelClosureVarInst>(next_inst)->GetClosureReg();
          if (reg == get_reg) {
            ++it;
            next_inst->ReplaceAllUsesWith(set_toplevel_closure_var->GetSrc());
            next_inst->EraseFromParent();
            changed = true;
          }
        }
      }
    }
  }
  return changed;
}

bool GetToplevelRelatedInstEliminationPass::RunOnModule(ModuleOp* mod) {
  bool changed = false;
  llvh::for_each(*mod, [&](FuncOp* f) { changed |= PerformFunctionRemove(f); });
  return changed;
}

Pass* CreateGetToplevelRelatedInstEliminationPass(IRContext* ir_ctx) {
  return new GetToplevelRelatedInstEliminationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
