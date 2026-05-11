// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/target_context.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

bool HasRegisterOverflow(RegisterAllocator* ra) {
  if (!ra) {
    return false;
  }

  for (const auto& item : ra->GetAllocatedMap()) {
    if (item.second.IsValid() &&
        item.second.GetIndex() >= Register::kMaxRegistersLimit) {
      return true;
    }
  }
  return false;
}

void TargetContext::SetForceRootFuncDeopt(bool val) {
  if (force_root_func_deopt_ == val) {
    return;
  }
  force_root_func_deopt_ = val;
  // The planning result depends on the request-side override, so invalidate any
  // previous planning when tests or compile entrypoints toggle it.
  root_func_deopt_planned_ = false;
}

bool TargetContext::GetForceRootFuncDeopt() const {
  return force_root_func_deopt_;
}

RegisterAllocator* TargetContext::GetRegisterAllocAnalysis(FuncOp* func) {
  auto iter = ra_map_.find(func);
  if (iter != ra_map_.end()) return iter->second.get();
  return nullptr;
}

void TargetContext::SetRegisterAllocAnalysis(
    FuncOp* func, std::unique_ptr<RegisterAllocator>& ra) {
  ra_map_[func] = std::move(ra);
}

void TargetContext::EnableRootFuncDeopt(const std::string& reason) {
  if (is_root_func_deopt_) {
    return;
  }
  is_root_func_deopt_ = true;
  LOGI("Lepus IR enable root function deopt, reason=" << reason);
}

// Root-function deopt policy:
// - if the toplevel/root function overflows the 8-bit register limit, mark only
//   the root to use deopt fallback;
// - if any non-root function overflows, reject compilation instead of trying to
//   create another mixed-mode boundary.
void TargetContext::PlanRootFuncDeopt(ModuleOp* mod) {
  if (root_func_deopt_planned_) {
    return;
  }
  is_root_func_deopt_ = false;
  auto* root = mod->GetRootFunction();
  if (force_root_func_deopt_) {
    EnableRootFuncDeopt("forced root function deopt for top-level function");
  }

  for (auto* func : *mod) {
    if (!HasRegisterOverflow(GetRegisterAllocAnalysis(func))) {
      continue;
    }

    if (func == root) {
      const std::string reason =
          "register allocation exceeds 8-bit register limit after register "
          "compaction";
      EnableRootFuncDeopt(reason);
      continue;
    }

    std::string reason =
        std::string("Lepus IR error: only top-level function register ") +
        "overflow can use root-function deopt; function " + func->GetName() +
        " exceeds 8-bit register limit after register compaction";
    throw ::lynx::lepus::CompileException(reason.c_str());
  }

  root_func_deopt_planned_ = true;
}

bool TargetContext::IsRootFuncDeopt() const { return is_root_func_deopt_; }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
