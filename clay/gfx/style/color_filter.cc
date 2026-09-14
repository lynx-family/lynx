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
  return SrgbToLinearGammaColorFilter::GetInstance();
}

std::shared_ptr<ColorFilter> ColorFilter::MakeLinearToSrgbGamma() {
  return LinearToSrgbGammaColorFilter::GetInstance();
}

const std::shared_ptr<SrgbToLinearGammaColorFilter>&
SrgbToLinearGammaColorFilter::GetInstance() {
  static const lynx::base::NoDestructor<
      std::shared_ptr<SrgbToLinearGammaColorFilter>>
      instance(std::make_shared<SrgbToLinearGammaColorFilter>());
  return *instance;
}

const GrColorFilterPtr& SrgbToLinearGammaColorFilter::GetSkFilter() {
  static const lynx::base::NoDestructor<GrColorFilterPtr> sk_filter(
      GrColorFilters::SRGBToLinearGamma());
  return *sk_filter;
}

const std::shared_ptr<LinearToSrgbGammaColorFilter>&
LinearToSrgbGammaColorFilter::GetInstance() {
  static const lynx::base::NoDestructor<
      std::shared_ptr<LinearToSrgbGammaColorFilter>>
      instance(std::make_shared<LinearToSrgbGammaColorFilter>());
  return *instance;
}

const GrColorFilterPtr& LinearToSrgbGammaColorFilter::GetSkFilter() {
  static const lynx::base::NoDestructor<GrColorFilterPtr> sk_filter(
      GrColorFilters::LinearToSRGBGamma());
  return *sk_filter;
}

}  // namespace clay
