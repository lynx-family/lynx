// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/fsp_tracing/area/fsp_area_tracer_impl.h"

#include <algorithm>
#include <bitset>
#include <utility>

#include "core/services/fsp_tracing/base/fsp_snapshot.h"

namespace lynx {
namespace tasm {
namespace timing {

struct FSPAreaSnapshot : FSPSnapshot {
  static constexpr size_t X_PROJECTIONS_LEN = 256;
  static constexpr size_t Y_PROJECTIONS_LEN = 512;
  static constexpr size_t PROJECTIONS_LEN =
      X_PROJECTIONS_LEN * Y_PROJECTIONS_LEN;

  std::bitset<PROJECTIONS_LEN> projections;

  long last_ui_sign = -1;
  double last_change_timestamp;
  lynx::base::geometry::IntSize container_size;
  size_t projection_area;
};

bool DiffAreaSnapshots(FSPAreaSnapshot& current, FSPAreaSnapshot* previous,
                       const FSPAreaConfig& config);

std::unique_ptr<FSPSnapshot> FSPAreaTracer::CreateSnapshot() {
  return std::make_unique<FSPAreaSnapshot>();
}

FSPResult FSPAreaTracer::CaptureSnapshotImpl(
    FSPSnapshot& snapshot, lynx::base::geometry::IntSize container_size) {
  auto& ss = static_cast<FSPAreaSnapshot&>(snapshot);
  ss = {};

  if (!container_size.IsEmpty() && container_size.Width() <= UINT16_MAX &&
      container_size.Height() <= UINT16_MAX) {
    // Check container size to avoid overflow in the mulplications below.
    ss.container_size = container_size;
  }

  callback_(ss);
  bool is_stable = DiffAreaSnapshots(
      ss, static_cast<FSPAreaSnapshot*>(previous_snapshot_.get()), config_);
  return ss.GetResult(is_stable);
}

void FSPAreaTracer::FillSnapshotImpl(FSPSnapshot& snapshot,
                                     const FSPContentInfo& info) {
  auto& ss = static_cast<FSPAreaSnapshot&>(snapshot);
  if (info.rect.IsEmpty() || ss.container_size.IsEmpty()) {
    return;
  }

  // Only update last_ui_sign when we have a newer content timestamp
  if (info.change_timestamp >= ss.last_change_timestamp) {
    ss.last_ui_sign = info.ui_sign;
    ss.last_change_timestamp = info.change_timestamp;
  }

  auto w = ss.container_size.Width();
  auto h = ss.container_size.Height();

  auto min_x = static_cast<size_t>(std::clamp(info.rect.X(), 0, w));
  min_x = min_x * FSPAreaSnapshot::X_PROJECTIONS_LEN / w;
  min_x = std::min(min_x, FSPAreaSnapshot::X_PROJECTIONS_LEN - 1);

  auto max_x = static_cast<size_t>(std::clamp(info.rect.MaxX(), 0, w));
  max_x = max_x * FSPAreaSnapshot::X_PROJECTIONS_LEN / w;
  max_x = std::min(max_x, FSPAreaSnapshot::X_PROJECTIONS_LEN - 1);

  auto min_y = static_cast<size_t>(std::clamp(info.rect.Y(), 0, h));
  min_y = min_y * FSPAreaSnapshot::Y_PROJECTIONS_LEN / h;
  min_y = std::min(min_y, FSPAreaSnapshot::Y_PROJECTIONS_LEN - 1);

  auto max_y = static_cast<size_t>(std::clamp(info.rect.MaxY(), 0, h));
  max_y = max_y * FSPAreaSnapshot::Y_PROJECTIONS_LEN / h;
  max_y = std::min(max_y, FSPAreaSnapshot::Y_PROJECTIONS_LEN - 1);

  for (auto y = min_y; y <= max_y; y++) {
    for (auto x = min_x; x <= max_x; x++) {
      ss.projections[y * FSPAreaSnapshot::X_PROJECTIONS_LEN + x] = true;
    }
  }
}

bool DiffAreaSnapshots(FSPAreaSnapshot& current, FSPAreaSnapshot* previous,
                       const FSPAreaConfig& config) {
  auto container_a = static_cast<size_t>(current.container_size.Width()) *
                     static_cast<size_t>(current.container_size.Height());
  auto projection_a = current.projections.count() * container_a /
                      FSPAreaSnapshot::PROJECTIONS_LEN;
  current.projection_area = projection_a;

  // check against completion rates.
  auto min_a = static_cast<long>(static_cast<double>(container_a) *
                                 config.minimum_completion_rate);
  if ((min_a > 0) && (projection_a < static_cast<size_t>(min_a))) {
    return false;
  }

  if (!previous) {
    // Initial snapshot always considered stable.
    return true;
  }

  // diff area changes.(fast)
  auto diff_a = projection_a - previous->projection_area;
  if (diff_a >= config.acceptable_difference_per_snapshot) {
    return false;
  }

  // diff interval in miliseconds
  auto diff_t =
      (current.last_change_timestamp - previous->last_change_timestamp) *
      1000.0;

  // Check if the interval is less than the minimum interval.
  if ((config.minimum_diff_interval_ms > 0.0) &&
      (diff_t < config.minimum_diff_interval_ms)) {
    return false;
  }

  // diff area change rates.(fast)
  auto rate_a = static_cast<double>(diff_a) / diff_t;
  if (rate_a >= config.acceptable_difference_per_ms) {
    return false;
  }

  // diff content change rates.(slow)
  auto diff_p = (current.projections ^ previous->projections).count() *
                container_a / FSPAreaSnapshot::PROJECTIONS_LEN;
  auto rate_p = static_cast<double>(diff_p) / diff_t;
  bool is_stable = !(rate_p >= config.acceptable_difference_per_ms);
  return is_stable;
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
