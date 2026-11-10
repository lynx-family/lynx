// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore EGLAPIENTRYP

#include "clay/gfx/shared_image/android_egl_image_representation.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <utility>

#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/android/surface_texture.h"
#include "clay/gfx/shared_image/android_hardwarebuffer_image_backing.h"
#include "clay/gfx/shared_image/android_surface_texture_image_backing.h"
#include "clay/gfx/shared_image/egl_image_backing.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {
namespace {
bool HasEGLExtension(EGLDisplay display, const char* name) {
  const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
  if (!extensions) return false;
  const size_t length = std::strlen(name);
  const char* match = extensions;
  while ((match = std::strstr(match, name))) {
    if ((match == extensions || match[-1] == ' ') &&
        (match[length] == ' ' || match[length] == '\0'))
      return true;
    match += length;
  }
  return false;
}

using CoreWaitSyncProc = EGLBoolean(EGLAPIENTRYP)(EGLDisplay, EGLSyncKHR,
                                                  EGLint);

CoreWaitSyncProc CoreWaitSync() {
  const char* version = eglQueryString(eglGetCurrentDisplay(), EGL_VERSION);
  if (!version || !((version[0] >= '2' && version[0] <= '9') ||
                    (version[0] == '1' && version[1] == '.' &&
                     version[2] >= '5' && version[2] <= '9'))) {
    return nullptr;
  }
  // Resolve dynamically so older Android releases need no EGL 1.5 symbols.
  return reinterpret_cast<CoreWaitSyncProc>(eglGetProcAddress("eglWaitSync"));
}

bool FinishGPUWork() {
  // GL errors belong to the context, not to an individual command. Discard
  // earlier recoverable errors before checking glFinish's own result. Context
  // loss and out-of-memory leave completion unproven even if reported earlier.
  // Bound the drain in case a driver keeps reporting an error.
  constexpr int kMaxErrorQueries = 16;
  for (int i = 0; i < kMaxErrorQueries; ++i) {
    const GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
      glFinish();
      return glGetError() == GL_NO_ERROR;
    }
    if (error == GL_CONTEXT_LOST_KHR || error == GL_OUT_OF_MEMORY) {
      return false;
    }
  }
  return false;
}

}  // namespace

AndroidEGLFenceSync::AndroidEGLFenceSync()
    : AndroidEGLFenceSync(CreationMode::kAllowBlockingFallback) {}

AndroidEGLFenceSync::AndroidEGLFenceSync(CreationMode mode) {
  fence_display_ = eglGetCurrentDisplay();
  unsupported_ = fence_display_ != EGL_NO_DISPLAY &&
                 eglGetCurrentContext() != EGL_NO_CONTEXT &&
                 !CheckEGLFenceSupported();
  if (fence_display_ != EGL_NO_DISPLAY &&
      eglGetCurrentContext() != EGL_NO_CONTEXT && CheckEGLFenceSupported()) {
    if (CheckAndroidNativeFenceSupported()) {
      fence_ = eglCreateSyncKHR(fence_display_, EGL_SYNC_NATIVE_FENCE_ANDROID,
                                nullptr);
      native_fence_ = fence_ != EGL_NO_SYNC_KHR;
    } else {
      fence_ = eglCreateSyncKHR(fence_display_, EGL_SYNC_FENCE_KHR, nullptr);
    }
  }
  if (fence_ == EGL_NO_SYNC_KHR) {
    FML_LOG(ERROR) << "Failed to create fence sync in EGL env, error: "
                   << eglGetError();
    // Publishing without a fence is safe only after the GPU has finished.
    if (mode == CreationMode::kAllowBlockingFallback) unsupported_ = false;
    if (mode == CreationMode::kAllowBlockingFallback &&
        fence_display_ != EGL_NO_DISPLAY &&
        eglGetCurrentContext() != EGL_NO_CONTEXT) {
      completed_ = FinishGPUWork();
    }
  }
  if (eglGetCurrentContext() != EGL_NO_CONTEXT) glFlush();
}

AndroidEGLFenceSync::~AndroidEGLFenceSync() {
  if (fence_ == EGL_NO_SYNC_KHR) {
    return;
  }
  FML_DCHECK(eglDestroySyncKHR);
  EGLBoolean success = eglDestroySyncKHR(fence_display_, fence_);
  if (success == EGL_FALSE) {
    FML_LOG(ERROR) << "Failed to delete fence sync in EGL env, error: "
                   << eglGetError();
  }
}

