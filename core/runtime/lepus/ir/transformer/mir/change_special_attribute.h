// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CHANGE_SPECIAL_ATTRIBUTE_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CHANGE_SPECIAL_ATTRIBUTE_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

// If an instruction carries both `ToplevelVarReg` and `ClosureVarReg`, they
// must be equal. This pass asserts that invariant and then clears
// `ClosureVarReg` by setting it to the sentinel value (-1 / UINT_MAX).
//
// This pass is designed to run right after `SSAIRVerifyPass`.
class ChangeSpecialAttributePass : public ModulePass {
 public:
  explicit ChangeSpecialAttributePass(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "change-special-attribute") {}

  bool RunOnModule(ModuleOp* mod) override;
};

Pass* CreateChangeSpecialAttributePass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CHANGE_SPECIAL_ATTRIBUTE_H_
