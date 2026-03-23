// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_REGISTER_COMPACTION_PASS_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_REGISTER_COMPACTION_PASS_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

// Post-register-allocation pass that renumbers physical registers into a
// dense range while preserving register order. This reduces the function's
// max register index/usage without changing instruction count or semantics.
class RegisterCompactionPass : public FunctionPass {
 public:
  explicit RegisterCompactionPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "reg-compact-pass") {}
  ~RegisterCompactionPass() override = default;

  bool RunOnFunction(FuncOp* func) override;
};

Pass* CreateRegisterCompactionPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_REGISTER_COMPACTION_PASS_H_
