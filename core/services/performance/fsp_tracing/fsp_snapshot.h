// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_FSP_TRACING_FSP_SNAPSHOT_H_
#define CORE_SERVICES_PERFORMANCE_FSP_TRACING_FSP_SNAPSHOT_H_

#include <bitset>
#include <utility>

#include "base/include/closure.h"
#include "base/include/geometry/rect.h"

namespace lynx {
namespace tasm {
namespace performance {

using IntRect = lynx::base::geometry::IntRect;
using IntSize = lynx::base::geometry::IntSize;

// FSP snapshot, used to store page content projection information at a certain
// moment
struct FSPSnapshot {
  static const size_t kXProjectionsLen = 256;
  static const size_t kYProjectionsLen = 512;

 public:
  using ReleaseCallback = base::MoveOnlyClosure<void, void*>;
  explicit FSPSnapshot(IntSize container_size, int64_t last_change_timestamp_us)
      : container_size_(std::move(container_size)),
        last_change_timestamp_us_(last_change_timestamp_us) {}
  FSPSnapshot() = default;
  ~FSPSnapshot() = default;

  std::bitset<kXProjectionsLen> x_projections_;  // X-axis projection bitmap
  std::bitset<kYProjectionsLen> y_projections_;  // Y-axis projection bitmap

  std::bitset<kXProjectionsLen>
      x_total_content_projections_;  // X-axis projection bitmap
  std::bitset<kYProjectionsLen>
      y_total_content_projections_;  // Y-axis projection bitmap

  IntSize container_size_ = {0, 0};           // Container size
  int64_t total_presented_content_area_ = 0;  // Presented effective total area
  int64_t total_content_area_ = 0;            // Effective total area
  int64_t last_change_timestamp_us_ = 0;  // Timestamp of last content change

  /// X-axis content fill percentage
  int32_t content_fill_percentage_x_ = 0;
  /// Y-axis content fill percentage
  int32_t content_fill_percentage_y_ = 0;
  /// Total content fill percentage
  /// Formula: (presented content area / content area) * 100
  int32_t content_fill_percentage_total_area_ = 0;
  /// Percentage of presented content area relative to container area.
  /// Formula: (presented content area / container area) * 100
  int32_t container_fill_percentage_container_area_ = 0;
#if ENABLE_TRACE_PERFETTO
  uint64_t trace_timestamp_us_ = 0;
  uint64_t trace_thread_id_ = 0;
#endif

  void FillContentToSnapshot(bool is_presented, IntRect rect,
                             int64_t first_presented_timestamp_us);

  FSPSnapshot(const FSPSnapshot&) = delete;
  FSPSnapshot& operator=(const FSPSnapshot&) = delete;
  FSPSnapshot(FSPSnapshot&&) = default;
  FSPSnapshot& operator=(FSPSnapshot&&) = default;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_FSP_TRACING_FSP_SNAPSHOT_H_
