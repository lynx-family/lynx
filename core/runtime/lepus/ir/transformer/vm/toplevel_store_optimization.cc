// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/toplevel_store_optimization.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

bool ToplevelStoreOptimizationPass::InitForModule(ModuleOp* mod) {
  mod_ = mod;
  root_ = mod ? mod->GetRootFunction() : nullptr;
  if (!root_) return false;

  ra_ = ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(root_);
  if (!ra_) return false;

  physical_reg_to_values_.clear();
  closure_anchor_values_.clear();
  closure_upvalue_map_dirty_ = false;

  BuildPhysicalRegToValuesMap();
  BuildClosureAnchorSet();
  return true;
}

void ToplevelStoreOptimizationPass::BuildPhysicalRegToValuesMap() {
  physical_reg_to_values_.reserve(ra_->GetAllocatedMap().size());
  for (auto& pair : ra_->GetAllocatedMap()) {
    if (!pair.second.IsValid()) continue;
    physical_reg_to_values_[pair.second.GetIndex()].push_back(pair.first);
  }
}

void ToplevelStoreOptimizationPass::BuildClosureAnchorSet() {
  auto& m = root_->GetClosureVarReg2ValueMap();
  closure_anchor_values_.reserve(m.size());
  for (auto& kv : m) {
    if (kv.second) closure_anchor_values_.insert(kv.second);
  }
}

bool ToplevelStoreOptimizationPass::IsRedundantStore(
    Instruction* src_inst, unsigned target_reg) const {
  if (!src_inst) return false;
  if (llvh::isa<PhiInst>(src_inst)) return false;
  if (!ra_->IsAllocated(src_inst)) return false;
  return ra_->GetRegister(src_inst).GetIndex() == target_reg;
}

void ToplevelStoreOptimizationPass::ReplaceSideTableAnchors(
    Instruction* old_anchor, Instruction* new_anchor) {
  if (!old_anchor || !new_anchor) return;

  // 1) Update closure var-reg -> value map if it points to the old anchor.
  bool replaced_closure_anchor = false;
  {
    auto& closure_map = root_->GetClosureVarReg2ValueMap();
    for (auto& kv : closure_map) {
      if (kv.second == old_anchor) {
        kv.second = new_anchor;
        replaced_closure_anchor = true;
      }
    }
  }

  if (replaced_closure_anchor) {
    closure_anchor_values_.erase(old_anchor);
    closure_anchor_values_.insert(new_anchor);
    // UpdateToplevelClosureVarPass already ran before this pass; any
    // physical-reg changes to closure anchors require refreshing the runtime
    // mapping (FuncOp::upvalue_index_to_toplevel_reg_).
    closure_upvalue_map_dirty_ = true;
  }

  // 2) Update IRContext::toplevel_variables_ if it points to the old anchor.
  {
    auto& toplevel_vars = ir_ctx_->GetToplevelVariables();
    for (auto& kv : toplevel_vars) {
      if (kv.second == old_anchor) {
        kv.second = new_anchor;
      }
    }
  }
}

Value* ToplevelStoreOptimizationPass::FindLiveValueInRegBefore(
    unsigned target_reg, Instruction* set_inst) const {
  if (!set_inst) return nullptr;
  if (!ra_->HasInstructionNumber(set_inst)) return nullptr;
  auto it = physical_reg_to_values_.find(target_reg);
  if (it == physical_reg_to_values_.end()) return nullptr;

  size_t probe = ra_->GetInstructionNumber(set_inst);
  if (probe > 0) probe -= 1;
  Segment probe_seg(probe, probe + 1);

  for (Value* v : it->second) {
    if (v == set_inst) continue;
    auto* inst = llvh::dyn_cast<Instruction>(v);
    if (!inst || !ra_->HasInstructionNumber(inst)) continue;
    if (ra_->GetInstructionInterval(inst).Intersects(probe_seg)) {
      return v;
    }
  }
  return nullptr;
}

