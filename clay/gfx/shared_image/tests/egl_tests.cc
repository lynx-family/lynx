// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/shared_image/android_hardwarebuffer_image_backing.h"
#include "clay/gfx/shared_image/egl_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"
#include "clay/gfx/shared_image/tests/api_contract.h"

#define FENCE_CHECK(x)                                     \
  do {                                                     \
    if (!(x)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); \
      std::abort();                                        \
    }                                                      \
  } while (0)
namespace {
EGLDisplay current_display = EGL_NO_DISPLAY;
EGLContext current_context = EGL_NO_CONTEXT;
const char* extensions = "EGL_KHR_fence_sync EGL_KHR_wait_sync";
bool native_proc = false, create_fails = false;
const char* egl_version = "1.4";
bool core_wait_proc = true;
EGLint poll_result = EGL_TIMEOUT_EXPIRED_KHR;
GLenum finish_error = GL_NO_ERROR;
std::deque<GLenum> gl_errors;
GLenum persistent_error = GL_NO_ERROR;
int finishes = 0, client_waits = 0, gpu_waits = 0, creates = 0;
EGLDisplay last_display = EGL_NO_DISPLAY;
EGLenum last_type = 0;
std::mutex wait_mutex;
std::condition_variable wait_cv;
bool block_wait = false, wait_entered = false, release_wait = false;
EGLSyncKHR Create(EGLDisplay display, EGLenum type, const EGLint*) {
  ++creates;
  last_display = display;
  last_type = type;
  return create_fails ? EGL_NO_SYNC_KHR : reinterpret_cast<EGLSyncKHR>(123);
}
EGLBoolean Destroy(EGLDisplay display, EGLSyncKHR) {
  last_display = display;
  return EGL_TRUE;
}
EGLint ClientWait(EGLDisplay display, EGLSyncKHR, EGLint flags,
                  EGLTimeKHR timeout) {
  if (timeout && block_wait) {
    std::unique_lock<std::mutex> lock(wait_mutex);
    wait_entered = true;
    wait_cv.notify_all();
    wait_cv.wait(lock, [] { return release_wait; });
    return EGL_CONDITION_SATISFIED_KHR;
  }
  ++client_waits;
  last_display = display;
  FENCE_CHECK(flags == 0 && (timeout == 0 || timeout == EGL_FOREVER_KHR));
  return poll_result;
}
EGLBoolean Wait(EGLDisplay display, EGLSyncKHR, EGLint) {
  ++gpu_waits;
  last_display = display;
  return EGL_TRUE;
}
EGLint Dup(EGLDisplay display, EGLSyncKHR) {
  last_display = display;
  int descriptors[2];
  FENCE_CHECK(pipe(descriptors) == 0);
  close(descriptors[1]);
  return descriptors[0];
}
ClayFenceSyncRef Ref(clay::FenceSync* fence) {
  return reinterpret_cast<ClayFenceSyncRef>(fence);
}
class FailedFence final : public clay::FenceSync {
 public:
  bool ClientWait() override { return false; }
  Type GetType() const override { return Type::kClientWaitOnly; }
};

void CheckFinishFallback(bool completed) {
  clay::AndroidEGLFenceSync fence;
  FENCE_CHECK(testFencePoll(Ref(&fence)) ==
              (completed ? kClayFenceSyncSignaled : kClayFenceSyncFailed));
  FENCE_CHECK(fence.ClientWait() == completed);
  FENCE_CHECK(testFenceServerWait(Ref(&fence)) == completed);
  int fd = 123;
  FENCE_CHECK(testFenceExportFD(Ref(&fence), &fd) ==
              (completed ? kClayFenceSyncSignaled : kClayFenceSyncFailed));
  FENCE_CHECK(fd == -1);
}

void TestFinishFallback() {
  // Exercise both absent extensions and a driver failing to create a fence.
  for (bool fail_creation : {false, true}) {
    extensions = fail_creation ? "EGL_KHR_fence_sync EGL_KHR_wait_sync" : "";
    create_fails = fail_creation;
    finish_error = GL_NO_ERROR;
    gl_errors = {GL_INVALID_ENUM, GL_INVALID_VALUE, GL_INVALID_OPERATION};
    int previous_finishes = finishes;
    CheckFinishFallback(true);
    FENCE_CHECK(finishes == previous_finishes + 1 && gl_errors.empty());

    for (GLenum error : {GL_CONTEXT_LOST_KHR, GL_OUT_OF_MEMORY}) {
      gl_errors = {GL_INVALID_ENUM, error};
      previous_finishes = finishes;
      CheckFinishFallback(false);
      FENCE_CHECK(finishes == previous_finishes);
    }

    // Errors raised by glFinish itself must never publish a ready fence.
    for (GLenum error :
         {GL_INVALID_OPERATION, GL_CONTEXT_LOST_KHR, GL_OUT_OF_MEMORY}) {
      gl_errors = {GL_INVALID_ENUM};
      finish_error = error;
      previous_finishes = finishes;
      CheckFinishFallback(false);
      FENCE_CHECK(finishes == previous_finishes + 1);
    }

    finish_error = GL_NO_ERROR;
    persistent_error = GL_INVALID_OPERATION;
    previous_finishes = finishes;
    CheckFinishFallback(false);
    FENCE_CHECK(finishes == previous_finishes);
    persistent_error = GL_NO_ERROR;

    // Without a current display/context there is no work we can finish.
    EGLDisplay saved_display = current_display;
    current_display = EGL_NO_DISPLAY;
    CheckFinishFallback(false);
    current_display = saved_display;
    EGLContext saved_context = current_context;
    current_context = EGL_NO_CONTEXT;
    CheckFinishFallback(false);
    current_context = saved_context;
    FENCE_CHECK(finishes == previous_finishes);
  }
  std::puts(
      "PASS EGL glFinish fallback: stale errors ignored, context loss "
      "and new errors fail closed, persistent errors terminate");
}

void TestConcurrentClientWait() {
  create_fails = false;
  finish_error = GL_NO_ERROR;
  extensions =
      "EGL_KHR_fence_sync EGL_KHR_wait_sync EGL_ANDROID_native_fence_sync";
  poll_result = EGL_TIMEOUT_EXPIRED_KHR;
  clay::AndroidEGLFenceSync fence;
  const auto ref = Ref(&fence);
  const int finished = finishes;
  block_wait = true;
  std::atomic<bool> completed{false};
  std::thread waiter([&] {
    FENCE_CHECK(ClayFenceSyncClientWait(ref));
    completed = true;
  });
  {
    std::unique_lock<std::mutex> lock(wait_mutex);
    wait_cv.wait(lock, [] { return wait_entered; });
  }
  // The driver has entered an infinite client wait. None of these public
  // operations may wait for its completion or invoke a completion fallback.
  FENCE_CHECK(testFencePoll(ref) == kClayFenceSyncPending);
  FENCE_CHECK(testFenceServerWait(ref));
  int fd = 123;
  FENCE_CHECK(
      testFenceExportFD(ref, &fd) ==
      (native_proc ? kClayFenceSyncSuccess : kClayFenceSyncUnsupported));
  if (native_proc)
    close(fd);
  else
    FENCE_CHECK(fd == -1);
  FENCE_CHECK(!completed && finishes == finished);
  {
    std::lock_guard<std::mutex> lock(wait_mutex);
    release_wait = true;
    wait_cv.notify_all();
  }
  waiter.join();
  block_wait = false;
  poll_result = EGL_CONDITION_SATISFIED_KHR;
  FENCE_CHECK(completed && testFencePoll(ref) == kClayFenceSyncSignaled);
  std::puts(
      "PASS EGL concurrent ClientWait: public poll/wait/export return before "
      "release");
}

void TestNonblockingCreation() {
  const int previous_finishes = finishes;
  const ClayFenceSyncWaitContext context{sizeof(context),
                                         kClayFenceSyncWaitCurrentGL, nullptr};
  for (bool fail_creation : {false, true}) {
    extensions = fail_creation ? "EGL_KHR_fence_sync" : "";
    create_fails = fail_creation;
    clay::AndroidEGLFenceSync fence(
        clay::AndroidEGLFenceSync::CreationMode::kNonblocking);
    FENCE_CHECK(fence.Poll() ==
                (fail_creation ? clay::FenceSync::PollResult::kFailed
                               : clay::FenceSync::PollResult::kUnsupported));
    FENCE_CHECK(!fence.EnqueueWait());
    FENCE_CHECK(
        ClayFenceSyncServerWait(Ref(&fence), &context) ==
        (fail_creation ? kClayFenceSyncFailed : kClayFenceSyncUnsupported));
    FENCE_CHECK(finishes == previous_finishes);
  }
  create_fails = false;
  extensions = "EGL_KHR_fence_sync";
  poll_result = EGL_TIMEOUT_EXPIRED_KHR;
  clay::AndroidEGLFenceSync fence(
      clay::AndroidEGLFenceSync::CreationMode::kNonblocking);
  FENCE_CHECK(fence.Poll() == clay::FenceSync::PollResult::kPending);
  FENCE_CHECK(!fence.EnqueueWait());
  FENCE_CHECK(ClayFenceSyncServerWait(Ref(&fence), &context) ==
              kClayFenceSyncUnsupported);
  poll_result = EGL_CONDITION_SATISFIED_KHR;
  FENCE_CHECK(fence.Poll() == clay::FenceSync::PollResult::kSignaled);
  FENCE_CHECK(finishes == previous_finishes);
}

auto MakeSink(clay::SharedImageSink::BufferMode mode) {
  auto sink = fml::MakeRefCounted<clay::SharedImageSinkManaged>(
      mode, [](skity::Vec2 size, auto) {
        return fml::MakeRefCounted<clay::EGLImageBacking>(
            clay::SharedImageBacking::PixelFormat::kNative8888, size);
      });
  sink->SetFrameAvailableCallback([] {});
  return sink;
}

fml::RefPtr<clay::SharedImageRepresentation> MakeRepresentation(
    fml::RefPtr<clay::SharedImageBacking> backing) {
  return fml::MakeRefCounted<clay::NativeBufferEGLImageRepresentation>(
      fml::Ref(static_cast<clay::EGLImageBacking*>(backing.get())));
}

void CheckReturnedFailure(clay::SharedImageBacking* backing,
                          clay::FenceSync* identity) {
  auto fence = backing->GetFenceSync();
  FENCE_CHECK(fence.get() == identity && !fence->ClientWait());
  backing->SetFenceSync(std::move(fence));
}

void TestFailedReadRelease(bool destroy_accessor) {
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  auto [back, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  FENCE_CHECK(sink->SwapBack(std::move(failure)));
  {
    clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
    auto front = reader.UpdateFront();
    FENCE_CHECK(front && front->GetBacking() == back.get());
    ClaySharedImageReadResult read{};
    FENCE_CHECK(!front->BeginRead(&read));
    if (!destroy_accessor) {
      reader.ReleaseFront();
      reader.ReleaseFront();
      auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
      FENCE_CHECK(returned == back);
      FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
      CheckReturnedFailure(returned.get(), identity);
    }
  }
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == back);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  CheckReturnedFailure(returned.get(), identity);
  clay::SharedImageSinkAccessor writer(sink, MakeRepresentation);
  auto [repr, writer_age] = writer.AcquireBack({1, 1});
  FENCE_CHECK(repr);
  ClaySharedImageWriteResult write{};
  FENCE_CHECK(!repr->BeginWrite(&write));
  FENCE_CHECK(!writer.SwapBack());
  CheckReturnedFailure(back.get(), identity);
}

void TestFailedReadAdvance() {
  auto sink = MakeSink(clay::SharedImageSink::kDoubleBuffer);
  auto [first, first_age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  FENCE_CHECK(sink->SwapBack(std::move(failure)));
  auto [second, second_age] = sink->AcquireBack({1, 1});
  FENCE_CHECK(second && second != first);
  FENCE_CHECK(sink->SwapBack(std::make_unique<FailedFence>()));
  clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
  auto front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == first.get());
  ClaySharedImageReadResult read{};
  FENCE_CHECK(!front->BeginRead(&read));
  front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == second.get());
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == first);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  CheckReturnedFailure(returned.get(), identity);
}

