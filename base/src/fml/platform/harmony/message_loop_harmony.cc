// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/harmony/message_loop_harmony.h"

#include <fcntl.h>
#include <uv.h>

#include "base/include/fml/platform/linux/timerfd.h"

// temporarily workaround to compile without logging
#undef LOGE
#undef DCHECK
#define LOGE(...)
#define LOGI(...)
#define DCHECK(...)

namespace lynx {
namespace fml {

fml::RefPtr<MessageLoopImpl> MessageLoopImpl::Create(void* platform_loop) {
  return fml::MakeRefCounted<MessageLoopHarmony>(platform_loop);
}

static constexpr int kClockType = CLOCK_MONOTONIC;

MessageLoopHarmony::MessageLoopHarmony(void* platform_loop)
    : timer_fd_(timerfd_create(kClockType, TFD_NONBLOCK | TFD_CLOEXEC)),
      running_(false) {
  // If the platform_loop is not null, use it to initialize the looper_
  if (platform_loop != nullptr) {
    looper_ = reinterpret_cast<uv_loop_t*>(platform_loop);
    is_looper_owner_ = false;
  } else {
    looper_ = uv_loop_new();
    is_looper_owner_ = true;
  }
  // Harmony Developer Beta1 Canary3 break
  uv_async_send(&looper_->wq_async);
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(timer_fd_.is_valid());

  auto read_event_fd = [](uv_poll_t* handle, int status, int events) {
    if (events & UV_READABLE) {
      reinterpret_cast<MessageLoopHarmony*>(handle->data)->OnEventFired();
    }
  };

  [[maybe_unused]] int init_result =
      uv_poll_init(looper_, &poll_, timer_fd_.get());
  poll_.data = this;
  LYNX_BASE_CHECK(init_result == 0);
  [[maybe_unused]] int start_result =
      uv_poll_start(&poll_, UV_READABLE, read_event_fd);
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(start_result == 0);
}

MessageLoopHarmony::~MessageLoopHarmony() {}

void MessageLoopHarmony::Run() {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  running_ = true;
  uv_run(looper_, UV_RUN_DEFAULT);

  if (is_looper_owner_) {
    uv_loop_delete(looper_);
    LOGI("MessageLoopHarmony::Run, this:" << this << ", delete uv_loop "
                                          << looper_);
  }
  looper_ = nullptr;
}

void MessageLoopHarmony::Terminate() {
  running_ = false;
  [[maybe_unused]] int stop = uv_poll_stop(&poll_);
  uv_close(reinterpret_cast<uv_handle_t*>(&poll_), NULL);
  LOGI("MessageLoopHarmony::Terminate, this:"
       << this << ", uv_poll_stop result:" << stop);
}

void MessageLoopHarmony::WakeUp(fml::TimePoint time_point) {
  [[maybe_unused]] bool result = TimerRearm(timer_fd_.get(), time_point);
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(result);
}

void MessageLoopHarmony::OnEventFired() {
  if (TimerDrain(timer_fd_.get())) {
    RunExpiredTasksNow();
  }
}

}  // namespace fml
}  // namespace lynx
