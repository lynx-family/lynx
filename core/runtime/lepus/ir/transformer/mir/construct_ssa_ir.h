// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CONSTRUCT_SSA_IR_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CONSTRUCT_SSA_IR_H_

#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

class ConstructSSAIRPass : public FunctionPass {
 public:
  explicit ConstructSSAIRPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "construct-ssa-ir") {}
  ~ConstructSSAIRPass() override = default;
  bool RunOnFunction(FuncOp* func) override;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_CONSTRUCT_SSA_IR_H_