FenceSync::NativeExportResult AndroidEGLFenceSync::DupNativeFenceFD(
    int* out_fd) {
  if (!out_fd) {
    return NativeExportResult::kFailed;
  }
  *out_fd = -1;
  if (completed_) {
    return NativeExportResult::kSignaled;
  }
  if (fence_ == EGL_NO_SYNC_KHR) {
    return unsupported_ ? NativeExportResult::kUnsupported
                        : NativeExportResult::kFailed;
  }
  if (!native_fence_ || !eglDupNativeFenceFDANDROID) {
    return NativeExportResult::kUnsupported;
  }
  const EGLint fd = eglDupNativeFenceFDANDROID(fence_display_, fence_);
  if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
    return NativeExportResult::kFailed;
  }
  *out_fd = fd;
  return NativeExportResult::kExported;
}

int32_t AndroidEGLFenceSync::GetCurrentFD() {
  int fd = -1;
  DupNativeFenceFD(&fd);
  return fd;
}

bool AndroidEGLFenceSync::ClientWait() {
  if (fence_ == EGL_NO_SYNC_KHR) {
    return completed_;
  }
  FML_DCHECK(eglClientWaitSyncKHR);
  EGLint result =
      eglClientWaitSyncKHR(fence_display_, fence_, 0, EGL_FOREVER_KHR);
  if (result != EGL_CONDITION_SATISFIED_KHR) {
    FML_LOG(ERROR) << "Failed to eglClientWaitSync in EGL env, error: "
                   << eglGetError();
  }
  return result == EGL_CONDITION_SATISFIED_KHR;
}

FenceSync::PollResult AndroidEGLFenceSync::Poll() {
  if (completed_) {
    return PollResult::kSignaled;
  }
  if (fence_ == EGL_NO_SYNC_KHR || !eglClientWaitSyncKHR) {
    return unsupported_ ? PollResult::kUnsupported : PollResult::kFailed;
  }
  const EGLint result = eglClientWaitSyncKHR(fence_display_, fence_, 0, 0);
  if (result == EGL_CONDITION_SATISFIED_KHR) {
    return PollResult::kSignaled;
  }
  if (result == EGL_TIMEOUT_EXPIRED_KHR) {
    return PollResult::kPending;
  }
  return PollResult::kFailed;
}

FenceSync::WaitResult AndroidEGLFenceSync::EnqueueWaitResult() {
  if (completed_) {
    return WaitResult::kSuccess;
  }
  if (unsupported_) return WaitResult::kUnsupported;
  if (fence_ == EGL_NO_SYNC_KHR || eglGetCurrentDisplay() != fence_display_ ||
      eglGetCurrentContext() == EGL_NO_CONTEXT) {
    return WaitResult::kFailed;
  }
  if (!CheckEGLWaitSupported()) {
    return WaitResult::kUnsupported;
  }
  return WaitEGLSync(fence_display_, fence_) ? WaitResult::kSuccess
                                             : WaitResult::kFailed;
}

void AndroidEGLFenceSync::ServerWait() {
  if (!EnqueueWait()) {
    ClientWait();
  }
}

// static
bool AndroidEGLFenceSync::CheckEGLWaitSupported() {
  return CheckEGLFenceSupported() &&
         ((eglWaitSyncKHR &&
           HasEGLExtension(eglGetCurrentDisplay(), "EGL_KHR_wait_sync")) ||
          CoreWaitSync());
}

bool AndroidEGLFenceSync::WaitEGLSync(EGLDisplay display, EGLSyncKHR sync) {
  if (display == EGL_NO_DISPLAY || display != eglGetCurrentDisplay() ||
      eglGetCurrentContext() == EGL_NO_CONTEXT || !CheckEGLFenceSupported()) {
    return false;
  }
  if (eglWaitSyncKHR && HasEGLExtension(display, "EGL_KHR_wait_sync")) {
    return eglWaitSyncKHR(display, sync, 0) == EGL_TRUE;
  }
  auto wait = CoreWaitSync();
  return wait && wait(display, sync, 0) == EGL_TRUE;
}

// static
bool AndroidEGLFenceSync::CheckAndroidNativeFenceSupported() {
  const EGLDisplay current_display = eglGetCurrentDisplay();
  return current_display != EGL_NO_DISPLAY && CheckEGLFenceSupported() &&
         eglDupNativeFenceFDANDROID &&
         HasEGLExtension(current_display, "EGL_ANDROID_native_fence_sync");
}

