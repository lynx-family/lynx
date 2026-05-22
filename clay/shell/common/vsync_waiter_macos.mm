// Copyright 2025 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/vsync_waiter_macos.h"

#include <functional>

#import <AppKit/NSScreen.h>

#include "clay/shell/common/vsync_waiter_fallback.h"

@implementation DisplayLinkManager {
 @public
  std::function<void(fml::TimePoint, fml::TimePoint)> callback_;
}

- (instancetype)initWith:(fml::RefPtr<fml::TaskRunner>)task_runner {
  self = [super init];
  if (self) {
    NSScreen *screen = [[NSScreen screens] firstObject];
    _displayLink = [screen displayLinkWithTarget:self selector:@selector(onDisplayLink:)];
    _displayLink.paused = YES;
    task_runner->PostTask([self]() {
      [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    });
  }
  return self;
}

- (void)destroy {
  [_displayLink invalidate];
}

- (void)onDisplayLink:(CADisplayLink *)link {
  _displayLink.paused = YES;
  if (!callback_) {
    return;
  }

  CFTimeInterval delay = CACurrentMediaTime() - link.timestamp;
  fml::TimePoint frame_start_time = fml::TimePoint::Now() - fml::TimeDelta::FromSecondsF(delay);
  CFTimeInterval duration = link.targetTimestamp - link.timestamp;
  fml::TimePoint frame_target_time = frame_start_time + fml::TimeDelta::FromSecondsF(duration);

  callback_(frame_start_time, frame_target_time);
}
@end

namespace clay {

class API_AVAILABLE(macos(14.0)) MacOSVsyncWaiterService : public VsyncWaiterService {
  std::unique_ptr<VsyncWaiter> CreateVsyncWaiter(
      fml::RefPtr<fml::TaskRunner> task_runner) const override {
    return std::make_unique<VsyncWaiterMacOS>(std::move(task_runner));
  }
};

std::shared_ptr<VsyncWaiterService> VsyncWaiterService::Create() {
  if (@available(macOS 14.0, *)) {
    return std::make_shared<MacOSVsyncWaiterService>();
  }
  return CreateFallbackVsyncWaiterService();
}

VsyncWaiterMacOS::VsyncWaiterMacOS(fml::RefPtr<fml::TaskRunner> task_runner)
    : VsyncWaiter(std::move(task_runner)) {
  manager_ = [[DisplayLinkManager alloc] initWith:task_runner_];
}

VsyncWaiterMacOS::~VsyncWaiterMacOS() {
  if (manager_) {
    [manager_ destroy];
    manager_ = nil;
  }
}

void VsyncWaiterMacOS::AwaitVSync() {
  if (manager_) {
    if (!manager_->callback_) {
      std::weak_ptr<VsyncWaiterMacOS> weak_this =
          std::static_pointer_cast<VsyncWaiterMacOS>(shared_from_this());
      manager_->callback_ = [weak_this](fml::TimePoint frame_start_time,
                                        fml::TimePoint frame_target_time) {
        if (auto vsync_waiter = weak_this.lock()) {
          vsync_waiter->FireCallback(frame_start_time, frame_target_time);
        }
      };
    }
    manager_.displayLink.paused = NO;
  }
}

}  // namespace clay
