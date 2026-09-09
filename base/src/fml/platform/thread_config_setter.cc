// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/thread_config_setter.h"

#include <atomic>

namespace lynx {
namespace fml {

namespace {

std::atomic<PlatformThreadPriority::ThreadSchedulingPolicyEnabledProvider>
    g_thread_scheduling_policy_enabled_provider{nullptr};

}  // namespace

void PlatformThreadPriority::SetThreadSchedulingPolicyEnabledProvider(
    ThreadSchedulingPolicyEnabledProvider provider) {
  g_thread_scheduling_policy_enabled_provider.store(provider,
                                                    std::memory_order_release);
}

bool PlatformThreadPriority::IsThreadSchedulingPolicyEnabled() {
  auto provider = g_thread_scheduling_policy_enabled_provider.load(
      std::memory_order_acquire);
  return provider != nullptr && provider();
}

}  // namespace fml
}  // namespace lynx
