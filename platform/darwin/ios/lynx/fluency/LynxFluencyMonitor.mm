// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFluencyMonitor.h"
#import "LynxEventReporter.h"
#import "LynxFPSMonitor.h"
#import "LynxScrollListener.h"
#import "LynxService.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxView+Internal.h"
#include "core/services/fluency/fluency_tracer.h"

typedef NS_ENUM(NSInteger, ForceStatus) {
  ForceStatusForcedOn,
  ForceStatusForcedOff,
  ForceStatusNonForced
};

@implementation LynxFluencyMonitor {
  ForceStatus _forceStatus;
  CGFloat _fluencyPageconfigProbability;
}

- (instancetype)init {
  if (self = [super init]) {
    _fluencyPageconfigProbability = -1;
    _forceStatus = ForceStatusNonForced;
  }
  return self;
}

- (BOOL)shouldSendAllScrollEvent {
  if (_forceStatus == ForceStatusNonForced) {
    return lynx::tasm::FluencyTracer::IsEnable();
  } else
    return _forceStatus == ForceStatusForcedOn;
}

- (void)startWithScrollInfo:(LynxScrollInfo *)info {
  if (info.lynxView == nil) {
    // This method should be called synchronic when a UIScrollView in LynxView is scrolling. Info's
    // lynxView should not be nil.
    return;
  }
  LynxFPSRecord *record = [[LynxFPSMonitor sharedInstance] beginWithKey:info];
  // If one scroll event does not stop after 10 seconds, we stop it manually.
  NSTimeInterval timeout = 10;
  [record setTimeout:timeout
          completion:^(LynxFPSRecord *_Nonnull record) {
            [self stopWithScrollInfo:(LynxScrollInfo *)(record.key)];
          }];
}

- (void)stopWithScrollInfo:(LynxScrollInfo *)info {
  LynxFPSRecord *record = [[LynxFPSMonitor sharedInstance] endWithKey:info];
  if (record.duration < 0.2) {
    // just ignore scroll event with duration less than 200ms.
    return;
  }
  [self reportWithRecord:record info:info];
}

- (NSDictionary *)jsonFromRecord:(LynxFPSRecord *)record info:(LynxScrollInfo *)info {
  if (record == nil) {
    return nil;
  }
  LynxFPSRawMetrics metrics = record.metrics;
  LynxFPSDerivedMetrics derivedMetrics = record.derivedMetrics;
  NSMutableDictionary *result = [[NSMutableDictionary alloc] initWithDictionary:@{
    @"lynxsdk_fluency_scene" : @"scroll",
    @"lynxsdk_fluency_tag" : info.scrollMonitorTagName ?: @"default_tag",
    @"lynxsdk_fluency_dur" : @(record.duration),
    @"lynxsdk_fluency_fps" : @(record.framesPerSecond),
    @"lynxsdk_fluency_frames_number" : @(record.frames),
    @"lynxsdk_fluency_maximum_frames" : @(record.maximumFramesPerSecond),
    @"lynxsdk_fluency_drop1_count" : @(metrics.drop1Count),
    @"lynxsdk_fluency_drop1_count_per_second" : @(derivedMetrics.drop1PerSecond),
    @"lynxsdk_fluency_drop3_count" : @(metrics.drop3Count),
    @"lynxsdk_fluency_drop3_count_per_second" : @(derivedMetrics.drop3PerSecond),
    @"lynxsdk_fluency_drop7_count" : @(metrics.drop7Count),
    @"lynxsdk_fluency_drop7_count_per_second" : @(derivedMetrics.drop7PerSecond),
    @"lynxsdk_fluency_drop25_count" : @(metrics.drop25Count),
    @"lynxsdk_fluency_drop25_count_per_second" : @(derivedMetrics.drop25PerSecond),
    @"lynxsdk_fluency_hitch_dur" : @(metrics.hitchDuration),
    @"lynxsdk_fluency_hitch_ratio" : @(derivedMetrics.hitchRatio),
    // unit is ms/s * 1000, which represents how many milliseconds dropx occurs per second.
    @"lynxsdk_fluency_drop1_ratio" : @(derivedMetrics.drop1Ratio * 1000),
    @"lynxsdk_fluency_drop3_ratio" : @(derivedMetrics.drop3Ratio * 1000),
    @"lynxsdk_fluency_drop7_ratio" : @(derivedMetrics.drop7Ratio * 1000),
    @"lynxsdk_fluency_drop25_ratio" : @(derivedMetrics.drop25Ratio * 1000),
    @"lynxsdk_fluency_pageconfig_probability" : @(_fluencyPageconfigProbability),
  }];

  return result;
}

- (void)reportWithRecord:(LynxFPSRecord *)record info:(LynxScrollInfo *)info {
  if (info == nil || info.lynxView == nil || record == nil) {
    // lynxView has been released, so we can directly drop this record.
    return;
  }

  int32_t instanceId = [info.lynxView instanceId];
  NSDictionary *json = [self jsonFromRecord:record info:info];
  [LynxEventReporter onEvent:@"lynxsdk_fluency_event" instanceId:instanceId props:json];
}

- (void)setFluencyPageconfigProbability:(CGFloat)probability {
  _fluencyPageconfigProbability = probability;
  if (probability >= 0) {
    int randomValue = arc4random_uniform(100);
    if (randomValue <= probability * 100) {
      _forceStatus = ForceStatusForcedOn;
    } else {
      _forceStatus = ForceStatusForcedOff;
    }
  }
}

@end
