// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_GET_TOPLEVEL_RELATED_INST_ELIMINATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_GET_TOPLEVEL_RELATED_INST_ELIMINATION_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

// After lowering (ProcessSpecialMovPass), we want to keep toplevel-related
// attributes only on SetToplevelVarInst / SetToplevelClosureVarInst. This pass:
// 1) Eliminates dead GetToplevelVarInst / GetToplevelClosureVarInst.
// 2) Strips toplevel-related attributes from Get* instructions.
// 3) Peephole: folds `SetToplevel*; GetToplevel*` pairs.
class GetToplevelRelatedInstEliminationPass : public ModulePass {
 public:
  explicit GetToplevelRelatedInstEliminationPass(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "get-toplevel-related-inst-elimination") {}
  ~GetToplevelRelatedInstEliminationPass() override = default;

  bool RunOnModule(ModuleOp* mod) override;

 private:
  bool PerformFunctionRemove(FuncOp* func);
};

Pass* CreateGetToplevelRelatedInstEliminationPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_GET_TOPLEVEL_RELATED_INST_ELIMINATION_H_
