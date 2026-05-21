// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/mov_elimination.h"

#include <utility>

#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

static inline uint64_t PackRegPair(unsigned src_reg, unsigned dst_reg) {
  // Ensure both registers fit in 32 bits before packing.
  if ((static_cast<uint64_t>(src_reg) >> 32) != 0) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: PackRegPair src_reg must fit in 32 bits");
  }
  if ((static_cast<uint64_t>(dst_reg) >> 32) != 0) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: PackRegPair dst_reg must fit in 32 bits");
  }
  return (static_cast<uint64_t>(src_reg) << 32) |
         static_cast<uint64_t>(dst_reg);
}

void MovEliminationPass::BuildSideTableAnchors(IRContext* ir_ctx, FuncOp* func,
                                               SideTableAnchors& anchors) {
  anchors.clear();
  if (!ir_ctx || !func) return;

  // IRContext toplevel variables / FuncOp closure var side tables are consumed
  // by later passes (UpdateToplevelVarRegPass / UpdateToplevelClosureVarPass
  // / instruction selection). Use O(1) membership here to avoid repeatedly
  // scanning those side tables on hot paths.
  anchors.reserve(ir_ctx->GetToplevelVariables().size() +
                  func->GetClosureVarReg2ValueMap().size());
  for (const auto& kv : ir_ctx->GetToplevelVariables()) {
    anchors.insert(kv.second);
  }
  for (const auto& kv : func->GetClosureVarReg2ValueMap()) {
    anchors.insert(kv.second);
  }
}

// Build per-register instruction lists for interval overlap checks.
// regToInsts maps each register index to the set of instructions allocated
// there. regHasNonInst marks registers that also hold non-instruction values.
void MovEliminationPass::BuildRegToInsts(RegisterAllocator* ra,
                                         unsigned max_regs,
                                         RegToInstsMap& regToInsts,
                                         RegHasNonInstMap& regHasNonInst) {
  regToInsts.clear();
  regToInsts.resize(max_regs);
  regHasNonInst.assign(max_regs, false);
  for (auto& pair : ra->GetAllocatedMap()) {
    if (pair.second.IsValid() && pair.second.GetIndex() < max_regs) {
      if (auto* i = llvh::dyn_cast<Instruction>(pair.first)) {
        regToInsts[pair.second.GetIndex()].push_back(i);
      } else {
        regHasNonInst[pair.second.GetIndex()] = true;
      }
    }
  }
}

