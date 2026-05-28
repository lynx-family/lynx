// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/shared_image_external_texture.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"
#if defined(ENABLE_SKITY)
#include "clay/gfx/skity/skity_image.h"
#endif

namespace clay {
namespace {

DrawableImage::FitMode ResolveExternalTextureFitMode(
    DrawableImage::FitMode fit_mode, const skity::Rect& bounds,
    float image_width, float image_height) {
  if (fit_mode == DrawableImage::FitMode::kClipToBounds &&
      (image_width < bounds.Width() || image_height < bounds.Height())) {
    // Resize can make the current layout bounds larger than the last produced
    // NativeView frame. Fill the bounds with the stale frame until the producer
    // presents a frame for the new size, otherwise newly exposed areas flicker
    // black.
    return DrawableImage::FitMode::kScaleToFill;
  }
  return fit_mode;
}

}  // namespace

SharedImageExternalTexture::SharedImageExternalTexture(
    fml::RefPtr<SharedImageSink> image_sink)
    : clay::SharedDrawableImage(image_sink) {}

SharedImageExternalTexture::~SharedImageExternalTexture() = default;

// |clay::DrawableImage|
DrawableImage::ImageType SharedImageExternalTexture::GetType() const {
  return ImageType::kSharedImageTexture;
}

// |clay::DrawableImage|
// Called from raster thread.
void SharedImageExternalTexture::Paint(PaintContext& context,
                                       const skity::Rect& bounds, bool freeze,
                                       const GrSamplingOptions& sampling,
                                       FitMode fit_mode) {
  gr_context_ = context.gr_context;

  if (!gr_context_) {
    FML_LOG(ERROR) << "No gpu context";
    return;
  }

  if (!EnsureAttached()) {
    return;
  }
  AdvanceFrameConsumption(freeze);

#ifndef ENABLE_SKITY
  if (!sk_image_) {
    return;
  }

  if (custom_painter_) {
    // FIXME(youfeng) alpha video paint maybe broken now.
    custom_painter_->Paint(context, sk_image_.get(), transform_, bounds,
                           sampling);
  } else {
    DrawSkiaImage(
        sk_image_, context, bounds, sampling,
        ResolveExternalTextureFitMode(fit_mode, bounds, sk_image_->width(),
                                      sk_image_->height()));
  }
#else
  if (!skity_image_) {
    return;
  }

  std::shared_ptr<skity::Image> skity_image = skity_image_->gr_image();
  if (!skity_image) {
    return;
  }
  if (custom_painter_) {
    // FIXME(youfeng) alpha video paint maybe broken now.
    custom_painter_->Paint(context, skity_image.get(), transform_, bounds,
                           sampling);
  } else {
    DrawSkityImage(
        skity_image, context, bounds, sampling,
        ResolveExternalTextureFitMode(fit_mode, bounds, skity_image->Width(),
                                      skity_image->Height()));
  }
#endif  // ENABLE_SKITY
  FlushAndReleaseFrontForSingleBuffer();
}

void SharedImageExternalTexture::SetCustomPainter(
    fml::RefPtr<ExternalTexturePainter> custom_painter) {
  custom_painter_ = std::move(custom_painter);
}

}  // namespace clay
