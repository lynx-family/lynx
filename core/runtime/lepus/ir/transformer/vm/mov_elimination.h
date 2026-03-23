// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_MOV_ELIMINATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_MOV_ELIMINATION_H_

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

  bool RemoveMovWithSameSrcAndDst(FuncOp* func);

  bool RemoveBackToBackReverseMov(FuncOp* func, RegisterAllocator* ra);

  bool EliminateCallFuncMov(FuncOp* func, RegisterAllocator* ra);

  void SetCallFuncMovFix(FuncOp* func);

 private:
  bool HasCallConflict(RegisterAllocator* ra, const Interval& interval,
                       unsigned reg_idx, Instruction* exclude_inst = nullptr);

  llvh::SmallVector<MovInst*, 16> to_removed_;
};

Pass* CreateMovEliminationPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_MOV_ELIMINATION_H_
