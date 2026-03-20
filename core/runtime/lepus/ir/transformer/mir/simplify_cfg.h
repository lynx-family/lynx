// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_SIMPLIFY_CFG_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_SIMPLIFY_CFG_H_

#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class SimplifyCFGPass : public ModulePass {
 public:
  explicit SimplifyCFGPass(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "simplify-cfg") {}
  ~SimplifyCFGPass() override = default;
  bool RunOnModule(ModuleOp* mod) override;

  bool OptimizeStaticBranches(FuncOp* f);
  bool AttemptBranchRemovalFromPhiNodes(Block* bb);
};

Pass* CreateSimplifyCFG(IRContext* ir_ctx);
bool OptIndirectJmp(Instruction* inst);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_SIMPLIFY_CFG_H_
