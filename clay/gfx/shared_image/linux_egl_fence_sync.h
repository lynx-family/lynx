// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_LINUX_EGL_FENCE_SYNC_H_
#define CLAY_GFX_SHARED_IMAGE_LINUX_EGL_FENCE_SYNC_H_

#include <atomic>
#include <cstring>

#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

// Include the owning backend's EGL/GL headers first. In software builds ANGLE
// and libepoxy can coexist: every fence retains the producer's dispatch rather
// than looking up symbols in the process or caching another backend's display.
class LinuxEGLFenceSync final : public FenceSync {
 public:
  struct Dispatch {
    const void* backend;
    EGLDisplay (*current_display)();
    EGLContext (*current_context)();
    const char* (*query_string)(EGLDisplay, EGLint);
    __eglMustCastToProperFunctionPointerType (*get_proc)(const char*);
    void (*flush)();
    bool (*finish)() = nullptr;
  };

  explicit LinuxEGLFenceSync(Dispatch api) : api_(api) {
    display_ = api_.current_display();
    if (display_ == EGL_NO_DISPLAY || api_.current_context() == EGL_NO_CONTEXT)
      return;
    const char* version = api_.query_string(display_, EGL_VERSION);
    const bool core_supported =
        version && ((version[0] >= '2' && version[0] <= '9') ||
                    (version[0] == '1' && version[1] == '.' &&
                     version[2] >= '5' && version[2] <= '9'));
    // Core and KHR sync entry points operate on the same EGL sync objects.
    // A KHR-created (including native-FD) fence can use the core GPU wait.
    if (core_supported) wait_core_ = Proc<PFNEGLWAITSYNCPROC>("eglWaitSync");
    create_ = Proc<PFNEGLCREATESYNCKHRPROC>("eglCreateSyncKHR");
    destroy_ = Proc<PFNEGLDESTROYSYNCKHRPROC>("eglDestroySyncKHR");
    client_wait_ = Proc<PFNEGLCLIENTWAITSYNCKHRPROC>("eglClientWaitSyncKHR");
    wait_ = Proc<PFNEGLWAITSYNCKHRPROC>("eglWaitSyncKHR");
    dup_fd_ =
        Proc<PFNEGLDUPNATIVEFENCEFDANDROIDPROC>("eglDupNativeFenceFDANDROID");
    if (!HasExtension("EGL_KHR_fence_sync") || !create_ || !destroy_ ||
        !client_wait_) {
      core_ = core_supported;
      create_core_ = Proc<PFNEGLCREATESYNCPROC>("eglCreateSync");
      destroy_core_ = Proc<PFNEGLDESTROYSYNCPROC>("eglDestroySync");
      client_wait_core_ = Proc<PFNEGLCLIENTWAITSYNCPROC>("eglClientWaitSync");
      if (!core_ || !create_core_ || !destroy_core_ || !client_wait_core_) {
        unsupported_ = true;
        FinishFallback();
        return;
      }
      sync_ = create_core_(display_, EGL_SYNC_FENCE, nullptr);
      if (sync_ != EGL_NO_SYNC)
        api_.flush();
      else
        FinishFallback();
      return;
    }
    native_ = dup_fd_ && HasExtension("EGL_ANDROID_native_fence_sync");
    sync_ = create_(
        display_, native_ ? EGL_SYNC_NATIVE_FENCE_ANDROID : EGL_SYNC_FENCE_KHR,
        nullptr);
    if (sync_ != EGL_NO_SYNC_KHR)
      api_.flush();
    else
      FinishFallback();
  }

  ~LinuxEGLFenceSync() override {
    if (sync_ == EGL_NO_SYNC_KHR) return;
    if (core_)
      destroy_core_(display_, sync_);
    else
      destroy_(display_, sync_);
  }
  Type GetType() const override { return Type::kLinuxEGL; }
  bool CanWaitOn(const Dispatch& consumer) const {
    return api_.backend && consumer.backend == api_.backend;
  }

