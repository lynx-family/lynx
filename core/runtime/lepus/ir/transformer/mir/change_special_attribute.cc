// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/change_special_attribute.h"

namespace lynx {
namespace lepus {
namespace ir {
// simplify the attribute
bool ChangeSpecialAttributePass::RunOnModule(ModuleOp* mod) {
  bool changed = false;
  for (auto* func : *mod) {
    for (auto& bb : *func) {
      for (auto& op : bb) {
        auto* inst = llvh::dyn_cast<Instruction>(&op);
        if (!inst) continue;

        if (inst->GetToplevelVarReg() != constants::kInvalidSignedValue &&
            inst->GetClosureVarReg() != constants::kInvalidSignedValue) {
          // "Delete" the ClosureVarReg attribute because the toplevelVarReg &
          // closureVarReg are the same.
          inst->SetClosureVarReg(constants::kInvalidSignedValue);
          changed = true;
        }
      }
    }
  }
  return changed;
}

Pass* CreateChangeSpecialAttributePass(IRContext* ir_ctx) {
  return new ChangeSpecialAttributePass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
