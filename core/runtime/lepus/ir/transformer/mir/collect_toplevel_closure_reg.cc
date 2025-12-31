// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/construct_ssa_ir.h"

namespace lynx {
namespace lepus {
namespace ir {

class CollectToplevelClosureReg : public ModulePass {
 public:
  explicit CollectToplevelClosureReg(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "collect-toplevel-closure-reg") {}
  ~CollectToplevelClosureReg() override = default;

  bool RunOnModule(ModuleOp* mod) override;
};

bool CollectToplevelClosureReg::RunOnModule(ModuleOp* mod) {
  auto root_function = mod->GetRootFunction();
  auto root_lepus_function = root_function->GetLepusFunction();

  for (const auto& child_lepus_func : root_lepus_function->GetChildFunction()) {
    for (auto i = 0; i < child_lepus_func->UpvaluesSize(); i++) {
      auto info = child_lepus_func->GetUpvalue(i);
      if (info->in_parent_vars_) {
        root_function->InsertToplevelClosureVarReg(info->register_);
      }
    }
  }

  return true;
}

Pass* CreateCollectToplevelClosureRegPass(IRContext* ir_ctx) {
  return new CollectToplevelClosureReg(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
