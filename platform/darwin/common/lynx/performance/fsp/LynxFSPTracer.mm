// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFSPTracer.h>
#include "core/public/pub_value.h"
#include "core/services/performance/performance_controller.h"
#include "core/services/performance/performance_event_sender.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "lynx/base/include/geometry/rect.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <memory>

namespace lynx {
namespace tasm {
namespace performance {

using IntRect = lynx::base::geometry::IntRect;
using IntSize = lynx::base::geometry::IntSize;

// Core FSP configuration, used to define FSP judgment criteria
struct LynxFSPConfig {
  double min_completion_rate_x_ = 0.0;             // Minimum content fill rate in X-axis direction
  double min_completion_rate_y_ = 0.0;             // Minimum content fill rate in Y-axis direction
  double min_completion_rate_content_area_ = 0.5;  // Minimum effective total area fill rate
  int acceptable_pixel_diff_per_snapshot_ = 10;    // Acceptable pixel area change between snapshots
  double acceptable_pixel_diff_per_ms_ = 0.01;  // Acceptable pixel area change rate per millisecond
  double acceptable_area_diff_per_ms_ = 0.01;   // Acceptable pixel area change rate per millisecond
  double min_diff_interval_ms_ = 300.0;         // Minimum time interval to determine stability
  int64_t hard_timeout_ms_ = 10 * 1000;         // 10 seconds
};

// FSP snapshot, used to store page content projection information at a certain moment
struct LynxFSPSnapshot {
  static const size_t kXProjectionsLen = 256;
  static const size_t kYProjectionsLen = 512;

 public:
  explicit LynxFSPSnapshot(IntSize container_size) : container_size_(std::move(container_size)) {}
  LynxFSPSnapshot() = default;
  ~LynxFSPSnapshot() = default;

  std::bitset<kXProjectionsLen> x_projections_;  // X-axis projection bitmap
  std::bitset<kYProjectionsLen> y_projections_;  // Y-axis projection bitmap

  IntSize container_size_ = {0, 0};           // Container size
  int64_t total_presented_content_area_ = 0;  // Presented effective total area
  int64_t total_content_area_ = 0;            // Effective total area
  int64_t last_change_timestamp_ms_ = 0;      // Timestamp of last content change

  void FillContentToSnapshot(bool is_presented, IntRect rect,
                             int64_t first_presented_timestamp_micros = -1);
};

void LynxFSPSnapshot::FillContentToSnapshot(bool is_presented, IntRect rect,
                                            int64_t first_presented_timestamp_micros) {
  // If content is invalid, return directly
  if (rect.IsEmpty()) {
    return;
  }

  int w = container_size_.Width();
  int h = container_size_.Height();

  // Optimization for edge case: Ignore if content is outside container bounds
  // Check if the rectangle is completely outside the container, if so, do not participate in
  // projection calculation
  if (rect.X() > w || rect.Y() > h || rect.MaxX() <= 0 || rect.MaxY() <= 0) {
    return;
  }

  // X/Y axis projection algorithm: "Flatten" the 2D rect area onto 1D X and Y axes respectively,
  // and mark the covered intervals Use std::max/min to limit coordinates within [0, w] and [0, h]
  // ranges to correctly handle partially visible rectangles
  size_t min_x = static_cast<size_t>(std::max(0, rect.X()));
  min_x = min_x * LynxFSPSnapshot::kXProjectionsLen / w;
  min_x = std::min(min_x, LynxFSPSnapshot::kXProjectionsLen - 1);

  size_t max_x = static_cast<size_t>(std::min(w, rect.MaxX()));
  max_x = max_x * LynxFSPSnapshot::kXProjectionsLen / w;
  max_x = std::min(max_x, LynxFSPSnapshot::kXProjectionsLen - 1);

  size_t min_y = static_cast<size_t>(std::max(0, rect.Y()));
  min_y = min_y * LynxFSPSnapshot::kYProjectionsLen / h;
  min_y = std::min(min_y, LynxFSPSnapshot::kYProjectionsLen - 1);

  size_t max_y = static_cast<size_t>(std::min(h, rect.MaxY()));
  max_y = max_y * LynxFSPSnapshot::kYProjectionsLen / h;
  max_y = std::min(max_y, LynxFSPSnapshot::kYProjectionsLen - 1);

  // Calculate effective area
  int content_area = static_cast<int>((max_x - min_x + 1) * (max_y - min_y + 1));
  total_content_area_ += content_area;
  if (!is_presented) {
    return;
  }

  // Accumulate presented effective total area
  total_presented_content_area_ += content_area;

  // Update X,Y projections
  for (size_t i = min_x; i <= max_x; i++) {
    x_projections_[i] = true;
  }
  for (size_t i = min_y; i <= max_y; i++) {
    y_projections_[i] = true;
  }

  // Update snapshot's last change timestamp
  if (first_presented_timestamp_micros >= last_change_timestamp_ms_) {
    last_change_timestamp_ms_ = first_presented_timestamp_micros;
  }
}

// C++ core implementation of FSP tracer
class LynxFSPTracer : public std::enable_shared_from_this<LynxFSPTracer> {
 public:
  explicit LynxFSPTracer(const LynxFSPConfig& config) : config_(config) {}
  ~LynxFSPTracer() = default;