llvh::SmallVector<Value*, 2>
ToplevelStoreOptimizationPass::CollectIgnoredValuesForToplevelSlot(
    unsigned target_reg, Instruction* set_toplevel_var_inst) const {
  llvh::SmallVector<Value*, 2> ignore_vals;
  ignore_vals.reserve(2);

  // `ignore_vals` is used only by the target-reg conflict checker.
  // It represents values currently associated with `target_reg` that are
  // expected to be overwritten by this SetToplevelVarInst.
  //
  // Rationale:
  // - The SetToplevelVarInst overwrites the toplevel slot at runtime.
  // - When we coalesce the producer into `target_reg`, we are essentially
  //   moving that overwrite earlier (to the producer definition point).
  // - Therefore, the value being overwritten by the Set is not a "real"
  //   blocking conflict for the rewrite; other values in the same physical
  //   register are still treated as conflicts.

  // Prefer ignoring the value that IRContext thinks is in that toplevel slot.
  // Note: it can be stale; we will add the actual live value as well.
  Value* ctx_anchor = nullptr;
  for (const auto& item : ir_ctx_->GetToplevelVariables()) {
    Instruction* def = item.second;
    if (def && ra_->IsAllocated(def) &&
        ra_->GetRegister(def).GetIndex() == target_reg) {
      ctx_anchor = def;
      break;
    }
  }
  if (ctx_anchor) ignore_vals.push_back(ctx_anchor);

  if (Value* overwritten =
          FindLiveValueInRegBefore(target_reg, set_toplevel_var_inst)) {
    if (overwritten != ctx_anchor) ignore_vals.push_back(overwritten);
  }
  return ignore_vals;
}

llvh::SmallVector<Value*, 2>
ToplevelStoreOptimizationPass::CollectIgnoredValuesForClosureSlot(
    unsigned target_reg, Instruction* set_inst, Value* closure_value) const {
  llvh::SmallVector<Value*, 2> ignore_vals;
  ignore_vals.reserve(2);

  // Similar to the toplevel case: `ignore_vals` is a whitelist for the
  // conflict checker, describing values in `target_reg` that will be
  // overwritten by this SetToplevelClosureVarInst.
  if (closure_value) ignore_vals.push_back(closure_value);
  if (Value* overwritten = FindLiveValueInRegBefore(target_reg, set_inst)) {
    if (overwritten != closure_value) ignore_vals.push_back(overwritten);
  }
  return ignore_vals;
}

bool ToplevelStoreOptimizationPass::IsIgnoredValue(
    llvh::ArrayRef<Value*> ignore_values, Value* v) const {
  return !ignore_values.empty() &&
         std::find(ignore_values.begin(), ignore_values.end(), v) !=
             ignore_values.end();
}

bool ToplevelStoreOptimizationPass::HasTargetRegLiveRangeConflict(
    Instruction* src_inst, Instruction* set_inst, unsigned target_reg,
    llvh::ArrayRef<Value*> ignore_values) const {
  auto it = physical_reg_to_values_.find(target_reg);
  if (it == physical_reg_to_values_.end()) return false;

  auto& src_interval = ra_->GetInstructionInterval(src_inst);
  for (Value* other : it->second) {
    if (other == src_inst || other == set_inst) continue;

    // `ignore_values` are the values that the Set would overwrite anyway, so we
    // don't treat them as blockers when deciding whether we can place
    // `src_inst` into `target_reg`.
    if (IsIgnoredValue(ignore_values, other)) continue;

    if (auto* other_inst = llvh::dyn_cast<Instruction>(other)) {
      if (src_interval.Intersects(ra_->GetInstructionInterval(other_inst))) {
        return true;
      }
    } else {
      // Conservative: treat non-instruction (params/preallocated) as
      // conflicting.
      return true;
    }
  }
  return false;
}

void ToplevelStoreOptimizationPass::UpdatePhysicalRegMapAfterReassign(
    Instruction* src_inst, unsigned old_reg, unsigned new_reg) {
  auto old_it = physical_reg_to_values_.find(old_reg);
  if (old_it != physical_reg_to_values_.end()) {
    auto& vec = old_it->second;
    for (size_t i = 0; i < vec.size(); ++i) {
      if (vec[i] == src_inst) {
        vec[i] = vec.back();
        vec.pop_back();
        break;
      }
    }
  }
  physical_reg_to_values_[new_reg].push_back(src_inst);
}

