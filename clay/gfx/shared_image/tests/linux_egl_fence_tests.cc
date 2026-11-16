// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

#include "clay/gfx/shared_image/linux_egl_fence_sync.h"

#define CHECK(x)                                           \
  do {                                                     \
    if (!(x)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); \
      std::abort();                                        \
    }                                                      \
  } while (0)
using Fence = clay::LinuxEGLFenceSync;
using Poll = clay::FenceSync::PollResult;
using Wait = clay::FenceSync::WaitResult;
using Export = clay::FenceSync::NativeExportResult;

template <int N>
struct Driver {
  static inline const char* extensions =
      "EGL_KHR_fence_sync EGL_KHR_wait_sync EGL_ANDROID_native_fence_sync";
  static inline const char* version = "1.4";
  static inline const char* missing = "";
  static inline bool current = true, wrong_display = false, fail_create = false,
                     fail_wait = false;
  static inline int creates = 0, destroys = 0, flushes = 0, waits = 0,
                    cpu_waits = 0, core_waits = 0;
  static inline std::mutex mutex;
  static inline std::condition_variable cv;
  static inline bool block_client = false, entered_client = false,
                     release_client = false;
  static inline EGLint status = EGL_TIMEOUT_EXPIRED_KHR;
  static EGLDisplay Display() {
    return reinterpret_cast<EGLDisplay>(N + (wrong_display ? 10 : 0));
  }
  static EGLContext Context() {
    return current ? reinterpret_cast<EGLContext>(N) : EGL_NO_CONTEXT;
  }
  static const char* Query(EGLDisplay, EGLint name) {
    return name == EGL_VERSION ? version : extensions;
  }
  static EGLSyncKHR Create(EGLDisplay display, EGLenum, const EGLint*) {
    CHECK(display == reinterpret_cast<EGLDisplay>(N));
    ++creates;
    return fail_create ? EGL_NO_SYNC_KHR : reinterpret_cast<EGLSyncKHR>(N);
  }
  static EGLSync CoreCreate(EGLDisplay d, EGLenum t, const EGLAttrib*) {
    return Create(d, t, nullptr);
  }
  static EGLBoolean Destroy(EGLDisplay display, EGLSyncKHR sync) {
    CHECK(display == reinterpret_cast<EGLDisplay>(N) &&
          sync == reinterpret_cast<EGLSyncKHR>(N));
    ++destroys;
    return EGL_TRUE;
  }
  static EGLint Client(EGLDisplay, EGLSyncKHR, EGLint flags,
                       EGLTimeKHR timeout) {
    CHECK(flags == 0);
    if (timeout) {
      ++cpu_waits;
      std::unique_lock<std::mutex> lock(mutex);
      if (block_client) {
        entered_client = true;
        cv.notify_all();
        cv.wait(lock, [] { return release_client; });
        return EGL_CONDITION_SATISFIED_KHR;
      }
    }
    return status;
  }
  static EGLint Server(EGLDisplay, EGLSyncKHR, EGLint flags) {
    CHECK(flags == 0);
    ++waits;
    return fail_wait ? EGL_FALSE : EGL_TRUE;
  }
  static EGLBoolean CoreServer(EGLDisplay d, EGLSync s, EGLint f) {
    ++core_waits;
    return Server(d, s, f);
  }
  static EGLint Dup(EGLDisplay, EGLSyncKHR) {
    int fd[2];
    CHECK(pipe(fd) == 0);
    close(fd[1]);
    return fd[0];
  }
  static void Flush() { ++flushes; }
  static __eglMustCastToProperFunctionPointerType Proc(const char* name) {
    if (!std::strcmp(name, missing)) return nullptr;
#define PROC(n, f)           \
  if (!std::strcmp(name, n)) \
  return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(f)
    PROC("eglCreateSyncKHR", Create);
    PROC("eglDestroySyncKHR", Destroy);
    PROC("eglClientWaitSyncKHR", Client);
    PROC("eglWaitSyncKHR", Server);
    PROC("eglDupNativeFenceFDANDROID", Dup);
    PROC("eglCreateSync", CoreCreate);
    PROC("eglDestroySync", Destroy);
    PROC("eglClientWaitSync", Client);
    PROC("eglWaitSync", CoreServer);
#undef PROC
    return nullptr;
  }
  static Fence::Dispatch API() {
    return {&creates, Display, Context, Query, Proc, Flush};
  }
};