// static
bool AndroidEGLFenceSync::CheckEGLFenceSupported() {
  const EGLDisplay current_display = eglGetCurrentDisplay();
  // Capability belongs to the current display. A probe before make-current
  // must not permanently cache an unavailable display or disable later fences.
  if (current_display == EGL_NO_DISPLAY ||
      !HasEGLExtension(current_display, "EGL_KHR_fence_sync")) {
    return false;
  }
  static std::once_flag flag;
  std::call_once(flag, [] {
    eglCreateSyncKHR = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(
        eglGetProcAddress("eglCreateSyncKHR"));
    eglDestroySyncKHR = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(
        eglGetProcAddress("eglDestroySyncKHR"));
    eglClientWaitSyncKHR = reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(
        eglGetProcAddress("eglClientWaitSyncKHR"));
    eglWaitSyncKHR = reinterpret_cast<PFNEGLWAITSYNCKHRPROC>(
        eglGetProcAddress("eglWaitSyncKHR"));
    eglDupNativeFenceFDANDROID =
        reinterpret_cast<PFNEGLDUPNATIVEFENCEFDANDROIDPROC>(
            eglGetProcAddress("eglDupNativeFenceFDANDROID"));
  });
  // Native FD export is optional; ordinary EGL fences still support GPU wait
  // and poll on implementations without the Android native-fence extension.
  return eglCreateSyncKHR && eglDestroySyncKHR && eglClientWaitSyncKHR;
}

NativeBufferEGLImageRepresentation::NativeBufferEGLImageRepresentation(
    fml::RefPtr<AHardwareBufferImageBacking> backing)
    : GLImageRepresentation(backing) {}

NativeBufferEGLImageRepresentation::NativeBufferEGLImageRepresentation(
    fml::RefPtr<EGLImageBacking> backing)
    : GLImageRepresentation(backing) {}

ImageRepresentationType NativeBufferEGLImageRepresentation::GetType() const {
  return ImageRepresentationType::kEGL;
}

std::optional<GLImageRepresentation::TextureInfo>
NativeBufferEGLImageRepresentation::GetTexImage() {
  if (texture_id_ == 0) {
    SharedImageBacking* backing = GetBacking();
    if (!backing) {
      return std::nullopt;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    switch (backing->GetType()) {
      case SharedImageBacking::BackingType::kAHardwareBuffer: {
        egl_display_ = eglGetCurrentDisplay();
        egl_image_ =
            static_cast<AHardwareBufferImageBacking*>(backing)->CreateEGLImage(
                egl_display_);
        if (egl_image_ == EGL_NO_IMAGE_KHR) {
          FML_LOG(ERROR) << "Failed to import hardware buffer into EGL";
          glDeleteTextures(1, &texture);
          return std::nullopt;
        }
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image_);
        FML_DCHECK(static_cast<EGLint>(EGL_SUCCESS) == eglGetError());
        break;
      }
      case SharedImageBacking::BackingType::kEGLImage: {
        static_cast<EGLImageBacking*>(backing)->BindToTexture(GL_TEXTURE_2D);
        break;
      }
      default: {
        // NOT SUPPORTED.
        FML_UNREACHABLE();
      }
    }
    texture_id_ = texture;
  }
  return TextureInfo{.target = GL_TEXTURE_2D,
                     .name = texture_id_,
                     .format = GL_RGBA,
                     .size = GetSize()};
}

bool NativeBufferEGLImageRepresentation::ReleaseTexImage() {
  if (texture_id_ == 0) {
    return false;
  }
  glDeleteTextures(1, &texture_id_);
  texture_id_ = 0;
  return true;
}

bool NativeBufferEGLImageRepresentation::UnbindFrameBuffer() {
  if (fbo_id_ == 0) {
    return false;
  }
  glDeleteFramebuffers(1, &fbo_id_);
  glDeleteRenderbuffers(1, &depth_stencil_id_);
  fbo_id_ = 0;
  depth_stencil_id_ = 0;
  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo>
NativeBufferEGLImageRepresentation::BindFrameBuffer() {
  if (fbo_id_ == 0) {
    auto texture_info = GetTexImage();
    if (!texture_info) {
      return std::nullopt;
    }
    GLuint backend_texture = texture_info->name;
    GLuint fbo_id;
    glGenFramebuffers(1, &fbo_id);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D,
                                                    backend_texture);
    clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER,
                                                            fbo_id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           backend_texture, 0);

    GLint old_rbo;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &old_rbo);

    glGenRenderbuffers(1, &depth_stencil_id_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_id_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, GetSize().x,
                          GetSize().y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, depth_stencil_id_);
    glBindRenderbuffer(GL_RENDERBUFFER, old_rbo);

#ifndef NDEBUG
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#else
    GLenum status = GL_FRAMEBUFFER_COMPLETE;
