// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TARGET_CONTEXT_H_
#define CORE_RUNTIME_LEPUS_IR_TARGET_CONTEXT_H_

#include <memory>
#include <string>

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

class FuncOp;
class ModuleOp;

// TargetContext holds target-specific analysis and side tables.
// Currently only VM register allocation analysis and root-function deopt state
// are required.
class TargetContext {
 public:
  TargetContext() = default;
  ~TargetContext() = default;

  void SetForceRootFuncDeopt(bool val);
  bool GetForceRootFuncDeopt() const;

  RegisterAllocator* GetRegisterAllocAnalysis(FuncOp* func);
  void SetRegisterAllocAnalysis(FuncOp* func,
                                std::unique_ptr<RegisterAllocator>& ra);

  // Plan root-function deopt under the current policy:
  // 1) only the top-level function may deopt to root-function fallback;
  // 2) any non-top-level register overflow is a compile error.
  void PlanRootFuncDeopt(ModuleOp* mod);
  bool IsRootFuncDeopt() const;

 private:
  void EnableRootFuncDeopt(const std::string& reason);

  llvh::DenseMap<FuncOp*, std::unique_ptr<RegisterAllocator>> ra_map_;
  // Request-side override injected by the compile entry. This is distinct from
  // `is_root_func_deopt_`, which is the effective result after policy/planning.
  bool force_root_func_deopt_ = false;
  bool is_root_func_deopt_ = false;
  bool root_func_deopt_planned_ = false;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TARGET_CONTEXT_H_