bool ToplevelStoreOptimizationPass::TryCoalesceProducerIntoTargetReg(
    Instruction* src_inst, Instruction* set_inst, unsigned target_reg,
    llvh::ArrayRef<Value*> ignore_values) {
  if (!src_inst || !set_inst) return false;
  if (!ra_->IsAllocated(src_inst)) return false;
  if (src_inst->IsFixReg()) return false;
  if (src_inst->GetToplevelVarReg() != constants::kInvalidSignedValue)
    return false;
  if (llvh::isa<PhiInst>(src_inst)) return false;

  // Conservative safety: avoid assigning to the same register as any operand
  // of the producer instruction. Some VM instructions may not be safe with
  // dst==src in the lowering.
  for (int i = 0, e = static_cast<int>(src_inst->GetNumOperands()); i < e;
       ++i) {
    Value* op = src_inst->GetOperand(i);
    if (!op || !ra_->IsAllocated(op)) continue;
    if (ra_->GetRegister(op).GetIndex() == target_reg) return false;
  }

  unsigned old_reg = ra_->GetRegister(src_inst).GetIndex();
  if (old_reg == target_reg) {
    // Even if already in place, ensure the producer is marked fixed so later
    // stages won't move it.
    src_inst->SetFixReg(true);
    return true;
  }

  if (HasTargetRegLiveRangeConflict(src_inst, set_inst, target_reg,
                                    ignore_values))
    return false;

  // Update register assignment.
  ra_->UpdateRegister(src_inst, Register(target_reg));
  src_inst->SetFixReg(true);

  if (closure_anchor_values_.count(src_inst)) {
    // The closure anchor's physical register changed; refresh mappings.
    closure_upvalue_map_dirty_ = true;
  }

  // Extend src live interval to cover eliminated store.
  if (ra_->HasInstructionNumber(set_inst) &&
      ra_->HasInstructionNumber(src_inst)) {
    ra_->GetInstructionInterval(src_inst).Add(
        ra_->GetInstructionInterval(set_inst));
  }

  // Update reg->values mapping for subsequent eliminations.
  UpdatePhysicalRegMapAfterReassign(src_inst, old_reg, target_reg);
  return true;
}

void ToplevelStoreOptimizationPass::EraseInstructionAndUpdateRA(
    Instruction* inst) {
  if (!inst) return;
  if (ra_->IsAllocated(inst)) {
    ra_->RemoveFromAllocated(inst);
  }
  inst->EraseFromParent();
}

void ToplevelStoreOptimizationPass::RefreshClosureUpvalueMapIfNeeded() {
  if (!closure_upvalue_map_dirty_) return;
  auto root_lepus_func = root_->GetLepusFunction();
  if (!root_lepus_func) return;
  auto& closure_map = root_->GetClosureVarReg2ValueMap();

  auto refresh_descendants = [&](auto&& self, FuncOp* parent_func_op) -> void {
    if (!parent_func_op) return;
    auto parent = parent_func_op->GetLepusFunction();
    if (!parent) return;
    for (auto child : parent->GetChildFunction()) {
      auto* child_func_op = ir_ctx_->GetFuncOp(child);
      if (!child_func_op) continue;
      for (int i = 0, e = child->UpvaluesSize(); i < e; i++) {
        auto upvalue = child->GetUpvalue(i);
        if (!upvalue || !upvalue->in_parent_vars_) continue;
        auto new_reg =
            parent_func_op->GetClosureVarToplevelReg(upvalue->register_);
        if (new_reg == -1) continue;
        child_func_op->RecordUpvalueIndex2ToplevelReg(i, new_reg);
      }
      self(self, child_func_op);
    }
  };

  for (auto child : root_lepus_func->GetChildFunction()) {
    auto* child_func_op = ir_ctx_->GetFuncOp(child);
    if (!child_func_op) continue;
    for (int i = 0, e = child->UpvaluesSize(); i < e; i++) {
      auto upvalue = child->GetUpvalue(i);
      if (!upvalue || !upvalue->in_parent_vars_) continue;

      auto it = closure_map.find(static_cast<uint32_t>(upvalue->register_));
      if (it == closure_map.end()) continue;
      Value* closure_value = it->second;
      if (!closure_value || !ra_->IsAllocated(closure_value)) continue;
      long new_reg =
          static_cast<long>(ra_->GetRegister(closure_value).GetIndex());
      child_func_op->RecordUpvalueIndex2ToplevelReg(i, new_reg);
    }
    refresh_descendants(refresh_descendants, child_func_op);
  }
}

