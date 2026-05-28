// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_PERFORMANCE_NATIVE_MEMORY_USAGE_QUERY_H_
#define CORE_SHELL_PERFORMANCE_NATIVE_MEMORY_USAGE_QUERY_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/include/closure.h"
#include "base/include/lynx_actor.h"

namespace lynx {
namespace shell {
class BTSRuntime;
class LynxEngine;
}  // namespace shell

namespace tasm {
namespace performance {

// Native memory sampled by one Lynx instance. This struct intentionally
// contains only Lynx-attributed categories; app-level memory is sampled by the
// platform collector when it builds the global result.
struct NativeMemoryUsageSnapshot {
  // Element tree bytes reported by the node manager.
  int64_t element_bytes_{0};
  // Element node count paired with element_bytes_ for diagnostics.
  int32_t element_node_count_{0};
  // Main-thread runtime heap bytes read from current heap stats without forcing
  // GC.
  int64_t main_thread_runtime_bytes_{0};
  // Background-thread runtime heap bytes read from current heap stats without
  // forcing GC.
  int64_t background_thread_runtime_bytes_{0};
  // Shared background runtime group id. The platform global collector uses this
  // to deduplicate.
  std::string bts_runtime_group_id_;
};

using NativeMemoryUsageCallback =
    base::MoveOnlyClosure<void, NativeMemoryUsageSnapshot>;

class NativeMemoryUsageQuery {
 public:
  NativeMemoryUsageQuery() = default;
  ~NativeMemoryUsageQuery() = default;

  NativeMemoryUsageQuery(const NativeMemoryUsageQuery&) = delete;
  NativeMemoryUsageQuery& operator=(const NativeMemoryUsageQuery&) = delete;
  NativeMemoryUsageQuery(NativeMemoryUsageQuery&&) = delete;
  NativeMemoryUsageQuery& operator=(NativeMemoryUsageQuery&&) = delete;

  void CollectAsync(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& engine_actor,
      const std::shared_ptr<shell::LynxActor<shell::BTSRuntime>>& runtime_actor,
      NativeMemoryUsageCallback callback);

 private:
  struct QueryState;
  using NativeMemoryUsagePartCallback =
      base::MoveOnlyClosure<void, NativeMemoryUsageSnapshot>;

  static void CollectEngineSnapshotAsync(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& engine_actor,
      NativeMemoryUsagePartCallback callback);
  static void CollectRuntimeSnapshotAsync(
      const std::shared_ptr<shell::LynxActor<shell::BTSRuntime>>& runtime_actor,
      NativeMemoryUsagePartCallback callback);
  static void CompletePartOnReportThread(
      std::shared_ptr<QueryState> query_state,
      NativeMemoryUsageSnapshot snapshot);
  static void ReturnSnapshotOnReportThread(NativeMemoryUsageCallback callback,
                                           NativeMemoryUsageSnapshot snapshot);
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SHELL_PERFORMANCE_NATIVE_MEMORY_USAGE_QUERY_H_
