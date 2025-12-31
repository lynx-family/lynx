// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CSE_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CSE_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class CSE : public FunctionPass {
 public:
  explicit CSE(IRContext* ir_ctx) : FunctionPass(ir_ctx, "cse") {}
  ~CSE() override = default;

  bool RunOnFunction(FuncOp* f) override;
};

Pass* CreateCSE(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CSE_H_
