// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// clang-format off
#include "core/runtime/lepus/ir/value_forward_declare.h"
// clang-format on
#include "core/runtime/lepus/ir/target_context.h"

#include <utility>

namespace lynx {
namespace lepus {
namespace ir {

RegisterAllocator* TargetContext::GetRegisterAllocAnalysis(FuncOp* func) {
  auto iter = ra_map_.find(func);
  if (iter != ra_map_.end()) return iter->second.get();
  return nullptr;
}

void TargetContext::SetRegisterAllocAnalysis(
    FuncOp* func, std::unique_ptr<RegisterAllocator>& ra) {
  ra_map_[func] = std::move(ra);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
