// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_HARDWAREBUFFER_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_HARDWAREBUFFER_IMAGE_BACKING_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/hardware_buffer.h>

#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/public/clay.h"

namespace clay {

class SharedImageRepresentation;

class AHardwareBufferImageBacking : public SharedImageBacking {
 public:
  AHardwareBufferImageBacking(
      PixelFormat pixel_format, skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> gfx_handle = {});
  ~AHardwareBufferImageBacking() override;

  AHardwareBuffer* GetAHardwareBuffer() const { return buffer_; }
  BackingType GetType() const override;
  GraphicsMemoryHandle GetGFXHandle() const override { return buffer_; }
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;
#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      skity::GPUContext* skity_context) override;
#endif  // ENABLE_SKITY

  static bool IsAHardwareBufferAvailable();

  EGLImageKHR CreateEGLImage(EGLDisplay egl_display);

 private:
  AHardwareBuffer* buffer_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_HARDWAREBUFFER_IMAGE_BACKING_H_
