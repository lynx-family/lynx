// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"

#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

void PassManager::AddPass(Pass* pass) {
  pipeline_.emplace_back(pass);
  pass->SetMode(mode_);
  idx_++;
}

void PassManager::Run(FuncOp* func_op) {
  for (const std::unique_ptr<Pass>& pass : pipeline_) {
    auto* fun_pass = llvh::dyn_cast<FunctionPass>(pass.get());
    if (LEPUS_UNLIKELY(!fun_pass)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: PassManager::Run(FuncOp) encountered non-function "
          "pass in pipeline");
    }
    fun_pass->RunOnFunction(func_op);
  }
}

void PassManager::Run(ModuleOp* mod) {
  uint32_t pass_idx = 0;
  for (std::unique_ptr<Pass>& pass : pipeline_) {
    /// Handle function passes:
    if (auto* func_pass = llvh::dyn_cast<FunctionPass>(pass.get())) {
      for (auto* func_op : *mod) {
        func_pass->RunOnFunction(func_op);
      }
    } else if (auto* mod_pass = llvh::dyn_cast<ModulePass>(pass.get())) {
      mod_pass->RunOnModule(mod);
    }

    // IR dump is only available when LEPUS_TEST is enabled.
#ifdef LEPUS_TEST
    if (ir_dump_path_ != nullptr) {
      auto pass_name = pass->GetName();
      if (pass_name == "verify-call-register" || pass_name == "verify-ssa-ir") {
        // skip Dump verify pass
        continue;
      }
      auto dump_pass = new CFGRawViewer(ir_ctx_);
      std::string prefix = ir_dump_path_;
      std::string target_path =
          prefix + "/" + std::to_string(pass_idx) + "_" + pass->GetName();
      dump_pass->SetTargetDir(target_path);
      dump_pass->SetMode(pass->GetMode());
      dump_pass->RunOnModule(mod);
      delete dump_pass;
    }
#endif
    pass_idx++;
  }
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