  // Start monitoring
  void Start();
  // User interaction occurred, cancel monitoring and send FSP event
  void CancelledByUserInteraction();
  // Capture current snapshot and compare with previous snapshot
  void OnCaptureSnapshot(LynxFSPSnapshot snapshot);

 private:
  bool IsSnapshotValuable(const LynxFSPSnapshot& snapshot, const LynxFSPConfig& config) const;
  // Compare two snapshots to determine if stable state is reached
  bool DiffSnapshot(const LynxFSPSnapshot& current, const LynxFSPSnapshot& previous,
                    const LynxFSPConfig& config_);

  void OnFSP(const LynxFSPSnapshot& fsp_snapshot);
  void OnFSPHardTimeOut(const LynxFSPSnapshot& current_snapshot);
  void OnFSPCancelledByUserInteraction(const LynxFSPSnapshot& current_snapshot);
  LynxFSPConfig config_;
  LynxFSPSnapshot previous_snapshot_;
  std::shared_ptr<lynx::pub::PubValueFactoryDefault> pub_value_factory_ =
      std::make_shared<pub::PubValueFactoryDefault>();
  bool is_running_ = false;
};

void LynxFSPTracer::Start() {
  if (is_running_) {
    return;
  }
  is_running_ = true;
  // Start hard timeout
  std::weak_ptr<LynxFSPTracer> weak_this = shared_from_this();
  PerformanceController::GetTaskRunner()->PostDelayedTask(
      [weak_this]() {
        auto this_ptr = weak_this.lock();
        if (this_ptr && this_ptr->is_running_) {
          this_ptr->is_running_ = false;
          this_ptr->OnFSPHardTimeOut(this_ptr->previous_snapshot_);
        }
      },
      fml::TimeDelta::FromMilliseconds(config_.hard_timeout_ms_));
}

void LynxFSPTracer::CancelledByUserInteraction() {
  if (!is_running_) {
    return;
  }
  is_running_ = false;
  // Send cancelled event
  OnFSPCancelledByUserInteraction(previous_snapshot_);
}

void LynxFSPTracer::OnCaptureSnapshot(LynxFSPSnapshot current_snapshot) {
  if (!IsSnapshotValuable(current_snapshot, config_)) {
    // If not satisfied, it proves instability, so the previous snapshot is also invalid
    previous_snapshot_ = {};
    return;
  }

  auto& previous_snapshot = previous_snapshot_;
  // 3. If previous snapshot exists (timestamp > 0), trigger comparison
  if (previous_snapshot.last_change_timestamp_ms_ > 0 &&
      DiffSnapshot(current_snapshot, previous_snapshot, config_)) {
    // Stable
    // ****************
    //   3. Whether to generate fsp
    // ****************
    // Check if the interval is less than the min interval.
    int64_t diff_t =
        current_snapshot.last_change_timestamp_ms_ - previous_snapshot.last_change_timestamp_ms_;
    if (diff_t >= config_.min_diff_interval_ms_) {
      // Generate FSP, previous_snapshot is the snapshot at FSP time
      // Construct FSPEntry based on previous_snapshot and send it out
      OnFSP(previous_snapshot);
    }
    return;
  }
  previous_snapshot_ = std::move(current_snapshot);
}

bool LynxFSPTracer::IsSnapshotValuable(const LynxFSPSnapshot& snapshot,
                                       const LynxFSPConfig& config) const {
  int projection_w = static_cast<int>(snapshot.x_projections_.count());
  int projection_h = static_cast<int>(snapshot.y_projections_.count());

  // 0. Check X and Y axis projection fill rates
  double completion_rate_x = static_cast<double>(projection_w) / LynxFSPSnapshot::kXProjectionsLen;
  if (completion_rate_x < config.min_completion_rate_x_) {
    return false;
  }
  double completion_rate_y = static_cast<double>(projection_h) / LynxFSPSnapshot::kYProjectionsLen;
  if (completion_rate_y < config.min_completion_rate_y_) {
    return false;
  }

  // 1. Check effective area fill rate
  double completion_rate_content_area =
      static_cast<double>(snapshot.total_presented_content_area_) / snapshot.total_content_area_;
  if (completion_rate_content_area < config.min_completion_rate_content_area_) {
    return false;
  }
  return true;
}

bool LynxFSPTracer::DiffSnapshot(const LynxFSPSnapshot& current, const LynxFSPSnapshot& previous,
                                 const LynxFSPConfig& config) {
  if (previous.last_change_timestamp_ms_ <= 0 || current.last_change_timestamp_ms_ <= 0) {
    // Initial snapshot always considered stable.
    return false;
  }
  // ****************
  //   2. Stability check
  // ****************
  auto container_w = current.container_size_.Width();
  auto container_h = current.container_size_.Height();
  int projection_w = static_cast<int>(current.x_projections_.count());
  int projection_h = static_cast<int>(current.y_projections_.count());
  // diff interval in milliseconds
  auto diff_t = current.last_change_timestamp_ms_ - previous.last_change_timestamp_ms_;

  // 2.1 (fast) Projection change rate
  double diff_projection_w = projection_w - static_cast<double>(previous.x_projections_.count());
  auto change_rate_w = diff_projection_w / diff_t;
  if (change_rate_w >= config.acceptable_pixel_diff_per_ms_) {
    return false;
  }
  double diff_projection_h = projection_h - static_cast<double>(previous.y_projections_.count());
  auto change_rate_h = diff_projection_h / diff_t;
  if (change_rate_h >= config.acceptable_pixel_diff_per_ms_) {
    return false;
  }

  // 2.1 (fast) Area projection change rate
  auto area_change_rate_w = static_cast<double>(current.total_presented_content_area_ -
                                                previous.total_presented_content_area_) /
                            diff_t;
  if ((area_change_rate_w >= config.acceptable_area_diff_per_ms_)) {
    return false;
  }

  // 2.2 (slow) XOR projection change rate
  auto xor_change_rate_x =
      static_cast<double>((current.x_projections_ ^ previous.x_projections_).count()) / diff_t;
  if ((xor_change_rate_x >= config.acceptable_pixel_diff_per_ms_)) {
    return false;
  }
  auto xor_change_rate_y =
      static_cast<double>((current.y_projections_ ^ previous.y_projections_).count()) / diff_t;
  return (xor_change_rate_y >= config.acceptable_pixel_diff_per_ms_);
}

void LynxFSPTracer::OnFSP(const LynxFSPSnapshot& fsp_snapshot) {
  auto entry_map = pub_value_factory_->CreateMap();
  entry_map->PushStringToMap(kPerformanceEventType, timing::kEntryTypeMetric);
  entry_map->PushStringToMap(kPerformanceEventName, "fsp");
  entry_map->PushStringToMap("state", "fulfill");
}

void LynxFSPTracer::OnFSPHardTimeOut(const LynxFSPSnapshot& current_snapshot) {
  auto entry_map = pub_value_factory_->CreateMap();
  entry_map->PushStringToMap(kPerformanceEventType, timing::kEntryTypeMetric);
  entry_map->PushStringToMap(kPerformanceEventName, "fsp");
  entry_map->PushStringToMap("state", "hardTimeout");
}

void LynxFSPTracer::OnFSPCancelledByUserInteraction(const LynxFSPSnapshot& current_snapshot) {
  auto entry_map = pub_value_factory_->CreateMap();
  entry_map->PushStringToMap(kPerformanceEventType, timing::kEntryTypeMetric);
  entry_map->PushStringToMap(kPerformanceEventName, "fsp");
  entry_map->PushStringToMap("state", "cancelledByUserInteraction");
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

@implementation LynxFSPConfigOC
@end

@implementation LynxMeaningfulContentInfo
@end

@interface LynxFSPTracer () {
  std::shared_ptr<lynx::tasm::performance::LynxFSPTracer> _impl;
}
@end

@implementation LynxFSPTracer

// Bridge method: Convert OC configuration to C++ configuration and initialize core implementation
- (instancetype)initWithConfig:(LynxFSPConfigOC*)config {
  if (self = [super init]) {
    lynx::tasm::performance::LynxFSPConfig cppConfig;
    if (config) {
      cppConfig.min_completion_rate_x_ = config.minCompletionRateX;
      cppConfig.min_completion_rate_y_ = config.minCompletionRateY;
      cppConfig.acceptable_pixel_diff_per_snapshot_ = config.acceptablePixelDiffPerMs;
      cppConfig.acceptable_pixel_diff_per_ms_ = config.acceptablePixelDiffPerMs;
      cppConfig.min_diff_interval_ms_ = config.minDiffIntervalMs;
    }
    _impl = std::make_shared<lynx::tasm::performance::LynxFSPTracer>(cppConfig);
  }
  return self;
}

- (void)Start {
  if (_impl) {
    _impl->Start();
  }
}

- (void)fill:(NSArray<LynxMeaningfulContentInfo*>*)contentsArray
    containerSize:(CGSize)containerSize {
  if ([contentsArray count] <= 0 || CGSizeEqualToSize(containerSize, CGSizeZero)) {
    return;
  }
  lynx::tasm::performance::PerformanceController::GetTaskRunner()->PostTask(
      [contentsArray, w = static_cast<int>(containerSize.width),
       h = static_cast<int>(containerSize.height)]() {
        lynx::tasm::performance::LynxFSPSnapshot snapshot(lynx::base::geometry::IntSize(w, h));
        for (LynxMeaningfulContentInfo* info in contentsArray) {
          snapshot.FillContentToSnapshot(
              (info.status == kLynxUIMeaningfulContentStatusPresented),
              lynx::base::geometry::IntRect(
                  {static_cast<int>(info.rect.origin.x), static_cast<int>(info.rect.origin.y)},
                  {static_cast<int>(info.rect.size.width),
                   static_cast<int>(info.rect.size.height)}),
              info.firstPresentedTimestampMicros);
        }
        _impl->OnCaptureSnapshot(std::move(snapshot));
      });
}
@end
