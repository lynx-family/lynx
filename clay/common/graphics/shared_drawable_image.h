// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_SHARED_DRAWABLE_IMAGE_H_
#define CLAY_COMMON_GRAPHICS_SHARED_DRAWABLE_IMAGE_H_

#include <memory>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/common/graphics/drawable_image.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/matrix.hpp"
#if defined(ENABLE_SKITY)
#include "clay/gfx/skity/skity_image.h"
#endif

namespace clay {

class SharedImageSink;
class SharedImageSinkAccessor;

class SharedDrawableImage : public clay::DrawableImage {
 public:
  explicit SharedDrawableImage(fml::RefPtr<SharedImageSink> image_sink);

  ~SharedDrawableImage() override;  // Called from raster thread.

  // |clay::DrawableImage|
  // Called on platform thread.
  void SetFrameAvailableCallback(const fml::closure& callback) override;

  // |clay::DrawableImage|
  // Called on raster thread.
  void MarkNewFrameAvailable() override;

  // |clay::DrawableImage|
  // Called on raster thread.
  void OnDrawableImageUnregistered() override;

  // |clay::ContextListener|
  // Called from raster thread.
  void OnGrContextCreated() override;

  // |clay::ContextListener|
  // Called from raster thread.
  void OnGrContextDestroyed() override;

 protected:
  enum class AttachmentState { uninitialized, attached, detached };

  virtual GrContext* GetGrContext() { return nullptr; }

  // Called from raster thread.
  bool EnsureAttached();

  // Called from raster thread.
  void AdvanceFrameConsumption(bool freeze);

#ifndef ENABLE_SKITY
  // Called from raster thread.
  void DrawSkiaImage(sk_sp<SkImage> sk_image, PaintContext& context,
                     const skity::Rect& bounds,
                     const GrSamplingOptions& sampling, FitMode fit_mode);
#else
  // Called from raster thread.
  void DrawSkityImage(std::shared_ptr<skity::Image> skity_image,
                      PaintContext& context, const skity::Rect& bounds,
                      const GrSamplingOptions& sampling, FitMode fit_mode);
#endif  // ENABLE_SKITY

  // Called from raster thread.
  void FlushAndReleaseFrontForSingleBuffer();

#ifndef ENABLE_SKITY
  sk_sp<SkImage> sk_image_;
#else
  std::shared_ptr<SkityImage> skity_image_;
#endif  // ENABLE_SKITY
  skity::Matrix transform_;

 private:
  [[nodiscard]] bool Update();

  fml::RefPtr<SharedImageSink> image_sink_;
  std::unique_ptr<SharedImageSinkAccessor> accessor_;
  uint32_t frame_consume_cnt_ = 0;
  uint32_t frame_produce_cnt_ = 0;

  AttachmentState state_ = AttachmentState::uninitialized;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_SHARED_DRAWABLE_IMAGE_H_
