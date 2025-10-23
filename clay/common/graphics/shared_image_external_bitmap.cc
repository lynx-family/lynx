// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/shared_image_external_bitmap.h"

#include <algorithm>
#include <memory>

#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"
#if defined(ENABLE_SKITY)
#include "clay/gfx/skity/skity_image.h"
#endif
namespace clay {

SharedImageExternalBitmap::SharedImageExternalBitmap(
    fml::RefPtr<SharedImageSink> image_sink)
    : clay::SharedDrawableImage(image_sink) {}

SharedImageExternalBitmap::~SharedImageExternalBitmap() = default;

// |clay::DrawableImage|
DrawableImage::ImageType SharedImageExternalBitmap::GetType() const {
  return ImageType::kSharedImageBitmap;
}

// |clay::DrawableImage|
// Called from raster thread.
void SharedImageExternalBitmap::Paint(PaintContext& context,
                                      const skity::Rect& bounds, bool freeze,
                                      const GrSamplingOptions& sampling,
                                      FitMode fit_mode) {
  if (!EnsureAttached()) {
    return;
  }
  AdvanceFrameConsumption(freeze);

#ifndef ENABLE_SKITY
  if (!sk_image_) {
    return;
  }

  DrawSkiaImage(sk_image_, context, bounds, sampling, fit_mode);
#else
  if (!skity_image_) {
    return;
  }

  std::shared_ptr<skity::Image> skity_image = skity_image_->gr_image();
  FML_DCHECK(skity_image);
  DrawSkityImage(skity_image, context, bounds, sampling, fit_mode);
#endif  // ENABLE_SKITY
  FlushAndReleaseFrontForSingleBuffer();
}

}  // namespace clay
