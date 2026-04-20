// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/darwin/macos/common/vsync_monitor_macos.h"

#include <functional>
#include <memory>

#import <AppKit/NSScreen.h>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/time/time_delta.h"
#include "platform/embedder/vsync_monitor_fallback.h"

using VSyncCallback = std::function<void(fml::TimePoint, fml::TimePoint)>;

@implementation DisplayLinkImpl {
  VSyncCallback callback_;
}

- (instancetype)initWith:(VSyncCallback)callback {
  self = [super init];
  if (self) {
    callback_ = std::move(callback);
    NSScreen *screen = [[NSScreen screens] firstObject];
    _displayLink = [screen displayLinkWithTarget:self selector:@selector(onDisplayLink:)];
    _displayLink.paused = YES;
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
  }
  return self;
}

- (void)destroy {
  // The host may call CADisplayLink invalidate, so we should avoid calling it here.
}

- (void)onDisplayLink:(CADisplayLink *)link {
  _displayLink.paused = YES;

  CFTimeInterval delay = CACurrentMediaTime() - link.timestamp;
  fml::TimePoint frame_start_time = fml::TimePoint::Now() - fml::TimeDelta::FromSecondsF(delay);
  CFTimeInterval duration = link.targetTimestamp - link.timestamp;
  fml::TimePoint frame_target_time = frame_start_time + fml::TimeDelta::FromSecondsF(duration);
  callback_(frame_start_time, frame_target_time);
}
@end

namespace lynx {
namespace base {

template <typename Block>
void ExecuteOnMainThreadAsync(Block block) {
  if ([NSThread isMainThread]) {
    block();
  } else {
    dispatch_async(dispatch_get_main_queue(), block);
  }
}

template <typename Block>
void ExecuteOnMainThreadSync(Block block) {
  if ([NSThread isMainThread]) {
    block();
  } else {
    dispatch_sync(dispatch_get_main_queue(), block);
  }
}

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  if (@available(macOS 14.0, *)) {
    return std::make_shared<lynx::base::VSyncMonitorMacOS>();
  }
  int framesPerSecond = 60;
  if (@available(macOS 12.0, *)) {
    NSScreen *screen = [[NSScreen screens] firstObject];
    framesPerSecond = screen.maximumFramesPerSecond;
  }
  return std::make_shared<lynx::base::VSyncMonitorFallback>(
      fml::TimeDelta::FromSecondsF(1.0 / framesPerSecond));
}

void VSyncMonitorMacOS::Init() {
  std::weak_ptr<VSyncMonitorMacOS> weak_this =
      std::static_pointer_cast<VSyncMonitorMacOS>(shared_from_this());

  ExecuteOnMainThreadSync(^{
    impl_ = [[DisplayLinkImpl alloc]
        initWith:[weak_this](fml::TimePoint start_time, fml::TimePoint target_time) {
          if (auto vsync_monitor = weak_this.lock()) {
            vsync_monitor->OnVSync(start_time.ToEpochDelta().ToNanoseconds(),
                                   target_time.ToEpochDelta().ToNanoseconds());
          }
        }];
  });
}

VSyncMonitorMacOS::~VSyncMonitorMacOS() {
  destroying_.store(true);

  auto impl = impl_;
  ExecuteOnMainThreadAsync(^{
    if (impl) {
      [impl destroy];
    }
  });
}

void VSyncMonitorMacOS::RequestVSync() {
  if (destroying_.load()) {
    return;
  }
  auto impl = impl_;
  ExecuteOnMainThreadAsync(^{
    if (impl) {
      impl.displayLink.paused = NO;
    }
  });
}

}  // namespace base
}  // namespace lynx
