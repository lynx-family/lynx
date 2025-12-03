// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/fsp_tracing/fsp_snapshot.h"

#include <algorithm>

#include "base/trace/native/trace_event.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace tasm {
namespace performance {
void FSPSnapshot::FillContentToSnapshot(bool is_presented, IntRect rect,
                                        int64_t first_presented_timestamp_us) {
  // If content is invalid, return directly
  if (rect.IsEmpty() || container_size_.IsEmpty()) {
    return;
  }

  int w = container_size_.Width();
  int h = container_size_.Height();

  // Optimization for edge case: Ignore if content is outside container bounds
  // Check if the rectangle is completely outside the container, if so, do not
  // participate in projection calculation
  if (rect.X() > w || rect.Y() > h || rect.MaxX() <= 0 || rect.MaxY() <= 0) {
    return;
  }

  // X/Y axis projection algorithm: "Flatten" the 2D rect area onto 1D X and Y
  // axes respectively, and mark the covered intervals Use std::max/min to limit
  // coordinates within [0, w] and [0, h] ranges to correctly handle partially
  // visible rectangles
  size_t min_x = static_cast<size_t>(std::max(0, rect.X()));
  size_t max_x = static_cast<size_t>(std::min(w, rect.MaxX()));

  size_t min_y = static_cast<size_t>(std::max(0, rect.Y()));
  size_t max_y = static_cast<size_t>(std::min(h, rect.MaxY()));

  // 1. Calculate pixel area of the content rectangle.
  int visible_content_area =
      static_cast<int>((max_x - min_x) * (max_y - min_y));
  total_content_area_ += visible_content_area;
  if (is_presented) {
    // Accumulate presented effective total area
    total_presented_content_area_ += visible_content_area;
  }

  // 2. Update X,Y projections
  size_t min_proj_x = min_x * FSPSnapshot::kXProjectionsLen / w;
  min_proj_x = std::min(min_proj_x, FSPSnapshot::kXProjectionsLen - 1);
  size_t max_proj_x = max_x * FSPSnapshot::kXProjectionsLen / w;
  max_proj_x = std::min(max_proj_x, FSPSnapshot::kXProjectionsLen - 1);

  size_t min_proj_y = min_y * FSPSnapshot::kYProjectionsLen / h;
  min_proj_y = std::min(min_proj_y, FSPSnapshot::kYProjectionsLen - 1);
  size_t max_proj_y = max_y * FSPSnapshot::kYProjectionsLen / h;
  max_proj_y = std::min(max_proj_y, FSPSnapshot::kYProjectionsLen - 1);

  for (size_t i = min_proj_x; i <= max_proj_x; i++) {
    if (is_presented) {
      x_projections_[i] = true;
    }
    x_total_content_projections_[i] = true;
  }
  for (size_t i = min_proj_y; i <= max_proj_y; i++) {
    if (is_presented) {
      y_projections_[i] = true;
    }
    y_total_content_projections_[i] = true;
  }

  // Update snapshot's last change timestamp
  if (first_presented_timestamp_us > last_change_timestamp_us_) {
    last_change_timestamp_us_ = first_presented_timestamp_us;
  }
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
