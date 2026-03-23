// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/simplify_cfg.h"

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallPtrSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/utils/block_utils.h"
#include "core/runtime/lepus/ir/utils/eval.h"

namespace lynx {
namespace lepus {
namespace ir {

static bool IsCatchBlock(const Block* bb) {
  if (!bb || bb->empty()) return false;
  // Catch label must remain a distinct boundary in bytecode layout.
  return llvh::isa<CatchInst>(bb->Front());
}

static bool OptimizeIndirectJump(FuncOp* f);
static bool OptimizeSingleEntryPhi(FuncOp* f);

bool SimplifyCFGPass::RunOnModule(ModuleOp* mod) {
  bool changed = false;

  llvh::for_each(*mod, [&](FuncOp* f) {
    bool iter_changed = false;
    // Keep iterating over deleting unreachable code and removing
    // trampolines as long as we are making progress.
    do {
      iter_changed = OptimizeIndirectJump(f) || OptimizeStaticBranches(f) ||
                     DeleteUnreachableBlocks(f) || OptimizeSingleEntryPhi(f);
      changed |= iter_changed;
    } while (iter_changed);
  });

  return changed;
}

/// \returns true if the control-flow edge between \p src to \p dest crosses
/// a catch region.
// static bool IsCrossCatchRegionBranch(Block* src, Block* dest) {
//   auto kind = dest->Front()->GetKind();
//   if (kind == ValueKind::TryStartInstKind || kind ==
//   ValueKind::TryEndInstKind)
//     return true;
//   return false;
// }

/// \returns true if the block \b bb is an input to a PHI node.
static bool IsUsedInPhiNode(Block* bb) {
  for (auto* use : bb->GetUsers()) {
    if (llvh::isa<PhiInst>(use)) return true;
  }
  return false;
}

static void RemoveEntryFromPhi(Block* bb, Block* edge) {
  // For all PHI nodes in block:
  for (auto& inst : *bb) {
    if (auto* p = llvh::dyn_cast<PhiInst>(&inst)) {
      // For each Phi entry:
      for (int i = 0, e = p->GetNumEntries(); i < e; i++) {
        // Remove the incoming edge.
        if (p->GetEntry(i).second == edge) {
          p->RemoveEntry(i);
          break;
        }
      }
    } else {
      break;  // Phi instructions are always at the beginning of the block
    }
  }
}

/// Delete the conditional branch and create a new direct branch to the
/// destination block \p dest.
static void ReplaceCondBranchWithDirectBranch(CondBranchInst* cb, Block* dest) {
  Block* current_block = cb->GetParent();
  auto* true_dest = cb->GetTrueDest();
  auto* false_dest = cb->GetFalseDest();

  if (true_dest != dest) RemoveEntryFromPhi(true_dest, current_block);
  if (false_dest != dest) RemoveEntryFromPhi(false_dest, current_block);

  auto ir_ctx = cb->GetIRCtx();
  if (LEPUS_UNLIKELY(!ir_ctx)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SimplifyCFG expected CondBranchInst to have valid "
        "IRContext");
  }
  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetInsertionPointToEnd(current_block);
  builder->Create<BranchInst>(cb->GetLocation(), dest);
  cb->EraseFromParent();
}

/// Try to remove a branch used by phi nodes.
bool SimplifyCFGPass::AttemptBranchRemovalFromPhiNodes(Block* bb) {
  // Only handle blocks that are a single, unconditional branch.
  if (bb->GetTerminator() != &*(bb->begin()) ||
      bb->GetTerminator()->GetKind() != ValueKind::BranchInstKind) {
    return false;
  }

  // Find our parents and also ensure that there aren't
  // any instructions we can't handle.
  llvh::SmallPtrSet<Block*, 8> block_parents;
  // Keep unique parents by the original order, which is deterministic.
  llvh::SmallVector<Block*, 8> ordered_parents;
  for (const auto* user : bb->GetUsers()) {
    switch (user->GetKind()) {
      case ValueKind::BranchInstKind:
      case ValueKind::CondBranchInstKind:
        // This is an instruction where the branch argument is a simple
        // jump target that can be substituted for any other branch.
        if (block_parents.count(user->GetParent()) == 0) {
          ordered_parents.push_back(user->GetParent());
        }
        block_parents.insert(user->GetParent());
        break;
      case ValueKind::PhiInstKind:
        // The branch argument is not a jump target, but we know how
        // to rewrite them.
        break;
      default:
        // This is some other instruction where we don't know whether we can
        // unconditionally substitute another branch. Bail for safety.
        return false;
    }
  }

  if (block_parents.empty()) {
    return false;
  }

  Block* phi_block = nullptr;

  // Verify that we'll be able to rewrite all relevant Phi nodes.
  for (auto* user : bb->GetUsers()) {
    if (auto* phi = llvh::dyn_cast<PhiInst>(user)) {
      if (phi_block && phi->GetParent() != phi_block) {
        // We have PhiInsts in multiple blocks referencing bb, but bb is a
        // single static branch. This is invalid, but the bug is elsewhere.
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: SimplifyCFG found invalid Phi use across multiple "
            "blocks for a single-static-jump block");
      }
      phi_block = phi->GetParent();

      Value* our_value = nullptr;
      for (unsigned int i = 0; i < phi->GetNumEntries(); i++) {
        auto entry = phi->GetEntry(i);
        if (entry.second == bb) {
          if (our_value) {
            // The incoming phi node is invalid. The problem is not here.
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: SimplifyCFG found Phi with multiple entries "
                "for the same incoming block");
          }
          our_value = entry.first;
        }
      }

      if (!our_value) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: SimplifyCFG expected PhiInst to reference current "
            "block in user list");
      }

