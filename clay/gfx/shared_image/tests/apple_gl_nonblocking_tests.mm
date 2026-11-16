// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <TargetConditionals.h>

#if TARGET_OS_IPHONE
#import <OpenGLES/ES3/gl.h>
#else
#import <OpenGL/gl3.h>
#endif

#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>

namespace {
std::atomic<int> flushes{0};
int finishes = 0;
void TestFinish() { ++finishes; }
GLenum TestGetError() { return GL_NO_ERROR; }
void TestFlush() { ++flushes; }
}  // namespace

// Inject only the GL boundary. The production fence state machine is real.
#define glGetError TestGetError
#define glFlush TestFlush
#define glFinish TestFinish
#include "clay/gfx/shared_image/apple_gl_fence_sync.h"
#undef glGetError
#undef glFlush
#undef glFinish

#define CHECK(x)                                           \
  do {                                                     \
    if (!(x)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); \
      std::abort();                                        \
    }                                                      \
  } while (0)

namespace {
struct Context {
  static constexpr clay::FenceSync::Type kType = clay::FenceSync::Type::kCGL;
  bool available = true, valid_context = true, compatible = true;
  std::atomic<GLenum> status{GL_TIMEOUT_EXPIRED};
  std::atomic<int> polls{0}, deletes{0}, gpu_waits{0};
  std::mutex mutex;
  bool supported() const { return available; }
  bool valid() const { return valid_context; }
  bool IsCurrentCompatible() const { return compatible; }
  GLsync CreateSync() { return reinterpret_cast<GLsync>(1); }
  void DeleteSync(GLsync) { ++deletes; }
  GLenum PollSync(GLsync) {
    ++polls;
    return status;
  }
  void WaitSync(GLsync) { ++gpu_waits; }
  struct ScopedCurrent {
    explicit ScopedCurrent(Context*) {}
    bool valid() const { return true; }
  };
};
using Fence = clay::AppleGLFenceSync<Context>;
using Result = clay::FenceSync::PollResult;
}  // namespace

int main() {
  alarm(10);
  auto context = std::make_shared<Context>();
  {
    Fence fence(context);
    CHECK(flushes == 1 && context->polls == 0);
    CHECK(fence.Poll() == Result::kPending);
    CHECK(fence.EnqueueWait() && context->gpu_waits == 1);
    CHECK(fence.Poll() == Result::kPending);  // GPU order is not completion.
    context->compatible = false;
    CHECK(fence.EnqueueWaitResult() == clay::FenceSync::WaitResult::kUnsupported);
    std::atomic<bool> locked{false}, release{false};
    std::thread holder([&] {
      std::lock_guard<std::mutex> lock(context->mutex);
      locked = true;
      while (!release) std::this_thread::yield();
    });
    while (!locked) std::this_thread::yield();
    const int polls = context->polls;
    CHECK(fence.Poll() == Result::kPending && context->polls == polls);
    release = true;
    holder.join();
    std::atomic<bool> completed{false};
    std::thread waiter([&] { completed = fence.ClientWait(); });
    while (context->polls == polls) std::this_thread::yield();
    CHECK(!completed && fence.Poll() == Result::kPending);
    context->status = GL_CONDITION_SATISFIED;
    waiter.join();
    CHECK(completed && fence.Poll() == Result::kSignaled);
  }
  CHECK(context->deletes == 1);
  context->status = GL_TIMEOUT_EXPIRED;
  { Fence pending(context); }
  CHECK(context->deletes == 2);  // Destruction does not wait for pending work.
  context->available = false;
  {
    Fence unsupported(context);
    CHECK(unsupported.Poll() == Result::kUnsupported);
  }
  CHECK(finishes == 0);
  {
    Fence legacy(context, true);
    CHECK(finishes == 1 && legacy.ClientWait());
    CHECK(legacy.Poll() == Result::kSignaled && legacy.EnqueueWait());
    CHECK(finishes == 1);
  }
  context->available = true;
  context->valid_context = false;
  {
    Fence failed(context);
    CHECK(failed.Poll() == Result::kFailed);
  }
  context->valid_context = true;
  context->status = GL_WAIT_FAILED;
  Fence failed(context);
  CHECK(failed.Poll() == Result::kFailed);
  context->status = GL_ALREADY_SIGNALED;
  CHECK(failed.Poll() == Result::kFailed && !failed.EnqueueWait());
  std::puts("PASS Apple GL: pending GPU order, zero-timeout poll, mutex contention, concurrent "
            "legacy wait, pending destruction, unsupported and sticky failure");
}
