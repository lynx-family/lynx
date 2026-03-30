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

static void BuildSideTableAnchors(IRContext* ir_ctx, FuncOp* func,
                                  llvh::SmallDenseSet<Value*, 32>& anchors) {
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
bool MovEliminationPass::RemoveMovWithSameSrcAndDst(FuncOp* func) {
  to_removed_.clear();

  RegisterAllocator* ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (!ra) return false;

  llvh::SmallDenseSet<Value*, 32> side_table_anchors;
  BuildSideTableAnchors(ir_ctx_, func, side_table_anchors);

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
bool MovEliminationPass::RemoveBackToBackReverseMov(FuncOp* func,
                                                    RegisterAllocator* ra) {
  if (!func || !ra) return false;

  to_removed_.clear();

  llvh::SmallDenseSet<Value*, 32> side_table_anchors;
  BuildSideTableAnchors(ir_ctx_, func, side_table_anchors);

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

bool MovEliminationPass::EliminateCallFuncMov(FuncOp* func,
                                              RegisterAllocator* ra) {
  bool changed = false;
  unsigned max_regs = ra->GetMaxRegisterUsage();

  llvh::SmallDenseSet<Value*, 32> side_table_anchors;
  BuildSideTableAnchors(ir_ctx_, func, side_table_anchors);

  // Keep Instruction* only to avoid repeated dyn_cast in conflict checks.
  llvh::SmallVector<llvh::SmallVector<Instruction*, 4>, 0> regToInsts;
  regToInsts.resize(max_regs);
  llvh::SmallVector<bool, 0> regHasNonInst(max_regs, false);
  for (auto& pair : ra->GetAllocatedMap()) {
    if (pair.second.IsValid() && pair.second.GetIndex() < max_regs) {
      if (auto* i = llvh::dyn_cast<Instruction>(pair.first)) {
        regToInsts[pair.second.GetIndex()].push_back(i);
      } else {
        regHasNonInst[pair.second.GetIndex()] = true;
      }
    }
  }

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

      if (!ra->IsAllocated(src_inst) || !ra->IsAllocated(mov)) continue;

      unsigned dst_reg_idx = ra->GetRegister(mov).GetIndex();
      unsigned src_reg_idx = ra->GetRegister(src_inst).GetIndex();

      if (dst_reg_idx == src_reg_idx) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: MovEliminationPass::EliminateCallFuncMov "
            "call-func MOV has dst_reg == src_reg");
      }

      // Conservative safety: avoid assigning to the same register as any
      // operand of the producer instruction. Some VM instructions may not be
      // safe with dst==src in the lowering.
      bool operand_in_dst_reg = false;
      for (int op_i = 0, op_e = static_cast<int>(src_inst->GetNumOperands());
           op_i < op_e; ++op_i) {
        Value* opv = src_inst->GetOperand(op_i);
        if (!opv || !ra->IsAllocated(opv)) continue;
        if (ra->GetRegister(opv).GetIndex() == dst_reg_idx) {
          operand_in_dst_reg = true;
          break;
        }
      }
      if (operand_in_dst_reg) continue;

      // Conservative Check:
      // If we eliminate this MOV, src_inst will be assigned dst_reg_idx.
      // We must ensure this doesn't violate the call convention for OTHER
      // calls that fall within src_inst's live range.
      Instruction* target_call_inst = nullptr;
      for (auto* user : mov->GetUsers()) {
        if (auto* call = llvh::dyn_cast<CallInst>(user)) {
          if (call->GetOperand(CallInst::methodIdx) == mov) {
            target_call_inst = call;
            break;
          }
        }
      }

      auto& src_interval = ra->GetInstructionInterval(src_inst);
      Interval combined_interval;
      combined_interval.Add(src_interval);
      // Add mov interval to check for conflicts in the extended live range
      if (ra->HasInstructionNumber(mov)) {
        combined_interval.Add(ra->GetInstructionInterval(mov));
      }

      if (HasCallConflict(ra, combined_interval, dst_reg_idx,
                          target_call_inst)) {
        continue;
      }

      // Check for conflicts
      bool conflict = false;
      if (dst_reg_idx >= max_regs || regHasNonInst[dst_reg_idx]) {
        conflict = true;
      } else {
        for (Instruction* other_inst : regToInsts[dst_reg_idx]) {
          if (other_inst == mov) continue;
          if (combined_interval.Intersects(
                  ra->GetInstructionInterval(other_inst))) {
            conflict = true;
            break;
          }
        }
      }

      if (!conflict) {
        // If we remove the call-func MOV by directly using `src_inst` as the
        // call target, we must conservatively extend `src_inst`'s live
        // interval to cover the MOV's interval (which typically spans the
        // call-site). Otherwise, later post-RA passes that rely on
        // RegisterAllocator intervals for conflict checks may make
        // unsound decisions, and the VM's CallRandom* argument materialization
        // can clobber live registers.
        if (ra->HasInstructionNumber(src_inst) &&
            ra->HasInstructionNumber(mov)) {
          ra->GetInstructionInterval(src_inst).Add(
              ra->GetInstructionInterval(mov));
        }

        ra->UpdateRegister(src_inst, ra->GetRegister(mov));
        // now the src and mov_inst have the same reg, the mov_inst will be
        // deleted in function `RemoveMovWithSameSrcAndDst`

        // This instruction's physical register is reassigned by this pass.
        // Mark it as fixed so later stages won't move it again.
        src_inst->SetFixReg(true);
        // Update regToValues mapping
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
        changed = true;
      }
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

bool MovEliminationPass::RunOnFunction(FuncOp* func) {
  RegisterAllocator* ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (!ra) return false;

  bool changed = false;
  do {
    changed = EliminateCallFuncMov(func, ra);
    changed |= RemoveMovWithSameSrcAndDst(func);
    changed |= RemoveBackToBackReverseMov(func, ra);
  } while (changed);

  SetCallFuncMovFix(func);

  // Keep RegisterAllocator::GetMaxRegisterUsage() consistent for later passes.
  ra->RebuildRegisterFileFromAllocated();

  return changed;
}

Pass* CreateMovEliminationPass(IRContext* ir_ctx) {
  return new MovEliminationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
