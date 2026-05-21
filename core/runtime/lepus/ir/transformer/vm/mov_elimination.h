// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_MOV_ELIMINATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_MOV_ELIMINATION_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

class MovInst;

class MovEliminationPass : public FunctionPass {
 public:
  explicit MovEliminationPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "mov-elimination") {}

  bool RunOnFunction(FuncOp* func) override;

  void SetCallFuncMovFix(FuncOp* func);

 protected:
  using RegToInstsMap =
      llvh::SmallVector<llvh::SmallVector<Instruction*, 4>, 0>;
  using RegHasNonInstMap = llvh::SmallVector<bool, 0>;
  using SideTableAnchors = llvh::SmallDenseSet<Value*, 32>;

  static void BuildSideTableAnchors(IRContext* ir_ctx, FuncOp* func,
                                    SideTableAnchors& anchors);
  static void BuildRegToInsts(RegisterAllocator* ra, unsigned max_regs,
                              RegToInstsMap& regToInsts,
                              RegHasNonInstMap& regHasNonInst);

  bool RemoveMovWithSameSrcAndDstImpl(FuncOp* func,
                                      const SideTableAnchors& anchors);
  bool EliminateGeneralMovImpl(FuncOp* func, RegisterAllocator* ra,
                               const SideTableAnchors& anchors,
                               RegToInstsMap& regToInsts,
                               RegHasNonInstMap& regHasNonInst);
  bool EliminatePhiUserMovImpl(FuncOp* func, RegisterAllocator* ra,
                               const SideTableAnchors& anchors,
                               RegToInstsMap& regToInsts,
                               RegHasNonInstMap& regHasNonInst);

 private:
  bool HasCallConflict(RegisterAllocator* ra, const Interval& interval,
                       unsigned reg_idx, Instruction* exclude_inst = nullptr);

  // Shared coalescing logic: checks conflicts and applies register reassignment
  // for a validated (mov, src_inst) pair. Returns true if coalescing succeeded.
  bool TryCoalesceMovSource(MovInst* mov, Instruction* src_inst,
                            RegisterAllocator* ra, unsigned max_regs,
                            RegToInstsMap& regToInsts,
                            RegHasNonInstMap& regHasNonInst);

  bool EliminateCallFuncMovImpl(FuncOp* func, RegisterAllocator* ra,
                                const SideTableAnchors& anchors,
                                RegToInstsMap& regToInsts,
                                RegHasNonInstMap& regHasNonInst);
  bool RemoveBackToBackReverseMovImpl(FuncOp* func, RegisterAllocator* ra,
                                      const SideTableAnchors& anchors);

  bool EliminateDeadInstructionsImpl(FuncOp* func, RegisterAllocator* ra,
                                     const SideTableAnchors& anchors);

  llvh::SmallVector<MovInst*, 16> to_removed_;
};

Pass* CreateMovEliminationPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_MOV_ELIMINATION_H_
