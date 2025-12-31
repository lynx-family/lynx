// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_DCE_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_DCE_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class DCE : public ModulePass {
 public:
  explicit DCE(IRContext* ir_ctx) : ModulePass(ir_ctx, "dce") {}
  ~DCE() override = default;

  bool RunOnModule(ModuleOp* mod) override;

 private:
  bool PerformFunctionDCE(FuncOp* func);
  bool DeleteUselessInst(Instruction* inst);
};

Pass* CreateDCE(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_DCE_H_
