// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_VERIFY_CALL_REGISTER_PASS_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_VERIFY_CALL_REGISTER_PASS_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class VerifyCallRegisterPass : public FunctionPass {
 public:
  explicit VerifyCallRegisterPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "verify-call-register") {}
  ~VerifyCallRegisterPass() override = default;
  bool RunOnFunction(FuncOp* func) override;
};

Pass* CreateVerifyCallRegisterPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_VERIFY_CALL_REGISTER_PASS_H_
