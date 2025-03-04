// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_LINUX_MESSAGE_LOOP_LINUX_H_
#define BASE_INCLUDE_FML_PLATFORM_LINUX_MESSAGE_LOOP_LINUX_H_

#include <atomic>

#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/unique_fd.h"

namespace lynx {
namespace fml {

class MessageLoopLinux : public MessageLoopImpl {
 private:
  fml::UniqueFD epoll_fd_;
  fml::UniqueFD timer_fd_;
  bool running_;

  MessageLoopLinux();

  ~MessageLoopLinux() override;

  // |fml::MessageLoopImpl|
  void Run() override;

  // |fml::MessageLoopImpl|
  void Terminate() override;

  // |fml::MessageLoopImpl|
  void WakeUp(fml::TimePoint time_point) override;

  void OnEventFired();

  bool AddOrRemoveTimerSource(bool add);

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopLinux);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopLinux);
  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoopLinux);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoopLinux;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_PLATFORM_LINUX_MESSAGE_LOOP_LINUX_H_
