// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android_egl_image_representation.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>

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
#include "clay/gfx/shared_image/vulkan_image_hardwarebuffer_representation.h"

namespace clay {

AndroidEGLFenceSync::AndroidEGLFenceSync() {
  FML_DCHECK(display != EGL_NO_DISPLAY);
  FML_DCHECK(eglCreateSyncKHR);
  if (CheckEGLFenceSupported()) {
    if (CheckAndroidNativeFenceSupported()) {
      fence_ =
          eglCreateSyncKHR(display, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
    } else {
      fence_ = eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, nullptr);
    }
  }
  if (fence_ == EGL_NO_SYNC_KHR) {
    FML_LOG(ERROR) << "Failed to create fence sync in EGL env, error: "
                   << eglGetError();
  }
  glFlush();
}

AndroidEGLFenceSync::~AndroidEGLFenceSync() {
  if (fence_ == EGL_NO_SYNC_KHR) {
    return;
  }
  FML_DCHECK(eglDestroySyncKHR);
  EGLBoolean success = eglDestroySyncKHR(display, fence_);
  if (success == EGL_FALSE) {
    FML_LOG(ERROR) << "Failed to delete fence sync in EGL env, error: "
                   << eglGetError();
  }
}

int32_t AndroidEGLFenceSync::GetCurrentFD() {
  if (!CheckAndroidNativeFenceSupported() || fence_ == EGL_NO_SYNC_KHR) {
    FML_LOG(ERROR) << "EGL Android native fence sync is not supported.";
    return EGL_NO_NATIVE_FENCE_FD_ANDROID;
  }
  FML_DCHECK(eglGetSyncAttribKHR);
  FML_DCHECK(display != EGL_NO_DISPLAY);
  EGLint sync_fd = eglDupNativeFenceFDANDROID(display, fence_);
  if (sync_fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
    FML_LOG(ERROR) << "eglDupNativeFenceFDANDROID copy fd failed "
                   << eglGetError();
  }
  return sync_fd;
}

bool AndroidEGLFenceSync::ClientWait() {
  if (fence_ == EGL_NO_SYNC_KHR) {
    return false;
  }
  FML_DCHECK(eglClientWaitSyncKHR);
  EGLint result = eglClientWaitSyncKHR(display, fence_, 0, EGL_FOREVER_KHR);
  if (result != EGL_CONDITION_SATISFIED_KHR) {
    FML_LOG(ERROR) << "Failed to eglClientWaitSync in EGL env, error: "
                   << eglGetError();
  }
  return result == EGL_CONDITION_SATISFIED_KHR;
}

void AndroidEGLFenceSync::ServerWait() {
  if (fence_ == EGL_NO_SYNC_KHR) {
    return;
  }
  FML_DCHECK(eglWaitSyncKHR);
  EGLint result = eglWaitSyncKHR(display, fence_, 0);
  if (result == EGL_FALSE) {
    FML_LOG(ERROR) << "Failed to eglWaitSyncKHR in EGL env, error: "
                   << eglGetError();
  }
}

// static
bool AndroidEGLFenceSync::CheckAndroidNativeFenceSupported() {
  static std::once_flag flag;
  static bool supported;
  auto func = [] {
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (!strstr(extensions, "EGL_ANDROID_native_fence_sync")) {
      supported = false;
    } else {
      supported = true;
    }
  };
  std::call_once(flag, func);
  return supported;
}

// static
bool AndroidEGLFenceSync::CheckEGLFenceSupported() {
  static std::once_flag flag;
  static bool supported;
  auto init_func = [] {
    display = eglGetCurrentDisplay();
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (!strstr(extensions, "EGL_KHR_fence_sync")) {
      supported = false;
    } else {
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
      supported = eglCreateSyncKHR && eglDestroySyncKHR &&
                  eglClientWaitSyncKHR && eglWaitSyncKHR &&
                  eglDupNativeFenceFDANDROID;
    }
    if (!supported) {
      FML_LOG(ERROR) << "EGLFenceSync is not supported.";
    }
  };
  std::call_once(flag, init_func);
  return supported;
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
        FML_DCHECK(egl_image_ != EGL_NO_IMAGE_KHR);
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

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create FBO, error: " << status;
      glDeleteFramebuffers(1, &fbo_id);
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

EGLDisplay AndroidEGLFenceSync::display = EGL_NO_DISPLAY;
PFNEGLCREATESYNCKHRPROC AndroidEGLFenceSync::eglCreateSyncKHR = nullptr;
PFNEGLDESTROYSYNCKHRPROC AndroidEGLFenceSync::eglDestroySyncKHR = nullptr;
PFNEGLCLIENTWAITSYNCKHRPROC AndroidEGLFenceSync::eglClientWaitSyncKHR = nullptr;
PFNEGLWAITSYNCKHRPROC AndroidEGLFenceSync::eglWaitSyncKHR = nullptr;
PFNEGLDUPNATIVEFENCEFDANDROIDPROC
AndroidEGLFenceSync::eglDupNativeFenceFDANDROID = nullptr;

class DummyFenceSync final : public FenceSync {
  bool ClientWait() override { return true; }

  Type GetType() const override { return Type::kClientWaitOnly; }
};

void NativeBufferEGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  if (fence_sync->GetType() == FenceSync::Type::kEGL) {
    AndroidEGLFenceSync* egl_fence_sync =
        static_cast<AndroidEGLFenceSync*>(fence_sync.get());
    egl_fence_sync->ServerWait();
    return;
  } else if (fence_sync->GetType() == FenceSync::Type::kVulkan) {
    // Try server wait.
    VkFenceSync* vulkan_fence_sync =
        static_cast<VkFenceSync*>(fence_sync.get());
    vulkan_fence_sync->ServerWait();
    return;
  }
  fence_sync->ClientWait();
}

std::unique_ptr<FenceSync> NativeBufferEGLImageRepresentation::ProduceFence() {
  if (AndroidEGLFenceSync::CheckEGLFenceSupported()) {
    return std::make_unique<AndroidEGLFenceSync>();
  }
  glFinish();
  return std::make_unique<DummyFenceSync>();
}

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
  if (fence) {
    fence->ClientWait();
  }
}

std::unique_ptr<FenceSync>
SurfaceTextureEGLImageRepresentation::ProduceFence() {
  // The fence is created in SurfaceTexture's internal implementations
  return nullptr;
}

std::optional<GLImageRepresentation::TextureInfo>
SurfaceTextureEGLImageRepresentation::GetTexImage() {
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

}  // namespace clay
