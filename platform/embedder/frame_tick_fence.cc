// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/frame_tick_fence.h"

#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace embedder {

struct FrameTickFence::State {
  State(uint64_t frame_id, int64_t frame_start_time_ns,
        CompleteCallback complete_callback)
      : frame_id(frame_id),
        frame_start_time_ns(frame_start_time_ns),
        complete_callback(std::move(complete_callback)) {}

  bool Acquire() {
    std::lock_guard<std::mutex> lock(mutex);
    if (sealed) {
      return false;
    }
    ++pending;
    return true;
  }

  void CompleteWork() {
    CompleteCallback callback;
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (pending == 0) {
        return;
      }
      --pending;
      if (!sealed || pending != 0) {
        return;
      }
      completed = true;
      callback = complete_callback;
    }
    if (callback) {
      callback(frame_id, frame_start_time_ns);
    }
  }

  void Seal() {
    CompleteCallback callback;
    {
      std::lock_guard<std::mutex> lock(mutex);
      sealed = true;
      if (pending != 0 || completed) {
        return;
      }
      completed = true;
      callback = complete_callback;
    }
    if (callback) {
      callback(frame_id, frame_start_time_ns);
    }
  }

  bool IsCompleted() {
    std::lock_guard<std::mutex> lock(mutex);
    return completed;
  }

  std::mutex mutex;
  const uint64_t frame_id;
  const int64_t frame_start_time_ns;
  uint32_t pending = 0;
  bool sealed = false;
  bool completed = false;
  CompleteCallback complete_callback;
};

FrameTickFence::Work::~Work() { Complete(); }

void FrameTickFence::Work::Complete() {
  auto state = std::move(state_);
  if (state) {
    state->CompleteWork();
  }
}

void FrameTickFence::SetCompleteCallback(CompleteCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  complete_callback_ = std::move(callback);
}

void FrameTickFence::Begin(uint64_t frame_id, int64_t frame_start_time_ns) {
  std::lock_guard<std::mutex> lock(mutex_);
  DCHECK(!current_ || current_->IsCompleted());
  current_ = std::make_shared<State>(frame_id, frame_start_time_ns,
                                     complete_callback_);
}

std::shared_ptr<base::VSyncFrameTickWork> FrameTickFence::AcquireWork(
    int64_t frame_start_time_ns, const char* /*source*/) {
  std::shared_ptr<State> state;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    state = current_;
  }
  if (!state || state->frame_start_time_ns != frame_start_time_ns ||
      !state->Acquire()) {
    return nullptr;
  }
  return std::shared_ptr<Work>(new Work(std::move(state)));
}

void FrameTickFence::Seal(int64_t frame_start_time_ns) {
  std::shared_ptr<State> state;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    state = current_;
  }
  if (state && state->frame_start_time_ns == frame_start_time_ns) {
    state->Seal();
  }
}

}  // namespace embedder
}  // namespace lynx