void TestFailedWriteRelease(bool destroy_accessor) {
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  auto [back, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  back->SetFenceSync(std::move(failure));
  {
    clay::SharedImageSinkAccessor writer(sink, MakeRepresentation);
    auto [repr, writer_age] = writer.AcquireBack({1, 1});
    FENCE_CHECK(repr && repr->GetBacking() == back.get());
    ClaySharedImageWriteResult write{};
    FENCE_CHECK(!repr->BeginWrite(&write));
    if (!destroy_accessor) {
      FENCE_CHECK(!writer.SwapBack());
      CheckReturnedFailure(back.get(), identity);
      FENCE_CHECK(!writer.SwapBack());
      auto [unavailable, unavailable_age, status] =
          sink->TryAcquireBack({1, 1});
      FENCE_CHECK(!unavailable);
      FENCE_CHECK(status ==
                  clay::SharedImageSink::AcquireBackStatus::kFullAttach);
    }
  }
  clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
  auto front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == back.get());
  ClaySharedImageReadResult read{};
  FENCE_CHECK(!front->BeginRead(&read));
  reader.ReleaseFront();
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == back);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  CheckReturnedFailure(returned.get(), identity);
}
}  // namespace

// Only the driver and backing allocation are simulated. Fence/representation
// behavior and all public API dispatch below use the production source units.
extern "C" EGLDisplay eglGetCurrentDisplay() { return current_display; }
extern "C" EGLContext eglGetCurrentContext() { return current_context; }
extern "C" const char* eglQueryString(EGLDisplay display, EGLint name) {
  return display ? (name == EGL_VERSION ? egl_version : extensions) : nullptr;
}
extern "C" EGLint eglGetError() { return EGL_SUCCESS; }
extern "C" void glFinish() {
  ++finishes;
  if (finish_error != GL_NO_ERROR) gl_errors.push_back(finish_error);
}
extern "C" void glFlush() {}
extern "C" GLenum glGetError() {
  if (gl_errors.empty()) return persistent_error;
  GLenum error = gl_errors.front();
  gl_errors.pop_front();
  return error;
}
extern "C" __eglMustCastToProperFunctionPointerType eglGetProcAddress(
    const char* name) {
#define PROC(n, f)           \
  if (!std::strcmp(name, n)) \
  return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(f)
  PROC("eglCreateSyncKHR", Create);
  PROC("eglDestroySyncKHR", Destroy);
  PROC("eglClientWaitSyncKHR", ClientWait);
  PROC("eglWaitSyncKHR", Wait);
  if (core_wait_proc) {
    PROC("eglWaitSync", Wait);
  }
  if (native_proc) {
    PROC("eglDupNativeFenceFDANDROID", Dup);
  }
#undef PROC
  return nullptr;
}
namespace clay {
EGLImageBacking::EGLImageBacking(PixelFormat format, skity::Vec2 size,
                                 std::optional<GraphicsMemoryHandle>)
    : SharedImageBacking(format, size) {}
EGLImageBacking::~EGLImageBacking() {
  FENCE_CHECK(egl_display_ == EGL_NO_DISPLAY &&
              egl_context_ == EGL_NO_CONTEXT && texture_id_ == 0);
}
void EGLImageBacking::BindToTexture(GLenum) { std::abort(); }
SharedImageBacking::BackingType EGLImageBacking::GetType() const {
  return BackingType::kEGLImage;
}
fml::RefPtr<SharedImageRepresentation> EGLImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig*) {
  return nullptr;
}

