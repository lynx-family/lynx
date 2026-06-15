// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/node/message_loop_node.h"

#include <utility>

namespace lynx {
namespace fml {

MessageLoopNode::MessageLoopNode() {
  [[maybe_unused]] int r = uv_loop_init(&uv_loop_);
  LYNX_BASE_CHECK(r == 0);

  [[maybe_unused]] int timer_init = uv_timer_init(&uv_loop_, &timer_);
  LYNX_BASE_CHECK(timer_init == 0);
  timer_.data = this;

  [[maybe_unused]] int async_init =
      uv_async_init(&uv_loop_, &async_, [](uv_async_t* h) {
        auto* self = static_cast<MessageLoopNode*>(h->data);
        self->OnEventFired();
      });
  async_.data = this;

  LYNX_BASE_CHECK(async_init == 0);
}

MessageLoopNode::~MessageLoopNode() {
  if (running_) {
    Terminate();
  }

  uv_walk(
      &uv_loop_,
      [](uv_handle_t* h, void*) {
        if (!uv_is_closing(h)) {
          uv_close(h, nullptr);
        }
      },
      nullptr);

  RunUV(UV_RUN_NOWAIT);
  uv_loop_close(&uv_loop_);
}

void MessageLoopNode::Run() {
  running_ = true;
  while (running_) {
    int result = RunUV(UV_RUN_ONCE);
    if (result == 0) {
      break;
    }
  }
  running_ = false;
}

void MessageLoopNode::Terminate() {
  running_ = false;

  int stop_res = uv_timer_stop(&timer_);
  uv_close(reinterpret_cast<uv_handle_t*>(&timer_), nullptr);
  uv_close(reinterpret_cast<uv_handle_t*>(&async_), nullptr);
  RunUV(UV_RUN_NOWAIT);

  uv_stop(&uv_loop_);
}

void MessageLoopNode::SetUVRunCallback(UVRunCallback callback) {
  uv_run_callback_ = std::move(callback);
}

int MessageLoopNode::RunUV(uv_run_mode mode) {
  if (uv_run_callback_) {
    return uv_run_callback_(&uv_loop_, mode);
  }
  return uv_run(&uv_loop_, mode);
}

void MessageLoopNode::WakeUp(fml::TimePoint time_point) {
  auto now = fml::TimePoint::Now();
  if (now >= time_point) {
    uv_async_send(&async_);
    return;
  }

  int start_res = uv_timer_start(
      &timer_,
      [](uv_timer_t* handle) {
        auto* self = reinterpret_cast<MessageLoopNode*>(handle->data);
        self->OnEventFired();
      },
      (time_point - now).ToMilliseconds(), 0);
  LYNX_BASE_CHECK(start_res == 0);
}

void MessageLoopNode::OnEventFired() { RunExpiredTasksNow(); }

}  // namespace fml
}  // namespace lynx
