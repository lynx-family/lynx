// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_APPLE_GL_FENCE_SYNC_H_
#define CLAY_GFX_SHARED_IMAGE_APPLE_GL_FENCE_SYNC_H_

#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <utility>

#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

// Included after the platform GL headers. A private shared context keeps sync
// objects queryable after the producer representation/context is released.
// Never bind the caller's producer context on a different thread.
inline bool HasAppleGLFenceExtension(const char* name) {
  const char* extensions =
      reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  if (!extensions) return false;
  const size_t length = std::strlen(name);
  for (const char* match = extensions; (match = std::strstr(match, name));
       match += length) {
    if ((match == extensions || match[-1] == ' ') &&
        (match[length] == ' ' || match[length] == '\0'))
      return true;
  }
  return false;
}

inline bool ClearAppleGLFenceErrors() {
  for (int i = 0; i < 16; ++i) {
    const auto error = glGetError();
    if (error == GL_NO_ERROR) return true;
    // GL_CONTEXT_LOST (also used by KHR_robustness) is not in older Apple
    // headers.
    if (error == GL_OUT_OF_MEMORY || error == 0x0507) return false;
  }
  return false;
}

template <class Context>
class AppleGLFenceSync final : public FenceSync {
 public:
  explicit AppleGLFenceSync(std::shared_ptr<Context> context,
                            bool allow_blocking_fallback = false)
      : context_(std::move(context)) {
    unsupported_ = context_ && !context_->supported();
    if (allow_blocking_fallback &&
        (unsupported_ || (context_ && !context_->valid()))) {
      completed_ = Finish();
      unsupported_ = false;
      return;
    }
    if (unsupported_) return;
    if (!context_ || !context_->valid() || !ClearAppleGLFenceErrors()) return;
    sync_ = context_->CreateSync();
    const auto error = glGetError();
    if (error != GL_NO_ERROR || !sync_) {
      if (allow_blocking_fallback && error != GL_OUT_OF_MEMORY &&
          error != 0x0507)
        completed_ = Finish();
      return;
    }
    glFlush();
    failed_ = glGetError() != GL_NO_ERROR;
  }

  ~AppleGLFenceSync() override {
    if (!sync_) return;
    std::lock_guard<std::mutex> lock(context_->mutex);
    typename Context::ScopedCurrent current(context_.get());
    if (current.valid()) context_->DeleteSync(sync_);
  }

  Type GetType() const override { return Context::kType; }

  PollResult Poll() override {
    if (completed_) return PollResult::kSignaled;
    if (unsupported_) return PollResult::kUnsupported;
    if (failed_) return PollResult::kFailed;
    // A legacy wait or another fence's query must not turn Poll into a wait.
    std::unique_lock<std::mutex> lock(context_->mutex, std::try_to_lock);
    if (!lock.owns_lock()) return PollResult::kPending;
    return WaitInQueryContext(0);
  }

  bool ClientWait() override {
    if (completed_) return true;
    if (unsupported_ || failed_) return false;
    // Let the driver wake us on completion. Bound each use of the shared
    // query context so other fences can also make progress.
    for (;;) {
      std::lock_guard<std::mutex> lock(context_->mutex);
      const auto result = WaitInQueryContext(1000000);  // 1 ms in nanoseconds.
      if (result != PollResult::kPending)
        return result == PollResult::kSignaled;
    }
  }

  WaitResult EnqueueWaitResult() override {
    if (completed_) return WaitResult::kSuccess;
    if (unsupported_) return WaitResult::kUnsupported;
    if (failed_) return WaitResult::kFailed;
    // The wait must go into the actual consumer stream, not the query context.
    if (!context_->IsCurrentCompatible()) return WaitResult::kUnsupported;
    if (!ClearAppleGLFenceErrors()) return WaitResult::kFailed;
    context_->WaitSync(sync_);
    return glGetError() == GL_NO_ERROR ? WaitResult::kSuccess
                                       : WaitResult::kFailed;
  }

 private:
  PollResult WaitInQueryContext(uint64_t timeout_ns) {
    typename Context::ScopedCurrent current(context_.get());
    if (!current.valid()) return PollResult::kFailed;
    const auto result = context_->ClientWaitSync(sync_, timeout_ns);
    if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
      return PollResult::kSignaled;
    if (result == GL_TIMEOUT_EXPIRED) return PollResult::kPending;
    failed_ = true;
    return PollResult::kFailed;
  }

  static bool Finish() {
    if (!ClearAppleGLFenceErrors()) return false;
    glFinish();
    return glGetError() == GL_NO_ERROR;
  }
  bool completed_ = false;
  std::shared_ptr<Context> context_;
  GLsync sync_ = nullptr;
  bool unsupported_ = false;
  std::atomic<bool> failed_{true};
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_APPLE_GL_FENCE_SYNC_H_
