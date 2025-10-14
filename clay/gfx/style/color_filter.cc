// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/color_filter.h"

namespace clay {
std::shared_ptr<ColorFilter> ColorFilter::From(GrColorFilter* sk_filter) {
  if (sk_filter == nullptr) {
    return nullptr;
  }
  if (sk_filter == SrgbToLinearGammaColorFilter::sk_filter_.get()) {
    // Skia implements these filters as a singleton.
    return SrgbToLinearGammaColorFilter::instance;
  }
  if (sk_filter == LinearToSrgbGammaColorFilter::sk_filter_.get()) {
    // Skia implements these filters as a singleton.
    return LinearToSrgbGammaColorFilter::instance;
  }
#ifndef ENABLE_SKITY
  {
    SkColor color;
    SkBlendMode mode;
    if (sk_filter->asAColorMode(&color, &mode)) {
      return std::make_shared<BlendColorFilter>(color,
                                                static_cast<BlendMode>(mode));
    }
  }
  {
    float matrix[20];
    if (sk_filter->asAColorMatrix(matrix)) {
      return std::make_shared<MatrixColorFilter>(matrix);
    }
  }
  return std::make_shared<UnknownColorFilter>(sk_ref_sp(sk_filter));
#else
  // Simply use the unknown color filter to manage the skity color filter.
  return std::make_shared<UnknownColorFilter>(
      std::shared_ptr<GrColorFilter>(sk_filter));
#endif  // ENABLE_SKITY
}

const std::shared_ptr<SrgbToLinearGammaColorFilter>
    SrgbToLinearGammaColorFilter::instance =
        std::make_shared<SrgbToLinearGammaColorFilter>();
const GrColorFilterPtr SrgbToLinearGammaColorFilter::sk_filter_ =
    GrColorFilters::SRGBToLinearGamma();

const std::shared_ptr<LinearToSrgbGammaColorFilter>
    LinearToSrgbGammaColorFilter::instance =
        std::make_shared<LinearToSrgbGammaColorFilter>();
const GrColorFilterPtr LinearToSrgbGammaColorFilter::sk_filter_ =
    GrColorFilters::LinearToSRGBGamma();

}  // namespace clay
