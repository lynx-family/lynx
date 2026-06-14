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
static bool OptPhiNotCondBranchJumpThreading(FuncOp* f);
static bool OptCorrelatedBranches(FuncOp* f);
static bool OptPhiReturnThreading(FuncOp* f);
static bool OptSameCondJumpThreading(FuncOp* f);
static bool OptMergeTrivialCondArm(FuncOp* f);

bool SimplifyCFGPass::RunOnModule(ModuleOp* mod) {
  bool changed = false;

  llvh::for_each(*mod, [&](FuncOp* f) {
    bool func_changed = false;
    bool outer_changed = false;
    do {
      outer_changed = false;
      // Inner fixpoint: cheap CFG passes (no dominator tree needed).
      bool cheap_changed = false;
      do {
        cheap_changed = OptimizeIndirectJump(f) || OptimizeStaticBranches(f) ||
                        DeleteUnreachableBlocks(f) ||
                        OptimizeSingleEntryPhi(f) ||
                        OptCondInstWithSameTrueAndFalseBB(f);

        cheap_changed = cheap_changed || OptPhiCondBranchJumpThreading(f) ||
                        OptPhiNotCondBranchJumpThreading(f) ||
                        OptPhiReturnThreading(f) || OptSameCondJumpThreading(f);
        outer_changed |= cheap_changed;
      } while (cheap_changed);

      // Expensive pass: correlated branches (builds dominator tree).
      // Run only after cheap passes converge to minimize DT rebuilds.
      if (OptCorrelatedBranches(f)) {
        outer_changed = true;
      }
      func_changed |= outer_changed;
    } while (outer_changed);

    // Run trivial-arm merging once after the fixpoint loop to avoid
    // repeated application that accumulates register pressure.
    func_changed |= OptMergeTrivialCondArm(f);
    changed |= func_changed;
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
      // pred is already a direct predecessor of succ. Verify values match.
      Value* exist_val = phi->GetEntry(static_cast<unsigned>(idx_pred)).first;
      if (exist_val != new_val) {
        return false;
      }
      // Don't remove mid's entry — mid may still be reachable from other
      // preds. Cleanup at PredEmpty(mid) handles removal when mid becomes
      // unreachable.
    } else {
      // Add a new entry for pred; keep mid's entry for remaining paths
      // through mid.
      phi->AddEntry(new_val, pred);
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

  if (auto* eq = llvh::dyn_cast<EqCondBranchInst>(term)) {
    bool changed = false;
    if (eq->GetTrueDest() == from) {
      eq->SetSuccessorImpl(0, to);
      changed = true;
    }
    if (eq->GetFalseDest() == from) {
      eq->SetSuccessorImpl(1, to);
      changed = true;
    }
    return changed;
  }

  if (auto* neq = llvh::dyn_cast<NeqCondBranchInst>(term)) {
    bool changed = false;
    if (neq->GetTrueDest() == from) {
      neq->SetSuccessorImpl(0, to);
      changed = true;
    }
    if (neq->GetFalseDest() == from) {
      neq->SetSuccessorImpl(1, to);
      changed = true;
    }
    return changed;
  }

  return false;
}

// Helper: returns true if the value is definitely null or undefined at compile
// time. Used for jump threading through phi-based branch patterns.
static bool IsDefinitelyNullOrUndefined(Value* v) {
  if (!v) return false;
  if (llvh::isa<LiteralNull>(v) || llvh::isa<LiteralUndefined>(v)) return true;
  if (llvh::isa<LoadNullOrUndefinedInst>(v)) return true;
  if (auto* type = v->GetType()) {
    return type->IsNullOrUndefinedType();
  }
  return false;
}

// Helper: returns true if the value is definitely truthy (non-null object).
static bool IsDefinitelyTruthy(Value* v) {
  if (!v) return false;
  if (llvh::isa<NewTableInst>(v) || llvh::isa<NewArrayInst>(v)) return true;
  if (auto* type = v->GetType()) {
    return type->IsTableType();
  }
  return false;
}

// Thread edges through a phi-only block where the phi is used as a branch
// condition, e.g.
//   pred: CondBranchInst %c, mid, other
//   mid:  %p = PhiInst(..., %c, pred, false, x); CondBranchInst %p, T, F
// If the value of %p on a specific incoming edge is known (literal bool,
// null/undefined as falsy, or object as truthy), rewrite pred to jump
// directly to T/F.
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

    // Safety: if any phi in mid has a non-phi external user, threading could
    // break SSA dominance (mid may no longer dominate its successors after a
    // predecessor is redirected past it).
    {
      bool safe_to_thread = true;
      for (auto& inst : *mid) {
        if (&inst == cbr_mid) break;
        for (const auto* user : inst.GetUsers()) {
          if (auto* user_inst = llvh::dyn_cast<Instruction>(user)) {
            if (user_inst->GetParent() != mid &&
                !llvh::isa<PhiInst>(user_inst)) {
              safe_to_thread = false;
              break;
            }
          }
        }
        if (!safe_to_thread) break;
      }
      if (!safe_to_thread) continue;
    }

    // Snapshot predecessors because we'll mutate CFG.
    llvh::SmallVector<Block*, 8> predecessors;
    for (auto* pred : Predecessors(mid)) {
      predecessors.push_back(pred);
    }

    for (auto* pred : predecessors) {
      if (!pred || IsCatchBlock(pred)) continue;

      auto* pred_term = pred->GetTerminator();
      if (!llvh::isa<BranchInst>(pred_term) &&
          !llvh::isa<CondBranchInst>(pred_term) &&
          !llvh::isa<EqCondBranchInst>(pred_term) &&
          !llvh::isa<NeqCondBranchInst>(pred_term)) {
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
      } else if (IsDefinitelyNullOrUndefined(incoming)) {
        // null/undefined are always falsy: ToBoolean(null) = false
        known = true;
        cond_value = false;
      } else if (IsDefinitelyTruthy(incoming)) {
        // Objects (tables/arrays) are always truthy
        known = true;
        cond_value = true;
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
      } else if (auto* eq_cbr = llvh::dyn_cast<EqCondBranchInst>(pred_term)) {
        // EqCondBranch(X, null) true-edge: X == null → X is falsy
        Value* lhs = eq_cbr->GetLeftHandSide();
        Value* rhs = eq_cbr->GetRightHandSide();
        if (incoming == lhs || incoming == rhs) {
          Value* other = (incoming == lhs) ? rhs : lhs;
          if (IsDefinitelyNullOrUndefined(other)) {
            if (eq_cbr->GetTrueDest() == mid) {
              known = true;
              cond_value = false;
            }
          }
        }
      } else if (auto* neq_cbr = llvh::dyn_cast<NeqCondBranchInst>(pred_term)) {
        // NeqCondBranch(X, null) false-edge: X == null → X is falsy
        Value* lhs = neq_cbr->GetLeftHandSide();
        Value* rhs = neq_cbr->GetRightHandSide();
        if (incoming == lhs || incoming == rhs) {
          Value* other = (incoming == lhs) ? rhs : lhs;
          if (IsDefinitelyNullOrUndefined(other)) {
            if (neq_cbr->GetFalseDest() == mid) {
              known = true;
              cond_value = false;
            }
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
        // Safety: only erase mid if no instruction has uses outside of mid.
        bool has_external_uses = false;
        for (auto& inst : *mid) {
          for (const auto* user : inst.GetUsers()) {
            if (auto* user_inst = llvh::dyn_cast<Instruction>(user)) {
              if (user_inst->GetParent() != mid) {
                has_external_uses = true;
                break;
              }
            }
          }
          if (has_external_uses) break;
        }
        if (!has_external_uses) {
          RemoveEntryFromPhi(true_succ, mid);
          if (false_succ != true_succ) {
            RemoveEntryFromPhi(false_succ, mid);
          }
          mid->EraseFromParent();
        }
      }
      // CFG changed; let outer iteration and other passes clean up.
      return true;
    }
  }

  return changed;
}

// Thread edges through a block that contains phi(s) + Not(phi) + CondBranch.
// This handles the common template pattern:
//   merge: %phi = Phi(%null from null_bb, %val from call_bb)
//          %not = Not(%phi)
//          CondBranch(%not, create_bb, use_bb)
// From null_bb: %phi=null, Not(null)=true → thread to create_bb.
// From call_bb: if %val is truthy, Not(truthy)=false → thread to use_bb.
static bool OptPhiNotCondBranchJumpThreading(FuncOp* f) {
  if (!f) return false;

  for (auto& it : *f) {
    Block* mid = &it;
    if (IsCatchBlock(mid)) continue;

    auto* cbr_mid = llvh::dyn_cast<CondBranchInst>(mid->GetTerminator());
    if (!cbr_mid) continue;

    // Check mid block structure: phi(s) + exactly one UnaryNotInst +
    // CondBranchInst.
    UnaryOperatorInst* not_inst = nullptr;
    bool valid_structure = true;
    for (auto& inst : *mid) {
      if (&inst == cbr_mid) break;
      if (llvh::isa<PhiInst>(&inst)) continue;
      if (inst.GetKind() == ValueKind::UnaryNotInstKind && !not_inst) {
        not_inst = llvh::cast<UnaryOperatorInst>(&inst);
        continue;
      }
      valid_structure = false;
      break;
    }
    if (!valid_structure || !not_inst) continue;

    // The CondBranch condition must be the Not result.
    if (cbr_mid->GetCondition() != not_inst) continue;

    // Safety: not_inst must only be used by the CondBranch; otherwise threading
    // could break SSA if target has non-phi uses of this value.
    if (!not_inst->HasOneUser()) continue;

    // The Not's operand must be a phi defined in this block.
    auto* not_operand_phi =
        llvh::dyn_cast<PhiInst>(not_inst->GetSingleOperand());
    if (!not_operand_phi || not_operand_phi->GetParent() != mid) continue;

    Block* true_succ = cbr_mid->GetTrueDest();
    Block* false_succ = cbr_mid->GetFalseDest();
    if (!true_succ || !false_succ) continue;
    if (IsCatchBlock(true_succ) || IsCatchBlock(false_succ)) continue;

    // Safety: if any instruction in mid has a non-phi external user, threading
    // could break SSA dominance (mid may no longer dominate its successors
    // after a predecessor is redirected past it).
    {
      bool safe_to_thread = true;
      for (auto& inst : *mid) {
        if (&inst == cbr_mid) break;
        for (const auto* user : inst.GetUsers()) {
          if (auto* user_inst = llvh::dyn_cast<Instruction>(user)) {
            if (user_inst->GetParent() != mid &&
                !llvh::isa<PhiInst>(user_inst)) {
              safe_to_thread = false;
              break;
            }
          }
        }
        if (!safe_to_thread) break;
      }
      if (!safe_to_thread) continue;
    }

    // Snapshot predecessors because we'll mutate CFG.
    llvh::SmallVector<Block*, 8> predecessors;
    for (auto* pred : Predecessors(mid)) {
      predecessors.push_back(pred);
    }

    bool changed = false;
    for (auto* pred : predecessors) {
      if (!pred || IsCatchBlock(pred)) continue;

      auto* pred_term = pred->GetTerminator();
      if (!llvh::isa<BranchInst>(pred_term) &&
          !llvh::isa<CondBranchInst>(pred_term) &&
          !llvh::isa<EqCondBranchInst>(pred_term) &&
          !llvh::isa<NeqCondBranchInst>(pred_term)) {
        continue;
      }

      Value* incoming = nullptr;
      if (!GetPhiIncomingValue(not_operand_phi, pred, incoming)) {
        continue;
      }

      // Determine if incoming has known truthiness for Not() evaluation.
      bool known = false;
      bool cond_value = false;

      if (IsDefinitelyNullOrUndefined(incoming)) {
        known = true;
        cond_value = true;  // Not(null/undefined) = true → go to true_succ
      } else if (IsDefinitelyTruthy(incoming)) {
        known = true;
        cond_value = false;  // Not(truthy_object) = false → go to false_succ
      } else if (auto* lit = llvh::dyn_cast<LiteralBool>(incoming)) {
        known = true;
        cond_value = !lit->GetValue();  // Not(bool) = !bool
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
      if (PredEmpty(mid)) {
        // Safety: only erase mid if no instruction in mid has uses outside
        // of mid itself.  If mid dominated a successor, its Phis/Not could
        // be used directly by non-Phi instructions there; erasing would
        // leave dangling references.
        bool has_external_uses = false;
        for (auto& inst : *mid) {
          for (const auto* user : inst.GetUsers()) {
            if (auto* user_inst = llvh::dyn_cast<Instruction>(user)) {
              if (user_inst->GetParent() != mid) {
                has_external_uses = true;
                break;
              }
            }
          }
          if (has_external_uses) break;
        }
        if (!has_external_uses) {
          RemoveEntryFromPhi(true_succ, mid);
          if (false_succ != true_succ) {
            RemoveEntryFromPhi(false_succ, mid);
          }
          mid->EraseFromParent();
        }
      }
      return true;
    }
  }

  return false;
}

// Correlated Branch Elimination: if a CondBranch tests the same condition as a
// dominating CondBranch, and the current block is only reachable through the
// true (or false) edge of that dominator, the inner branch result is known and
// can be folded to an unconditional jump.
static bool OptCorrelatedBranches(FuncOp* f) {
  if (!f) return false;
  DominanceInfo DT(f);
  bool changed = false;

  // Verify that all predecessors of |edge_dest| are either |dom_bb| itself
  // or dominated by |edge_dest| (back-edges). If any other predecessor exists,
  // the CFG has a cross-edge from the opposite branch into |edge_dest|, which
  // means dominance alone cannot guarantee path exclusivity.
  auto AllPredecessorsAreExclusive = [&](Block* edge_dest,
                                         Block* dom_bb) -> bool {
    for (auto it = PredBegin(edge_dest); it != PredEnd(edge_dest); ++it) {
      Block* pred = *it;
      if (pred == dom_bb) continue;
      if (DT.dominates(edge_dest, pred)) continue;  // back-edge
      return false;
    }
    return true;
  };

  for (auto& bb : *f) {
    if (IsCatchBlock(&bb)) continue;

    auto* cbr = llvh::dyn_cast<CondBranchInst>(bb.GetTerminator());
    if (!cbr) continue;

    Value* cond = cbr->GetCondition();

    // Walk the immediate dominator chain looking for a dominator that also
    // branches on the same condition value.
    auto* node = DT.getNode(&bb);
    if (!node) continue;

    for (auto* i_dom = node->getIDom(); i_dom; i_dom = i_dom->getIDom()) {
      Block* dom_bb = i_dom->getBlock();
      if (!dom_bb) break;

      auto* dom_cbr = llvh::dyn_cast<CondBranchInst>(dom_bb->GetTerminator());
      if (!dom_cbr || dom_cbr->GetCondition() != cond) continue;

      // Check if &bb is reachable exclusively through the true or false edge.
      Block* dom_true = dom_cbr->GetTrueDest();
      Block* dom_false = dom_cbr->GetFalseDest();
      if (dom_true == dom_false) continue;

      if (DT.ProperlyDominates(dom_true, &bb) &&
          !DT.dominates(dom_true, dom_bb)) {
        // dom_true must only be entered from dom_bb's true edge (plus
        // back-edges). A cross-edge from the false sub-graph would invalidate
        // the condition-value inference — skip this opportunity.
        if (!AllPredecessorsAreExclusive(dom_true, dom_bb)) {
          continue;
        }
        // cond is known TRUE on this path → fold to true target.
        ReplaceCondBranchWithDirectBranch(cbr, cbr->GetTrueDest());
        changed = true;
        break;
      }
      if (DT.ProperlyDominates(dom_false, &bb) &&
          !DT.dominates(dom_false, dom_bb)) {
        // dom_false must only be entered from dom_bb's false edge (plus
        // back-edges). A cross-edge from the true sub-graph would invalidate
        // the condition-value inference — skip this opportunity.
        if (!AllPredecessorsAreExclusive(dom_false, dom_bb)) {
          continue;
        }
        // cond is known FALSE on this path → fold to false target.
        ReplaceCondBranchWithDirectBranch(cbr, cbr->GetFalseDest());
        changed = true;
        break;
      }
      continue;  // neither edge dominates bb, try higher dominator
    }

    if (changed) return true;  // CFG invalidated, restart
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

// Eliminate trivial re-branch blocks: a block whose only instruction is a
// CondBranch on a condition that a predecessor already branched on.
//   pred: CondBranch(C, mid, ...)   →   pred: CondBranch(C, mid_true, ...)
//   mid:  CondBranch(C, T, F)           (mid becomes unreachable)
static bool OptSameCondJumpThreading(FuncOp* f) {
  if (!f) return false;
  bool changed = false;

  for (auto& it : *f) {
    Block* mid = &it;
    if (IsCatchBlock(mid)) continue;

    auto* mid_cbr = llvh::dyn_cast<CondBranchInst>(mid->GetTerminator());
    if (!mid_cbr) continue;
    if (mid->Front() != mid_cbr) continue;

    Value* mid_cond = mid_cbr->GetCondition();
    Block* mid_true = mid_cbr->GetTrueDest();
    Block* mid_false = mid_cbr->GetFalseDest();

    // Skip if mid branches back to itself (self-loop).
    if (mid_true == mid || mid_false == mid) continue;

    // Never thread edges into a catch block.
    if (IsCatchBlock(mid_true) || IsCatchBlock(mid_false)) continue;

    llvh::SmallVector<Block*, 4> predecessors;
    for (auto* p : Predecessors(mid)) predecessors.push_back(p);

    for (auto* pred : predecessors) {
      if (!pred || IsCatchBlock(pred)) continue;
      auto* pred_cbr = llvh::dyn_cast<CondBranchInst>(pred->GetTerminator());
      if (!pred_cbr || pred_cbr->GetCondition() != mid_cond) continue;

      if (pred_cbr->GetTrueDest() == mid && mid_true != mid) {
        if (!RewriteSuccPhisForThreadedEdge(mid_true, mid, pred, mid)) continue;
        pred_cbr->SetTrueDest(mid_true);
        changed = true;
      }
      if (pred_cbr->GetFalseDest() == mid && mid_false != mid) {
        if (!RewriteSuccPhisForThreadedEdge(mid_false, mid, pred, mid))
          continue;
        pred_cbr->SetFalseDest(mid_false);
        changed = true;
      }
    }
  }
  return changed;
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

/// Merge a trivial conditional arm (LoadNull/LoadConst + Branch) into the
/// predecessor block. This eliminates the trivial block and enables better
/// block layout in ISel, saving one jump instruction per ternary pattern.
///
/// Pattern detected:
///   pred:      CondBranch(cond, true_bb, trivial_bb)
///   trivial_bb: v = LoadNull/LoadConst; Branch(join)
///   join:      phi(..., v/trivial_bb, ...)
///
/// Transformed to:
///   pred:      v = LoadNull/LoadConst; CondBranch(cond, true_bb, join)
///   join:      phi(..., v/pred, ...)
///   (trivial_bb removed)
///
/// Also handles the symmetric case where true_bb is the trivial arm.
// Maximum function size for trivial-arm merging. Beyond this threshold,
// hoisting values extends live ranges enough to cause register pressure.

static bool OptMergeTrivialCondArm(FuncOp* f) {
  // Only apply to small-to-medium functions. In large functions, hoisting
  // values before CondBranch extends live ranges enough to cause register
  // pressure issues (more MOV spills than saved Jmps).
  auto ExceedsInstLimit = [&]() -> bool {
    size_t count = 0;
    for (auto& bb : *f) {
      for (auto it = bb.InstBegin(); it != bb.InstEnd(); ++it) {
        if (++count > constants::kMergeTrivialCondArmMaxInsts) return true;
      }
    }
    return false;
  };
  if (ExceedsInstLimit()) return false;

  bool changed = false;
  // Collect blocks to erase after the loop to avoid iterator invalidation
  // (trivial_bb might be the next node in the intrusive block list).
  // SAFETY: After we erase the BranchInst from trivial_bb, it has no
  // terminator. If the outer for-loop encounters it, GetTerminator()
  // returns nullptr and we `continue`, so it's harmless to iterate over.
  llvh::SmallVector<Block*, 4> blocks_to_erase;

  for (auto& bb : *f) {
    auto* term = bb.GetTerminator();
    if (!term) continue;
    auto* cbr = llvh::dyn_cast<CondBranchInst>(term);
    if (!cbr) continue;

    Block* true_bb = cbr->GetTrueDest();
    Block* false_bb = cbr->GetFalseDest();
    if (true_bb == false_bb) continue;

    // Try both arms: prefer merging the false arm (more common for ternary).
    for (int arm = 0; arm < 2; ++arm) {
      Block* trivial_bb = (arm == 0) ? false_bb : true_bb;
      Block* other_bb = (arm == 0) ? true_bb : false_bb;

      // trivial_bb must have exactly one predecessor (pred).
      if (PredCount(trivial_bb) != 1) continue;

      // trivial_bb must not be a catch block.
      if (IsCatchBlock(trivial_bb)) continue;

      // trivial_bb must have exactly 2 instructions: one value + Branch.
      // (Use instruction count, not OpList size which includes non-inst ops.)
      auto it = trivial_bb->InstBegin();
      auto end = trivial_bb->InstEnd();
      if (it == end) continue;
      Instruction* val_inst = *it;
      ++it;
      if (it == end) continue;
      Instruction* branch_inst = *it;
      ++it;
      if (it != end) continue;  // More than 2 instructions.

      // The second instruction must be an unconditional branch.
      auto* br = llvh::dyn_cast<BranchInst>(branch_inst);
      if (!br) continue;

      Block* join_bb = br->GetBranchDest();

      // The first instruction must be side-effect-free and produce a value.
      // Accept: LoadNullOrUndefinedInst, LoadConstInst.
      if (!llvh::isa<LoadNullOrUndefinedInst>(val_inst) &&
          !llvh::isa<LoadConstInst>(val_inst)) {
        continue;
      }

      // join_bb must not be the same as pred or trivial_bb.
      if (join_bb == &bb || join_bb == trivial_bb) {
        continue;
      }

      // Never redirect an edge into a catch block.
      if (IsCatchBlock(&bb) || IsCatchBlock(other_bb) ||
          IsCatchBlock(join_bb)) {
        continue;
      }

      // Verify that other_bb also branches to join_bb (diamond pattern).
      // This ensures the phi in join_bb references both arms.
      {
        auto* other_term = other_bb->GetTerminator();
        bool other_reaches_join = false;
        if (auto* other_br = llvh::dyn_cast<BranchInst>(other_term)) {
          other_reaches_join = (other_br->GetBranchDest() == join_bb);
        } else if (auto* other_cbr =
                       llvh::dyn_cast<CondBranchInst>(other_term)) {
          other_reaches_join = (other_cbr->GetTrueDest() == join_bb ||
                                other_cbr->GetFalseDest() == join_bb);
        }
        if (!other_reaches_join) continue;
      }

      // Verify that trivial_bb's value is only used in phi nodes of join_bb.
      // If it has other users, we can't safely hoist it.
      bool val_safe = true;
      for (const auto* user : val_inst->GetUsers()) {
        auto* phi_user = llvh::dyn_cast<PhiInst>(user);
        if (!phi_user || phi_user->GetParent() != join_bb) {
          val_safe = false;
          break;
        }
      }
      if (!val_safe) continue;

      // All checks passed. Perform the transformation.

      // 1. Move val_inst before the CondBranch in pred.
      val_inst->MoveBefore(cbr);

      // 2. Update phi nodes in join_bb: replace trivial_bb with pred (&bb).
      for (auto& inst : *join_bb) {
        auto* phi = llvh::dyn_cast<PhiInst>(&inst);
        if (!phi) break;
        for (unsigned i = 0, e = phi->GetNumEntries(); i < e; i++) {
          if (phi->GetEntry(i).second == trivial_bb) {
            phi->ForceUpdateEntry(i, phi->GetEntry(i).first, &bb);
            break;
          }
        }
      }

      // 3. Redirect the CondBranch edge from trivial_bb to join_bb.
      if (arm == 0) {
        cbr->SetFalseDest(join_bb);
      } else {
        cbr->SetTrueDest(join_bb);
      }

      // 4. Remove branch inst now; defer block erasure to avoid
      //    invalidating the outer block-list iterator.
      br->EraseFromParent();
      blocks_to_erase.push_back(trivial_bb);

      changed = true;
      break;  // Break inner arm loop, continue outer block loop.
    }
  }

  // Erase collected trivial blocks after iteration is complete.
  for (Block* blk : blocks_to_erase) {
    blk->ReplaceAllUsesWith(nullptr);
    blk->EraseFromParent();
  }

  return changed;
}

Pass* CreateSimplifyCFG(IRContext* ir_ctx) {
  return new SimplifyCFGPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
