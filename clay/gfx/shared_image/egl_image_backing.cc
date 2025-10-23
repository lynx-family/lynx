// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/egl_image_backing.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "base/include/fml/message_loop.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/public/clay.h"
namespace clay {

EGLImageBacking::EGLImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                                 std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  egl_display_ = eglGetCurrentDisplay();
  egl_context_ = eglGetCurrentContext();

  FML_DCHECK(EGL_NO_CONTEXT != egl_context_);
  FML_DCHECK(EGL_NO_DISPLAY != egl_display_);

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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);

  const EGLint egl_attrib_list[] = {
      EGL_GL_TEXTURE_LEVEL_KHR, 0, EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
  EGLClientBuffer egl_buffer = reinterpret_cast<EGLClientBuffer>(texture);
  EGLenum egl_target = EGL_GL_TEXTURE_2D_KHR;

  egl_image_ = eglCreateImageKHR(egl_display_, egl_context_, egl_target,
                                 egl_buffer, egl_attrib_list);

  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    FML_LOG(ERROR) << "eglCreateImageKHR for cross-thread sharing failed: "
                   << eglGetError();
    glDeleteTextures(1, &texture);
    return;
  }
  texture_id_ = texture;

#ifndef NDEBUG
  GLint max_texture_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  if (size.x > max_texture_size || size.y > max_texture_size) {
    FML_LOG(ERROR) << "texture size is too large, supported max size is: "
                   << max_texture_size;
  }
#endif
}

SharedImageBacking::BackingType EGLImageBacking::GetType() const {
  return BackingType::kEGLImage;
}

void EGLImageBacking::BindToTexture(GLenum target) {
  FML_DCHECK(egl_image_ != EGL_NO_IMAGE_KHR);
  glEGLImageTargetTexture2DOES(target, egl_image_);
  FML_DCHECK(static_cast<EGLint>(EGL_SUCCESS) == eglGetError());
}

fml::RefPtr<SharedImageRepresentation> EGLImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  ClaySharedImageRepresentationType type = config->type;
  switch (type) {
    case kClaySharedImageRepresentationTypeGL: {
      return fml::MakeRefCounted<NativeBufferEGLImageRepresentation>(
          fml::Ref(this));
    }
    default: {
      // NOT SUPPORTED.
      FML_LOG(ERROR)
          << "Unable to call EGLImageBacking::CreateRepresentation with type: "
          << static_cast<uint32_t>(type);
      return nullptr;
    }
  }
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation> EGLImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
    case GrBackendApi::kOpenGL: {
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<NativeBufferEGLImageRepresentation>(
                          fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR) << "Unable to call "
                        "EGLImageBacking::CreateSkiaRepresentation with "
                        "backend: "
                     << static_cast<uint32_t>(gr_context->backend());
      return nullptr;
    }
  }
}
#else
fml::RefPtr<SkityImageRepresentation>
EGLImageBacking::CreateSkityRepresentation(skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kOpenGL: {
      return fml::MakeRefCounted<SkityGLImageRepresentation>(
          skity_context,
          fml::MakeRefCounted<NativeBufferEGLImageRepresentation>(
              fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR) << "Unable to call "
                        "EGLImageBacking::CreateSkityRepresentation with "
                        "backend: "
                     << static_cast<uint32_t>(skity_context->GetBackendType());
      return nullptr;
    }
  }
}
#endif  // ENABLE_SKITY

EGLImageBacking::~EGLImageBacking() {
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    return;
  }
  if (task_runner_) {
    fml::TaskRunner::RunNowOrPostTask(
        task_runner_, [egl_context = egl_context_, egl_display = egl_display_,
                       egl_image = egl_image_, texture_id = texture_id_] {
          FML_DCHECK(eglGetCurrentContext() == egl_context);
          eglDestroyImageKHR(egl_display, egl_image);
          glDeleteTextures(1, &texture_id);
        });
  }
}

}  // namespace clay
