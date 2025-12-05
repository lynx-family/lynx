// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/color_filter.h"

namespace clay {
std::shared_ptr<ColorFilter> ColorFilter::MakeBlend(Color color,
                                                    BlendMode mode) {
  return std::make_shared<BlendColorFilter>(color, mode);
}

std::shared_ptr<ColorFilter> ColorFilter::MakeMatrix(const float matrix[20]) {
  return std::make_shared<MatrixColorFilter>(matrix);
}

std::shared_ptr<ColorFilter> ColorFilter::MakeSrgbToLinearGamma() {
  return SrgbToLinearGammaColorFilter::instance;
}

std::shared_ptr<ColorFilter> ColorFilter::MakeLinearToSrgbGamma() {
  return LinearToSrgbGammaColorFilter::instance;
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
