// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_EGL_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_EGL_IMAGE_BACKING_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <android/hardware_buffer.h>

#include "base/include/fml/task_runner.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

class SharedImageRepresentation;

class EGLImageBacking : public SharedImageBacking {
 public:
  EGLImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                  std::optional<GraphicsMemoryHandle> gfx_handle = {});
  ~EGLImageBacking() override;
  BackingType GetType() const override;
  GraphicsMemoryHandle GetGFXHandle() const override { return nullptr; }
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;
#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      skity::GPUContext* skity_context) override;
#endif  // ENABLE_SKITY

  void BindToTexture(GLenum target);

 private:
  EGLImageKHR egl_image_ = nullptr;
  EGLDisplay egl_display_ = nullptr;
  EGLContext egl_context_ = nullptr;

  GLuint texture_id_ = 0;

  fml::RefPtr<fml::TaskRunner> task_runner_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_EGL_IMAGE_BACKING_H_
