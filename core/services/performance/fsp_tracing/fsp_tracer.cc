// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/fsp_tracing/fsp_tracer.h"

#include <cmath>
#include <cstdint>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/public/performance_controller_platform_impl.h"
#include "core/services/performance/performance_controller.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace tasm {
namespace performance {

void FSPTracer::Start(CompletionCallback completion_callback) {
  if (!config_.enable_ || is_running_) {
    return;
  }
  completion_callback_ = std::move(completion_callback);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::Start");
  is_running_ = true;
  // Start hard timeout
  if (config_.hard_timeout_ms_ <= 0) {
    return;
  }
  std::weak_ptr<FSPTracer> weak_this = shared_from_this();
  PerformanceController::GetTaskRunner()->PostDelayedTask(
      [weak_this]() {
        auto this_ptr = weak_this.lock();
        if (this_ptr && this_ptr->is_running_) {
          this_ptr->is_running_ = false;
          this_ptr->OnFSPHardTimeOut(this_ptr->previous_snapshot_,
                                     base::CurrentSystemTimeMicroseconds());
        }
      },
      fml::TimeDelta::FromMilliseconds(config_.hard_timeout_ms_));
}

void FSPTracer::Stop(int64_t current_timestamp_us) {
  if (!config_.enable_ || !is_running_) {
    return;
  }
  is_running_ = false;
  OnFSPStop(previous_snapshot_, current_timestamp_us);
}

void FSPTracer::CancelledByUserInteraction(int64_t current_timestamp_us) {
  if (!config_.enable_ || !is_running_) {
    return;
  }
  is_running_ = false;
  // Send cancelled event
  OnFSPCancelledByUserInteraction(previous_snapshot_, current_timestamp_us);
}

void FSPTracer::OnCaptureSnapshot(FSPSnapshot current_snapshot) {
  if (!config_.enable_ || !is_running_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::OnCaptureSnapshot");
  // 1. Check if the snapshot is valuable.
  if (!IsSnapshotValuable(current_snapshot, config_)) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::ResetPreviousSnapshot");
    // If not satisfied, it proves instability, so the previous snapshot is also
    // invalid
    previous_snapshot_.reset();
    return;
  }

  auto& previous_snapshot = previous_snapshot_;
  // 2. If previous snapshot exists (timestamp > 0), check if the snapshot is
  // stable.
  if (previous_snapshot.has_value() &&
      previous_snapshot->last_change_timestamp_us_ > 0 &&
      IsSnapshotStable(current_snapshot, *previous_snapshot, config_)) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::SnapshotStable");
    int64_t diff_t_ms = (current_snapshot.last_change_timestamp_us_ -
                         previous_snapshot->last_change_timestamp_us_) /
                        1000;
    // If snapshot is stable, check if the interval is less than the min
    // interval.
    if (diff_t_ms >= config_.min_diff_interval_ms_) {
      // If the interval is greater than the min interval, generate FSP.
      // Generate FSP, previous_snapshot is the snapshot at FSP time
      // Construct FSPEntry based on previous_snapshot and send it out.
      OnFSP(previous_snapshot);
    }
    return;
  }
  // 3. Update previous snapshot, if the snapshot is valuable.
  previous_snapshot_ = std::move(current_snapshot);
}

bool FSPTracer::IsSnapshotValuable(FSPSnapshot& snapshot,
                                   const FSPConfig& config) const {
  // 1. Check X and Y axis projection fill rates
  auto total_projection_w = snapshot.x_total_content_projections_.count();
  if (total_projection_w <= 0) {
    return false;
  }
  snapshot.content_fill_percentage_x_ = static_cast<int32_t>(
      snapshot.x_projections_.count() * 100 / total_projection_w);
  if (snapshot.content_fill_percentage_x_ <
      config.min_content_fill_percentage_x_) {
    return false;
  }

  // 2. Check effective area fill rate
  auto total_projection_h = snapshot.y_total_content_projections_.count();
  if (total_projection_h <= 0) {
    return false;
  }
  snapshot.content_fill_percentage_y_ = static_cast<int32_t>(
      (snapshot.y_projections_.count() * 100 / total_projection_h));
  if (snapshot.content_fill_percentage_y_ <
      config.min_content_fill_percentage_y_) {
    return false;
  }

  // 3. Check effective area fill rate
  if (snapshot.total_content_area_ <= 0) {
    return false;
  }
  snapshot.content_fill_percentage_total_area_ =
      static_cast<int32_t>(snapshot.total_presented_content_area_ * 100 /
                           snapshot.total_content_area_);
  return snapshot.content_fill_percentage_total_area_ >=
         config.min_content_fill_percentage_total_area_;
}

