// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/epoxy_shm_image_backing.h"

#include <epoxy/egl.h>
#include <epoxy/gl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <tuple>

#include "base/include/fml/message_loop.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/epoxy_shm_image_representation.h"
#include "clay/gfx/shared_image/linux_shm_image_representation.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/public/clay.h"

namespace clay {

namespace {

std::tuple<void*, int> CreateShmBuffer(
    SharedImageBacking::PixelFormat pixel_format, skity::Vec2 size) {
  static size_t count = 0;
  std::string shm_name =
      "/clay_shared_image_backing" + std::to_string(getpid()) + "_" +
      std::to_string(pthread_self()) + "_" + std::to_string(count++);
  size_t shm_size = size.x * size.y * 4;

  int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
  ftruncate(shm_fd, shm_size);
  shm_unlink(shm_name.c_str());

  return {mmap(NULL, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0), shm_fd};
}

}  // namespace

bool EpoxyShmImageBacking::CreateEGLImageIfNeeded() {
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    egl_display_ = eglGetCurrentDisplay();
    egl_context_ = eglGetCurrentContext();

    if (egl_context_ == EGL_NO_CONTEXT || egl_display_ == EGL_NO_DISPLAY) {
      return false;
    }

    fml::MessageLoop::EnsureInitializedForCurrentThread();
    fml::RefPtr<fml::TaskRunner> task_runner =
        fml::MessageLoop::GetCurrent().GetTaskRunner();
    FML_DCHECK(task_runner);
    task_runner_ = task_runner;

    [[maybe_unused]] const char* extensions =
        eglQueryString(egl_display_, EGL_EXTENSIONS);
    FML_DCHECK(strstr(extensions, "EGL_KHR_image") ||
               strstr(extensions, "EGL_KHR_gl_texture_2D_image") ||
               strstr(extensions, "GL_OES_EGL_image"));

    GLuint texture;
    glGenTextures(1, &texture);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_.x, size_.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    const EGLint egl_attrib_list[] = {EGL_GL_TEXTURE_LEVEL_KHR, 0,
                                      EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                      EGL_NONE};
    EGLClientBuffer egl_buffer = reinterpret_cast<EGLClientBuffer>(texture);
    EGLenum egl_target = EGL_GL_TEXTURE_2D_KHR;

    egl_image_ = eglCreateImageKHR(egl_display_, egl_context_, egl_target,
                                   egl_buffer, egl_attrib_list);

    if (egl_image_ == EGL_NO_IMAGE_KHR) {
      FML_LOG(ERROR) << "eglCreateImageKHR for cross-thread sharing failed: "
                     << eglGetError();
      glDeleteTextures(1, &texture);
      return false;
    }
    texture_id_ = texture;

#ifndef NDEBUG
    GLint max_texture_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    if (size_.x > max_texture_size || size_.y > max_texture_size) {
      FML_LOG(ERROR) << "texture size is too large, supported max size is: "
                     << max_texture_size;
    }
#endif
  }
  return true;
}

EpoxyShmImageBacking::EpoxyShmImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  if (gfx_handle) {
    shm_buffer_ = gfx_handle.value();
  } else {
    std::tuple<void*, int> result = CreateShmBuffer(pixel_format, size);
    shm_buffer_ = std::get<0>(result);
    shm_fd_ = std::get<1>(result);
  }
}

SharedImageBacking::BackingType EpoxyShmImageBacking::GetType() const {
  return BackingType::kShmImage;
}

void EpoxyShmImageBacking::BindToTexture(EGLenum target) {
  CreateEGLImageIfNeeded();
  glEGLImageTargetTexture2DOES(target, egl_image_);
  FML_DCHECK(static_cast<EGLint>(EGL_SUCCESS) == eglGetError());
}

fml::RefPtr<SharedImageRepresentation>
EpoxyShmImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  ClaySharedImageRepresentationType type = config->type;
  switch (type) {
    case kClaySharedImageRepresentationTypeShm: {
      return fml::MakeRefCounted<LinuxShmImageRepresentation>(fml::Ref(this));
    }
    case kClaySharedImageRepresentationTypeGL: {
      return fml::MakeRefCounted<EpoxyShmImageRepresentation>(fml::Ref(this));
    }
    default: {
      // NOT SUPPORTED.
      FML_LOG(ERROR) << "Unable to call "
                        "EpoxyShmImageBacking::CreateRepresentation with type: "
                     << static_cast<uint32_t>(type);
      return nullptr;
    }
  }
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation>
EpoxyShmImageBacking::CreateSkiaRepresentation(GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
    case GrBackendApi::kOpenGL: {
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context,
          fml::MakeRefCounted<EpoxyShmImageRepresentation>(fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR) << "Unable to call "
                        "EpoxyShmImageBacking::CreateSkiaRepresentation with "
                        "backend: "
                     << static_cast<uint32_t>(gr_context->backend());
      return nullptr;
    }
  }
}
#else
fml::RefPtr<SkityImageRepresentation>
EpoxyShmImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kOpenGL: {
      return fml::MakeRefCounted<SkityGLImageRepresentation>(
          skity_context,
          fml::MakeRefCounted<EpoxyShmImageRepresentation>(fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR) << "Unable to call "
                        "EpoxyShmImageBacking::CreateSkityRepresentation with "
                        "backend: "
                     << static_cast<uint32_t>(skity_context->GetBackendType());
      return nullptr;
    }
  }
}
#endif  // ENABLE_SKITY

void EpoxyShmImageBacking::CopyPixelsToTexture() {
  if (CreateEGLImageIfNeeded()) {
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_.x, size_.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, shm_buffer_);
  }
}

void EpoxyShmImageBacking::CopyPixelsToShm() {
  glFinish();
  glReadPixels(0, 0, size_.x, size_.y, GL_RGBA, GL_UNSIGNED_BYTE, shm_buffer_);
}

int EpoxyShmImageBacking::GetShmFd() const { return shm_fd_; }

EpoxyShmImageBacking::~EpoxyShmImageBacking() {
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    return;
  }
  if (task_runner_) {
    fml::TaskRunner::RunNowOrPostTask(
        task_runner_, [task_runner = task_runner_, egl_display = egl_display_,
                       egl_image = egl_image_, texture_id = texture_id_,
                       shm_buffer = shm_buffer_, shm_size = size_,
                       shm_fd = shm_fd_]() mutable {
          eglDestroyImageKHR(egl_display, egl_image);
          glDeleteTextures(1, &texture_id);

          if (munmap(shm_buffer, shm_size.x * shm_size.y * 4) == -1) {
            FML_LOG(ERROR) << "EpoxyShmImageBacking munmap failed";
            return;
          }
          close(shm_fd);
          shm_buffer = nullptr;
          shm_fd = 0;
        });
  }
}

}  // namespace clay
