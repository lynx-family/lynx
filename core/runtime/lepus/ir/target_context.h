// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TARGET_CONTEXT_H_
#define CORE_RUNTIME_LEPUS_IR_TARGET_CONTEXT_H_

#include <memory>

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

class FuncOp;

// TargetContext holds target-specific analysis and side tables.
// Currently only VM register allocation analysis is required.
class TargetContext {
 public:
  TargetContext() = default;
  ~TargetContext() = default;

  RegisterAllocator* GetRegisterAllocAnalysis(FuncOp* func);
  void SetRegisterAllocAnalysis(FuncOp* func,
                                std::unique_ptr<RegisterAllocator>& ra);

 private:
  llvh::DenseMap<FuncOp*, std::unique_ptr<RegisterAllocator>> ra_map_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TARGET_CONTEXT_H_
