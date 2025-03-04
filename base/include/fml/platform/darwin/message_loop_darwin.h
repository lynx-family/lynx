// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_DARWIN_MESSAGE_LOOP_DARWIN_H_
#define BASE_INCLUDE_FML_PLATFORM_DARWIN_MESSAGE_LOOP_DARWIN_H_

#include <CoreFoundation/CoreFoundation.h>

#include <atomic>

#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/platform/darwin/cf_utils.h"

namespace lynx {
namespace fml {

class MessageLoopDarwin : public MessageLoopImpl {
 public:
  // A custom CFRunLoop mode used when processing flutter messages,
  // so that the CFRunLoop can be run without being interrupted by UIKit,
  // while still being able to receive and be interrupted by framework messages.
  static CFStringRef kMessageLoopCFRunLoopMode;

 protected:
  void WakeUp(fml::TimePoint time_point) override;

  MessageLoopDarwin();

  ~MessageLoopDarwin() override;

 private:
  std::atomic_bool running_;
  CFRef<CFRunLoopTimerRef> delayed_wake_timer_;
  CFRef<CFRunLoopRef> loop_;
  CFRef<CFRunLoopSourceRef> work_source_;

  // |fml::MessageLoopImpl|
  void Run() override;

  // |fml::MessageLoopImpl|
  void Terminate() override;

  static void OnSourceFire(MessageLoopDarwin* loop);

  static void OnTimerFire(CFRunLoopTimerRef timer, MessageLoopDarwin* loop);

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopDarwin);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopDarwin);
  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoopDarwin);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoopDarwin;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_PLATFORM_DARWIN_MESSAGE_LOOP_DARWIN_H_