      for (unsigned int i = 0; i < phi->GetNumEntries(); i++) {
        auto entry = phi->GetEntry(i);
        if (block_parents.count(entry.second)) {
          // We have a PhiInst referencing our block bb and its parent, e.g.
          // %BB0:
          // CondBranchInst %1, %BB1, %BB2
          // %BB1:
          // BranchInst %BB2
          // %BB2:
          // PhiInst ??, %BB0, ??, %BB1
          if (entry.first == our_value) {
            // Fortunately, the two values are equal, so we can rewrite to:
            // PhiInst ??, %BB0
          } else {
            // Unfortunately, the value is different in each case,
            // which naively would have led to an invalid rewrite like:
            // PhiInst %1, %BB0, %2, %BB0
            return false;
          }
        }
      }
    }
  }
  if (!phi_block) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SimplifyCFG expected to rewrite Phi nodes but none "
        "were found");
  }

  // This branch is removable. Start by rewriting the Phi nodes.
  for (auto* user : bb->GetUsers()) {
    if (auto* phi = llvh::dyn_cast<PhiInst>(user)) {
      Value* our_value = nullptr;

      const unsigned int num_entries = phi->GetNumEntries();
      for (unsigned int i = 0; i < num_entries; i++) {
        auto entry = phi->GetEntry(i);
        if (entry.second == bb) {
          our_value = entry.first;
        }
      }
      if (LEPUS_UNLIKELY(!our_value)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: SimplifyCFG failed to find incoming value for "
            "PhiInst when rewriting");
      }

      for (int i = phi->GetNumEntries() - 1; i >= 0; i--) {
        auto pair = phi->GetEntry(i);
        if (pair.second == bb || block_parents.count(pair.second)) {
          phi->RemoveEntry(i);
        }
      }

      // Add parents back in sorted order to avoid any non-determinism
      for (Block* parent : ordered_parents) {
        phi->AddEntry(our_value, parent);
      }
    }
  }
  // We verified earlier that all uses are branches and phis, so now that
  // we've rewritten the phis, we can have all branches jump there directly.
  bb->ReplaceAllUsesWith(phi_block);
  bb->EraseFromParent();
  return true;
}

Block* IdentifySequentialBranch(Block* true_bb, Block* false_bb) {
  if (true_bb == false_bb) return true_bb;

  auto is_trampoline = [](Block* cur_bb, Block* target) -> bool {
    if (cur_bb->GetOpList().size() == 1) {
      if (auto* br = llvh::dyn_cast<BranchInst>(&*cur_bb->begin())) {
        return br->GetBranchDest() == target;
      }
    }
    return false;
  };

  if (is_trampoline(true_bb, false_bb)) return false_bb;
  if (is_trampoline(false_bb, true_bb)) return true_bb;
  return nullptr;
}

Block* IdentifyConvergingBranch(Block* true_bb, Block* false_bb) {
  if (true_bb->GetOpList().size() == 1 && false_bb->GetOpList().size() == 1) {
    auto* true_br = llvh::dyn_cast<BranchInst>(&*true_bb->begin());
    auto* false_br = llvh::dyn_cast<BranchInst>(&*false_bb->begin());

    if (true_br && false_br &&
        true_br->GetBranchDest() == false_br->GetBranchDest()) {
      return true_br->GetBranchDest();
    }
  }
  return nullptr;
}

