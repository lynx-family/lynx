// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_SHARED_IMAGE_EXTERNAL_BITMAP_H_
#define CLAY_COMMON_GRAPHICS_SHARED_IMAGE_EXTERNAL_BITMAP_H_

#include "clay/common/graphics/shared_drawable_image.h"

namespace clay {

class SharedImageSink;
class SharedImageSinkAccessor;

class SharedImageExternalBitmap final : public clay::SharedDrawableImage {
 public:
  explicit SharedImageExternalBitmap(fml::RefPtr<SharedImageSink> image_sink);

  ~SharedImageExternalBitmap() override;  // Called from raster thread.

  // |clay::DrawableImage|
  DrawableImage::ImageType GetType() const override;

  // |clay::DrawableImage|
  // Called from raster thread.
  void Paint(PaintContext& context, const skity::Rect& bounds, bool freeze,
             const GrSamplingOptions& sampling, FitMode fit_mode) override;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_SHARED_IMAGE_EXTERNAL_BITMAP_H_
