// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/linux/message_loop_linux.h"

#include <sys/epoll.h>
#include <unistd.h>

#include "base/include/fml/eintr_wrapper.h"
#include "base/include/fml/platform/linux/timerfd.h"

namespace lynx {
namespace fml {

fml::RefPtr<MessageLoopImpl> MessageLoopImpl::Create(void* platform_loop) {
  return fml::MakeRefCounted<MessageLoopLinux>();
}

static constexpr int kClockType = CLOCK_MONOTONIC;

MessageLoopLinux::MessageLoopLinux()
    : epoll_fd_(FML_HANDLE_EINTR(::epoll_create(1 /* unused */))),
      timer_fd_(::timerfd_create(kClockType, TFD_NONBLOCK | TFD_CLOEXEC)),
      running_(false) {
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(epoll_fd_.is_valid());
  LYNX_BASE_CHECK(timer_fd_.is_valid());
  [[maybe_unused]] bool added_source = AddOrRemoveTimerSource(true);
  LYNX_BASE_CHECK(added_source);
}

MessageLoopLinux::~MessageLoopLinux() {
  [[maybe_unused]] bool removed_source = AddOrRemoveTimerSource(false);
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(removed_source);
}

bool MessageLoopLinux::AddOrRemoveTimerSource(bool add) {
  struct epoll_event event = {};

  event.events = EPOLLIN;
  // The data is just for informational purposes so we know when we were worken
  // by the FD.
  event.data.fd = timer_fd_.get();

  int ctl_result =
      ::epoll_ctl(epoll_fd_.get(), add ? EPOLL_CTL_ADD : EPOLL_CTL_DEL,
                  timer_fd_.get(), &event);
  return ctl_result == 0;
}

// |fml::MessageLoopImpl|
void MessageLoopLinux::Run() {
  running_ = true;

  while (running_) {
    struct epoll_event event = {};

    int epoll_result = FML_HANDLE_EINTR(
        ::epoll_wait(epoll_fd_.get(), &event, 1, -1 /* timeout */));

    // Errors are fatal.
    if (event.events & (EPOLLERR | EPOLLHUP)) {
      running_ = false;
      continue;
    }

    // Timeouts are fatal since we specified an infinite timeout already.
    // Likewise, > 1 is not possible since we waited for one result.
    if (epoll_result != 1) {
      running_ = false;
      continue;
    }

    if (event.data.fd == timer_fd_.get()) {
      OnEventFired();
    }
  }
}

// |fml::MessageLoopImpl|
void MessageLoopLinux::Terminate() {
  running_ = false;
  WakeUp(fml::TimePoint::Now());
}

// |fml::MessageLoopImpl|
void MessageLoopLinux::WakeUp(fml::TimePoint time_point) {
  bool result = TimerRearm(timer_fd_.get(), time_point);
  (void)result;
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(result);
}

void MessageLoopLinux::OnEventFired() {
  if (TimerDrain(timer_fd_.get())) {
    RunExpiredTasksNow();
  }
}

}  // namespace fml
}  // namespace lynx