int main() {
  alarm(10);
  using A = Driver<1>;
  using B = Driver<2>;
  {
    Fence a(A::API()), b(B::API());
    CHECK(A::creates == 1 && B::creates == 1);
    CHECK(a.CanWaitOn(A::API()) && !a.CanWaitOn(B::API()));
    CHECK(a.Poll() == Poll::kPending && b.Poll() == Poll::kPending);
    CHECK(a.EnqueueWait() && A::waits == 1 && B::waits == 0);
    CHECK(a.Poll() == Poll::kPending && A::cpu_waits == 0);
    int first = -1, second = -1;
    CHECK(a.DupNativeFenceFD(&first) == Export::kExported);
    CHECK(a.DupNativeFenceFD(&second) == Export::kExported && first != second);
    close(first);
    CHECK(fcntl(second, F_GETFD) >= 0);
    close(second);
    A::wrong_display = true;
    CHECK(a.EnqueueWaitResult() == Wait::kFailed);
    A::wrong_display = false;
    A::current = false;
    CHECK(a.Poll() == Poll::kPending && a.EnqueueWaitResult() == Wait::kFailed);
    A::current = true;
    A::status = EGL_CONDITION_SATISFIED_KHR;
    CHECK(a.Poll() == Poll::kSignaled);
    CHECK(a.ClientWait() && A::cpu_waits == 1);
    A::current = false;  // Destruction requires no current context or CPU wait.
  }
  CHECK(A::destroys == 1 && B::destroys == 1 && A::cpu_waits == 1);
  {
    Fence invalid(A::API());
    CHECK(invalid.Poll() == Poll::kFailed);
  }
  A::current = true;
  A::extensions = "EGL_KHR_fence_sync_extra";
  {
    Fence unsupported(A::API());
    CHECK(unsupported.Poll() == Poll::kUnsupported);
  }
  A::extensions = "EGL_KHR_fence_sync";
  {
    Fence no_wait(A::API());
    CHECK(no_wait.EnqueueWaitResult() == Wait::kUnsupported);
    int fd = 123;
    CHECK(no_wait.DupNativeFenceFD(&fd) == Export::kUnsupported && fd == -1);
  }
  A::extensions = "EGL_KHR_fence_sync EGL_KHR_wait_sync";
  A::missing = "eglWaitSyncKHR";
  {
    Fence no_proc(A::API());
    CHECK(no_proc.EnqueueWaitResult() == Wait::kUnsupported);
  }
  A::missing = "";
  A::fail_create = true;
  {
    Fence failed(A::API());
    CHECK(failed.Poll() == Poll::kFailed && !failed.EnqueueWait());
  }
  A::fail_create = false;
  A::status = EGL_FALSE;
  {
    Fence failed(A::API());
    CHECK(failed.Poll() == Poll::kFailed);
    A::status = EGL_CONDITION_SATISFIED_KHR;
    CHECK(failed.Poll() == Poll::kFailed);
  }
  A::extensions = "";
  A::version = "1.5 Mesa";
  A::status = EGL_TIMEOUT_EXPIRED_KHR;
  {
    Fence core(A::API());
    CHECK(core.Poll() == Poll::kPending && core.EnqueueWait());
    A::status = EGL_CONDITION_SATISFIED_KHR;
    CHECK(core.Poll() == Poll::kSignaled);
  }
  // EGL 1.5 may expose KHR fence creation without the optional KHR wait.
  // Preserve native FD export while selecting the core GPU-wait entry point.
  A::extensions = "EGL_KHR_fence_sync EGL_ANDROID_native_fence_sync";
  A::status = EGL_TIMEOUT_EXPIRED_KHR;
  for (const char* missing : {"", "eglWaitSyncKHR"}) {
    A::missing = missing;
    const int before = A::core_waits;
    Fence mixed(A::API());
    CHECK(mixed.EnqueueWait() && A::core_waits == before + 1);
    CHECK(mixed.Poll() == Poll::kPending);
    int fd = -1;
    CHECK(mixed.DupNativeFenceFD(&fd) == Export::kExported);
    close(fd);
  }
  A::extensions = "EGL_KHR_fence_sync EGL_KHR_wait_sync";
  A::missing = "eglWaitSyncKHR";
  {
    Fence mixed(A::API());
    CHECK(mixed.EnqueueWait());
  }
  A::extensions = "EGL_KHR_fence_sync";
  A::missing = "eglWaitSync";
  {
    Fence missing(A::API());
    CHECK(missing.EnqueueWaitResult() == Wait::kUnsupported);
  }
  A::missing = "";
  A::extensions = "EGL_KHR_fence_sync EGL_ANDROID_native_fence_sync";
  {
    Fence concurrent(A::API());
    A::block_client = true;
    std::thread waiter([&] { CHECK(concurrent.ClientWait()); });
    {
      std::unique_lock<std::mutex> lock(A::mutex);
      A::cv.wait(lock, [] { return A::entered_client; });
    }
    // The legacy wait has not been released: any implicit CPU wait in these
    // operations deadlocks and the watchdog fails the test.
    CHECK(concurrent.Poll() == Poll::kPending);
    CHECK(concurrent.EnqueueWait());
    int fd = 123;
    CHECK(concurrent.DupNativeFenceFD(&fd) == Export::kExported && fd >= 0);
    close(fd);
    {
      std::lock_guard<std::mutex> lock(A::mutex);
      A::release_client = true;
      A::cv.notify_all();
    }
    waiter.join();
  }
  A::extensions = "";
  A::missing = "eglCreateSync";
  {
    Fence unsupported(A::API());
    CHECK(unsupported.Poll() == Poll::kUnsupported);
  }
  {
    auto api = A::API();
    api.finish = [] { return true; };
    Fence legacy(api);
    CHECK(legacy.ClientWait() && legacy.Poll() == Poll::kSignaled);
    CHECK(legacy.EnqueueWait());
    int fd = 123;
    CHECK(legacy.DupNativeFenceFD(&fd) == Export::kSignaled && fd == -1);
    api.finish = [] { return false; };
    Fence failed(api);
    CHECK(!failed.ClientWait() && failed.Poll() == Poll::kFailed);
  }
  CHECK(A::cpu_waits == 2 && B::cpu_waits == 0);
  std::puts(
      "PASS Linux EGL: KHR/core sync, isolated dispatch, "
      "pending/signaled/error, GPU waits, missing capabilities, independent "
      "FDs, context-free poll/destruction, no implicit CPU wait");
}