// Remove redundant MOVs after register allocation.
//
// This function performs two local (per-basic-block) optimizations on MovInst:
//
// 1) Self-move elimination:
//    If a MovInst's source and destination are allocated to the same physical
//    register, the move is a no-op and can be removed.
//
// 2) Duplicate move coalescing within a block:
//    If we see the *same* (src_reg -> dst_reg) move multiple times in a block,
//    and there is no instruction in-between that could change either register,
//    later moves can be replaced by the earlier move result.
//
// Safety constraints:
// - Ignore MovInst with special attributes (ClosureVarReg / ToplevelVarReg):
//   they are used by later lowering passes and must not be deleted or rewritten
//   here.
// - MovInst referenced by side tables (toplevel vars / closure vars) are
//   treated as an invariant violation (they should carry special attributes).
// - When reusing an earlier move, we extend that move's live interval. We must
//   ensure the extension does NOT violate call-related register constraints.
//
// Implementation notes:
// - We maintain a small map `active_movs` per block, keyed by packed
//   (src_reg, dst_reg). The entry is invalidated whenever an instruction writes
//   to either src_reg or dst_reg.
bool MovEliminationPass::RemoveMovWithSameSrcAndDstImpl(
    FuncOp* func, const SideTableAnchors& side_table_anchors) {
  to_removed_.clear();

  RegisterAllocator* ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (!ra) return false;

  bool changed = false;
  for (auto& block : *func) {
    // Track currently-valid (src_reg -> dst_reg) moves in this block.
    // The mapping is only valid until either register is clobbered.
    llvh::SmallDenseMap<uint64_t, MovInst*, 32> active_movs;

    for (auto& op : block) {
      if (!llvh::isa<Instruction>(&op)) continue;
      auto* inst = llvh::dyn_cast<Instruction>(&op);

      bool redundant = false;
      Value* replacement = nullptr;
      unsigned src_reg = 0, dst_reg = 0;
      std::pair<unsigned, unsigned> key;
      bool is_tracked_mov = false;

      if (auto* mov = llvh::dyn_cast<MovInst>(inst)) {
        // Only touch MovInst without side-table attributes (toplevel/closure).
        // Call-func MOVs are allowed here, but their dedicated elimination is
        // handled by EliminateCallFuncMov().
        unsigned closure = mov->GetClosureVarReg();
        unsigned toplevel = mov->GetToplevelVarReg();
        if (closure == constants::kInvalidSignedValue &&
            toplevel == constants::kInvalidSignedValue) {
          // A MovInst without special attributes must never be referenced by
          // side tables. If it is, later lowering would become inconsistent.
          if (side_table_anchors.count(mov)) {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: "
                "MovEliminationPass::RemoveMovWithSameSrcAndDst encountered "
                "side-table anchor MovInst without special attributes");
          }
          if (mov->GetNumUsers() == 0) {
            // This pass is allowed to delete dead MOVs (unlike a full DCE).
            to_removed_.push_back(llvh::cast<MovInst>(inst));
            continue;
          }

          Value* src = mov->GetSingleOperand();
          // MovEliminationPass runs after RegisterAllocationPass, but some MOV
          // instructions can be unallocated (e.g. they are kept only for
          // correctness/metadata reasons or will be removed later). Never call
          // GetRegister on an unallocated Value.
          bool allocated = ra->IsAllocated(src) && ra->IsAllocated(mov);
          if (allocated) {
            src_reg = ra->GetRegister(src).GetIndex();
            dst_reg = ra->GetRegister(mov).GetIndex();

            if (src_reg == dst_reg) {
              // Self-move is always redundant.
              redundant = true;
              replacement = src;

              // Update the live interval of the replacement instruction to
              // include the interval of the redundant one.
              if (auto* src_inst = llvh::dyn_cast<Instruction>(src)) {
                ra->GetInstructionInterval(src_inst).Add(
                    ra->GetInstructionInterval(inst));
              }
            } else {
              uint64_t packed_key = PackRegPair(src_reg, dst_reg);
              key = std::make_pair(src_reg, dst_reg);
              auto it = active_movs.find(packed_key);
              if (it != active_movs.end()) {
                // Same move already exists and is still valid (no clobber).
                // Reuse the earlier move result and delete the current one.
                MovInst* original_mov = it->second;

                // If we replace `inst` with `original_mov`, we extend
                // `original_mov`'s live range. We must verify this extension
                // doesn't violate call register constraints.
                Interval extended_interval;
                if (ra->HasInstructionNumber(original_mov) &&
                    ra->HasInstructionNumber(inst)) {
                  extended_interval = ra->GetInstructionInterval(original_mov);
                  extended_interval.Add(ra->GetInstructionInterval(inst));

                  if (!HasCallConflict(ra, extended_interval, dst_reg)) {
                    redundant = true;
                    replacement = original_mov;
                    ra->GetInstructionInterval(original_mov) =
                        extended_interval;
                  }
                }
              }
            }
            is_tracked_mov = true;
          }
        }
      }

      if (redundant) {
        inst->ReplaceAllUsesWith(replacement);
        ra->RemoveFromAllocated(inst);
        to_removed_.push_back(llvh::cast<MovInst>(inst));
        changed = true;
      } else {
        // Not redundant. If this instruction writes to some register, it may
        // invalidate cached (src_reg -> dst_reg) moves that rely on that reg.
        if (inst->HasOutput() && ra->IsAllocated(inst)) {
          unsigned out_reg = ra->GetRegister(inst).GetIndex();
          // Invalidate any active move whose src or dst equals out_reg.
          for (auto active_it = active_movs.begin();
               active_it != active_movs.end();) {
            uint64_t packed = active_it->first;
            unsigned active_src_reg = static_cast<unsigned>(packed >> 32);
            unsigned active_dst_reg = static_cast<unsigned>(packed);
            if (active_src_reg == out_reg || active_dst_reg == out_reg) {
              auto to_erase = active_it++;
              active_movs.erase(to_erase);
            } else {
              ++active_it;
            }
          }
        }

        // If it was a tracked MovInst, add it to active_movs now
        if (is_tracked_mov) {
          active_movs[PackRegPair(src_reg, dst_reg)] =
              llvh::cast<MovInst>(inst);
        }
      }
    }
  }
  llvh::for_each(to_removed_, [](MovInst* inst) { inst->EraseFromParent(); });
  return changed;
}

