// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/ios/vsync_monitor_darwin.h"
#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

@interface LynxVSyncPulse : NSObject
@property(atomic) CADisplayLink *displayLink;
@property(atomic) BOOL isInBackground;

- (instancetype)initWithCallback:(lynx::shell::VSyncMonitor::Callback)callback;

- (void)requestPulse;

- (void)invalidate;

@end

@implementation LynxVSyncPulse {
  lynx::shell::VSyncMonitor::Callback _callback;
  CADisplayLink *_displayLink;
}

- (instancetype)initWithCallback:(lynx::shell::VSyncMonitor::Callback)callback {
  self = [super init];
  if (self) {
    _callback = std::move(callback);
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onMainDisplay:)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    _displayLink.paused = YES;

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appWillEnterForeground:)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appDidEnterBackground:)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
  }
  return self;
}

- (void)appWillEnterForeground:(UIApplication *)application {
  _isInBackground = NO;
}

- (void)appDidEnterBackground:(UIApplication *)application {
  _isInBackground = YES;
}

- (void)requestPulse {
  _displayLink.paused = NO;
}

- (void)SetHighRefreshRate {
  if (@available(iOS 15.0, tvOS 15.0, *)) {
    CAFrameRateRange frameRateRange = CAFrameRateRangeMake(30, 120, 120);
    _displayLink.preferredFrameRateRange = frameRateRange;
  } else if (@available(iOS 10.0, tvOS 10.0, *)) {
    _displayLink.preferredFramesPerSecond = 120;
  }
}

- (void)onMainDisplay:(CADisplayLink *)link {
  // TODO: This is a temporary solution, and a more reasonable solution should only stop GL related
  // operations.
  if (_isInBackground) {
    return;
  }
  link.paused = YES;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxVSyncPulse::onMainDisplay");
  if (_callback) {
    CFTimeInterval timestamp = _displayLink.timestamp;
    _callback(timestamp * 1e+9, (timestamp + _displayLink.duration) * 1e+9);
  }
}

- (void)invalidate {
  [_displayLink invalidate];
}

- (void)dealloc {
  [_displayLink invalidate];
  _displayLink = nil;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}
@end

namespace lynx {
namespace shell {

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create() {
  return std::make_shared<lynx::shell::VSyncMonitorIOS>(false);
}

class LynxVSyncPulsePuppet {
 public:
  LynxVSyncPulsePuppet(VSyncMonitorIOS *vsync_monitor_ios) {
    if (!delegate) {
      delegate = [[LynxVSyncPulse alloc]
          initWithCallback:std::bind(&VSyncMonitor::OnVSync, vsync_monitor_ios,
                                     std::placeholders::_1, std::placeholders::_2)];
    }
  }
  ~LynxVSyncPulsePuppet() { [delegate invalidate]; }
  void RequestPulse() { [delegate requestPulse]; }
  void SetHighRefreshRate() { [delegate SetHighRefreshRate]; }

 private:
  LynxVSyncPulse *delegate = nullptr;
};

VSyncMonitorIOS::VSyncMonitorIOS(bool init_in_current_loop, bool is_vsync_post_task_by_emergency)
    : VSyncMonitor(is_vsync_post_task_by_emergency) {
  if (init_in_current_loop) {
    Init();
  }
}

void VSyncMonitorIOS::Init() {
  if (!delegate_) {
    delegate_ = std::make_unique<LynxVSyncPulsePuppet>(this);
  }
}

void VSyncMonitorIOS::SetHighRefreshRate() {
  if (!delegate_) {
    Init();
  }
  delegate_->SetHighRefreshRate();
}

VSyncMonitorIOS::~VSyncMonitorIOS() {}

void VSyncMonitorIOS::RequestVSync() {
  if (!delegate_) {
    Init();
  }
  delegate_->RequestPulse();
}

void VSyncMonitorIOS::RequestVSyncOnUIThread(Callback callback) {
  if (callback_) {
    // request during a frame interval, just return
    return;
  }
  callback_ = std::move(callback);
  if ([NSThread isMainThread]) {
    RequestVSync();
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      RequestVSync();
    });
  }
}

}  // namespace shell
}  // namespace lynx
