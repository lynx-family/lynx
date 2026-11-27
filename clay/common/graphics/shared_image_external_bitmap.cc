// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/shared_image_external_bitmap.h"

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
  if (!freeze) frozen_image_ = nullptr;
#ifndef ENABLE_SKITY
  const auto& current_image = sk_image_;
#else
  const auto& current_image = skity_image_;
#endif
  auto freeze_snapshot = [&] {
    if (freeze && !frozen_image_) {
      frozen_image_ = current_image;
      frozen_transform_ = transform_;
    }
  };
  freeze_snapshot();
  // Drain completed frames even when displaying an older snapshot, so freeze
  // does not pin producer buffers. The displayed image owns independent pixels.
  AdvanceFrameConsumption(false);
  ReleaseFrontAfterSnapshot();
  freeze_snapshot();

  auto image = frozen_image_ ? frozen_image_ : current_image;
  if (!image) {
    return;
  }
  const auto latest_transform = transform_;
  if (frozen_image_) transform_ = frozen_transform_;
#ifndef ENABLE_SKITY
  DrawSkiaImage(image, context, bounds, sampling, fit_mode);
#else
  std::shared_ptr<skity::Image> skity_image = image->gr_image();
  FML_DCHECK(skity_image);
  DrawSkityImage(skity_image, context, bounds, sampling, fit_mode);
#endif  // ENABLE_SKITY
  transform_ = latest_transform;
}

void SharedImageExternalBitmap::OnGrContextDestroyed() {
  frozen_image_ = nullptr;
  SharedDrawableImage::OnGrContextDestroyed();
}

}  // namespace clay
