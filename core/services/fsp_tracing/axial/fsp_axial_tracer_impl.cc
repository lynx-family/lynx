// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/fsp_tracing/axial/fsp_axial_tracer_impl.h"

#include <algorithm>
#include <bitset>
#include <memory>
#include <utility>

#include "core/services/fsp_tracing/base/fsp_snapshot.h"

namespace lynx {
namespace tasm {
namespace timing {

struct FSPAxialSnapshot : public FSPSnapshot {
  static constexpr size_t X_PROJECTIONS_LEN = 256;
  static constexpr size_t Y_PROJECTIONS_LEN = 512;

  std::bitset<X_PROJECTIONS_LEN> x_projections;
  std::bitset<Y_PROJECTIONS_LEN> y_projections;

  lynx::base::geometry::IntSize projection_size;
};

bool DiffAxialSnapshots(FSPAxialSnapshot& current, FSPAxialSnapshot* previous,
                        const FSPAxialConfig& config);

std::unique_ptr<FSPSnapshot> FSPAxialTracer::CreateSnapshot() {
  return std::make_unique<FSPAxialSnapshot>();
}

FSPResult FSPAxialTracer::CaptureSnapshotImpl(
    FSPSnapshot& snapshot, lynx::base::geometry::IntSize container_size) {
  auto& ss = static_cast<FSPAxialSnapshot&>(snapshot);
  ss = {};

  if (!container_size.IsEmpty() && (container_size.Width() <= UINT16_MAX) &&
      (container_size.Height() <= UINT16_MAX)) {
    // Check container size to avoid overflow in the mulplications below.
    ss.container_size = container_size;
  }

  callback_(ss);
  bool is_stable = DiffAxialSnapshots(
      ss, static_cast<FSPAxialSnapshot*>(previous_snapshot_.get()), config_);
  return ss.GetResult(is_stable);
}

void FSPAxialTracer::FillSnapshotImpl(FSPSnapshot& snapshot,
                                      const FSPContentInfo& info) {
  auto& ss = static_cast<FSPAxialSnapshot&>(snapshot);
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
  min_x = min_x * FSPAxialSnapshot::X_PROJECTIONS_LEN / w;
  min_x = std::min(min_x, FSPAxialSnapshot::X_PROJECTIONS_LEN - 1);

  auto max_x = static_cast<size_t>(std::clamp(info.rect.MaxX(), 0, w));
  max_x = max_x * FSPAxialSnapshot::X_PROJECTIONS_LEN / w;
  max_x = std::min(max_x, FSPAxialSnapshot::X_PROJECTIONS_LEN - 1);

  for (auto i = min_x; i <= max_x; i++) {
    ss.x_projections[i] = true;
  }

  auto min_y = static_cast<size_t>(std::clamp(info.rect.Y(), 0, h));
  min_y = min_y * FSPAxialSnapshot::Y_PROJECTIONS_LEN / h;
  min_y = std::min(min_y, FSPAxialSnapshot::Y_PROJECTIONS_LEN - 1);

  auto max_y = static_cast<size_t>(std::clamp(info.rect.MaxY(), 0, h));
  max_y = max_y * FSPAxialSnapshot::Y_PROJECTIONS_LEN / h;
  max_y = std::min(max_y, FSPAxialSnapshot::Y_PROJECTIONS_LEN - 1);

  for (auto i = min_y; i <= max_y; i++) {
    ss.y_projections[i] = true;
  }
}

bool DiffAxialSnapshots(FSPAxialSnapshot& current, FSPAxialSnapshot* previous,
                        const FSPAxialConfig& config) {
  auto container_w = current.container_size.Width();
  auto container_h = current.container_size.Height();
  auto projection_w =
      static_cast<int>(current.x_projections.count() * container_w /
                       FSPAxialSnapshot::X_PROJECTIONS_LEN);
  auto projection_h =
      static_cast<int>(current.y_projections.count() * container_h /
                       FSPAxialSnapshot::Y_PROJECTIONS_LEN);
  current.projection_size =
      lynx::base::geometry::IntSize(projection_w, projection_h);

  // check against completion rates.
  auto min_w =
      static_cast<int>(static_cast<double>(current.container_size.Width()) *
                       config.minimum_completion_rate_x);
  auto min_h =
      static_cast<int>(static_cast<double>(current.container_size.Height()) *
                       config.minimum_completion_rate_y);
  if (((min_w > 0) && (projection_w < min_w)) ||
      ((min_h > 0) && (projection_h < min_h))) {
    return false;
  }

  if (!previous) {
    // Initial snapshot always considered stable.
    return true;
  }

  // diff dimension changes.(fast)
  auto diff_w = projection_w - previous->projection_size.Width();
  auto diff_h = projection_h - previous->projection_size.Height();
  if ((diff_w >= config.acceptable_pixel_difference_per_snapshot) ||
      (diff_h >= config.acceptable_pixel_difference_per_snapshot)) {
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

  // diff dimension change rates.(fast)
  auto rate_w = static_cast<double>(diff_w) / diff_t;
  auto rate_h = static_cast<double>(diff_h) / diff_t;
  if ((rate_w >= config.acceptable_pixel_difference_per_ms) ||
      (rate_h >= config.acceptable_pixel_difference_per_ms)) {
    return false;
  }

  // diff content change rates.(slow)
  auto diff_x = (current.x_projections ^ previous->x_projections).count() *
                current.container_size.Width() /
                FSPAxialSnapshot::X_PROJECTIONS_LEN;
  auto diff_y = (current.y_projections ^ previous->y_projections).count() *
                current.container_size.Height() /
                FSPAxialSnapshot::Y_PROJECTIONS_LEN;
  auto rate_x = static_cast<double>(diff_x) / diff_t;
  auto rate_y = static_cast<double>(diff_y) / diff_t;
  bool is_stable = !(rate_x >= config.acceptable_pixel_difference_per_ms ||
                     rate_y >= config.acceptable_pixel_difference_per_ms);
  return is_stable;
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