inline bool IsPhiCanOpt(PhiInst* phi, Value* condition, Block* bb,
                        Block* true_bb, Block* false_bb) {
  if (!phi) return false;

  if (phi->GetNumEntries() != 2 || !phi->HasOneUser()) return false;

  auto* val1 = phi->GetEntry(0).first;
  auto* val2 = phi->GetEntry(1).first;

  if (!llvh::isa<Literal>(val1) && val1 != condition) {
    return false;
  }

  if (!llvh::isa<Literal>(val2) && val2 != condition) {
    return false;
  }

  auto* bb1 = phi->GetEntry(0).second;
  auto* bb2 = phi->GetEntry(1).second;

  if (bb1 != bb && bb1 != true_bb && bb1 != false_bb) return false;
  if (bb2 != bb && bb2 != true_bb && bb2 != false_bb) return false;

  return true;
}

bool OptIndirectJmp(Instruction* inst) {
  Block* cond_true_bb = nullptr;
  Block* cond_false_bb = nullptr;
  CondBranchInst* cbr = nullptr;

  if (llvh::isa<CondBranchInst>(inst)) {
    cbr = llvh::dyn_cast<CondBranchInst>(inst);
    cond_true_bb = cbr->GetTrueDest();
    cond_false_bb = cbr->GetFalseDest();
  } else {
    return false;
  }

  auto PhiCondBranchPatternFN = [&](Block* bb) -> CondBranchInst* {
    if (bb->GetOpList().size() != 2 || bb->GetNumUsers() != 2) return nullptr;
    auto* phi = llvh::dyn_cast<PhiInst>(*bb->InstBegin());
    if (IsPhiCanOpt(phi, cbr ? cbr->GetCondition() : nullptr, inst->GetBlock(),
                    cond_true_bb, cond_false_bb)) {
      auto* cond_branch_after_phi =
          llvh::dyn_cast<CondBranchInst>(*(++bb->InstBegin()));
      if (cond_branch_after_phi &&
          cond_branch_after_phi->GetCondition() == phi) {
        return cond_branch_after_phi;
      }
    }
    return nullptr;
  };
  // optimize the following pattern:
  // %BB4:
  //   %2 = mir.PhiInst (:boolean) false: boolean, %BB.block.2, true:
  //   boolean, %BB.block.1
  // mir.CondBranchInst %2: boolean, %BB7, %BB2
  // %BB3:
  //   mir.CondBranchInst %7: boolean, %BB.block.1, %BB.block.2
  // %BB.block.1:
  //   mir.BranchInst %BB4
  // %BB.block.2:
  //   mir.BranchInst %BB4
  // ==>
  // %BB3:
  // mir.CondBranchInst %7: boolean, %BB7, %BB2
  if (auto* target_bb = IdentifyConvergingBranch(cond_true_bb, cond_false_bb)) {
    if (auto* cond_branch_inst = PhiCondBranchPatternFN(target_bb)) {
      if (cbr) {
        cbr->SetTrueDest(cond_branch_inst->GetTrueDest());
        cbr->SetFalseDest(cond_branch_inst->GetFalseDest());
      }
      // NumIJB++;
      return true;
    }
  }
  // optimize the following pattern:
  // %BB2:
  // %4 = mir.BinaryStrictlyNotEqualInst (:boolean) %1: int64, 0: int64
  // mir.CondBranchInst %4: boolean, %BB3, %BB4
  // %BB4:
  // %2 = mir.PhiInst (:boolean) false: boolean, %BB3, true: boolean, %BB2
  // mir.CondBranchInst %2: boolean, %BB5, %BB2
  // %BB3:
  // mir.BranchInst %BB4
  // ==>
  // %BB2:
  // %4 = mir.BinaryStrictlyNotEqualInst (:boolean) %1: int64, 0: int64
  // mir.CondBranchInst %4: boolean, %BB5, %BB2
  if (auto* target_bb = IdentifySequentialBranch(cond_true_bb, cond_false_bb)) {
    if (auto* cond_branch_inst = PhiCondBranchPatternFN(target_bb)) {
      if (cbr) {
        cbr->SetTrueDest(cond_branch_inst->GetTrueDest());
        cbr->SetFalseDest(cond_branch_inst->GetFalseDest());
      }

      // NumIJB++;
      return true;
    }
  }
  return false;
}

static bool OptimizeIndirectJump(FuncOp* f) {
  bool changed = false;
  for (auto& it : *f) {
    Block* bb = &it;

    if (auto* cbr = llvh::dyn_cast<CondBranchInst>(bb->GetTerminator())) {
      changed = OptIndirectJmp(cbr);
    }
  }

  return changed;
}

