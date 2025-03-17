// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxDevToolFrameCapturer.h>
#include <mach/mach_time.h>
#include <chrono>

@implementation LynxDevToolFrameCapturer

- (instancetype)init {
  self = [super init];
  if (self) {
    self.uiView = nil;
    self.snapshotCache = nil;
    self.snapshotInterval = 32000;
    self.lastScreenshotTime = 0;
    self.observer = nil;
    self.hasSnapshotTask = NO;
  }
  return self;
}

- (void)attachView:(VIEW_CLASS*)uiView {
  self.uiView = uiView;
}

- (void)startFrameViewTrace {
  self.snapshotCache = nil;
  if ([self.delegate isEnabled]) {
    if (!self.observer) {
      __weak typeof(self) _self = self;
      self.observer = CFRunLoopObserverCreateWithHandler(
          CFAllocatorGetDefault(), kCFRunLoopBeforeWaiting | kCFRunLoopExit, true,
          LONG_MAX,  // Min priority
          ^(CFRunLoopObserverRef observer, CFRunLoopActivity activity) {
            __strong typeof(_self) strongSelf = _self;
            if (strongSelf) {
              [strongSelf.delegate onFrameChanged];
            }
          });
    }
    CFRunLoopAddObserver(CFRunLoopGetMain(), self.observer, kCFRunLoopCommonModes);
  }
}

- (void)stopFrameViewTrace {
  if (self.observer) {
    CFRunLoopRemoveObserver(CFRunLoopGetMain(), self.observer, kCFRunLoopCommonModes);
    CFRelease(self.observer);
    self.observer = nil;
  }
}

- (uint64_t)getSystemBootTimeUs {
  auto init_time_factor = []() -> uint64_t {
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    return timebase_info.numer / timebase_info.denom;
  };
  static uint64_t monotonic_timebase_factor = init_time_factor();
  return std::chrono::nanoseconds(mach_absolute_time() * monotonic_timebase_factor).count() / 1000;
}

- (void)screenshot {
  uint64_t currentTime = [self getSystemBootTimeUs];
  if (currentTime - self.lastScreenshotTime > self.snapshotInterval) {
    self.lastScreenshotTime = currentTime;
    if (!self.hasSnapshotTask) {
      self.hasSnapshotTask = YES;
      dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate takeSnapshot:self.uiView];
      });
    }
  }
}

- (void)onTakeSnapShot:(NSString*)snapshot {
  if (snapshot == nil) {
    self.hasSnapshotTask = NO;
    return;
  }
  if (!self.snapshotCache || [self.snapshotCache compare:snapshot] != NSOrderedSame) {
    self.snapshotCache = snapshot;
    [self.delegate onNewSnapshot:self.snapshotCache];
  }

  self.hasSnapshotTask = NO;
}

@end
