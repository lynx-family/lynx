// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTraceEvent.h>
#import <LynxDevtool/LynxFPSTrace.h>
#import <LynxDevtool/LynxFrameTraceService.h>
#include <mach/mach_time.h>
#include <chrono>
#if OS_OSX
#import <AppKit/AppKit.h>
#endif

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#import "tracing/platform/fps_trace_plugin_darwin.h"
#endif

static uint64_t startTimestamp = 0;

@implementation LynxFPSTrace {
  dispatch_queue_t _fpsRecordQueue;
  CFRunLoopObserverRef _beginObserver;
  CFRunLoopObserverRef _endObserver;
  uint64_t _frameIntervalNanos;
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  std::unique_ptr<lynx::trace::TracePlugin> _fps_trace_plugin;
#endif
}

+ (instancetype)shareInstance {
  static LynxFPSTrace *instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (id)init {
  if (self = [super init]) {
    _fpsRecordQueue = dispatch_queue_create("FPSTrace.queue", DISPATCH_QUEUE_SERIAL);
#if OS_IOS
    if (@available(iOS 10.3, *)) {
      _frameIntervalNanos =
          static_cast<uint64_t>(1000000000 / UIScreen.mainScreen.maximumFramesPerSecond);
    } else {
      _frameIntervalNanos = static_cast<uint64_t>(1000000000 / 60);
    }
#elif OS_OSX
    if (@available(macOS 12.0, *)) {
      _frameIntervalNanos =
          static_cast<uint64_t>(1000000000 / NSScreen.mainScreen.maximumFramesPerSecond);
    } else {
      _frameIntervalNanos = static_cast<uint64_t>(1000000000 / 60);
    }
#endif

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
    _fps_trace_plugin = std::make_unique<lynx::trace::FPSTracePluginDarwin>();
#endif
  }
  return self;
}

- (void)startFPSTrace {
  if ([LynxTraceEvent categoryEnabled:@"disabled-by-default-devtools.timeline.frame"]) {
    [[LynxFrameTraceService shareInstance] initializeService];
    CFRunLoopObserverContext context = {0, (__bridge void *)self, NULL, NULL, NULL};

    _beginObserver = CFRunLoopObserverCreate(CFAllocatorGetDefault(), kCFRunLoopBeforeSources, true,
                                             LONG_MIN, RunloopBeginCallBack, &context);
    CFRunLoopAddObserver(CFRunLoopGetMain(), _beginObserver, kCFRunLoopCommonModes);

    _endObserver = CFRunLoopObserverCreate(CFAllocatorGetDefault(), kCFRunLoopBeforeWaiting, true,
                                           LONG_MAX, RunloopEndCallBack, &context);
    CFRunLoopAddObserver(CFRunLoopGetMain(), _endObserver, kCFRunLoopCommonModes);
  }
}

- (uint64_t)getSystemBootTimeNs {
  auto init_time_factor = []() -> uint64_t {
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    return timebase_info.numer / timebase_info.denom;
  };
  static uint64_t monotonic_timebase_factor = init_time_factor();

  return std::chrono::nanoseconds(mach_absolute_time() * monotonic_timebase_factor).count();
}

- (void)stopFPSTrace {
  if (_endObserver != nil) {
    CFRunLoopRemoveObserver(CFRunLoopGetMain(), _endObserver, kCFRunLoopCommonModes);
  }
  if (_beginObserver != nil) {
    CFRunLoopRemoveObserver(CFRunLoopGetMain(), _beginObserver, kCFRunLoopCommonModes);
  }
}

- (void)record:(uint64_t)startTime withEndTime:(uint64_t)endTime {
  if (endTime - startTime >= _frameIntervalNanos) {
    dispatch_async(_fpsRecordQueue, ^{
      [[LynxFrameTraceService shareInstance] FPSTrace:startTime withEndTime:endTime];
    });
  }
}

- (intptr_t)getFPSTracePlugin {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  return reinterpret_cast<intptr_t>(_fps_trace_plugin.get());
#endif
  return 0;
}

void RunloopBeginCallBack(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
  startTimestamp = [[LynxFPSTrace shareInstance] getSystemBootTimeNs];
}

void RunloopEndCallBack(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
  uint64_t currentTime = [[LynxFPSTrace shareInstance] getSystemBootTimeNs];
  [[LynxFPSTrace shareInstance] record:startTimestamp withEndTime:currentTime];
}

@end