EGLImageKHR AHardwareBufferImageBacking::CreateEGLImage(EGLDisplay) {
  std::abort();
}
}  // namespace clay

int main(int argc, char**) {
  alarm(20);
  native_proc = argc > 1;  // Separate processes exercise optional-proc absence.
  FENCE_CHECK(testFencePublicContract());
  FENCE_CHECK(!clay::AndroidEGLFenceSync::CheckEGLFenceSupported());
  FENCE_CHECK(!clay::AndroidEGLFenceSync::CheckAndroidNativeFenceSupported());
  current_display = reinterpret_cast<EGLDisplay>(1);
  current_context = reinterpret_cast<EGLContext>(2);
  FENCE_CHECK(clay::AndroidEGLFenceSync::CheckEGLFenceSupported());
  FENCE_CHECK(!clay::AndroidEGLFenceSync::CheckAndroidNativeFenceSupported());
  {
    clay::AndroidEGLFenceSync fence;
    FENCE_CHECK(last_type == EGL_SYNC_FENCE_KHR && finishes == 0);
    FENCE_CHECK(testFencePoll(Ref(&fence)) == kClayFenceSyncPending);
    FENCE_CHECK(testFenceServerWait(Ref(&fence)) && gpu_waits == 1);
    int fd = 123;
    FENCE_CHECK(testFenceExportFD(Ref(&fence), &fd) ==
                    kClayFenceSyncUnsupported &&
                fd == -1);
    current_display = reinterpret_cast<EGLDisplay>(3);
    const int waits = client_waits;
    FENCE_CHECK(!testFenceServerWait(Ref(&fence)) && client_waits == waits);
    FENCE_CHECK(testFencePoll(Ref(&fence)) == kClayFenceSyncPending &&
                last_display == reinterpret_cast<EGLDisplay>(1));
    current_display = reinterpret_cast<EGLDisplay>(1);
    poll_result = EGL_CONDITION_SATISFIED_KHR;
    FENCE_CHECK(testFencePoll(Ref(&fence)) == kClayFenceSyncSignaled);
  }
  {
    extensions = "EGL_KHR_fence_sync";
    clay::AndroidEGLFenceSync fence(
        clay::AndroidEGLFenceSync::CreationMode::kNonblocking);
    const int waits = client_waits, finished = finishes, gpu = gpu_waits;
    FENCE_CHECK(!testFenceServerWait(Ref(&fence)));
    egl_version = "1.5";
    FENCE_CHECK(testFenceServerWait(Ref(&fence)) && gpu_waits == gpu + 1);
    core_wait_proc = false;
    FENCE_CHECK(!testFenceServerWait(Ref(&fence)));
    FENCE_CHECK(client_waits == waits && finishes == finished);
    core_wait_proc = true;
    egl_version = "1.4";
  }
  extensions =
      "EGL_KHR_fence_sync EGL_KHR_wait_sync "
      "EGL_ANDROID_native_fence_sync_extra";
  FENCE_CHECK(!clay::AndroidEGLFenceSync::CheckAndroidNativeFenceSupported());
  extensions =
      "EGL_KHR_fence_sync EGL_KHR_wait_sync EGL_ANDROID_native_fence_sync";
  FENCE_CHECK(clay::AndroidEGLFenceSync::CheckAndroidNativeFenceSupported() ==
              native_proc);
  if (native_proc) {
    clay::AndroidEGLFenceSync fence;
    FENCE_CHECK(last_type == EGL_SYNC_NATIVE_FENCE_ANDROID);
    int first = -1, second = -1;
    FENCE_CHECK(testFenceExportFD(Ref(&fence), &first) ==
                kClayFenceSyncSuccess);
    FENCE_CHECK(testFenceExportFD(Ref(&fence), &second) ==
                kClayFenceSyncSuccess);
    FENCE_CHECK(first >= 0 && second >= 0 && first != second);
    close(first);
    FENCE_CHECK(fcntl(second, F_GETFD) >= 0);
    close(second);
  }
  extensions = "EGL_KHR_fence_sync_extra EGL_KHR_wait_sync";
  FENCE_CHECK(!clay::AndroidEGLFenceSync::CheckEGLFenceSupported());
  {
    clay::AndroidEGLFenceSync complete;
    FENCE_CHECK(testFencePoll(Ref(&complete)) == kClayFenceSyncSignaled);
  }
  create_fails = true;
  finish_error = GL_INVALID_OPERATION;
  {
    clay::AndroidEGLFenceSync failed;
    FENCE_CHECK(testFencePoll(Ref(&failed)) == kClayFenceSyncFailed);
  }
  TestConcurrentClientWait();
  TestFinishFallback();
  TestNonblockingCreation();
  current_display = EGL_NO_DISPLAY;
  current_context = EGL_NO_CONTEXT;
  {
    clay::AndroidEGLFenceSync failed;
    FENCE_CHECK(testFencePoll(Ref(&failed)) == kClayFenceSyncFailed &&
                !failed.ClientWait());
  }
  auto backing = fml::MakeRefCounted<clay::EGLImageBacking>(
      clay::SharedImageBacking::PixelFormat::kNative8888, skity::Vec2{1, 1});
  {
    auto representation =
        fml::MakeRefCounted<clay::NativeBufferEGLImageRepresentation>(backing);
    current_display = reinterpret_cast<EGLDisplay>(1);
    current_context = reinterpret_cast<EGLContext>(2);
    extensions = "";
    const int previous_finishes = finishes;
    auto unsupported = representation->ProduceFence();
    FENCE_CHECK(unsupported->Poll() == clay::FenceSync::PollResult::kSignaled);
    FENCE_CHECK(unsupported->ClientWait());
    FENCE_CHECK(finishes == previous_finishes + 1);
    extensions = "EGL_KHR_fence_sync";
    create_fails = true;
    auto failed = representation->ProduceFence();
    FENCE_CHECK(failed->Poll() == clay::FenceSync::PollResult::kSignaled);
    FENCE_CHECK(failed->ClientWait());
    FENCE_CHECK(finishes == previous_finishes + 2);
    current_display = EGL_NO_DISPLAY;
    current_context = EGL_NO_CONTEXT;
    std::puts(
        "PASS EGL legacy representation retains blocking completion fallback");
  }
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  backing->SetFenceSync(std::move(failure));
  for (int i = 0; i < 2; ++i) {
    auto representation =
        fml::MakeRefCounted<clay::NativeBufferEGLImageRepresentation>(backing);
    representation->ConsumeFence(backing->GetFenceSync());
    ClaySharedImageReadResult read{};
    ClaySharedImageWriteResult write{};
    FENCE_CHECK(!representation->BeginRead(&read) &&
                !representation->EndRead());
    FENCE_CHECK(!representation->BeginWrite(&write) &&
                !representation->EndWrite());
    auto returned = representation->ProduceFence();
    FENCE_CHECK(returned.get() == identity && !returned->ClientWait());
    auto missing = representation->ProduceFence();
    FENCE_CHECK(testFencePoll(Ref(missing.get())) == kClayFenceSyncFailed);
    backing->SetFenceSync(std::move(returned));
  }
  std::puts(
      "PASS EGL capabilities, per-display fences, optional FD export and "
      "failed-proof preservation");
  TestFailedReadRelease(false);
  TestFailedReadRelease(true);
  TestFailedReadAdvance();
  TestFailedWriteRelease(false);
  TestFailedWriteRelease(true);
  std::puts(
      "PASS actual EGL accessor: failed read/write returns slots, "
      "sequence advances, destruction cleans up, failure proof survives");
}
