// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_DATA_H_
#define CLAY_GFX_IMAGE_IMAGE_DATA_H_

#include <array>

#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/style/length.h"
#if defined(ENABLE_SKITY)
#include "clay/gfx/image/base_image.h"
#endif

namespace clay {

enum class FillMode {
  // fill whole rect, can display whole image content.
  kScaleToFill = 0,
  // keep aspect ratio, can display whole image content.
  kAspectFit,
  // keep aspect ratio, can not display whole image content.
  kAspectFill,
  // don't zoom, only show the center.
  kCenter,
};

/// How to paint any portions of a box not covered by an image.
enum class ImageRepeat {
  // Repeat the image in both the x and y directions until the box is filled.
  kRepeat = 0,
  // Leave uncovered portions of the box transparent.
  kNoRepeat,
  // Repeat the image in the x direction until the box is filled horizontally.
  kRepeatX,
  // Repeat the image in the y direction until the box is filled vertically.
  kRepeatY,
};

// Provide some simple and easy use effects like inverse color / blur ...
enum class ImageEffect {
  kEffectNone = 0,
  kInverseColor = 1,
};

struct ImageData {
#ifndef ENABLE_SKITY
  ImageResource* image_resource = nullptr;
#else
  BaseImageInstance* image_resource = nullptr;
#endif  // ENABLE_SKITY
  FillMode mode = FillMode::kScaleToFill;
  ImageRepeat repeat = ImageRepeat::kRepeat;
  float drop_shadow_offset_x = 0.0f;
  float drop_shadow_offset_y = 0.0f;
  float drop_shadow_blur_radius = 0.0f;
  GrColor drop_shadow_color = ToSk(Color::kBlack());
  bool has_cap_insets = false;
  std::array<Length, 4> cap_insets = {};
  float cap_insets_scale = 1.0f;
  float image_opacity = 1.0f;
  std::optional<Color> tint_color = std::nullopt;
  skity::RRect round_rect = {};
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_DATA_H_
