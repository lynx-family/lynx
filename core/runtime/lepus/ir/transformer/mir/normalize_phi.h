// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_NORMALIZE_PHI_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_NORMALIZE_PHI_H_

#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class IRContext;
class PhiInst;

class NormalizePhiPass : public FunctionPass {
 public:
  explicit NormalizePhiPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "normalize-phi-inst") {}
  ~NormalizePhiPass() override = default;
  bool RunOnFunction(FuncOp* func) override;
  void SetPhiType(PhiInst* phi_inst);
  void SetInstLocation(FuncOp* func);
  void NormalizePhi(FuncOp* func);

 private:
  llvh::SmallVector<PhiInst*, 16> to_removed_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_NORMALIZE_PHI_H_
