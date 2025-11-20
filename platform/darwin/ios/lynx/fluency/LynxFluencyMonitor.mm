// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFluencyMonitor.h"
#import <Lynx/LynxBooleanOption.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxScrollListener.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxView+Internal.h>
#import "LynxFPSMonitor.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEventDef.h"
#include "core/services/fluency/fluency_tracer.h"

typedef NS_ENUM(NSInteger, ForceStatus) {
  ForceStatusForcedOn,
  ForceStatusForcedOff,
  ForceStatusNonForced
};

@implementation LynxFluencyConfig

- (instancetype)initWithKey:(id<NSCopying>)key
                    tagName:(NSString *)tagName
       scrollMonitorTagName:(NSString *)scrollMonitorTagName
                 instanceId:(int32_t)instanceId {
  self = [super init];
  if (self) {
    _key = key;
    _tagName = tagName;
    _scrollMonitorTagName = scrollMonitorTagName;
    _instanceId = instanceId;
  }
  return self;
}

@end

@implementation LynxFluencyMonitor {
  ForceStatus _forceStatus;
  BOOL _probabilityDetermined;
  LynxBooleanOption _enabledBySampling;
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

- (void)startWithFluencyConfig:(LynxFluencyConfig *)config {
  LynxFPSRecord *record = [[LynxFPSMonitor sharedInstance] beginWithKey:config.key];
  // If one scroll event does not stop after 10 seconds, we stop it manually.
  NSTimeInterval timeout = 10;
  [record setTimeout:timeout
          completion:^(LynxFPSRecord *_Nonnull record) {
            [self stopWithFluencyConfig:(LynxFluencyConfig *)(config)];
          }];
  // FIXME: Add the 'scene' parameter to the trace later to differentiate whether it is an animation
  // scene, a scroll scene, etc.
  LYNX_TRACE_INSTANT_WITH_DEBUG_INFO(
      LYNX_TRACE_CATEGORY_WRAPPER, FLUENCY_MONITOR_START_FLUENCY_TRACE, @{@"tag" : config.tagName});
}

- (void)stopWithFluencyConfig:(LynxFluencyConfig *)config {
  LynxFPSRecord *record = [[LynxFPSMonitor sharedInstance] endWithKey:config.key];
  if (record.duration < 0.2) {
    // just ignore scroll event with duration less than 200ms.
    return;
  }
  [self reportWithRecord:record config:config];
  LYNX_TRACE_INSTANT(LYNX_TRACE_CATEGORY_WRAPPER, FLUENCY_MONITOR_STOP_FLUENCY_TRACE);
}

- (NSDictionary *)jsonFromRecord:(LynxFPSRecord *)record config:(LynxFluencyConfig *)config {
  if (record == nil) {
    return nil;
  }
  LynxFPSRawMetrics metrics = record.metrics;
  LynxFPSDerivedMetrics derivedMetrics = record.derivedMetrics;
  NSMutableDictionary *result = [[NSMutableDictionary alloc] initWithDictionary:@{
    @"lynxsdk_fluency_scene" : @"scroll",
    @"lynxsdk_fluency_tag" : config.scrollMonitorTagName ?: @"default_tag",
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
    @"lynxsdk_fluency_enabled_by_sampling" : @(_enabledBySampling),
  }];

  return result;
}

- (void)reportWithRecord:(LynxFPSRecord *)record config:(LynxFluencyConfig *)config {
  if (config == nil || config.instanceId == -1 || record == nil) {
    // lynxView has been released, so we can directly drop this record.
    return;
  }

  int32_t instanceId = config.instanceId;
  NSDictionary *json = [self jsonFromRecord:record config:config];
  [LynxEventReporter onEvent:@"lynxsdk_fluency_event" instanceId:instanceId props:json];
}

- (void)setEnabledBySampling:(LynxBooleanOption)enabledBySampling {
  _enabledBySampling = enabledBySampling;
  [self updateStatus];
}

- (void)setPageConfigProbability:(CGFloat)probability {
  _fluencyPageconfigProbability = probability;
  _probabilityDetermined = NO;  // reset the flag when page config probability is set again.
  [self updateStatus];
}

- (void)updateStatus {
  double probability = _fluencyPageconfigProbability;
  if (!_probabilityDetermined && probability >= 0) {
    _probabilityDetermined = YES;
    int randomValue = arc4random_uniform(100);
    if (randomValue <= probability * 100) {
      _forceStatus = ForceStatusForcedOn;
    } else {
      _forceStatus = ForceStatusForcedOff;
    }
  } else if (_enabledBySampling != LynxBooleanOptionUnset) {
    _forceStatus =
        _enabledBySampling == LynxBooleanOptionTrue ? ForceStatusForcedOn : ForceStatusForcedOff;
  }
}

@end
