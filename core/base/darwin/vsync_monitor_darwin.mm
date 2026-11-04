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

namespace {

enum class PulseOperation { kInit, kRequest, kSetHighRefreshRate, kInvalidate };

}  // namespace

@interface LynxVSyncPulse : NSObject

- (instancetype)initWithMonitor:(lynx::base::VSyncMonitorIOS *)monitor;

- (void)run:(PulseOperation)operation;

@end

@implementation LynxVSyncPulse {
  lynx::base::VSyncMonitor *_legacyMonitor;
  std::weak_ptr<lynx::base::VSyncMonitor> _uiMonitor;
  CADisplayLink *_displayLink;
  BOOL _isInBackground;
}

- (instancetype)initWithMonitor:(lynx::base::VSyncMonitorIOS *)monitor {
  self = [super init];
  if (self) {
    if (monitor->RequestsVSyncOnUIThread()) {
      _uiMonitor = monitor->weak_from_this();
    } else {
      _legacyMonitor = monitor;
      [self registerDisplayLinkOnRunLoop:[NSRunLoop currentRunLoop]];
    }
  }
  return self;
}

- (void)registerDisplayLinkOnRunLoop:(NSRunLoop *)runLoop {
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

- (void)run:(PulseOperation)operation {
  if (_legacyMonitor || [NSThread isMainThread]) {
    [self performOperation:operation];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self performOperation:operation];
    });
  }
}

- (void)performOperation:(PulseOperation)operation {
  if (operation == PulseOperation::kInvalidate) {
    [_displayLink invalidate];
    return;
  }
  if (!_legacyMonitor) {
    if (_uiMonitor.expired()) {
      return;
    }
    if (!_displayLink) {
      [self registerDisplayLinkOnRunLoop:[NSRunLoop mainRunLoop]];
    }
  }
  if (operation == PulseOperation::kRequest) {
    _displayLink.paused = NO;
  } else if (operation == PulseOperation::kSetHighRefreshRate) {
    [self setHighRefreshRate];
  }
}

- (void)appWillEnterForeground:(UIApplication *)application {
  _isInBackground = NO;
}

- (void)appDidEnterBackground:(UIApplication *)application {
  _isInBackground = YES;
}

- (void)setHighRefreshRate {
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
  CFTimeInterval timestamp = link.timestamp;
  CFTimeInterval frameEnd;
  if (@available(iOS 15.0, *)) {
    frameEnd = link.targetTimestamp;
  } else {
    frameEnd = timestamp + link.duration;
  }
  if (_legacyMonitor) {
    _legacyMonitor->OnVSync(timestamp * 1e+9, frameEnd * 1e+9);
  } else if (auto monitor = _uiMonitor.lock()) {
    monitor->OnVSync(timestamp * 1e+9, frameEnd * 1e+9);
  }
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
  return VSyncMonitorIOS::Create(is_on_ui_thread);
}

class LynxVSyncPulsePuppet {
 public:
  explicit LynxVSyncPulsePuppet(VSyncMonitorIOS *monitor)
      : delegate_([[LynxVSyncPulse alloc] initWithMonitor:monitor]) {}
  ~LynxVSyncPulsePuppet() { [delegate_ run:PulseOperation::kInvalidate]; }
  void Run(PulseOperation operation) { [delegate_ run:operation]; }

 private:
  LynxVSyncPulse *delegate_;
};

std::shared_ptr<VSyncMonitorIOS> VSyncMonitorIOS::Create(bool is_on_ui_thread,
                                                         bool is_vsync_post_task_by_emergency) {
  auto monitor = std::make_shared<VSyncMonitorIOS>(ConstructionKey{}, is_on_ui_thread,
                                                   is_vsync_post_task_by_emergency);
  if (is_on_ui_thread) {
    monitor->delegate_ = std::make_unique<LynxVSyncPulsePuppet>(monitor.get());
  }
  return monitor;
}

VSyncMonitorIOS::VSyncMonitorIOS(ConstructionKey, bool is_on_ui_thread,
                                 bool is_vsync_post_task_by_emergency)
    : VSyncMonitor(is_on_ui_thread, is_vsync_post_task_by_emergency) {}

void VSyncMonitorIOS::Init() {
  if (!delegate_) {
    delegate_ = std::make_unique<LynxVSyncPulsePuppet>(this);
  } else if (RequestsVSyncOnUIThread()) {
    delegate_->Run(PulseOperation::kInit);
  }
}

void VSyncMonitorIOS::SetHighRefreshRate() {
  if (!delegate_) {
    Init();
  }
  delegate_->Run(PulseOperation::kSetHighRefreshRate);
}

VSyncMonitorIOS::~VSyncMonitorIOS() = default;

void VSyncMonitorIOS::RequestVSync() {
  if (!delegate_) {
    Init();
  }
  delegate_->Run(PulseOperation::kRequest);
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
  if (RequestsVSyncOnUIThread() || [NSThread isMainThread]) {
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