static bool OptimizeSingleEntryPhi(FuncOp* f) {
  bool changed = false;
  for (auto& bb : *f) {
    for (auto it = bb.begin(), e = bb.end(); it != e;) {
      auto* phi_inst = llvh::dyn_cast<PhiInst>(&*it++);
      if (!phi_inst) break;

      if (phi_inst->GetNumEntries() == 1) {
        auto* val = phi_inst->GetEntry(0).first;
        phi_inst->ReplaceAllUsesWith(val);
        f->GetIRCtx()->UpdateSpecialAttribute(phi_inst, val);
        phi_inst->EraseFromParent();
        changed = true;
      }
    }
  }
  return changed;
}

/// Get rid of trampolines and merge basic blocks that are split by static
/// non-conditional branches.
bool SimplifyCFGPass::OptimizeStaticBranches(FuncOp* f) {
  bool changed = false;
  auto* builder = f->GetIRCtx()->GetOpBuilder();

  // Remove conditional branches with a constant condition.
  for (auto& it : *f) {
    Block* bb = &it;

    auto* cbr = llvh::dyn_cast<CondBranchInst>(bb->GetTerminator());
    if (!cbr) continue;

    Block* true_dest = cbr->GetTrueDest();
    Block* false_dest = cbr->GetFalseDest();

    // If both sides of the branch point to the same block then just jump to it
    // directly.
    if (true_dest == false_dest) {
      ReplaceCondBranchWithDirectBranch(cbr, true_dest);
      changed = true;

      // ++NumSB;
      continue;
    }

    // If the condition is optimized to a literal bool then replace the branch
    // with a non-conditional branch.
    auto* cond = cbr->GetCondition();
    Block* dest = nullptr;

    if (LiteralBool* B = EvalToBoolean(builder, cond)) {
      if (B->GetValue())
        dest = true_dest;
      else
        dest = false_dest;
    }

    if (dest != nullptr) {
      ReplaceCondBranchWithDirectBranch(cbr, dest);

      // ++NumSB;
      changed = true;
      continue;
    }
  }  // for all blocks.

  // Check if a basic block is a simple trampoline (empty non-conditional branch
  // to another basic block) and get rid of it. Replace all uses of the current
  // block with the destination of this block.
  for (auto& it : *f) {
    Block* bb = &it;
    auto* br = llvh::dyn_cast<BranchInst>(bb->GetTerminator());
    if (!br) continue;

    Block* dest = br->GetBranchDest();

    // Never rewrite or merge blocks across a catch label boundary.
    if (IsCatchBlock(bb) || IsCatchBlock(dest)) {
      continue;
    }

    // Don't try to optimize infinite loops or unreachable blocks.
    if (dest == bb) continue;

    // Don't handle edges that go across any catch region.
    // if (IsCrossCatchRegionBranch(bb, dest)) continue;

    // Handle branches used in phi nodes specially.
    if (IsUsedInPhiNode(bb)) {
      if (AttemptBranchRemovalFromPhiNodes(bb)) {
        // ++NumSB;

        changed = true;
        break;  // Iterator invalidated.
      }
      continue;
    }

    // Check if the terminator is the only instruction in the block.
    bool is_single_instr = (&*bb->begin() == br);

    // If the first and only instruction is a static branch, and it does not
    // cross a catch boundary then redirect all predecessors to the destination.
    if (is_single_instr && !PredEmpty(bb)) {
      bb->ReplaceAllUsesWith(dest);
      // ++NumSB;
      changed = true;
      if (LEPUS_UNLIKELY(PredCount(bb) != 0)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: SimplifyCFG expected removed block to have zero "
            "predecessors after replacement");
      }
      continue;
    }

    // If the source block is not empty then try to slurp the destination block
    // and eliminate it altogether.
    if (PredCount(dest) == 1 && dest != bb) {
      if (IsCatchBlock(dest)) {
        continue;
      }
      // Slurp the instructions from the destination block one by one.
      while (dest->InstBegin() != dest->InstEnd()) {
        (*dest->InstBegin())->MoveBefore(br);
      }

      // Now that we moved all of the instructions from the destination block we
      // can delete the original terminator and delete the destination block.
      dest->ReplaceAllUsesWith(bb);
      br->EraseFromParent();
      dest->EraseFromParent();

      // ++NumSB;

      changed = true;

      // We are invalidating the iterator here. Stop the scan and continue
      // afresh in the next iteration.
      break;
    }
  }  // for all blocks.

  return changed;
}

Pass* CreateSimplifyCFG(IRContext* ir_ctx) {
  return new SimplifyCFGPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
