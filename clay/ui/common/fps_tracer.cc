// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/fps_tracer.h"

namespace clay {

namespace {
constexpr float kCapacityReserveFactor = 1.2;

constexpr int kDesiredRefreshRate = 60;
constexpr double kDesiredFrameIntervalInMillis =
    1.0 / kDesiredRefreshRate * 1000.0;
constexpr int kOffsetToMillis = 100;
constexpr int kDesiredFrameIntervalInMillis100 =
    kDesiredFrameIntervalInMillis * kOffsetToMillis;

constexpr int64_t kTimeMsToNs = 1000000;
constexpr int64_t kTimeSToMs = 1000;
}  // namespace

FpsTracer::FpsTracer(const std::string scene, const std::string tag,
                     int32_t max_refresh_rate, int session_timeout_s)
    : config_{scene, tag}, max_refresh_rate_(max_refresh_rate) {
  const int refresh_rate =
      max_refresh_rate_ > 0 ? max_refresh_rate_ : kDesiredRefreshRate;
  refresh_interval_ns_ = 1.0 * kTimeSToMs * kTimeMsToNs / refresh_rate;

  frame_time_samples_.reserve(static_cast<size_t>(
      refresh_rate * session_timeout_s * kCapacityReserveFactor));
}
FpsTracer::~FpsTracer() = default;

void FpsTracer::Start() { start_time_ = fml::TimePoint::Now(); }

void FpsTracer::Stop() { end_time_ = fml::TimePoint::Now(); }

void FpsTracer::GetFpsMetrics(FpsRawMetrics& fps_metrics) {
  if (frame_time_samples_.size() < 2) {
    return;
  }

  size_t sample_count = frame_time_samples_.size();
  int frame_count = sample_count - 1;

  int64_t duration_ms = (end_time_ - start_time_).ToMilliseconds();
  fps_metrics.duration_ms = duration_ms;
  fps_metrics.max_refresh_rate = max_refresh_rate_;

  int64_t interval_ms =
      (frame_time_samples_[sample_count - 1] - frame_time_samples_[0]) /
      kTimeMsToNs;
  fps_metrics.frames = frame_count;
  fps_metrics.fps = (fps_metrics.frames * kTimeSToMs) / interval_ms;

  for (size_t i = 1; i < sample_count; ++i) {
    const int64_t cost_mills =
        (frame_time_samples_[i] - frame_time_samples_[i - 1]) / kTimeMsToNs;
    const int64_t cost_mills100 = cost_mills * kOffsetToMillis;
    int drop_count =
        computeDropCount(cost_mills100, kDesiredFrameIntervalInMillis100);
    int64_t drop_duration = cost_mills - kDesiredFrameIntervalInMillis;
    if (drop_count >= 1) {
      fps_metrics.drop1_count++;
      fps_metrics.drop1_duration_ms += drop_duration;
    } else {
      continue;
    }

    if (drop_count >= 3) {
      fps_metrics.drop3_count++;
      fps_metrics.drop3_duration_ms += drop_duration;
    } else {
      continue;
    }

    if (drop_count >= 7) {
      fps_metrics.drop7_count++;
      fps_metrics.drop7_duration_ms += drop_duration;
    } else {
      continue;
    }

    if (drop_count >= 25) {
      fps_metrics.drop25_count++;
      fps_metrics.drop25_duration_ms += drop_duration;
    }
  }
}

// Calculates the number of vsync intervals missed between two frames.
// This is equivalent to ceil(cost_millis / frame_interval_millis) - 1,
// but uses integer arithmetic for efficiency.
int FpsTracer::computeDropCount(int64_t cost_mills100,
                                int frame_interval_ms100) {
  return static_cast<int>(
      (cost_mills100 + (frame_interval_ms100 - 1)) / frame_interval_ms100 - 1);
}

void FpsTracer::AddFrameTimeSample(int64_t frame_time_nanos) {
  frame_time_samples_.push_back(frame_time_nanos);
}

}  // namespace clay
