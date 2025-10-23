// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_RENDERING_MATERIAL_H_
#define CLAY_GFX_SKITY_RENDERING_MATERIAL_H_

#include <memory>

#include "clay/gfx/style/color_filter.h"
#include "clay/gfx/style/color_source.h"
#include "clay/gfx/style/image_filter.h"
#include "clay/gfx/style/path_effect.h"
#include "skity/effect/color_filter.hpp"
#include "skity/effect/image_filter.hpp"
#include "skity/effect/path_effect.hpp"
#include "skity/effect/shader.hpp"
#include "skity/geometry/matrix.hpp"
#include "skity/graphic/alpha_type.hpp"
#include "skity/graphic/tile_mode.hpp"

namespace clay {

class RenderingMaterial {
 public:
  static std::shared_ptr<skity::Shader> ToSkityLinearGradient(
      const LinearGradientColorSource* linear_gradient_source);

  static std::shared_ptr<skity::Shader> ToSkityRadialGradient(
      const RadialGradientColorSource* radial_gradient_source);

  static std::shared_ptr<skity::PathEffect> ToSkityPathEffect(
      const PathEffect* effect);

  static std::shared_ptr<skity::ColorFilter> ToSkityColorFilter(
      const ColorFilter* color_filter);

  static skity::BlendMode ToSkityBlendMode(BlendMode mode);

  static skity::TileMode ToSkityTileMode(TileMode tile_mode);

  static std::shared_ptr<skity::ImageFilter> ToSkityImageFilter(
      const clay::ImageFilter* image_filter);
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_RENDERING_MATERIAL_H_
