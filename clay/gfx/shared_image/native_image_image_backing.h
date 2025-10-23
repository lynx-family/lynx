// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_NATIVE_IMAGE_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_NATIVE_IMAGE_IMAGE_BACKING_H_

#include <GLES2/gl2.h>
#include <native_image/native_image.h>

#include <mutex>

#include "clay/gfx/shared_image/shared_image_backing.h"
#include "skity/geometry/matrix.hpp"

namespace clay {

class NativeImageImageBacking final : public SharedImageBackingUnmanaged {
 public:
  NativeImageImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                          std::optional<GraphicsMemoryHandle> gfx_handle = {});
  ~NativeImageImageBacking() override;

  OH_NativeImage* GetNativeImage() const { return native_image_; }

  const skity::Vec2 GetSize() const override;
  BackingType GetType() const override;
  GraphicsMemoryHandle GetGFXHandle() const override;
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
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
  GLuint EnsureAttachedToGLContext();
  void DetachGLContext();
  void TriggerFrameCallback();

 private:
  OH_NativeImage* native_image_ = nullptr;
  GLuint texture_ = 0;
  std::mutex frame_callback_mutex_;
  fml::closure frame_callback_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_NATIVE_IMAGE_IMAGE_BACKING_H_
