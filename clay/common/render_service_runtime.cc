// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/render_service_runtime.h"

#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>
namespace clay::display {
std::shared_ptr<RenderServiceRuntime> RenderServiceRuntime::Acquire(
    uint64_t device, fml::RefPtr<fml::TaskRunner> platform,
    fml::RefPtr<fml::TaskRunner> worker) {
  using Key = std::tuple<uint64_t, fml::TaskRunner*, fml::TaskRunner*>;
  static std::mutex mutex;
  static std::map<Key, std::weak_ptr<RenderServiceRuntime>> runtimes;
  static std::map<uint64_t, std::weak_ptr<TextureBudget>> budgets;
  std::lock_guard<std::mutex> lock(mutex);
  for (auto it = runtimes.begin(); it != runtimes.end();) {
    if (it->second.expired())
      it = runtimes.erase(it);
    else
      ++it;
  }
  auto& weak = runtimes[{device, platform.get(), worker.get()}];
  auto runtime = weak.lock();
  if (!runtime) {
    auto budget = budgets[device].lock();
    if (!budget) {
      budget = std::make_shared<TextureBudget>();
      budgets[device] = budget;
    }
    runtime = std::shared_ptr<RenderServiceRuntime>(
        new RenderServiceRuntime(platform, worker, std::move(budget)));
    weak = runtime;
  }
  return runtime;
}
}  // namespace clay::display
