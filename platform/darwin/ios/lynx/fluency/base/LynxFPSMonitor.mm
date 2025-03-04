// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//  This file is copied from AWEPerfSDK, Created by Fengwei Liu.
//  Some unused logic remove, structuct changes are made, so it meets Lynx's code format.
//  Original License:
//
//  AWEPerfFPSMonitor.m
//  AWEPerfSDK
//
//  Created by Fengwei Liu on 8/12/19.
//

#import "LynxFPSMonitor.h"

#include <vector>

#ifndef likely
#define likely(x) (__builtin_expect((int)(x), 1))
#endif

#ifndef unlikely
#define unlikely(x) (__builtin_expect((int)(x), 0))
#endif

#define ASSERT_MAIN_THREAD_ONLY_API \
  NSAssert([NSThread isMainThread], @"Main thread only API called background thread.")
#define ASSERT_MAIN_THREAD_ONLY_API_C \
  NSCAssert([NSThread isMainThread], @"Main thread only API called background thread.")

typedef struct {
  /// frame duration
  NSTimeInterval duration;
  /// number of frames dropped: 1, 3, 5, 7, 25
  UInt32 drop;
  /// The duration for which the current frame exceeds the expected time for a refresh,
  /// representing potential lag or 'hitch' in the frame display.
  NSTimeInterval hitchDuration;
} LynxFPSFrame;

static void LynxFPSRawMetricsAddFrame(LynxFPSRawMetrics *metrics, LynxFPSFrame frame) {
  metrics->frames += 1;
  metrics->duration += frame.duration;
  metrics->hitchDuration += frame.hitchDuration;

  switch (frame.drop) {
    case 25:
      metrics->drop25Count += 1;
      metrics->drop25Duration += frame.hitchDuration;
    case 7:
      metrics->drop7Count += 1;
      metrics->drop7Duration += frame.hitchDuration;
    case 3:
      metrics->drop3Count += 1;
      metrics->drop3Duration += frame.hitchDuration;
    case 1:
      metrics->drop1Count += 1;
      metrics->drop1Duration += frame.hitchDuration;
    default:
      break;
  }
}

#pragma mark - LynxFPSMonitor

@implementation LynxFPSMonitor {
  NSRunLoop *_mainRunLoop;

  CADisplayLink *_displayLink;
  BOOL _supportsTargetTimestamp;
  BOOL _supportsDynamicFrameRate;
  BOOL _canSupportDynamicFrameRate;

  LynxFPSFrame _lastFrame;
  NSTimeInterval _lastTimestamp;
  NSTimeInterval _lastTargetTimestamp;
  NSTimeInterval _lastVSyncInterval;

  std::vector<LynxFPSRecord *> _activeRecords;
  std::vector<LynxFPSRecord *> _timeoutRecords;
  NSMutableDictionary<id<NSCopying>, LynxFPSRecord *> *_keyedRecords;
}

+ (instancetype)sharedInstance {
  static LynxFPSMonitor *sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[LynxFPSMonitor alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _mainRunLoop = [NSRunLoop mainRunLoop];
    _activeRecords = {};
    _timeoutRecords = {};
    _keyedRecords = [[NSMutableDictionary alloc] init];
    if (@available(iOS 15.0, *)) {
      _supportsTargetTimestamp = YES;
      _supportsDynamicFrameRate = UIScreen.mainScreen.maximumFramesPerSecond > 60.0;
      _canSupportDynamicFrameRate = _supportsDynamicFrameRate;
    } else if (@available(iOS 10.0, *)) {
      _supportsTargetTimestamp = YES;
      _supportsDynamicFrameRate = NO;
    } else {
      _supportsTargetTimestamp = NO;
      _supportsDynamicFrameRate = NO;
    }

    [self setupNotifications];
  }
  return self;
}

- (void)setSupportsDynamicFrameRate:(BOOL)flag {
  _supportsDynamicFrameRate = flag && _canSupportDynamicFrameRate;
}

- (BOOL)isActive {
  return !_displayLink.isPaused;
}

#pragma mark Managing Records

- (LynxFPSRecord *)recordWithKey:(id<NSCopying>)key {
  ASSERT_MAIN_THREAD_ONLY_API;
  return _keyedRecords[key];
}

- (void)setActive:(BOOL)active forRecord:(LynxFPSRecord *)record {
  if (active) {
    _activeRecords.push_back(record);
    [self setupDisplayLinkPaused:NO];
  } else {
    // activeRecords are supposedly small, linear iteration should be fine.
    auto it = std::find(_activeRecords.begin(), _activeRecords.end(), record);
    if (it != _activeRecords.end()) {
      _activeRecords.erase(it);
      if (_activeRecords.empty()) {
        [self setupDisplayLinkPaused:YES];
      }
    }
  }
}

- (void)setState:(LynxFPSRecordState)state forRecord:(LynxFPSRecord *)record {
  ASSERT_MAIN_THREAD_ONLY_API;
  NSAssert(_keyedRecords[record.key] == record,
           @"Updating state for an invalid record instance %@.", record);
  switch (state) {
    case LynxFPSRecordStateActive: {
      if (record->_state < LynxFPSRecordStateActive) {
        record->_state = LynxFPSRecordStateActive;
        [self setActive:YES forRecord:record];
      }
    } break;
    case LynxFPSRecordStatePaused: {
      if (record->_state <= LynxFPSRecordStateActive) {
        record->_state = LynxFPSRecordStatePaused;
        [self setActive:NO forRecord:record];
      }
    } break;
    default: {
      if (record->_state != LynxFPSRecordStateEnded) {
        record->_state = LynxFPSRecordStateEnded;
        [_keyedRecords removeObjectForKey:record.key];
        [self setActive:NO forRecord:record];
      }
    } break;
  }
}

