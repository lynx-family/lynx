// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/darwin/vsync_monitor_darwin.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/trace/trace_event_def.h"
#include "core/renderer/utils/lynx_env.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

@interface LynxVSyncPulse : NSObject
@property(atomic) CADisplayLink *displayLink;
@property(atomic) BOOL isInBackground;

- (instancetype)initWithCallback:(lynx::base::VSyncMonitor::Callback)callback
                         runLoop:(NSRunLoop *)runLoop;

- (void)requestPulse;

- (void)invalidate;

@end

@implementation LynxVSyncPulse {
  lynx::base::VSyncMonitor::Callback _callback;
  CADisplayLink *_displayLink;
}

- (instancetype)initWithCallback:(lynx::base::VSyncMonitor::Callback)callback
                         runLoop:(NSRunLoop *)runLoop {
  self = [super init];
  if (self) {
    _callback = std::move(callback);
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onMainDisplay:)];
    _displayLink.paused = YES;
    [_displayLink addToRunLoop:runLoop forMode:NSRunLoopCommonModes];

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

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  return std::make_shared<lynx::base::VSyncMonitorIOS>(false, is_on_ui_thread);
}

class LynxVSyncPulsePuppet {
 public:
  LynxVSyncPulsePuppet(VSyncMonitorIOS *vsync_monitor_ios) {
    if (!delegate) {
      delegate = [[LynxVSyncPulse alloc]
          initWithCallback:[vsync_monitor_ios](int64_t start, int64_t target) {
            vsync_monitor_ios->OnVSync(start, target);
          }
                   runLoop:[NSRunLoop currentRunLoop]];
    }
  }
  ~LynxVSyncPulsePuppet() { [delegate invalidate]; }
  void RequestPulse() { [delegate requestPulse]; }
  void SetHighRefreshRate() { [delegate SetHighRefreshRate]; }

 private:
  LynxVSyncPulse *delegate = nullptr;
};

// The monitor and its subscribers stay on their bound runner. Only this native
// source state is accessed on the main queue, including after monitor teardown.
class LynxUIVSyncPulse : public std::enable_shared_from_this<LynxUIVSyncPulse> {
 public:
  enum class Operation { kInit, kRequestPulse, kSetHighRefreshRate, kInvalidate };

  void Run(Operation operation, std::weak_ptr<VSyncMonitor> monitor = {}) {
    auto self = shared_from_this();
    dispatch_block_t task = ^{
      if (operation == Operation::kInvalidate) {
        self->invalidated_ = true;
        self->monitor_.reset();
        [self->delegate_ invalidate];
        self->delegate_ = nil;
        return;
      }
      self->InitOnUI(monitor);
      if (self->invalidated_) {
        return;
      }
      if (operation == Operation::kRequestPulse) {
        [self->delegate_ requestPulse];
      } else if (operation == Operation::kSetHighRefreshRate) {
        [self->delegate_ SetHighRefreshRate];
      }
    };

    if ([NSThread isMainThread]) {
      task();
    } else {
      dispatch_async(dispatch_get_main_queue(), task);
    }
  }

 private:
  void InitOnUI(const std::weak_ptr<VSyncMonitor> &monitor) {
    if (invalidated_) {
      return;
    }
    // A constructor may call Init before shared ownership is established.
    // The first request supplies the live weak reference in that case.
    if (!monitor.expired()) {
      monitor_ = monitor;
    }
    if (!delegate_) {
      auto callback = [weak_self = weak_from_this()](int64_t start, int64_t target) {
        if (auto self = weak_self.lock()) {
          if (auto monitor = self->monitor_.lock()) {
            monitor->OnVSync(start, target);
          }
        }
      };
      delegate_ = [[LynxVSyncPulse alloc] initWithCallback:std::move(callback)
                                                   runLoop:[NSRunLoop mainRunLoop]];
    }
  }

  bool invalidated_{false};
  std::weak_ptr<VSyncMonitor> monitor_;
  LynxVSyncPulse *delegate_{nil};
};

VSyncMonitorIOS::VSyncMonitorIOS(bool init_in_current_loop, bool is_on_ui_thread,
                                 bool is_vsync_post_task_by_emergency)
    : VSyncMonitor(is_on_ui_thread, is_vsync_post_task_by_emergency),
      ui_delegate_(is_on_ui_thread ? std::make_shared<LynxUIVSyncPulse>() : nullptr) {
  if (init_in_current_loop) {
    Init();
  }
}

void VSyncMonitorIOS::Init() {
  if (ui_delegate_) {
    ui_delegate_->Run(LynxUIVSyncPulse::Operation::kInit, weak_from_this());
    return;
  }
  if (!delegate_) {
    delegate_ = std::make_unique<LynxVSyncPulsePuppet>(this);
  }
}

void VSyncMonitorIOS::SetHighRefreshRate() {
  if (ui_delegate_) {
    ui_delegate_->Run(LynxUIVSyncPulse::Operation::kSetHighRefreshRate, weak_from_this());
    return;
  }
  if (!delegate_) {
    Init();
  }
  delegate_->SetHighRefreshRate();
}

VSyncMonitorIOS::~VSyncMonitorIOS() {
  if (ui_delegate_) {
    ui_delegate_->Run(LynxUIVSyncPulse::Operation::kInvalidate);
  }
}

void VSyncMonitorIOS::RequestVSync() {
  if (ui_delegate_) {
    ui_delegate_->Run(LynxUIVSyncPulse::Operation::kRequestPulse, weak_from_this());
    return;
  }
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
  RequestVSyncOnUIThread();
}

void VSyncMonitorIOS::RequestVSyncOnUIThread() {
  if (ui_delegate_ || [NSThread isMainThread]) {
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
