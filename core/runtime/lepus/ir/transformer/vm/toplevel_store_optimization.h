// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_TOPLEVEL_STORE_OPTIMIZATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_TOPLEVEL_STORE_OPTIMIZATION_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ArrayRef.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class FuncOp;
class Instruction;
class ModuleOp;
class RegisterAllocator;
class Value;

// This is a post-register-allocation optimization pass.
//
// In the VM backend, `SetToplevelVarInst` is lowered in instruction selection
// as a plain `TypeOp_Move(toplevel_reg, src)`.
//
// This pass optimizes SetToplevel* writes in the root (toplevel) function by:
// - removing redundant stores when `src` is already in the target physical
//   register, and
// - coalescing the producer instruction into the target physical register and
//   then erasing the Set.
//
// When it rewrites/erases a Set that is referenced by side tables
// (IRContext::toplevel_variables_ / FuncOp closure-var maps), it updates those
// anchors accordingly. If a closure anchor's physical register changes, it also
// refreshes the runtime upvalue-index -> toplevel-reg mapping for descendants.
//
// This pass runs after UpdateToplevelClosureVarPass.
class ToplevelStoreOptimizationPass : public ModulePass {
 public:
  explicit ToplevelStoreOptimizationPass(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "set-toplevel-elimination") {}

  bool RunOnModule(ModuleOp* mod) override;

 private:
  bool InitForModule(ModuleOp* mod);
  void BuildPhysicalRegToValuesMap();
  void BuildClosureAnchorSet();
  void RefreshClosureUpvalueMapIfNeeded();

  bool IsRedundantStore(Instruction* src_inst, unsigned target_reg) const;
  void ReplaceSideTableAnchors(Instruction* old_anchor,
                               Instruction* new_anchor);
  Value* FindLiveValueInRegBefore(unsigned target_reg,
                                  Instruction* set_inst) const;

  llvh::SmallVector<Value*, 2> CollectIgnoredValuesForToplevelSlot(
      unsigned target_reg, Instruction* set_inst) const;
  llvh::SmallVector<Value*, 2> CollectIgnoredValuesForClosureSlot(
      unsigned target_reg, Instruction* set_inst, Value* closure_value) const;

  bool TryCoalesceProducerIntoTargetReg(Instruction* src_inst,
                                        Instruction* set_inst,
                                        unsigned target_reg,
                                        llvh::ArrayRef<Value*> ignore_values);
  bool HasTargetRegLiveRangeConflict(
      Instruction* src_inst, Instruction* set_inst, unsigned target_reg,
      llvh::ArrayRef<Value*> ignore_values) const;
  bool IsIgnoredValue(llvh::ArrayRef<Value*> ignore_values, Value* v) const;
  void UpdatePhysicalRegMapAfterReassign(Instruction* src_inst,
                                         unsigned old_reg, unsigned new_reg);
  void EraseInstructionAndUpdateRA(Instruction* inst);

 private:
  ModuleOp* mod_{nullptr};
  FuncOp* root_{nullptr};
  RegisterAllocator* ra_{nullptr};

  // A helper index for conflict checks:
  //   physical_reg -> all Values currently assigned to it (Instruction +
  //   non-Instruction).
  llvh::DenseMap<unsigned, llvh::SmallVector<Value*, 4>>
      physical_reg_to_values_;
  llvh::SmallDenseSet<Value*, 32> closure_anchor_values_;
  bool closure_upvalue_map_dirty_{false};
};

Pass* CreateSetToplevelEliminationPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_TOPLEVEL_STORE_OPTIMIZATION_H_
