// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//  This file is copied from AWEPerfSDK, Created by Fengwei Liu.
//  Some unused logic remove, structuct changes are made, so it meets Lynx's code format.

#import "LynxFPSRecord.h"

double LynxFPSCap(double fps) {
  static double g_maxFPS = 0.0;
  if (!g_maxFPS) {
    if (@available(iOS 10.3, *)) {
      g_maxFPS = (double)UIScreen.mainScreen.maximumFramesPerSecond;
    } else {
      g_maxFPS = 60.0;
    }
  }
  return fps >= g_maxFPS ? g_maxFPS : fps;
}

static double LynxFPSRawMetricsGetFPS(const LynxFPSRawMetrics *metrics) {
  if (metrics->duration < DBL_MIN) {
    return -1.0;
  } else {
    return LynxFPSCap((double)(metrics->frames) / metrics->duration);
  }
}

static LynxFPSDerivedMetrics LynxFPSDerivedMetricsFromRaw(LynxFPSRawMetrics metric) {
  LynxFPSDerivedMetrics ret = (LynxFPSDerivedMetrics){0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double duration = metric.duration;
  if (duration > 0.01) {
    ret.fps = LynxFPSCap((double)metric.frames / metric.duration);
    ret.drop1PerSecond = metric.drop1Count / metric.duration;
    ret.drop3PerSecond = metric.drop3Count / metric.duration;
    ret.drop7PerSecond = metric.drop7Count / metric.duration;
    ret.drop25PerSecond = metric.drop25Count / metric.duration;
    ret.hitchRatio = metric.hitchDuration / metric.duration;
    ret.drop1Ratio = metric.drop1Duration / metric.duration;
    ret.drop3Ratio = metric.drop3Duration / metric.duration;
    ret.drop7Ratio = metric.drop7Duration / metric.duration;
    ret.drop25Ratio = metric.drop25Duration / metric.duration;
  }
  return ret;
}

@implementation LynxFPSRecord

@synthesize state = _state, key = _key;

- (instancetype)initWithKey:(id<NSCopying>)key {
  NSParameterAssert(key != nil);
  self = [super init];
  if (self) {
    _key = [(id)key copy];
    _maximumFramesPerSecond = -1;
  }
  return self;
}

- (NSString *)name {
  NSObject *keyObject = (NSObject *)_key;
  if ([keyObject isKindOfClass:NSString.class]) {
    return (NSString *)keyObject;
  }
  return nil;
}

- (NSUInteger)frames {
  return _totalMetrics.frames;
}

- (NSTimeInterval)duration {
  return _totalMetrics.duration;
}

- (double)framesPerSecond {
  return LynxFPSRawMetricsGetFPS(&_totalMetrics);
}

- (LynxFPSRawMetrics)metrics {
  return _totalMetrics;
}

- (LynxFPSDerivedMetrics)derivedMetrics {
  return LynxFPSDerivedMetricsFromRaw(_totalMetrics);
}

- (void)reset {
  _totalMetrics = (LynxFPSRawMetrics){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  _maximumFramesPerSecond = -1;
}

- (void)setTimeout:(NSTimeInterval)timeout completion:(void (^)(LynxFPSRecord *))completion {
  if (timeout > DBL_MIN && !completion) {
    NSAssert(NO, @"Completion handler is required.");
    timeout = 0.0;
  }

  timeoutInterval = timeout;
  timeoutCompletion = [completion copy];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  LynxFPSRecord *record = [[LynxFPSRecord alloc] init];
  record->_totalMetrics = _totalMetrics;
  return record;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@: %p; key = %@, state = %d, fps = %.2f>",
                                    NSStringFromClass(self.class), self, self.key, self.state,
                                    self.framesPerSecond];
}

@end
