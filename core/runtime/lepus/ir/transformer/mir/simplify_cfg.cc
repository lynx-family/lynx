// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/simplify_cfg.h"

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
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
static bool OptCondInstWithSameTrueAndFalseBB(FuncOp* f);
static bool OptPhiCondBranchJumpThreading(FuncOp* f);
static bool OptPhiReturnThreading(FuncOp* f);

bool SimplifyCFGPass::RunOnModule(ModuleOp* mod) {
  bool changed = false;

  llvh::for_each(*mod, [&](FuncOp* f) {
    bool iter_changed = false;
    // Keep iterating over deleting unreachable code and removing
    // trampolines as long as we are making progress.
    do {
      iter_changed = OptimizeIndirectJump(f) || OptimizeStaticBranches(f) ||
                     DeleteUnreachableBlocks(f) || OptimizeSingleEntryPhi(f) ||
                     OptCondInstWithSameTrueAndFalseBB(f);

      iter_changed = iter_changed || OptPhiCondBranchJumpThreading(f) ||
                     OptPhiReturnThreading(f);

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

static bool GetPhiIncomingValue(PhiInst* phi, Block* pred, Value*& out_val) {
  if (!phi || !pred) return false;
  Value* found = nullptr;
  for (unsigned i = 0, e = phi->GetNumEntries(); i < e; i++) {
    auto entry = phi->GetEntry(i);
    if (entry.second == pred) {
      if (found) {
        // Invalid phi: multiple entries from the same predecessor.
        return false;
      }
      found = entry.first;
    }
  }
  if (!found) return false;
  out_val = found;
  return true;
}

static bool TranslateValueThroughPhiOnlyBlock(Value* val, Block* pred,
                                              Block* mid, Value*& out_val) {
  if (!val) return false;
  // Values defined outside mid are still available if mid is bypassed.
  auto* inst = llvh::dyn_cast<Instruction>(val);
  if (!inst || inst->GetParent() != mid) {
    out_val = val;
    return true;
  }

  // Only allow translating PhiInsts in mid. Any other instruction would be
  // skipped by bypassing mid and could break SSA.
  auto* phi = llvh::dyn_cast<PhiInst>(inst);
  if (!phi) return false;
  return GetPhiIncomingValue(phi, pred, out_val);
}

// When threading an edge pred -> mid -> succ into pred -> succ, rewrite succ's
// Phi entries that referenced mid to now reference pred. Any value defined in
// mid must be translated to the corresponding incoming value from pred.
static bool RewriteSuccPhisForThreadedEdge(Block* succ, Block* mid, Block* pred,
                                           Block* mid_block_for_value_map) {
  if (!succ || !mid || !pred) return false;

  for (auto& inst : *succ) {
    auto* phi = llvh::dyn_cast<PhiInst>(&inst);
    if (!phi) break;

    int idx_mid = -1;
    int idx_pred = -1;
    for (int i = 0, e = static_cast<int>(phi->GetNumEntries()); i < e; i++) {
      auto entry = phi->GetEntry(i);
      if (entry.second == mid) idx_mid = i;
      if (entry.second == pred) idx_pred = i;
    }
    if (idx_mid < 0) {
      // If succ has phi nodes but none refer to mid, IR is inconsistent.
      // Bail out for safety.
      return false;
    }

    Value* old_val = phi->GetEntry(static_cast<unsigned>(idx_mid)).first;
    Value* new_val = nullptr;
    if (!TranslateValueThroughPhiOnlyBlock(old_val, pred,
                                           mid_block_for_value_map, new_val)) {
      return false;
    }

    if (idx_pred >= 0) {
      // succ already has pred as a predecessor. Only allow if the incoming
      // values match; otherwise we'd need two entries from the same block.
      Value* exist_val = phi->GetEntry(static_cast<unsigned>(idx_pred)).first;
      if (exist_val != new_val) {
        return false;
      }
      phi->RemoveEntry(static_cast<unsigned>(idx_mid));
    } else {
      phi->UpdateEntry(static_cast<unsigned>(idx_mid), new_val, pred);
    }
  }
  return true;
}

static bool RedirectTerminatorEdge(Block* pred, Block* from, Block* to) {
  if (!pred || !from || !to) return false;
  auto* term = pred->GetTerminator();
  if (!term) return false;

  if (auto* br = llvh::dyn_cast<BranchInst>(term)) {
    if (br->GetBranchDest() != from) return false;
    br->SetBranchDest(to);
    return true;
  }

  if (auto* cbr = llvh::dyn_cast<CondBranchInst>(term)) {
    bool changed = false;
    if (cbr->GetTrueDest() == from) {
      cbr->SetTrueDest(to);
      changed = true;
    }
    if (cbr->GetFalseDest() == from) {
      cbr->SetFalseDest(to);
      changed = true;
    }
    return changed;
  }
  return false;
}

// Thread edges through a phi-only block where the phi is used as a branch
// condition, e.g.
//   pred: CondBranchInst %c, mid, other
//   mid:  %p = PhiInst(..., %c, pred, false, x); CondBranchInst %p, T, F
// If the value of %p on a specific incoming edge is known (literal bool or
// implied by pred's taken branch), rewrite pred to jump directly to T/F.
static bool OptPhiCondBranchJumpThreading(FuncOp* f) {
  if (!f) return false;
  bool changed = false;

  for (auto& it : *f) {
    Block* mid = &it;
    if (IsCatchBlock(mid)) continue;

    auto* cbr_mid = llvh::dyn_cast<CondBranchInst>(mid->GetTerminator());
    if (!cbr_mid) continue;

    // Require the mid block to be: phi(s) + terminator only.
    for (auto& inst : *mid) {
      if (&inst == mid->GetTerminator()) break;
      if (!llvh::isa<PhiInst>(&inst)) {
        cbr_mid = nullptr;
        break;
      }
    }
    if (!cbr_mid) continue;

    auto* cond_phi = llvh::dyn_cast<PhiInst>(cbr_mid->GetCondition());
    if (!cond_phi || cond_phi->GetParent() != mid) continue;
    if (!cond_phi->HasOneUser()) continue;

    Block* true_succ = cbr_mid->GetTrueDest();
    Block* false_succ = cbr_mid->GetFalseDest();
    if (!true_succ || !false_succ) continue;
    if (IsCatchBlock(true_succ) || IsCatchBlock(false_succ)) continue;

    // Snapshot predecessors because we'll mutate CFG.
    llvh::SmallVector<Block*, 8> predecessors;
    for (auto* pred : Predecessors(mid)) {
      predecessors.push_back(pred);
    }

    for (auto* pred : predecessors) {
      if (!pred || IsCatchBlock(pred)) continue;

      // Be conservative: only thread through simple branch terminators.
      // Eq/NeqCondBranchInst lower to short cmp-jmp bytecodes with 8-bit
      // offsets, which can overflow if we retarget far-away blocks.
      auto* pred_term = pred->GetTerminator();
      if (!llvh::isa<BranchInst>(pred_term) &&
          !llvh::isa<CondBranchInst>(pred_term)) {
        continue;
      }

      Value* incoming = nullptr;
      if (!GetPhiIncomingValue(cond_phi, pred, incoming)) {
        continue;
      }

      bool known = false;
      bool cond_value = false;

      if (auto* lit = llvh::dyn_cast<LiteralBool>(incoming)) {
        known = true;
        cond_value = lit->GetValue();
      } else if (auto* pred_cbr = llvh::dyn_cast<CondBranchInst>(pred_term)) {
        // If pred branches on the same SSA value and this edge is the taken
        // true/false edge, then the condition is implied on this edge.
        if (incoming == pred_cbr->GetCondition()) {
          if (pred_cbr->GetTrueDest() == mid) {
            known = true;
            cond_value = true;
          } else if (pred_cbr->GetFalseDest() == mid) {
            known = true;
            cond_value = false;
          }
        }
      }

      if (!known) continue;

      Block* target = cond_value ? true_succ : false_succ;
      if (!target) continue;

      // Only thread edges that actually go to mid.
      if (!SuccContains(pred, mid)) continue;

      // Rewrite successor phis and then redirect pred's terminator edge.
      if (!RewriteSuccPhisForThreadedEdge(target, mid, pred, mid)) {
        continue;
      }
      if (!RedirectTerminatorEdge(pred, mid, target)) {
        continue;
      }

      // Remove pred entry from mid's phi nodes.
      RemoveEntryFromPhi(mid, pred);

      changed = true;
    }

    if (changed) {
      // If we threaded away all incoming edges, mid is now unreachable. Clean
      // up successor phi entries and erase it immediately to avoid leaving
      // invalid 0-entry phis around.
      if (PredEmpty(mid)) {
        RemoveEntryFromPhi(true_succ, mid);
        if (false_succ != true_succ) {
          RemoveEntryFromPhi(false_succ, mid);
        }
        mid->EraseFromParent();
      }
      // CFG changed; let outer iteration and other passes clean up.
      return true;
    }
  }

  return changed;
}

// If a join block only contains phi nodes and then immediately returns a phi,
// thread the return back into each predecessor:
//   pred_i: BranchInst join
//   join:   %v = PhiInst(...); ReturnInst %v
static bool OptPhiReturnThreading(FuncOp* f) {
  if (!f) return false;
  auto* ir_ctx = f->GetIRCtx();
  if (!ir_ctx) return false;
  auto* builder = ir_ctx->GetOpBuilder();
  if (!builder) return false;

  for (auto& it : *f) {
    Block* join = &it;
    if (IsCatchBlock(join)) continue;

    auto* ret = llvh::dyn_cast<ReturnInst>(join->GetTerminator());
    if (!ret) continue;

    // Require join block to be phi(s) + ReturnInst only.
    for (auto& inst : *join) {
      if (&inst == ret) break;
      if (!llvh::isa<PhiInst>(&inst)) {
        ret = nullptr;
        break;
      }
    }
    if (!ret) continue;

    auto* ret_phi = llvh::dyn_cast<PhiInst>(ret->GetValue());
    if (!ret_phi || ret_phi->GetParent() != join) continue;
    if (!ret_phi->HasOneUser()) continue;

    // Snapshot predecessors.
    llvh::SmallVector<Block*, 8> predecessors;
    for (auto* pred : Predecessors(join)) {
      predecessors.push_back(pred);
    }
    if (predecessors.empty()) continue;

    // Only handle simple predecessors that unconditionally branch to join.
    for (auto* pred : predecessors) {
      auto* br =
          llvh::dyn_cast<BranchInst>(pred ? pred->GetTerminator() : nullptr);
      if (!br || br->GetBranchDest() != join) {
        predecessors.clear();
        break;
      }
      if (IsCatchBlock(pred)) {
        predecessors.clear();
        break;
      }
    }
    if (predecessors.empty()) continue;

    // Rewrite each pred terminator to ReturnInst(incoming).
    for (auto* pred : predecessors) {
      Value* incoming = nullptr;
      if (!GetPhiIncomingValue(ret_phi, pred, incoming)) {
        predecessors.clear();
        break;
      }

      auto* old_term = pred->GetTerminator();
      if (LEPUS_UNLIKELY(!old_term)) {
        predecessors.clear();
        break;
      }

      OpBuilderRestoreInsertPointerRAII _restore(builder);
      builder->SetInsertionPointToEnd(pred);
      builder->Create<ReturnInst>(ret->GetLocation(), incoming);
      old_term->EraseFromParent();
    }
    if (predecessors.empty()) continue;

    // Now join is unreachable; erase it.
    join->EraseFromParent();
    return true;
  }
  return false;
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

// If `bb` ends with a conditional branch whose true/false destinations are the
// same, replace it with an unconditional BranchInst.
static bool SimplifySameDestBranchToBranchInPlace(Block* bb,
                                                  OpBuilder* builder) {
  if (!bb || !builder) return false;
  auto* term = bb->GetTerminator();
  if (!term) return false;

  Block* dst = nullptr;
  if (llvh::isa<BranchInst>(term)) {
    return false;
  } else if (auto* br = llvh::dyn_cast<CondBranchInst>(term)) {
    if (br->GetTrueDest() == br->GetFalseDest()) dst = br->GetTrueDest();
  } else if (auto* br = llvh::dyn_cast<EqCondBranchInst>(term)) {
    if (br->GetTrueDest() == br->GetFalseDest()) dst = br->GetTrueDest();
  } else if (auto* br = llvh::dyn_cast<NeqCondBranchInst>(term)) {
    if (br->GetTrueDest() == br->GetFalseDest()) dst = br->GetTrueDest();
  }
  if (!dst) return false;

  OpBuilderRestoreInsertPointerRAII _restore(builder);
  builder->SetInsertionPointToEnd(bb);
  builder->Create<BranchInst>(term->GetLocation(), dst);
  term->EraseFromParent();
  return true;
}

static bool OptCondInstWithSameTrueAndFalseBB(FuncOp* func) {
  auto* builder = func->GetIRCtx()->GetOpBuilder();
  if (!func || !builder) return false;
  bool changed = false;
  for (auto& bb : *func) {
    if (SimplifySameDestBranchToBranchInPlace(&bb, builder)) {
      changed = true;
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
