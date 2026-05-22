// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_IMAGE_BACKING_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <android/hardware_buffer.h>

#include "base/include/platform/android/scoped_java_ref.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/public/clay.h"

namespace clay {

class SurfaceTexture;
class SharedImageRepresentation;

class SurfaceTextureImageBacking final : public SharedImageBackingUnmanaged {
 public:
  SurfaceTextureImageBacking(
      PixelFormat pixel_format, skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> gfx_handle = {});
  ~SurfaceTextureImageBacking() override;

  const skity::Vec2 GetSize() const override;
  BackingType GetType() const override;
  GraphicsMemoryHandle GetGFXHandle() const override;
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;
#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      skity::GPUContext* skity_context) override;
#endif  // ENABLE_SKITY
  const skity::Matrix GetTransformation() const override;
  void SetTransformation(const skity::Matrix& mat) override;

  /// `SharedImageBackingUnmanaged`
  void SetFrameAvailableCallback(const fml::closure& callback) override;
  bool UpdateFront() override;
  void ReleaseFront() override;
  uint32_t AcquireBack() override;
  bool SwapBack() override;
  uint32_t Capacity() const override;

  bool SetSize(skity::Vec2 size) override;
  const fml::jni::JavaRef<jobject>& GetSurfaceTexture() const;
  GLuint EnsureAttachedToGLContext();
  void DetachGLContext();

 private:
  fml::RefPtr<SurfaceTexture> surface_texture_;
  GLuint texture_ = 0;
};

}  // namespace clay
   //
#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_IMAGE_BACKING_H_
