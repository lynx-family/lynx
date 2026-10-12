// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_GLOBAL_DEVTOOL_PLATFORM_EMBEDDER_MEMORY_H_
#define DEVTOOL_EMBEDDER_CORE_GLOBAL_DEVTOOL_PLATFORM_EMBEDDER_MEMORY_H_

#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/closure.h"
#include "core/services/performance/memory_monitor/memory_monitor_data.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

// Timeout and monitor completion can arrive on different task runners.
class EmbedderMemoryUsageRequest {
 public:
  using Callback =
      base::MoveOnlyClosure<void, const std::string&, const std::string&>;

  explicit EmbedderMemoryUsageRequest(Callback callback)
      : callback_(std::move(callback)) {}

  void Complete(const std::string& result, const std::string& error);

 private:
  std::mutex mutex_;
  Callback callback_;
};

// Adapt the shared monitor's snapshot without collecting platform UI data or
// recomputing its VM-deduplicated totals.
Json::Value BuildEmbedderMemoryUsageResult(
    const tasm::performance::GlobalMemoryUsage& usage,
    int64_t collection_start_ms, int64_t collection_duration_ms,
    int64_t collection_timeout_ms, int64_t app_bytes);

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_GLOBAL_DEVTOOL_PLATFORM_EMBEDDER_MEMORY_H_
