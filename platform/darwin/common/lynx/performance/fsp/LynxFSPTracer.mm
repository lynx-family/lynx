// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Foundation/Foundation.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxFSPTracer+Native.h>
#import <Lynx/LynxMetricFcpEntry.h>
#import <Lynx/LynxMetricFspEntry.h>
#import <Lynx/LynxPerformanceController+Native.h>
#include <cstdint>
#include "core/public/performance_controller_platform_impl.h"
#include "core/services/performance/fsp_tracing/fsp_tracer.h"
#include "core/services/performance/performance_controller.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_utils.h"
#include "core/services/trace/service_trace_event_def.h"

using namespace lynx::tasm::timing;
using namespace lynx::tasm::performance;

@interface LynxFSPTracer () {
  std::shared_ptr<FSPTracer> _impl;
}

@property(nonatomic, assign) BOOL isRunning;
@property(nonatomic, assign) int snapshotIntervalMs;

@end

@implementation LynxFSPTracer

#pragma mark - Public

- (BOOL)enable {
  return [[LynxEnv sharedInstance] enableFSP];
}

// Call on main thread
- (void)startWithCaptureHandler:(LynxMeaningfulContentSnapshot* (^)(void))captureHandler {
  if (self.isRunning || !self.enable) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  [self ActAsync:^(const std::unique_ptr<PerformanceController>&) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    [strongSelf internalStart];
  }];
  // start timer.
  [self setupSnapshotInterval];
  [self scheduleNextCapture:captureHandler];
}

- (void)stop {
  if (!self.isRunning || !self.enable || !_impl) {
    return;
  }
  uint64_t timestampUs = lynx::base::CurrentSystemTimeMicroseconds();
  __weak typeof(self) weakSelf = self;
  [self ActAsync:^(const std::unique_ptr<PerformanceController>&) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf || !strongSelf.isRunning) {
      return;
    }
    strongSelf.isRunning = NO;
    if (strongSelf->_impl) {
      strongSelf->_impl->Stop(timestampUs);
    }
  }];
}

- (void)stopByUserInteraction {
  if (!self.enable || !_impl || !self.isRunning) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  uint64_t timestampUs = lynx::base::CurrentSystemTimeMicroseconds();
  [self ActAsync:^(const std::unique_ptr<PerformanceController>&) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf || !strongSelf.isRunning || !strongSelf->_impl) {
      return;
    }
    strongSelf->_impl->CancelledByUserInteraction(timestampUs);
  }];
}

- (void)setNativeActor:(const std::shared_ptr<PerformanceControllerActor>&)nativeActor {
  _nativeWeakActorPtr = nativeActor;
  if (_impl && nativeActor) {
    _impl->SetInstanceId(nativeActor->GetInstanceId());
  }
}

#pragma mark - Private
// Run on report thread.
- (void)internalStart {
  if (self.isRunning) {
    return;
  }

  // 1. create tracer impl.
  if (!_impl) {
    FSPConfig config;
    config.enable_ = YES;
    [self setupFSPConfig:config];
    _impl = std::make_shared<FSPTracer>(std::move(config));
    auto actor = _nativeWeakActorPtr.lock();
    if (actor) {
      _impl->SetInstanceId(actor->GetInstanceId());
    }
  }
  __weak typeof(self) weakSelf = self;
  _impl->Start([weakSelf](FSPResult result) {
    // Run on report thread.
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    [strongSelf stop];
    [strongSelf sendFSPEntry:std::move(result)];
  });
  self.isRunning = YES;
}

- (void)setupFSPConfig:(FSPConfig&)config {
  NSDictionary<NSString*, id>* fspDict = [[LynxEnv sharedInstance] fspConfig];
  if (!fspDict) {
    return;
  }
  // Helper function to simplify FSP config parsing
  auto parseFSPConfigValue = [](NSDictionary* dict, NSString* key, int& configField) {
    id value = [dict objectForKey:key];
    if ([value isKindOfClass:[NSNumber class]]) {
      configField = [value intValue];
    }
  };
  parseFSPConfigValue(fspDict, @"min_content_fill_percentage_x",
                      config.min_content_fill_percentage_x_);
  parseFSPConfigValue(fspDict, @"min_content_fill_percentage_y",
                      config.min_content_fill_percentage_y_);
  parseFSPConfigValue(fspDict, @"min_content_fill_percentage_total_area",
                      config.min_content_fill_percentage_total_area_);
  parseFSPConfigValue(fspDict, @"min_container_fill_percentage_container_area",
                      config.min_container_fill_percentage_container_area_);
  parseFSPConfigValue(fspDict, @"acceptable_pixel_diff_per_sec",
                      config.acceptable_pixel_diff_per_sec_);
  parseFSPConfigValue(fspDict, @"acceptable_area_diff_per_sec",
                      config.acceptable_area_diff_per_sec_);
  parseFSPConfigValue(fspDict, @"min_diff_interval_ms", config.min_diff_interval_ms_);
  parseFSPConfigValue(fspDict, @"hard_timeout_ms", config.hard_timeout_ms_);
}

