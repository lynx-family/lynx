// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/memory/process_memory_info.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "devtool/embedder/core/global_devtool_platform_embedder.h"
#include "devtool/embedder/core/global_devtool_platform_embedder_memory.h"

namespace lynx {
namespace devtool {
namespace {

constexpr int64_t kDefaultMemoryUsageTimeoutMs = 2000;
constexpr char kMemoryUsageTimeout[] = "Lynx memory collection timed out.";

}  // namespace

void GlobalDevtoolPlatformEmbedder::GetAllMemoryUsage(
    int64_t timeout_ms,
    base::MoveOnlyClosure<void, const std::string&, const std::string&>
        callback) {
  if (!callback) return;
  if (!tasm::performance::MemoryMonitor::Enable()) {
    std::move(callback)(
        "{}",
        "Lynx memory monitoring is disabled. Enable it before creating "
        "Lynx instances.");
    return;
  }
  auto* loop = fml::MessageLoop::IsInitializedForCurrentThread();
  if (!loop) {
    std::move(callback)("{}", "Lynx memory query requires a task runner.");
    return;
  }
  const auto start = std::chrono::steady_clock::now();
  const int64_t start_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  timeout_ms = timeout_ms > 0 ? timeout_ms : kDefaultMemoryUsageTimeoutMs;
  auto request =
      std::make_shared<EmbedderMemoryUsageRequest>(std::move(callback));
  // The caller's DevTool runner remains responsive while the report runner
  // waits for VM owners. A late monitor result must not respond twice.
  loop->GetTaskRunner()->PostDelayedTask(
      [request] { request->Complete("{}", kMemoryUsageTimeout); },
      fml::TimeDelta::FromMilliseconds(timeout_ms));
  tasm::performance::GlobalMemoryMonitor::GetInstance().GetGlobalMemoryUsage(
      [request, start, start_ms,
       timeout_ms](tasm::performance::GlobalMemoryUsage usage) {
        const int64_t duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count();
        if (duration_ms >= timeout_ms) {
          request->Complete("{}", kMemoryUsageTimeout);
          return;
        }
        auto result = BuildEmbedderMemoryUsageResult(
            usage, start_ms, duration_ms, timeout_ms,
            base::GetProcessPssBytes());
        Json::FastWriter writer;
        request->Complete(writer.write(result), "");
      });
}

}  // namespace devtool
}  // namespace lynx
