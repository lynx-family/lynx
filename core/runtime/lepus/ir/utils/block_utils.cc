// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/utils/block_utils.h"

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/op_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

static bool IsCatchBlock(const Block* bb) {
  if (!bb || bb->empty()) return false;
  // Catch label blocks are reachable via the VM's exception scan
  // (TypeLabel_Catch) even if there is no explicit CFG edge in the IR.
  return llvh::isa<CatchInst>(bb->Front());
}

void InsertAfterPhi(OpBuilder* builder, Block* block) {
  if (block->empty() || (block->size() == 1 && block->HasTerminalInst())) {
    builder->SetInsertionPointToStart(block);
    return;
  }

  if (!llvh::isa<PhiInst>(block->Front())) {
    builder->SetInsertionPointToStart(block);
    return;
  }

  Instruction* insert_after_inst = nullptr;
  for (auto& op : *block) {
    if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
      if (llvh::isa<PhiInst>(inst)) {
        insert_after_inst = inst;
      } else {
        break;
      }
    }
  }

  if (!insert_after_inst) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: InsertAfterPhi failed to find last PhiInst");
  }
  builder->SetInsertionPointAfter(insert_after_inst);
}

static Instruction* FindOtherValueWithSameToplevelVarReg(
    FuncOp* f, llvh::SmallPtrSet<Block*, 16>& visited,
    unsigned toplevel_var_reg) {
  for (auto it = f->begin(), e = f->end(); it != e;) {
    auto* bb = &*it++;
    if (visited.count(bb)) {
      for (auto& op : *bb) {
        if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
          if (inst->GetToplevelVarReg() == toplevel_var_reg) {
            return inst;
          }
        }
      }
    }
  }
  return nullptr;
}

static void ProcessBBWithToplevelVarReg(FuncOp* f,
                                        llvh::SmallPtrSet<Block*, 16>& visited,
                                        Block* bb) {
  // Process the basic block with toplevel var reg.
  for (auto& op : *bb) {
    if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
      if (inst->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        [[maybe_unused]] auto toplevel_variable =
            f->GetIRCtx()->GetToplevelVariables();
        // create function to find other value attribute with the same
        // toplevel var reg.
        auto other_inst = FindOtherValueWithSameToplevelVarReg(
            f, visited, inst->GetToplevelVarReg());
        if (!other_inst) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: ProcessBBWithToplevelVarReg failed to find "
              "matching toplevel var instruction");
        }
        f->GetIRCtx()->UpdateToplevelVar(inst, other_inst);
      }
    }
  }
}

bool DeleteUnreachableBlocks(FuncOp* f) {
  // Visit all reachable blocks.
  llvh::SmallPtrSet<Block*, 16> visited;
  llvh::SmallVector<Block*, 32> work_list;

  // The entry block is always a root.
  work_list.push_back(&*f->begin());
  // Catch blocks can be entered by the VM without an explicit IR edge.
  for (auto it = f->begin(), e = f->end(); it != e; ++it) {
    Block* bb = &*it;
    if (IsCatchBlock(bb)) {
      work_list.push_back(bb);
    }
  }
  while (!work_list.empty()) {
    auto* bb = work_list.pop_back_val();
    // Already visited?
    if (!visited.insert(bb).second) continue;

    for (auto* succ : Successors(bb)) work_list.push_back(succ);
  }

  // Collect unreachable blocks.
  llvh::SmallVector<Block*, 16> unreachable;
  for (auto it = f->begin(), e = f->end(); it != e; ++it) {
    Block* bb = &*it;
    if (!visited.count(bb)) {
      unreachable.push_back(bb);
    }
  }

  if (unreachable.empty()) return false;

  // Phase 1: Remove all phi entries that reference unreachable blocks (as the
  // incoming-block component). This must happen before any block is erased,
  // because EraseFromParent RAUW's each instruction to nullptr — if a value
  // defined in unreachable block B is used in a phi entry associated with a
  // different unreachable block C, erasing B first would null-out that phi
  // value, and a subsequent RemoveEntry(C) would crash in
  // RecalculateResultType.
  for (auto* bb : unreachable) {
    Value::UseListTy users(bb->GetUsers().begin(), bb->GetUsers().end());
    for (auto* user : users) {
      if (auto* phi = llvh::dyn_cast<PhiInst>(user)) {
        phi->RemoveEntry(bb);
      }
    }
  }

  // Phase 2: Process toplevel var bookkeeping and erase.
  for (auto* bb : unreachable) {
    ProcessBBWithToplevelVarReg(f, visited, bb);
    bb->ReplaceAllUsesWith(nullptr);
    bb->EraseFromParent();
  }

  return true;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
