// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_SSA_IR_VERIFY_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_SSA_IR_VERIFY_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class IRContext;

class SSAIRVerifyPass : public ModulePass {
 public:
  explicit SSAIRVerifyPass(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "verify-ssa-ir") {}
  ~SSAIRVerifyPass() override = default;
  bool RunOnModule(ModuleOp* mod) override;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_SSA_IR_VERIFY_H_
