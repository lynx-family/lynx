// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/darwin/message_loop_darwin.h"

#include <CoreFoundation/CFRunLoop.h>
#import <Foundation/Foundation.h>

namespace lynx {
namespace fml {

fml::RefPtr<MessageLoopImpl> MessageLoopImpl::Create(void* platform_loop) {
  return fml::MakeRefCounted<MessageLoopDarwin>();
}

static constexpr CFTimeInterval kDistantFuture = 1.0e10;

CFStringRef MessageLoopDarwin::kMessageLoopCFRunLoopMode = CFSTR("fmlMessageLoop");

MessageLoopDarwin::MessageLoopDarwin()
    : running_(false), loop_((CFRunLoopRef)CFRetain(CFRunLoopGetCurrent())) {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(loop_ != nullptr);

  // Setup the delayed wake source.
  CFRunLoopTimerContext timer_context = {
      .info = this,
  };
  delayed_wake_timer_.Reset(
      CFRunLoopTimerCreate(kCFAllocatorDefault, kDistantFuture /* fire date */,
                           HUGE_VAL /* interval */, 0 /* flags */, 0 /* order */,
                           reinterpret_cast<CFRunLoopTimerCallBack>(&MessageLoopDarwin::OnTimerFire)
                           /* callout */,
                           &timer_context /* context */));
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(delayed_wake_timer_ != nullptr);
  CFRunLoopAddTimer(loop_, delayed_wake_timer_, kCFRunLoopCommonModes);
  // This mode will be used by FlutterKeyboardManager.
  CFRunLoopAddTimer(loop_, delayed_wake_timer_, kMessageLoopCFRunLoopMode);

  // Setup the non-delayed wake source.
  CFRunLoopSourceContext source_context = CFRunLoopSourceContext();
  source_context.info = this;
  source_context.perform = reinterpret_cast<void (*)(void*)>(&MessageLoopDarwin::OnSourceFire);
  work_source_.Reset(CFRunLoopSourceCreate(NULL, 1, /* priority*/
                                           &source_context));
  CFRunLoopAddSource(loop_, work_source_, kCFRunLoopCommonModes);
  CFRunLoopAddSource(loop_, work_source_, kMessageLoopCFRunLoopMode);
}

MessageLoopDarwin::~MessageLoopDarwin() {
  CFRunLoopTimerInvalidate(delayed_wake_timer_);
  CFRunLoopRemoveTimer(loop_, delayed_wake_timer_, kCFRunLoopCommonModes);
  CFRunLoopRemoveTimer(loop_, delayed_wake_timer_, kMessageLoopCFRunLoopMode);
  CFRunLoopRemoveSource(loop_, work_source_, kCFRunLoopCommonModes);
  CFRunLoopRemoveSource(loop_, work_source_, kMessageLoopCFRunLoopMode);
}

void MessageLoopDarwin::Run() {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(loop_ == CFRunLoopGetCurrent());

  running_ = true;

  while (running_) {
    @autoreleasepool {
      int result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, kDistantFuture, YES);
      if (result == kCFRunLoopRunStopped || result == kCFRunLoopRunFinished) {
        // This handles the case where the loop is terminated using
        // CoreFoundation APIs.
        @autoreleasepool {
          RunExpiredTasksNow();
        }
        running_ = false;
      }
    }
  }
}

void MessageLoopDarwin::Terminate() {
  running_ = false;
  CFRunLoopStop(loop_);
}

void MessageLoopDarwin::WakeUp(fml::TimePoint time_point) {
  // Rearm the timer. The time bases used by CoreFoundation and FXL are
  // different and must be accounted for.
  auto now = TimePoint::Now();
  // wake up immediately
  if (time_point <= now) {
    CFRunLoopSourceSignal(work_source_);
    CFRunLoopWakeUp(loop_);
    return;
  }

  CFRunLoopTimerSetNextFireDate(delayed_wake_timer_,
                                CFAbsoluteTimeGetCurrent() + (time_point - now).ToSecondsF());
}

void MessageLoopDarwin::OnSourceFire(MessageLoopDarwin* loop) {
  @autoreleasepool {
    // RunExpiredTasksNow rearms the timer as appropriate via a call to WakeUp.
    loop->RunExpiredTasksNow();
  }
}

void MessageLoopDarwin::OnTimerFire(CFRunLoopTimerRef timer, MessageLoopDarwin* loop) {
  OnSourceFire(loop);
}

}  // namespace fml
}  // namespace lynx
