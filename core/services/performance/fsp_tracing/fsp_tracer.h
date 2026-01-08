// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_FSP_TRACING_FSP_TRACER_H_
#define CORE_SERVICES_PERFORMANCE_FSP_TRACING_FSP_TRACER_H_

#include <cstdint>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "base/include/flex_optional.h"
#include "core/services/performance/fsp_tracing/fsp_snapshot.h"

namespace lynx {
namespace tasm {
namespace performance {
// Core FSP configuration, used to define FSP judgment criteria
struct FSPConfig {
 public:
  bool enable_ = false;
  // Minimum content fill rate in X-axis direction, default is 30%.
  int32_t min_content_fill_percentage_x_ = 30;
  // Minimum content fill rate in Y-axis direction, default is 30%.
  int32_t min_content_fill_percentage_y_ = 30;
  // Minimum content fill rate in total area direction, default is 30%.
  int32_t min_content_fill_percentage_total_area_ = 30;
  // Minimum container fill rate in container area direction, default is 1%.
  int32_t min_container_fill_percentage_container_area_ = 1;
  // Acceptable pixel change rate on projection axis per second, default is
  // 10 pixels per second.
  int32_t acceptable_pixel_diff_per_sec_ = 10;
  // Acceptable total pixel area change rate per second, default is 10*10
  // pixels per second.
  int32_t acceptable_area_diff_per_sec_ = 100;
  // Minimum time interval to determine stability, default is 300ms.
  int32_t min_diff_interval_ms_ = 300;
  // Hard timeout in milliseconds, default is 10 seconds.
  int32_t hard_timeout_ms_ = 10 * 1000;
};

struct FSPResult {
 public:
  constexpr static const char kFSPSuccess[] = "success";
  constexpr static const char kFSPCancelByUserInteraction[] =
      "cancelByUserInteraction";
  constexpr static const char kFSPCancelByTimeout[] = "timeout";
  constexpr static const char kFSPStop[] = "stop";
  constexpr static const char kFSPError[] = "error";

  FSPResult(const char* status, int64_t fsp_timestamp_us,
            int32_t content_fill_percentage_x = 0.0,
            int32_t content_fill_percentage_y = 0.0,
            int32_t content_fill_percentage_total_area = 0.0,
            int32_t container_fill_percentage_container_area = 0.0)
      : status_(status),
        fsp_timestamp_us_(fsp_timestamp_us),
        content_fill_percentage_x_(content_fill_percentage_x),
        content_fill_percentage_y_(content_fill_percentage_y),
        content_fill_percentage_total_area_(content_fill_percentage_total_area),
        container_fill_percentage_container_area_(
            container_fill_percentage_container_area) {}
  ~FSPResult() = default;

  FSPResult(const FSPResult& result) = delete;
  FSPResult& operator=(const FSPResult&) = delete;
  FSPResult(FSPResult&& other) = default;
  FSPResult& operator=(FSPResult&& other) = default;

  const char* status_ = nullptr;
  int64_t fsp_timestamp_us_ = 0;
  /// X-axis content fill percentage
  int32_t content_fill_percentage_x_ = 0;
  /// Y-axis content fill percentage
  int32_t content_fill_percentage_y_ = 0;
  /// Total content fill percentage
  int32_t content_fill_percentage_total_area_ = 0;
  /// Container fill rate
  int32_t container_fill_percentage_container_area_ = 0;
#if ENABLE_TRACE_PERFETTO
  int64_t trace_fsp_timestamp_us_ = 0;
#endif
};

/// FSPTracer is responsible for monitoring and tracing FSP events.
class FSPTracer : public std::enable_shared_from_this<FSPTracer> {
 public:
  using CompletionCallback = base::MoveOnlyClosure<void, FSPResult>;

  explicit FSPTracer(FSPConfig config) : config_(std::move(config)) {}
  ~FSPTracer() = default;

  // Start monitoring
  void Start(CompletionCallback completion_callback);
  void Stop(int64_t current_timestamp_us);
  // User interaction occurred, cancel monitoring and send FSP event
  void CancelledByUserInteraction(int64_t current_timestamp_us);
  // Capture current snapshot and compare with previous snapshot
  void OnCaptureSnapshot(FSPSnapshot snapshot);
  // Check if the snapshot is valuable for FSP calculation
  bool IsSnapshotValuable(FSPSnapshot& snapshot, const FSPConfig& config) const;
  // Compare two snapshots to determine if stable state is reached
  bool IsSnapshotStable(const FSPSnapshot& current, const FSPSnapshot& previous,
                        const FSPConfig& config_);
  void UpdateStartTimestamp();

  inline void SetInstanceId(int32_t instance_id) { instance_id_ = instance_id; }

 private:
  // Send FSP event with the given snapshot
  void OnFSP(const base::flex_optional<FSPSnapshot>& fsp_snapshot);
  // Send FSP event with the current snapshot when hard timeout occurs
  void OnFSPStop(const base::flex_optional<FSPSnapshot>& current_snapshot,
                 int64_t current_timestamp_us);
  // Send FSP event with the current snapshot when hard timeout occurs
  void OnFSPHardTimeOut(
      const base::flex_optional<FSPSnapshot>& current_snapshot,
      int64_t current_timestamp_us);
  // Send FSP event with the current snapshot when user interaction occurs
  void OnFSPCancelledByUserInteraction(
      const base::flex_optional<FSPSnapshot>& current_snapshot,
      int64_t current_timestamp_us);
  void HandleFSPResult(const char* status,
                       const base::flex_optional<FSPSnapshot>& current_snapshot,
                       int64_t current_timestamp_us);

  FSPTracer(const FSPTracer& tracer) = delete;
  FSPTracer& operator=(const FSPTracer&) = delete;
  FSPTracer(FSPTracer&& other) = delete;
  FSPTracer& operator=(FSPTracer&& other) = delete;

  // FSP configuration
  FSPConfig config_;
  // Previous snapshot, used to compare with current snapshot
  base::flex_optional<FSPSnapshot> previous_snapshot_;
  // Whether the tracer is running
  bool is_running_ = false;
  CompletionCallback completion_callback_;
  int32_t instance_id_ = -1;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_FSP_TRACING_FSP_TRACER_H_