- (LynxFPSRecord *)setState:(LynxFPSRecordState)state forRecordWithKey:(id<NSCopying>)key {
  ASSERT_MAIN_THREAD_ONLY_API;
  LynxFPSRecord *record = _keyedRecords[key];
  if (record) {
    [self setState:state forRecord:record];
  }
  return record;
}

#pragma mark Begin/Pause/End Monitoring

- (LynxFPSRecord *)beginWithKey:(id<NSCopying>)key {
  LynxFPSRecord *record = _keyedRecords[key];
  if (record) {
    [self setState:LynxFPSRecordStateActive forRecord:record];
    return record;
  }

  record = [[LynxFPSRecord alloc] initWithKey:key];
  _keyedRecords[key] = record;
  [self setState:LynxFPSRecordStateActive forRecord:record];
  return record;
}

- (LynxFPSRecord *)pauseWithKey:(id<NSCopying>)key {
  return [self setState:LynxFPSRecordStatePaused forRecordWithKey:key];
}

- (LynxFPSRecord *)endWithKey:(id<NSCopying>)key {
  return [self setState:LynxFPSRecordStateEnded forRecordWithKey:key];
}

#pragma mark CADisplayLink

- (void)setupDisplayLinkPaused:(BOOL)paused {
  if (!_displayLink) {
    _displayLink = [CADisplayLink displayLinkWithTarget:self
                                               selector:@selector(displayLinkDidUpdate:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
  BOOL actualPaused = paused || _activeRecords.size() == 0;
  if (_displayLink.paused != actualPaused) {
    _displayLink.paused = actualPaused;

    if (!actualPaused) {
      _lastFrame = (LynxFPSFrame){0, 0, 0};
      _lastTimestamp = 0;
      _lastTargetTimestamp = 0;
    }
  }
}

- (void)displayLinkDidUpdate:(CADisplayLink *)displayLink {
  CFTimeInterval currentTimestamp = displayLink.timestamp;
  CFTimeInterval desiredFrameInterval = 1.0 / 60.0;
  NSInteger maximumFramesPerSecond = 60;
  if (@available(iOS 10.3, *)) {
    maximumFramesPerSecond = UIScreen.mainScreen.maximumFramesPerSecond;
  }

  if (likely(_lastTimestamp != 0.0)) {
    LynxFPSFrame frame = (LynxFPSFrame){0, 0, 0};
    frame.duration = currentTimestamp - _lastTimestamp;
    frame.hitchDuration = currentTimestamp - _lastTargetTimestamp;

    if (frame.hitchDuration >= 25.0 * desiredFrameInterval) {
      frame.drop = 25;
    } else if (frame.hitchDuration >= 7.0 * desiredFrameInterval) {
      frame.drop = 7;
    } else if (frame.hitchDuration >= 3.0 * desiredFrameInterval) {
      frame.drop = 3;
    } else if (frame.hitchDuration >= desiredFrameInterval) {
      frame.drop = 1;
    }

    for (auto record : _activeRecords) {
      NSAssert(record->_state == LynxFPSRecordStateActive, @"Internal Inconsistency");

      if (record.maximumFramesPerSecond == -1) {
        record.maximumFramesPerSecond = maximumFramesPerSecond;
      }

      CFTimeInterval oldDuration = record->_totalMetrics.duration;
      LynxFPSRawMetricsAddFrame(&record->_totalMetrics, frame);

      CFTimeInterval timeout = record->timeoutInterval;
      CFTimeInterval newDuration = record->_totalMetrics.duration;
      if (unlikely(timeout > 0.0 && oldDuration < timeout && newDuration >= timeout)) {
        _timeoutRecords.push_back(record);
      }
    }
    for (auto record : _timeoutRecords) {
      record->timeoutCompletion(record);
    }
    _timeoutRecords.clear();

    _lastFrame = frame;
  }

  _lastTimestamp = currentTimestamp;
  if (_supportsTargetTimestamp) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
    _lastTargetTimestamp = displayLink.targetTimestamp;
#pragma clang diagnostic pop
  }

  if (_supportsDynamicFrameRate) {
    // The code below only runs on >= iOS 15
    NSTimeInterval vsyncInterval = displayLink.duration;
    if (_lastVSyncInterval != vsyncInterval) {
      NSInteger lastFPS = (NSInteger)ceil(1.0 / _lastVSyncInterval);
      NSInteger currentFPS = (NSInteger)ceil(1.0 / vsyncInterval);
      if (lastFPS != currentFPS) {
        _lastVSyncInterval = vsyncInterval;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
        // Already guarded by flag `supportsDynamicFrameRate`.
        _displayLink.preferredFrameRateRange = CAFrameRateRangeMake(10.0, currentFPS, 0.0);
#pragma clang diagnostic pop
      }
    }
  }
}

#pragma mark Notifications

- (void)setupNotifications {
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(onAppNotification:)
                                               name:UIApplicationWillResignActiveNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(onAppNotification:)
                                               name:UIApplicationDidBecomeActiveNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(onAppNotification:)
                                               name:UIApplicationWillTerminateNotification
                                             object:nil];
}

- (void)onAppNotification:(NSNotification *)notif {
  [self setupDisplayLinkPaused:notif.name != UIApplicationDidBecomeActiveNotification];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
