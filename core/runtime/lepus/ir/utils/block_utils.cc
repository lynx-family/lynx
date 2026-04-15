// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/utils/block_utils.h"

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
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

/// Process the deletion of the basic block, and erase it.
static void DeleteBlock(Block* b) {
  // Remove all uses of this basic block.

  // Copy the uses of the block aside because removing the users invalidates the
  // iterator.
  Value::UseListTy users(b->GetUsers().begin(), b->GetUsers().end());

  // Remove the block from all Phi instructions referring to it. Note that
  // reachable blocks could end up with Phi instructions referring to
  // unreachable blocks.
  for (auto* user : users) {
    if (auto* phi = llvh::dyn_cast<PhiInst>(user)) {
      phi->RemoveEntry(b);
      continue;
    }
  }

  // There may still be uses of the block from other unreachable blocks.
  b->ReplaceAllUsesWith(nullptr);
  // Erase this basic block.
  b->EraseFromParent();
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
  bool changed = false;

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

  // Delete all blocks that weren't visited.
  for (auto it = f->begin(), e = f->end(); it != e;) {
    auto* bb = &*it++;
    if (!visited.count(bb)) {
      ProcessBBWithToplevelVarReg(f, visited, bb);
      DeleteBlock(bb);
      changed = true;
    }
  }

  return changed;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
