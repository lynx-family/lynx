// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/darwin/vsync_monitor_darwin.h"

#include <cstdint>

#include "base/include/fml/message_loop.h"
#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/trace/trace_event_def.h"
#include "core/renderer/utils/lynx_env.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

@interface LynxVSyncPulse : NSObject
@property(atomic) CADisplayLink *displayLink;
@property(atomic) BOOL isInBackground;

- (instancetype)initWithCallback:(lynx::base::VSyncMonitor::Callback)callback;

- (void)requestPulse;

- (void)invalidate;

@end

@implementation LynxVSyncPulse {
  lynx::base::VSyncMonitor::Callback _callback;
  CADisplayLink *_displayLink;
}

- (instancetype)initWithCallback:(lynx::base::VSyncMonitor::Callback)callback {
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, VSYNC_MONITOR_DARWIN_ON_MAIN_DISPLAY);
  if (_callback) {
    CFTimeInterval timestamp = _displayLink.timestamp;
    CFTimeInterval frameEnd;
    if (@available(iOS 15.0, *)) {
      frameEnd = _displayLink.targetTimestamp;
    } else {
      frameEnd = timestamp + _displayLink.duration;
    }
    _callback(timestamp * 1e+9, frameEnd * 1e+9);
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
namespace base {

namespace {

VSyncMonitor &GetUIFrameSource(bool high_refresh_rate) {
  // Both sources are registered and used only on the main run loop.
  // Share frames between pages with the same refresh preference.
  struct FrameSources {
    std::shared_ptr<VSyncMonitor> standard;
    std::shared_ptr<VSyncMonitor> high_refresh;
  };
  static base::NoDestructor<FrameSources> sources;
  auto &monitor = high_refresh_rate ? sources->high_refresh : sources->standard;
  if (!monitor) {
    monitor = VSyncMonitor::Create(false);
    monitor->BindTaskRunner(fml::MessageLoop::EnsureInitializedForCurrentThread().GetTaskRunner());
    monitor->Init();
    if (high_refresh_rate) {
      monitor->SetHighRefreshRate();
    }
  }
  return *monitor;
}

class UIVSyncMonitorIOS final : public VSyncMonitor {
 public:
  UIVSyncMonitorIOS() : VSyncMonitor(true) {}

 private:
  void RequestVSyncOnUIThread() override {
    const bool high_refresh_rate = high_refresh_rate_;
    auto weak_monitor = weak_from_this();
    dispatch_block_t request = ^{
      if (weak_monitor.expired()) {
        return;
      }
      // Each TASM monitor already coalesces its own requests. These IDs are
      // assigned on main, where the shared source registers its callbacks.
      static uintptr_t next_request_id = 0;
      GetUIFrameSource(high_refresh_rate)
          .ScheduleVSyncSecondaryCallback(++next_request_id,
                                          [weak_monitor](int64_t start, int64_t target) {
                                            if (auto monitor = weak_monitor.lock()) {
                                              monitor->OnVSync(start, target);
                                            }
                                          });
    };
    if ([NSThread isMainThread]) {
      request();
    } else {
      dispatch_async(dispatch_get_main_queue(), request);
    }
  }

  void SetHighRefreshRate() override { high_refresh_rate_ = true; }

  bool high_refresh_rate_ = false;
};

}  // namespace

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  if (is_on_ui_thread) {
    return std::make_shared<UIVSyncMonitorIOS>();
  }
  return std::make_shared<VSyncMonitorIOS>(false, false);
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

VSyncMonitorIOS::VSyncMonitorIOS(bool init_in_current_loop, bool is_on_ui_thread,
                                 bool is_vsync_post_task_by_emergency)
    : VSyncMonitor(is_on_ui_thread, is_vsync_post_task_by_emergency) {
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
    std::weak_ptr<VSyncMonitorIOS> weak_self =
        std::static_pointer_cast<VSyncMonitorIOS>(shared_from_this());
    dispatch_async(dispatch_get_main_queue(), ^{
      auto strong_self = weak_self.lock();
      if (strong_self) {
        strong_self->RequestVSync();
      }
    });
  }
}

void VSyncMonitorIOS::RequestVSyncOnUIThread() {
  if ([NSThread isMainThread]) {
    RequestVSync();
  } else {
    std::weak_ptr<VSyncMonitorIOS> weak_self =
        std::static_pointer_cast<VSyncMonitorIOS>(shared_from_this());
    dispatch_async(dispatch_get_main_queue(), ^{
      auto strong_self = weak_self.lock();
      if (strong_self) {
        strong_self->RequestVSync();
      }
    });
  }
}

}  // namespace base
}  // namespace lynx