// Post-RA SetToplevel* cleanup.
//
// After register allocation + UpdateToplevelVarRegPass, SetToplevelVarInst is
// lowered to a single `TypeOp_Move(toplevel_reg, src)` in instruction
// selection.
//
// This pass removes redundant SetToplevel* writes in the root (toplevel)
// function via two mechanisms:
//
// 1) Redundant store removal:
//    If `src` is already assigned to the target physical register, the Set does
//    not change any runtime state and can be erased.
//
// 2) Producer coalescing:
//    If `src` is a single-use producer instruction and it is safe to assign it
//    to the target register (no liveness conflicts, not fixed, not Phi, and no
//    dst==operand hazards), we move the producer to the target register and
//    then erase the Set.
//
// Side tables:
// `SetToplevelVarInst` / `SetToplevelClosureVarInst` may be referenced by side
// tables (IRContext::toplevel_variables_ / FuncOp closure-old-reg anchors).
// When a Set is erased, we rewrite those anchors to point at the replacement
// value. If a closure anchor's physical register changes, we refresh the
// runtime upvalue-index -> toplevel-reg mapping for descendant functions.
bool ToplevelStoreOptimizationPass::RunOnModule(ModuleOp* mod) {
  if (!InitForModule(mod)) return false;

  bool changed = false;
  llvh::SmallVector<Instruction*, 8> to_delete;

  // This pass only scans the root (toplevel) function. Non-toplevel functions
  // keep SetToplevelClosureVarInst so the VM can preserve its special bytecode
  // semantics.
  for (auto& bb : *root_) {
    for (auto& op : bb) {
      if (auto* set_toplevel_var_inst =
              llvh::dyn_cast<SetToplevelVarInst>(&op)) {
        auto* src_inst =
            llvh::dyn_cast<Instruction>(set_toplevel_var_inst->GetSrc());
        if (!src_inst) continue;
        if (!llvh::isa<LiteralUint32>(set_toplevel_var_inst->GetToplevelReg()))
          continue;
        unsigned target_reg =
            llvh::cast<LiteralUint32>(set_toplevel_var_inst->GetToplevelReg())
                ->GetValue();

        if (IsRedundantStore(src_inst, target_reg)) {
          ReplaceSideTableAnchors(set_toplevel_var_inst, src_inst);
          to_delete.push_back(set_toplevel_var_inst);
          changed = true;
          continue;
        }

        if (src_inst->GetNumUsers() != 1) continue;

        auto ignore_vals = CollectIgnoredValuesForToplevelSlot(
            target_reg, set_toplevel_var_inst);
        if (TryCoalesceProducerIntoTargetReg(src_inst, set_toplevel_var_inst,
                                             target_reg, ignore_vals)) {
          ReplaceSideTableAnchors(set_toplevel_var_inst, src_inst);
          to_delete.push_back(set_toplevel_var_inst);
          changed = true;
        }
      } else if (auto* set_toplevel_closure_var_inst =
                     llvh::dyn_cast<SetToplevelClosureVarInst>(&op)) {
        auto* src_inst = llvh::dyn_cast<Instruction>(
            set_toplevel_closure_var_inst->GetSrc());
        if (!src_inst) continue;
        if (!llvh::isa<LiteralUint32>(
                set_toplevel_closure_var_inst->GetClosureReg()))
          continue;
        unsigned original_reg =
            llvh::cast<LiteralUint32>(
                set_toplevel_closure_var_inst->GetClosureReg())
                ->GetValue();

        // Resolve the physical register chosen for this closure variable.
        auto& map = root_->GetClosureVarReg2ValueMap();
        auto map_it = map.find(original_reg);
        if (map_it == map.end()) continue;
        Value* closure_value = map_it->second;
        if (!closure_value || !ra_->IsAllocated(closure_value)) continue;
        unsigned target_reg = ra_->GetRegister(closure_value).GetIndex();

        if (IsRedundantStore(src_inst, target_reg)) {
          ReplaceSideTableAnchors(set_toplevel_closure_var_inst, src_inst);
          to_delete.push_back(set_toplevel_closure_var_inst);
          changed = true;
          continue;
        }

        if (src_inst->GetNumUsers() != 1) continue;

        auto ignore_vals = CollectIgnoredValuesForClosureSlot(
            target_reg, set_toplevel_closure_var_inst, closure_value);
        if (TryCoalesceProducerIntoTargetReg(src_inst,
                                             set_toplevel_closure_var_inst,
                                             target_reg, ignore_vals)) {
          ReplaceSideTableAnchors(set_toplevel_closure_var_inst, src_inst);
          to_delete.push_back(set_toplevel_closure_var_inst);
          changed = true;
        }
      }
    }
  }

  for (auto* inst : to_delete) {
    EraseInstructionAndUpdateRA(inst);
  }

  RefreshClosureUpvalueMapIfNeeded();

  return changed;
}

Pass* CreateSetToplevelEliminationPass(IRContext* ir_ctx) {
  return new ToplevelStoreOptimizationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