// Remove "back-copy" MOV pairs like:
//   mov  rA <- rB
//   mov  rB <- rA
//
// When these two moves are adjacent, the second move is always a no-op:
// after the first instruction, register rA already holds the value of rB, and
// writing it back into rB does not change rB.
//
// This pattern typically appears after register allocation / compaction when a
// value is copied into a temporary register and then immediately copied back.
// Removing the second MOV improves bytecode size and avoids redundant debug
// line/col entries.
bool MovEliminationPass::RemoveBackToBackReverseMovImpl(
    FuncOp* func, RegisterAllocator* ra,
    const SideTableAnchors& side_table_anchors) {
  to_removed_.clear();

  bool changed = false;
  for (auto& block : *func) {
    for (auto it = block.begin(); it != block.end();) {
      auto next_it = it;
      ++next_it;
      if (next_it == block.end()) break;

      auto* mov1 = llvh::dyn_cast<MovInst>(&*it);
      auto* mov2 = llvh::dyn_cast<MovInst>(&*next_it);
      if (!mov1 || !mov2) {
        ++it;
        continue;
      }

      // Only handle normal MOVs. Special/call-func MOVs must not be rewritten.
      if (mov1->IsCallFuncMov() || mov2->IsCallFuncMov()) {
        ++it;
        continue;
      }
      if (mov1->GetClosureVarReg() != constants::kInvalidSignedValue ||
          mov1->GetToplevelVarReg() != constants::kInvalidSignedValue ||
          mov2->GetClosureVarReg() != constants::kInvalidSignedValue ||
          mov2->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        ++it;
        continue;
      }
      if (side_table_anchors.count(mov1) || side_table_anchors.count(mov2)) {
        ++it;
        continue;
      }

      Value* src1_val = mov1->GetSingleOperand();
      Value* src2_val = mov2->GetSingleOperand();

      if (!ra->IsAllocated(mov1) || !ra->IsAllocated(mov2) ||
          !ra->IsAllocated(src1_val) || !ra->IsAllocated(src2_val)) {
        ++it;
        continue;
      }

      const unsigned src1_reg = ra->GetRegister(src1_val).GetIndex();
      const unsigned dst1_reg = ra->GetRegister(mov1).GetIndex();
      const unsigned src2_reg = ra->GetRegister(src2_val).GetIndex();
      const unsigned dst2_reg = ra->GetRegister(mov2).GetIndex();

      // Match the adjacent "back-copy" pattern.
      // dst1 <- src1
      // dst2 <- src2
      // and we want (dst2 == src1) && (src2 == dst1).
      if (!(dst2_reg == src1_reg && src2_reg == dst1_reg)) {
        ++it;
        continue;
      }

      // Rewrite mov2 users to read directly from mov1's source.
      // This preserves the physical register (dst2_reg == src1_reg).
      mov2->ReplaceAllUsesWith(src1_val);

      // Preserve liveness info: the original src value must cover mov2's range.
      if (auto* src1_inst = llvh::dyn_cast<Instruction>(src1_val)) {
        ra->GetInstructionInterval(src1_inst).Add(
            ra->GetInstructionInterval(mov2));
      }

      ra->RemoveFromAllocated(mov2);
      to_removed_.push_back(mov2);
      changed = true;

      // Skip both instructions. `mov2` will be erased after the scan.
      it = next_it;
      ++it;
    }
  }

  if (changed) {
    llvh::for_each(to_removed_, [](MovInst* inst) { inst->EraseFromParent(); });
  }
  return changed;
}

