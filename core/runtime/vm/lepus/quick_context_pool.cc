// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/quick_context_pool.h"

#include <algorithm>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/quick_context.h"

namespace lynx {
namespace lepus {

std::shared_ptr<QuickContextPool> QuickContextPool::Create(
    const std::shared_ptr<ContextBundle>& context_bundle) {
  return std::shared_ptr<QuickContextPool>(
      new QuickContextPool(context_bundle));
}

void QuickContextPool::FillPool(int32_t count) {
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [count, weak_pool = std::weak_ptr<QuickContextPool>(
                  shared_from_this())]() mutable {
        auto context_pool = weak_pool.lock();
        if (!context_pool) {
          return;
        }

        count = context_pool->TryCheckSettings(count);

        if (count <= 0) {
          return;
        }

        context_pool->AddContextSafely(count);
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

int32_t QuickContextPool::TryCheckSettings(int32_t default_value) {
  if (need_check_settings_) {
    need_check_settings_ = false;
    return tasm::LynxEnv::GetInstance().GetGlobalQuickContextPoolSize(
        default_value);
  }
  return default_value;
}

void QuickContextPool::AddContextSafely(int32_t count) {
  // build contexts without lock
  decltype(contexts_) temp_contexts;
  for (; count > 0; --count) {
    auto context = std::make_shared<QuickContext>();
    // if context_bundle_ exists, should call DeSerialize. And if DeSerialize
    // fails, just return.
    if (context_bundle_ &&
        !context->DeSerialize(*context_bundle_, false, nullptr)) {
      return;
    }
    temp_contexts.emplace_back(std::move(context));
  }

  // lock and insert
  std::lock_guard<std::mutex> lock{mtx_};
  for (auto& c : temp_contexts) {
    contexts_.emplace_back(std::move(c));
  }
}

std::shared_ptr<lepus::QuickContext> QuickContextPool::TakeContextSafely() {
  std::shared_ptr<lepus::QuickContext> context = nullptr;
  {
    // lock to take context safely
    std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
    if (!lock.owns_lock() || contexts_.empty()) {
      return nullptr;
    }
    context.swap(contexts_.back());
    contexts_.pop_back();
  }

  // generate a new context
  if (enable_auto_generate_) {
    FillPool(1);
  }

  return context;
}

void QuickContextPool::SetEnableAutoGenerate(bool enable) {
  enable_auto_generate_ = enable;
}

}  // namespace lepus
}  // namespace lynx