bool FSPTracer::IsSnapshotStable(const FSPSnapshot& current,
                                 const FSPSnapshot& previous,
                                 const FSPConfig& config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::IsSnapshotStable");
  if (previous.last_change_timestamp_us_ <= 0 ||
      current.last_change_timestamp_us_ <= 0) {
    // Initial snapshot always considered stable.
    return false;
  }
  // diff interval in milliseconds
  int64_t diff_t_ms =
      (current.last_change_timestamp_us_ - previous.last_change_timestamp_us_) /
      1000;
  if (diff_t_ms <= 0) {
    return false;
  }
  // 1. (fast) Projection change rate
  int acceptable_pixel_diff_per_sec = config.acceptable_pixel_diff_per_sec_;
  int change_rate_w =
      std::abs(static_cast<int>(
          (current.x_projections_.count() - previous.x_projections_.count()))) *
      1000 / diff_t_ms;
  if (change_rate_w > acceptable_pixel_diff_per_sec) {
    return false;
  }
  int change_rate_h =
      std::abs(static_cast<int>(
          (current.y_projections_.count() - previous.y_projections_.count()))) *
      1000 / diff_t_ms;
  if (change_rate_h > acceptable_pixel_diff_per_sec) {
    return false;
  }

  // 2. (fast) Area projection change rate
  int area_change_rate_w =
      std::abs(static_cast<int>((current.total_presented_content_area_ -
                                 previous.total_presented_content_area_))) *
      1000 / diff_t_ms;
  if (area_change_rate_w > config.acceptable_area_diff_per_sec_) {
    return false;
  }

  // 3. (slow) XOR projection change rate
  auto xor_change_rate_x =
      static_cast<int>(
          (current.x_projections_ ^ previous.x_projections_).count()) *
      1000 / diff_t_ms;
  if (xor_change_rate_x > acceptable_pixel_diff_per_sec) {
    return false;
  }
  auto xor_change_rate_y =
      static_cast<int>(
          (current.y_projections_ ^ previous.y_projections_).count()) *
      1000 / diff_t_ms;
  return (xor_change_rate_y <= acceptable_pixel_diff_per_sec);
}

void FSPTracer::OnFSP(const base::flex_optional<FSPSnapshot>& fsp_snapshot) {
  is_running_ = false;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::OnFSP");
  if (completion_callback_) {
    if (!fsp_snapshot.has_value()) {
      completion_callback_(FSPResult(FSPResult::kFSPError, -1));
      return;
    }
    FSPResult result(FSPResult::kFSPSuccess,
                     fsp_snapshot->last_change_timestamp_us_,
                     fsp_snapshot->content_fill_percentage_x_,
                     fsp_snapshot->content_fill_percentage_y_,
                     fsp_snapshot->content_fill_percentage_total_area_);
    completion_callback_(std::move(result));
  }
}

void FSPTracer::OnFSPStop(
    const base::flex_optional<FSPSnapshot>& previous_snapshot,
    int64_t current_timestamp_us) {
  if (completion_callback_) {
    if (!previous_snapshot.has_value()) {
      completion_callback_(
          FSPResult(FSPResult::kFSPStop, current_timestamp_us));
      return;
    }
    FSPResult result(FSPResult::kFSPStop,
                     previous_snapshot->last_change_timestamp_us_,
                     previous_snapshot->content_fill_percentage_x_,
                     previous_snapshot->content_fill_percentage_y_,
                     previous_snapshot->content_fill_percentage_total_area_);
    completion_callback_(std::move(result));
  }
}

void FSPTracer::OnFSPHardTimeOut(
    const base::flex_optional<FSPSnapshot>& current_snapshot,
    int64_t current_timestamp_us) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FSPTracer::OnFSPHardTimeOut");
  if (completion_callback_) {
    if (!current_snapshot.has_value()) {
      completion_callback_(
          FSPResult(FSPResult::kFSPCancelByTimeout, current_timestamp_us));
      return;
    }
    FSPResult result(FSPResult::kFSPCancelByTimeout,
                     current_snapshot->last_change_timestamp_us_,
                     current_snapshot->content_fill_percentage_x_,
                     current_snapshot->content_fill_percentage_y_,
                     current_snapshot->content_fill_percentage_total_area_);
    completion_callback_(std::move(result));
  }
}

void FSPTracer::OnFSPCancelledByUserInteraction(
    const base::flex_optional<FSPSnapshot>& current_snapshot,
    int64_t current_timestamp_us) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "FSPTracer::OnFSPCancelledByUserInteraction");
  if (completion_callback_) {
    if (!current_snapshot.has_value()) {
      completion_callback_(FSPResult(FSPResult::kFSPCancelByUserInteraction,
                                     current_timestamp_us));
      return;
    }
    FSPResult result(FSPResult::kFSPCancelByUserInteraction,
                     current_snapshot->last_change_timestamp_us_,
                     current_snapshot->content_fill_percentage_x_,
                     current_snapshot->content_fill_percentage_y_,
                     current_snapshot->content_fill_percentage_total_area_);
    completion_callback_(std::move(result));
  }
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
