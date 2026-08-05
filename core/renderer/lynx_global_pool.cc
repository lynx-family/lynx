// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/lynx_global_pool.h"

#include <utility>

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

LynxGlobalPool& LynxGlobalPool::GetInstance() {
  static base::NoDestructor<LynxGlobalPool> instance;
  return *instance;
}

void LynxGlobalPool::PreparePool() {
  PreparePool(runtime::ContextType::LepusNGContextType, -1);
}

void LynxGlobalPool::PreparePool(runtime::ContextType context_type,
                                 int32_t count) {
  if (context_type == runtime::ContextType::LepusNGContextType && count <= 0) {
    constexpr int32_t kGlobalQuickContextPoolSize = 5;
    count = tasm::LynxEnv::GetInstance().GetGlobalQuickContextPoolSize(
        kGlobalQuickContextPoolSize);
  }
  if (count <= 0) {
    return;
  }
  auto* pool = GetPool(context_type);
  if (pool == nullptr) {
    return;
  }
  pool->FillPool(count);
}

void LynxGlobalPool::PreparePoolSync(runtime::ContextType context_type,
                                     int32_t count) {
  if (count <= 0) {
    return;
  }
  auto* pool = GetPool(context_type);
  if (pool == nullptr) {
    return;
  }
  pool->FillPoolSync(count);
}

shell::MTSRuntimePool* LynxGlobalPool::GetPool(
    runtime::ContextType context_type) {
  std::lock_guard<std::mutex> lock(context_pools_mutex_);
  auto it = context_pools_.find(context_type);
  if (it != context_pools_.end()) {
    return it->second.get();
  }
  auto pool = shell::MTSRuntimePool::Create(context_type, false);
  auto* pool_ptr = pool.get();
  context_pools_.emplace(context_type, std::move(pool));
  return pool_ptr;
}

}  // namespace tasm
}  // namespace lynx
