// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_INST_COMBINE_INST_COMBINE_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_INST_COMBINE_INST_COMBINE_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class InstCombinePass : public FunctionPass {
 public:
  explicit InstCombinePass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "inst-combine") {}
  ~InstCombinePass() override {}
  bool RunOnFunction(FuncOp* func) override;

 private:
  bool changed_ = false;
};

Instruction* CombineCompareAndJmp(OpBuilder* builder, Instruction* inst);
Instruction* CombineCondBranch(OpBuilder* builder, Instruction* inst);
Value* ConstantFold(OpBuilder* builder, Instruction* inst);
Value* FoldBinaryOperatorInst(OpBuilder* builder, Instruction* inst);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_INST_COMBINE_INST_COMBINE_H_
