// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_SHARED_IMAGE_EXTERNAL_TEXTURE_H_
#define CLAY_COMMON_GRAPHICS_SHARED_IMAGE_EXTERNAL_TEXTURE_H_

#include "clay/common/graphics/shared_drawable_image.h"
#include "clay/gfx/gpu_ref_object.h"

namespace clay {

class SharedImageSink;
class SharedImageSinkAccessor;
class SkityImage;

class ExternalTexturePainter : public GPURefObject {
 public:
  // This method is called in raster thread.
  virtual void Paint(clay::DrawableImage::PaintContext& context,
                     const GrImage* sk_image, const skity::Matrix& uv_transform,
                     const skity::Rect& bounds,
                     const GrSamplingOptions& sampling) = 0;
};

class SharedImageExternalTexture final : public clay::SharedDrawableImage {
 public:
  explicit SharedImageExternalTexture(fml::RefPtr<SharedImageSink> image_sink);

  ~SharedImageExternalTexture() override;  // Called from raster thread.

  // |clay::DrawableImage|
  DrawableImage::ImageType GetType() const override;

  // |clay::DrawableImage|
  // Called from raster thread.
  void Paint(PaintContext& context, const skity::Rect& bounds, bool freeze,
             const GrSamplingOptions& sampling, FitMode fit_mode) override;

  void SetCustomPainter(fml::RefPtr<ExternalTexturePainter> custom_painter);

 protected:
  GrContext* GetGrContext() override { return gr_context_; }

 private:
  GrContext* gr_context_ = nullptr;

  fml::RefPtr<ExternalTexturePainter> custom_painter_;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_SHARED_IMAGE_EXTERNAL_TEXTURE_H_