#endif
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create FBO, error: " << status;
      glDeleteFramebuffers(1, &fbo_id);
      glDeleteRenderbuffers(1, &depth_stencil_id_);
      depth_stencil_id_ = 0;
      return std::nullopt;
    }
    fbo_id_ = fbo_id;
  }
  return FramebufferInfo{.target = GL_FRAMEBUFFER, .name = fbo_id_};
}

NativeBufferEGLImageRepresentation::~NativeBufferEGLImageRepresentation() {
  UnbindFrameBuffer();
  ReleaseTexImage();
  if (egl_image_ && egl_display_) {
    FML_DCHECK(eglGetCurrentContext() != EGL_NO_CONTEXT);
    eglDestroyImageKHR(egl_display_, egl_image_);
  }
}

PFNEGLCREATESYNCKHRPROC AndroidEGLFenceSync::eglCreateSyncKHR = nullptr;
PFNEGLDESTROYSYNCKHRPROC AndroidEGLFenceSync::eglDestroySyncKHR = nullptr;
PFNEGLCLIENTWAITSYNCKHRPROC AndroidEGLFenceSync::eglClientWaitSyncKHR = nullptr;
PFNEGLWAITSYNCKHRPROC AndroidEGLFenceSync::eglWaitSyncKHR = nullptr;
PFNEGLDUPNATIVEFENCEFDANDROIDPROC
AndroidEGLFenceSync::eglDupNativeFenceFDANDROID = nullptr;

bool NativeBufferEGLImageRepresentation::BeginRead(
    ClaySharedImageReadResult* out) {
  return access_ready_ && GLImageRepresentation::BeginRead(out);
}

bool NativeBufferEGLImageRepresentation::EndRead() {
  return access_ready_ && GLImageRepresentation::EndRead();
}

bool NativeBufferEGLImageRepresentation::BeginWrite(
    ClaySharedImageWriteResult* out) {
  return access_ready_ && GLImageRepresentation::BeginWrite(out);
}

bool NativeBufferEGLImageRepresentation::EndWrite() {
  return access_ready_ && GLImageRepresentation::EndWrite();
}

void NativeBufferEGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) return;
  access_ready_ = fence_sync->EnqueueWait() || fence_sync->ClientWait();
  if (!access_ready_) {
    GetBacking()->SetFenceSync(std::move(fence_sync));
  }
}

std::unique_ptr<FenceSync> NativeBufferEGLImageRepresentation::ProduceFence() {
  if (!access_ready_) {
    auto failed = GetBacking()->GetFenceSync();
    if (failed) return failed;
    return std::make_unique<FailedFenceSync>();
  }
  // Preserve legacy ClientWait when native sync creation is unavailable.
  return std::make_unique<AndroidEGLFenceSync>();
}

#ifndef ENABLE_HEADLESS
SurfaceTextureEGLImageRepresentation::SurfaceTextureEGLImageRepresentation(
    fml::RefPtr<SurfaceTextureImageBacking> backing)
    : GLImageRepresentation(backing), backing_(backing) {}

SurfaceTextureEGLImageRepresentation::~SurfaceTextureEGLImageRepresentation() {
  UnbindFrameBuffer();
  ReleaseTexImage();
}

ImageRepresentationType SurfaceTextureEGLImageRepresentation::GetType() const {
  return ImageRepresentationType::kEGL;
}

void SurfaceTextureEGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence) {
  backing_->AcceptFence(std::move(fence));
}

std::unique_ptr<FenceSync>
SurfaceTextureEGLImageRepresentation::ProduceFence() {
  return backing_->ProduceReadFence();
}

bool SurfaceTextureEGLImageRepresentation::EndRead() {
  return backing_->EndRead();
}

std::optional<GLImageRepresentation::TextureInfo>
SurfaceTextureEGLImageRepresentation::GetTexImage() {
  if (!backing_->BeginRead()) return std::nullopt;
  return TextureInfo{.target = GL_TEXTURE_EXTERNAL_OES,
                     .name = backing_->EnsureAttachedToGLContext(),
                     .format = GL_RGBA8_OES,
                     .size = GetSize()};
}

bool SurfaceTextureEGLImageRepresentation::ReleaseTexImage() {
  backing_->DetachGLContext();

  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo>
SurfaceTextureEGLImageRepresentation::BindFrameBuffer() {
  // TODO(youfeng) support write to Surface
  return {};
}

bool SurfaceTextureEGLImageRepresentation::UnbindFrameBuffer() {
  // TODO(youfeng) support write to Surface
  return false;
}
#endif  // ENABLE_HEADLESS

}  // namespace clay
