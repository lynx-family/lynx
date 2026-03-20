// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/dce.h"

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
namespace lynx {
namespace lepus {
namespace ir {

bool DCE::DeleteUselessInst(Instruction* inst) {
  if (llvh::isa<CreateClosureInst>(inst)) {
    return inst->GetNumUsers() == 0;
  }

  if (!llvh::isa<NewTableInst>(inst) && !llvh::isa<NewArrayInst>(inst))
    return false;

  bool matched = llvh::all_of(inst->GetUsers(), [&](Value* val) {
    auto set_table = llvh::dyn_cast<SetTableInst>(val);
    if (!set_table) return false;
    return set_table->GetStoreVal() != inst && set_table->GetObject() == inst;
  });

  if (LEPUS_LIKELY(!matched)) return false;

  llvh::for_each(inst->GetUsers(), [&](Value* val) {
    auto inst = llvh::cast<Instruction>(val);
    inst->EraseFromParent();
  });

  if (LEPUS_LIKELY(inst->HasUsers()))
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: DCE::DeleteUselessInst expected instruction to have "
        "no users");
  return true;
}

bool DCE::PerformFunctionDCE(FuncOp* func) {
  bool changed = false;
  do {
    changed = false;
    PostOrderAnalysis po(func);

    // Scan the function in post order (from end to start). We want to visit the
    // uses of the instruction before we visit the instruction itself in order
    // to allow the optimization to delete long chains of dead code.
    for (auto* block : po) {
      // Scan the instructions in the block from end to start.
      for (auto it = block->InstRbegin(), e = block->InstRend(); it != e;) {
        Instruction* inst = *it;
        ++it;

        // do not eliminate inst with special attribute
        if (inst->GetClosureVarReg() != constants::kInvalidSignedValue ||
            inst->GetToplevelVarReg() != constants::kInvalidSignedValue)
          continue;

        // for setTableInst, if the obj and key are array and string
        // respectively, this inst can be deleted, according to TypeOp_SetTable
        // in runtime
        if (auto set_table_inst = llvh::dyn_cast<SetTableInst>(inst)) {
          if (set_table_inst->GetObject()->GetType()->IsArrayType() &&
              set_table_inst->GetProp()->GetType()->IsStringType()) {
            inst->EraseFromParent();
            changed = true;
            continue;
          }
        }

        // Skip if the instruction has side effects that prevent us from
        // deleting it, or if it is a terminator. bool skip = true;
        if (llvh::isa<TerminatorInst>(inst)) continue;

        if (inst->GetSideEffect().HasSideEffect()) {
          if (!DeleteUselessInst(inst)) continue;
        }

        if (inst->GetNumUsers() == 0) {
          inst->EraseFromParent();
          changed = true;
        }
      }
    }
  } while (changed);

  return true;
}

bool DCE::RunOnModule(ModuleOp* mod) {
  bool changed = false;

  // Perform per-function DCE.
  llvh::for_each(*mod, [&](FuncOp* f) { changed |= PerformFunctionDCE(f); });

  return changed;
}

Pass* CreateDCE(IRContext* ir_ctx) { return new DCE(ir_ctx); }
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