  PollResult Poll() override {
    if (completed_) return PollResult::kSignaled;
    if (unsupported_) return PollResult::kUnsupported;
    if (failed_ || sync_ == EGL_NO_SYNC_KHR) return PollResult::kFailed;
    const EGLint result = core_ ? client_wait_core_(display_, sync_, 0, 0)
                                : client_wait_(display_, sync_, 0, 0);
    if (result == EGL_CONDITION_SATISFIED_KHR) return PollResult::kSignaled;
    if (result == EGL_TIMEOUT_EXPIRED_KHR) return PollResult::kPending;
    failed_ = true;
    return PollResult::kFailed;
  }

  bool ClientWait() override {
    if (completed_) return true;
    if (failed_ || sync_ == EGL_NO_SYNC_KHR) return false;
    const EGLint result =
        core_ ? client_wait_core_(display_, sync_, 0, EGL_FOREVER)
              : client_wait_(display_, sync_, 0, EGL_FOREVER_KHR);
    if (result != EGL_CONDITION_SATISFIED_KHR) failed_ = true;
    return result == EGL_CONDITION_SATISFIED_KHR;
  }
  WaitResult EnqueueWaitResult() override {
    if (completed_) return WaitResult::kSuccess;
    if (unsupported_) return WaitResult::kUnsupported;
    if (failed_ || sync_ == EGL_NO_SYNC_KHR ||
        api_.current_display() != display_ ||
        api_.current_context() == EGL_NO_CONTEXT)
      return WaitResult::kFailed;
    if (!core_ && wait_ && HasExtension("EGL_KHR_wait_sync")) {
      return wait_(display_, sync_, 0) == EGL_TRUE ? WaitResult::kSuccess
                                                   : WaitResult::kFailed;
    }
    if (wait_core_) {
      return wait_core_(display_, sync_, 0) == EGL_TRUE ? WaitResult::kSuccess
                                                        : WaitResult::kFailed;
    }
    return WaitResult::kUnsupported;
  }

  NativeExportResult DupNativeFenceFD(int* fd) override {
    if (!fd) return NativeExportResult::kFailed;
    *fd = -1;
    if (completed_) return NativeExportResult::kSignaled;
    if (unsupported_) return NativeExportResult::kUnsupported;
    if (failed_ || sync_ == EGL_NO_SYNC_KHR) return NativeExportResult::kFailed;
    if (!native_) return NativeExportResult::kUnsupported;
    *fd = dup_fd_(display_, sync_);
    return *fd >= 0 ? NativeExportResult::kExported
                    : NativeExportResult::kFailed;
  }

 private:
  void FinishFallback() {
    if (!api_.finish) return;
    unsupported_ = false;
    completed_ = api_.finish();
  }
  bool completed_ = false;
  template <class T>
  T Proc(const char* name) {
    return reinterpret_cast<T>(api_.get_proc(name));
  }
  bool HasExtension(const char* name) const {
    const char* extensions = api_.query_string(display_, EGL_EXTENSIONS);
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

  Dispatch api_;
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLSyncKHR sync_ = EGL_NO_SYNC_KHR;
  bool unsupported_ = false;
  bool native_ = false;
  bool core_ = false;
  std::atomic<bool> failed_{false};
  PFNEGLCREATESYNCPROC create_core_ = nullptr;
  PFNEGLDESTROYSYNCPROC destroy_core_ = nullptr;
  PFNEGLCLIENTWAITSYNCPROC client_wait_core_ = nullptr;
  PFNEGLWAITSYNCPROC wait_core_ = nullptr;
  PFNEGLCREATESYNCKHRPROC create_ = nullptr;
  PFNEGLDESTROYSYNCKHRPROC destroy_ = nullptr;
  PFNEGLCLIENTWAITSYNCKHRPROC client_wait_ = nullptr;
  PFNEGLWAITSYNCKHRPROC wait_ = nullptr;
  PFNEGLDUPNATIVEFENCEFDANDROIDPROC dup_fd_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_LINUX_EGL_FENCE_SYNC_H_
