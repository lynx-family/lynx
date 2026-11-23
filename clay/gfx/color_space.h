// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_COLOR_SPACE_H_
#define CLAY_GFX_COLOR_SPACE_H_

#include <memory>

#include "clay/public/clay.h"

#ifdef ENABLE_SKITY
#include "skity/graphic/color_space.hpp"
#else
#include "third_party/skia/include/core/SkColorSpace.h"
#endif

namespace clay {

#ifdef ENABLE_SKITY
inline std::shared_ptr<const skity::ColorSpace> ToSkityColorSpace(
    ClayColorSpace color_space) {
  return color_space == kClayColorSpaceDisplayP3
             ? skity::ColorSpace::DisplayP3()
             : skity::ColorSpace::SRGB();
}
#else
inline sk_sp<SkColorSpace> ToSkColorSpace(ClayColorSpace color_space) {
  return color_space == kClayColorSpaceDisplayP3
             ? SkColorSpace::MakeRGB(SkNamedTransferFn::kSRGB,
                                     SkNamedGamut::kDisplayP3)
             : SkColorSpace::MakeSRGB();
}
#endif

}  // namespace clay

#endif  // CLAY_GFX_COLOR_SPACE_H_