bool MovEliminationPass::HasCallConflict(RegisterAllocator* ra,
                                         const Interval& interval,
                                         unsigned reg_idx,
                                         Instruction* exclude_inst) {
  // Check whether extending a value's live interval would violate call
  // register constraints.
  //
  // Background:
  // In this VM, argument materialization for a CallInst may clobber registers
  // below (or equal to) the callee register. Therefore, for any point inside
  // the candidate interval, we require:
  //   callee_reg > reg_idx
  // Otherwise the value in reg_idx may be overwritten before the call reads
  // its callee, making MOV elimination / coalescing unsafe.
  //
  // Parameters:
  // - ra: RegisterAllocator that can map instruction-number -> Instruction and
  //   Value -> physical register.
  // - interval: the (possibly extended) live interval we want to validate.
  // - reg_idx: the physical register index whose liveness is being extended.
  // - exclude_inst: an optional instruction to ignore (typically the MOV being
  //   eliminated) to avoid self-checking.
  for (const auto& seg : interval.segments_) {
    for (size_t val = seg.start_; val < seg.end_; ++val) {
      if (val == 0) continue;
      unsigned inst_idx = val - 1;
      Instruction* inst = ra->GetInstructionByNumber(inst_idx);
      if (!inst || inst == exclude_inst) continue;

      if (auto* call_inst = llvh::dyn_cast<CallInst>(inst)) {
        Value* callee = call_inst->GetFunction();
        if (ra->IsAllocated(callee)) {
          unsigned func_reg = ra->GetRegister(callee).GetIndex();
          if (func_reg <= reg_idx) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

// Shared coalescing logic for EliminateCallFuncMovImpl and
// EliminateGeneralMovImpl. Caller must have already validated that mov and
// src_inst are eligible for coalescing (correct MOV type, not a side-table
// anchor, not FixReg, not PhiInst, etc.). This method handles:
//   1) Allocation / self-move guard
//   2) Operand-in-dst-reg conflict check
//   3) target_call_inst search (MOV → CallInst method operand)
//   4) HasCallConflict + interval overlap checks
//   5) Apply: extend interval, UpdateRegister, SetFixReg, update regToInsts
// Returns true if coalescing was applied.
bool MovEliminationPass::TryCoalesceMovSource(MovInst* mov,
                                              Instruction* src_inst,
                                              RegisterAllocator* ra,
                                              unsigned max_regs,
                                              RegToInstsMap& regToInsts,
                                              RegHasNonInstMap& regHasNonInst) {
  if (!ra->IsAllocated(src_inst) || !ra->IsAllocated(mov)) return false;

  unsigned dst_reg_idx = ra->GetRegister(mov).GetIndex();
  unsigned src_reg_idx = ra->GetRegister(src_inst).GetIndex();

  // Already a self-move — let RemoveMovWithSameSrcAndDst handle it.
  if (dst_reg_idx == src_reg_idx) return false;

  // Conservative safety: avoid assigning to the same register as any
  // operand of the producer instruction. Some VM instructions may not be
  // safe with dst==src in the lowering.
  for (int op_i = 0, op_e = static_cast<int>(src_inst->GetNumOperands());
       op_i < op_e; ++op_i) {
    Value* opv = src_inst->GetOperand(op_i);
    if (!opv || !ra->IsAllocated(opv)) continue;
    if (ra->GetRegister(opv).GetIndex() == dst_reg_idx) {
      return false;
    }
  }

  // Find if this MOV feeds a call as the function operand.
  // Only call-func MOVs can serve as CallInst::methodIdx, so skip the
  // search for general MOVs to avoid unnecessary user-list traversal.
  Instruction* target_call_inst = nullptr;
  if (mov->IsCallFuncMov()) {
    for (auto* user : mov->GetUsers()) {
      if (auto* call = llvh::dyn_cast<CallInst>(user)) {
        if (call->GetOperand(CallInst::methodIdx) == mov) {
          target_call_inst = call;
          break;
        }
      }
    }
  }

  // Bail out early if either instruction lacks an instruction number.
  if (!ra->HasInstructionNumber(src_inst) || !ra->HasInstructionNumber(mov)) {
    return false;
  }

  // Build combined_interval (src + mov) so that conflict checks cover the
  // full post-coalescing live range. This is consistent with the pattern used
  // in RemoveMovWithSameSrcAndDstImpl.
  auto combined_interval = ra->GetInstructionInterval(src_inst);
  combined_interval.Add(ra->GetInstructionInterval(mov));

  if (HasCallConflict(ra, combined_interval, dst_reg_idx, target_call_inst)) {
    return false;
  }

  // Check for interval overlap with other values at dst_reg.
  if (dst_reg_idx >= max_regs || regHasNonInst[dst_reg_idx]) {
    return false;
  }
  for (Instruction* other_inst : regToInsts[dst_reg_idx]) {
    if (other_inst == mov) continue;
    if (combined_interval.Intersects(ra->GetInstructionInterval(other_inst))) {
      return false;
    }
  }

  // All checks passed — apply coalescing.
  // Commit the combined interval as src's new live range.
  ra->GetInstructionInterval(src_inst) = combined_interval;

  // Reassign src to dst register — MOV becomes self-move.
  ra->UpdateRegister(src_inst, ra->GetRegister(mov));

  // Mark as fixed so later stages won't move it again.
  src_inst->SetFixReg(true);

  // Update regToInsts mapping.
  if (src_reg_idx < max_regs) {
    auto& src_list = regToInsts[src_reg_idx];
    for (size_t j = 0; j < src_list.size(); ++j) {
      if (src_list[j] == src_inst) {
        src_list[j] = src_list.back();
        src_list.pop_back();
        break;
      }
    }
  }
  if (dst_reg_idx < max_regs) {
    regToInsts[dst_reg_idx].push_back(src_inst);
  }
  return true;
}

bool MovEliminationPass::EliminateCallFuncMovImpl(
    FuncOp* func, RegisterAllocator* ra,
    const SideTableAnchors& side_table_anchors, RegToInstsMap& regToInsts,
    RegHasNonInstMap& regHasNonInst) {
  bool changed = false;
  unsigned max_regs = ra->GetMaxRegisterUsage();

  for (auto& block : *func) {
    for (auto& op : block) {
      if (!llvh::isa<MovInst>(&op)) continue;
      auto* mov = llvh::dyn_cast<MovInst>(&op);
      // we only opt callFuncMov
      if (!mov->IsCallFuncMov()) continue;

      if (side_table_anchors.count(mov)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: MovEliminationPass::EliminateCallFuncMov "
            "call-func MOV must not be a side-table anchor");
      }
      if (mov->GetClosureVarReg() != constants::kInvalidSignedValue ||
          mov->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: MovEliminationPass::EliminateCallFuncMov "
            "call-func MOV must not carry special attributes");
      }

      Value* src = mov->GetSingleOperand();
      auto* src_inst = llvh::dyn_cast<Instruction>(src);
      if (!src_inst) continue;

      // If src is a side-table anchor (toplevel vars / closure vars), do not
      // rewrite its physical register here. Otherwise, later passes relying
      // on those side tables may see stale mappings.
      if (side_table_anchors.count(src_inst)) continue;

      // Skip PhiInst because reassigning its register requires updating all
      // predecessors' MOVs, which is not handled here.
      if (llvh::isa<PhiInst>(src_inst)) continue;

      if (src_inst->IsFixReg()) continue;

      if (src_inst->GetClosureVarReg() != constants::kInvalidSignedValue ||
          src_inst->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        continue;
      }

      if (TryCoalesceMovSource(mov, src_inst, ra, max_regs, regToInsts,
                               regHasNonInst)) {
        changed = true;
      }
    }
  }
  return changed;
}

bool MovEliminationPass::EliminateGeneralMovImpl(
    FuncOp* func, RegisterAllocator* ra,
    const SideTableAnchors& side_table_anchors, RegToInstsMap& regToInsts,
    RegHasNonInstMap& regHasNonInst) {
  bool changed = false;
  unsigned max_regs = ra->GetMaxRegisterUsage();

  for (auto& block : *func) {
    for (auto& op : block) {
      auto* mov = llvh::dyn_cast<MovInst>(&op);
      if (!mov) continue;

      // Skip call-func MOVs (handled by EliminateCallFuncMov).
      if (mov->IsCallFuncMov()) continue;

      // Skip MOVs with special attributes (closure/toplevel mappings).
      if (mov->GetClosureVarReg() != constants::kInvalidSignedValue ||
          mov->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        continue;
      }

      if (side_table_anchors.count(mov)) continue;

      Value* src = mov->GetSingleOperand();
      auto* src_inst = llvh::dyn_cast<Instruction>(src);
      if (!src_inst) continue;

      // Skip if source is a side-table anchor.
      if (side_table_anchors.count(src_inst)) continue;

      // Skip PhiInst: reassigning its register affects all predecessor MOVs.
      if (llvh::isa<PhiInst>(src_inst)) continue;

      if (src_inst->IsFixReg()) continue;

      // Skip if source carries special attributes.
      if (src_inst->GetClosureVarReg() != constants::kInvalidSignedValue ||
          src_inst->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        continue;
      }

      if (TryCoalesceMovSource(mov, src_inst, ra, max_regs, regToInsts,
                               regHasNonInst)) {
        changed = true;
      }
    }
  }

  return changed;
}

// Coalesce Phi nodes with their single-user MOVs.
//
// Pattern:
//   pred1: %entry_mov1 = MovInst(%val1)  [reg: phi_reg]
//   pred2: %entry_mov2 = MovInst(%val2)  [reg: phi_reg]
//   merge: %phi = PhiInst(...)           [reg: phi_reg]
//          %user_mov = MovInst(%phi)     [reg: dst_reg]
//
// If dst_reg is free across the Phi's entire live interval (which includes
// entry MOV positions), we reassign the Phi and all entry MOVs to dst_reg.
// The user_mov then becomes a self-move (src_reg == dst_reg) and is removed
// by RemoveMovWithSameSrcAndDst in the next iteration.
bool MovEliminationPass::EliminatePhiUserMovImpl(
    FuncOp* func, RegisterAllocator* ra,
    const SideTableAnchors& side_table_anchors, RegToInstsMap& regToInsts,
    RegHasNonInstMap& regHasNonInst) {
  bool changed = false;
  unsigned max_regs = ra->GetMaxRegisterUsage();

  for (auto& block : *func) {
    for (auto& op : block) {
      auto* mov = llvh::dyn_cast<MovInst>(&op);
      if (!mov) continue;
      if (mov->IsCallFuncMov()) continue;
      if (mov->GetClosureVarReg() != constants::kInvalidSignedValue ||
          mov->GetToplevelVarReg() != constants::kInvalidSignedValue)
        continue;
      if (side_table_anchors.count(mov)) continue;

      auto* phi = llvh::dyn_cast<PhiInst>(mov->GetSingleOperand());
      if (!phi) continue;

      // Only coalesce if the Phi has exactly one user (this MOV).
      // Otherwise the narrowed interval [phi_num, mov_num+1] doesn't cover
      // the full live range and we risk register conflicts.
      if (phi->GetNumUsers() != 1) continue;

      if (!ra->IsAllocated(phi) || !ra->IsAllocated(mov)) continue;

      unsigned phi_reg_idx = ra->GetRegister(phi).GetIndex();
      unsigned dst_reg_idx = ra->GetRegister(mov).GetIndex();
      if (phi_reg_idx == dst_reg_idx) continue;

      // Don't target invalid or non-instruction registers.
      if (dst_reg_idx >= max_regs || regHasNonInst[dst_reg_idx]) continue;

      // Narrowed interval check: we only need dst_reg to be free between the
      // Phi definition and the user MOV. Beyond the MOV, dst_reg already holds
      // the MOV's result (which is at dst_reg by construction).
      //
      // BACK-EDGE CORRECTNESS ARGUMENT:
      // For loop-header Phis, a back-edge predecessor has an entry_mov with
      // instruction number > mov_num (later in the loop body). This narrow
      // interval [phi_num, mov_num+1] does NOT cover back-edge positions.
      // Safety is ensured by the per-entry_mov checks below:
      //   (c) Interval overlap check at each entry_mov's position catches any
      //       value at dst_reg that is live across the entry_mov (including
      //       values live on back-edge paths).
      //   (d) Post-entry_mov scan in the predecessor block catches any later
      //       instruction that reads/writes dst_reg (covers the case where
      //       another MOV or use at dst_reg follows the entry_mov before the
      //       back-edge branch).
      // Together, (c) and (d) guarantee that reassigning entry_movs on
      // back-edge predecessors to dst_reg does not introduce conflicts.
      if (!ra->HasInstructionNumber(phi) || !ra->HasInstructionNumber(mov))
        continue;
      unsigned phi_num = ra->GetInstructionNumber(phi);
      unsigned mov_num = ra->GetInstructionNumber(mov);
      if (phi_num >= mov_num) continue;
      Interval narrow_interval(phi_num, mov_num + 1);

      // Check call conflict for dst_reg across the narrow interval.
      if (HasCallConflict(ra, narrow_interval, dst_reg_idx, nullptr)) continue;

      // Check interval overlap: no other value at dst_reg conflicts with the
      // narrowed interval. Exclude the user mov itself (will become self-move).
      bool conflict = false;
      for (Instruction* other_inst : regToInsts[dst_reg_idx]) {
        if (other_inst == mov) continue;
        if (narrow_interval.Intersects(
                ra->GetInstructionInterval(other_inst))) {
          conflict = true;
          break;
        }
      }
      if (conflict) continue;

      // Verify all Phi entries are MOVs at phi_reg that we can safely
      // reassign to dst_reg. An entry_mov is safe to reassign iff:
      //   (a) It lives at phi_reg (same register web as the Phi).
      //   (b) It is not pinned (IsFixReg, closure/toplevel var, side-table).
      //   (c) No other value at dst_reg is live at the entry_mov's position
      //       (interval overlap — covers critical-edge cases).
      //   (d) No instruction after entry_mov in the same predecessor block
      //       reads/writes dst_reg (reassigning would clobber those uses).
      llvh::SmallVector<MovInst*, 4> entry_movs;
      bool can_coalesce = true;
      for (unsigned i = 0, e = phi->GetNumEntries(); i < e; ++i) {
        auto entry = phi->GetEntry(i);
        auto* entry_mov = llvh::dyn_cast<MovInst>(entry.first);
        if (!entry_mov || !ra->IsAllocated(entry_mov)) {
          can_coalesce = false;
          break;
        }
        // (a) entry_mov must be part of the same phi_reg web.
        if (ra->GetRegister(entry_mov).GetIndex() != phi_reg_idx) {
          can_coalesce = false;
          break;
        }
        // (b) entry_mov must not be pinned by prior coalescing or special use.
        if (entry_mov->IsFixReg()) {
          can_coalesce = false;
          break;
        }
        if (entry_mov->GetClosureVarReg() != constants::kInvalidSignedValue ||
            entry_mov->GetToplevelVarReg() != constants::kInvalidSignedValue) {
          can_coalesce = false;
          break;
        }
        if (side_table_anchors.count(entry_mov)) {
          can_coalesce = false;
          break;
        }
        // (c) Interval overlap: no other value at dst_reg is live at
        // entry_mov's position. Catches critical-edge cases where a value at
        // dst_reg is live across entry_mov to another successor.
        if (ra->HasInstructionNumber(entry_mov)) {
          auto& em_interval = ra->GetInstructionInterval(entry_mov);
          for (Instruction* other : regToInsts[dst_reg_idx]) {
            if (other == mov || other == phi || other == entry_mov) continue;
            if (em_interval.Intersects(ra->GetInstructionInterval(other))) {
              can_coalesce = false;
              break;
            }
          }
          if (!can_coalesce) break;
        }
        // (d) Post-entry_mov scan: no later instruction in the predecessor
        // block reads/writes dst_reg. Reassigning entry_mov to dst_reg would
        // clobber those uses (e.g. other Phi entry MOVs targeting dst_reg).
        Block* pred_block = entry.second;
        // Guard: after RemoveMovWithSameSrcAndDst RAUW, entry_mov may have
        // been replaced with an instruction from a different block.
        if (entry_mov->GetParent() != pred_block) {
          can_coalesce = false;
          break;
        }
        bool found_entry = false;
        for (auto it = pred_block->InstBegin(), ie = pred_block->InstEnd();
             it != ie; ++it) {
          if (*it == entry_mov) {
            found_entry = true;
            continue;
          }
          if (!found_entry) continue;
          Instruction* later_inst = *it;
          // Check if later_inst's result is allocated at dst_reg.
          if (ra->IsAllocated(later_inst) &&
              ra->GetRegister(later_inst).GetIndex() == dst_reg_idx) {
            can_coalesce = false;
            break;
          }
          // Check if any operand of later_inst uses dst_reg.
          for (unsigned oi = 0, oe = later_inst->GetNumOperands(); oi < oe;
               ++oi) {
            Value* operand = later_inst->GetOperand(oi);
            if (operand && ra->IsAllocated(operand) &&
                ra->GetRegister(operand).GetIndex() == dst_reg_idx) {
              can_coalesce = false;
              break;
            }
          }
          if (!can_coalesce) break;
          // Check CallInst implicit clobber: argument materialization writes
          // registers [func_reg+1, func_reg+argc]. If dst_reg falls in this
          // range (and isn't a self-copy slot), coalescing is unsafe.
          if (auto* call_inst = llvh::dyn_cast<CallInst>(later_inst)) {
            Value* func_val = call_inst->GetFunction();
            if (func_val && ra->IsAllocated(func_val)) {
              unsigned func_reg = ra->GetRegister(func_val).GetIndex();
              unsigned argc =
                  static_cast<unsigned>(call_inst->GetNumArguments());
              if (argc > 0) {
                unsigned clobber_lo = func_reg + 1;
                unsigned clobber_hi = func_reg + argc;
                if (dst_reg_idx >= clobber_lo && dst_reg_idx <= clobber_hi) {
                  // Check if the argument at this slot is already in dst_reg
                  // (self-copy is safe — materialization is a no-op).
                  unsigned slot_idx = dst_reg_idx - clobber_lo;
                  Value* arg = call_inst->GetArgument(slot_idx);
                  bool is_self_copy =
                      arg && ra->IsAllocated(arg) &&
                      ra->GetRegister(arg).GetIndex() == dst_reg_idx;
                  if (!is_self_copy) {
                    can_coalesce = false;
                    break;
                  }
                }
              }
            }
          }
        }
        if (!can_coalesce) break;
        entry_movs.push_back(entry_mov);
      }
      if (!can_coalesce) continue;

      // All checks passed — reassign phi and all entry MOVs to dst_reg.
      // Extend phi's interval to cover the user MOV's lifetime so later
      // passes see the correct live range (consistent with EliminateGeneralMov
      // and EliminateCallFuncMov which also extend intervals on coalescing).
      if (ra->HasInstructionNumber(phi) && ra->HasInstructionNumber(mov)) {
        ra->GetInstructionInterval(phi).Add(ra->GetInstructionInterval(mov));
      }

      ra->UpdateRegister(phi, Register(dst_reg_idx));
      phi->SetFixReg(true);
      {
        auto& phi_list = regToInsts[phi_reg_idx];
        for (size_t j = 0; j < phi_list.size(); ++j) {
          if (phi_list[j] == phi) {
            phi_list[j] = phi_list.back();
            phi_list.pop_back();
            break;
          }
        }
        regToInsts[dst_reg_idx].push_back(phi);
      }

      for (auto* entry_mov : entry_movs) {
        ra->UpdateRegister(entry_mov, Register(dst_reg_idx));
        entry_mov->SetFixReg(true);
        auto& em_list = regToInsts[phi_reg_idx];
        for (size_t j = 0; j < em_list.size(); ++j) {
          if (em_list[j] == entry_mov) {
            em_list[j] = em_list.back();
            em_list.pop_back();
            break;
          }
        }
        regToInsts[dst_reg_idx].push_back(entry_mov);
      }

      changed = true;
    }
  }

  return changed;
}

void MovEliminationPass::SetCallFuncMovFix(FuncOp* func) {
  for (auto& block : *func) {
    for (auto& op : block) {
      if (!llvh::isa<MovInst>(&op)) continue;
      auto* mov = llvh::dyn_cast<MovInst>(&op);
      if (!mov->IsCallFuncMov()) continue;
      mov->SetFixReg(true);
    }
  }
}

bool MovEliminationPass::EliminateDeadInstructionsImpl(
    FuncOp* func, RegisterAllocator* ra,
    const SideTableAnchors& side_table_anchors) {
  bool any_changed = false;

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto& block : *func) {
      for (auto it = block.InstRbegin(), e = block.InstRend(); it != e;) {
        Instruction* inst = *it;
        ++it;

        if (inst->GetNumUsers() != 0) continue;

        // Never remove terminators.
        if (llvh::isa<TerminatorInst>(inst)) continue;

        // Never remove instructions with side effects.
        if (inst->GetSideEffect().HasSideEffect()) continue;

        // Never remove side-table anchors.
        if (side_table_anchors.count(inst)) continue;

        // Never remove instructions with special attributes.
        if (inst->GetClosureVarReg() != constants::kInvalidSignedValue ||
            inst->GetToplevelVarReg() != constants::kInvalidSignedValue)
          continue;

        ra->RemoveFromAllocated(inst);
        inst->EraseFromParent();
        changed = true;
        any_changed = true;
      }
    }
  }

  return any_changed;
}

bool MovEliminationPass::RunOnFunction(FuncOp* func) {
  RegisterAllocator* ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (!ra) return false;

  SideTableAnchors side_table_anchors;
  BuildSideTableAnchors(ir_ctx_, func, side_table_anchors);

  bool ever_changed = false;
  bool changed = false;
  do {
    // Rebuild regToInsts once per iteration from the allocator's current state.
    unsigned max_regs = ra->GetMaxRegisterUsage();
    RegToInstsMap regToInsts;
    RegHasNonInstMap regHasNonInst;
    BuildRegToInsts(ra, max_regs, regToInsts, regHasNonInst);

    changed = EliminateCallFuncMovImpl(func, ra, side_table_anchors, regToInsts,
                                       regHasNonInst);
    changed |= EliminateGeneralMovImpl(func, ra, side_table_anchors, regToInsts,
                                       regHasNonInst);
    changed |= EliminatePhiUserMovImpl(func, ra, side_table_anchors, regToInsts,
                                       regHasNonInst);
    changed |= RemoveMovWithSameSrcAndDstImpl(func, side_table_anchors);
    changed |= RemoveBackToBackReverseMovImpl(func, ra, side_table_anchors);
    ever_changed |= changed;
  } while (changed);

  // Remove instructions that became dead after MOV elimination.
  ever_changed |= EliminateDeadInstructionsImpl(func, ra, side_table_anchors);

  SetCallFuncMovFix(func);

  // Keep RegisterAllocator::GetMaxRegisterUsage() consistent for later passes.
  ra->RebuildRegisterFileFromAllocated();

  return ever_changed;
}

Pass* CreateMovEliminationPass(IRContext* ir_ctx) {
  return new MovEliminationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