- (void)ActAsync:(void (^)(const std::unique_ptr<PerformanceController>&))callback {
  auto actorPtr = _nativeWeakActorPtr.lock();
  if (!actorPtr) {
    return;
  }
  actorPtr->ActAsync([callback](auto& performance) { callback(performance); });
}

- (void)setupSnapshotInterval {
  if (!self.enable || _snapshotIntervalMs > 0) {
    return;
  }
  // default is 17(16.7)ms;
  _snapshotIntervalMs = 17;
  NSDictionary<NSString*, id>* dict = [[LynxEnv sharedInstance] fspConfig];
  if (!dict) {
    return;
  }
  id value = [dict objectForKey:@"fsp_snapshot_interval_ms"];
  if (value && [value isKindOfClass:[NSNumber class]]) {
    _snapshotIntervalMs = [value intValue];
  }
}

#pragma mark - Snapshot
- (void)scheduleNextCapture:(LynxMeaningfulContentSnapshot* (^)(void))captureHandler {
  if (self.snapshotIntervalMs <= 0) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, (int64_t)(self.snapshotIntervalMs * NSEC_PER_MSEC)),
      dispatch_get_main_queue(), ^{
        __strong __typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf || !strongSelf.isRunning || !captureHandler) {
          return;
        }
        [strongSelf handleSnapshotCapture:captureHandler()];
        [strongSelf scheduleNextCapture:captureHandler];
      });
}

// Run on any thread.
- (void)handleSnapshotCapture:(LynxMeaningfulContentSnapshot*)rawSnapshot {
  if (!rawSnapshot || [rawSnapshot.contentsArray count] <= 0 ||
      CGSizeEqualToSize(rawSnapshot.containerSize, CGSizeZero)) {
    return;
  }
  uint64_t currentTimestampUs = lynx::base::CurrentSystemTimeMicroseconds();
  __weak typeof(self) weakSelf = self;
  [self ActAsync:^(const std::unique_ptr<PerformanceController>&) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf || !strongSelf->_impl) {
      return;
    }

    FSPSnapshot snapshot(lynx::base::geometry::IntSize(rawSnapshot.containerSize.width,
                                                       rawSnapshot.containerSize.height),
                         currentTimestampUs);
#if ENABLE_TRACE_PERFETTO
    snapshot.trace_timestamp_us_ = rawSnapshot.traceTimestampUs;
    snapshot.trace_thread_id_ = rawSnapshot.traceThreadId;
#endif

    for (LynxMeaningfulContentInfo* info in rawSnapshot.contentsArray) {
      bool is_presented = info.status == kLynxUIMeaningfulContentStatusPresented;
      lynx::base::geometry::IntRect rect(
          {static_cast<int>(info.rect.origin.x), static_cast<int>(info.rect.origin.y)},
          {static_cast<int>(info.rect.size.width), static_cast<int>(info.rect.size.height)});
      snapshot.FillContentToSnapshot(is_presented, std::move(rect),
                                     info.firstPresentedTimestampMicros);
    }
    strongSelf->_impl->OnCaptureSnapshot(std::move(snapshot));
  }];
}

#pragma mark - FSPEntry
// Run on report thread.
- (void)sendFSPEntry:(FSPResult)result {
  auto actorPtr = _nativeWeakActorPtr.lock();
  if (!actorPtr) {
    return;
  }
  actorPtr->Act([result = std::move(result)](auto& perfController) mutable {
    perfController->GetTimingHandler().SetFSPInfo(kFSPStatus, result.status_);
    perfController->GetTimingHandler().SetFSPInfo(
        kContentFillPercentageX, std::to_string(result.content_fill_percentage_x_));
    perfController->GetTimingHandler().SetFSPInfo(
        kContentFillPercentageY, std::to_string(result.content_fill_percentage_y_));
    perfController->GetTimingHandler().SetFSPInfo(
        kContentFillPercentageTotalArea,
        std::to_string(result.content_fill_percentage_total_area_));
    perfController->GetTimingHandler().SetFSPInfo(
        kContainerFillPercentageContainerArea,
        std::to_string(result.container_fill_percentage_container_area_));
    TimestampKey timingKey(kFSPEnd);
    lynx::tasm::PipelineID pipeline;
    perfController->GetTimingHandler().SetTiming(
        timingKey, static_cast<TimestampUs>(result.fsp_timestamp_us_), pipeline);
  });
}

@end
