// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_SAMPLING_OPTIONS_H_
#define CLAY_GFX_STYLE_SAMPLING_OPTIONS_H_

#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

enum class FilterMode {
  kNearest,  // single sample point (nearest neighbor)
  kLinear,   // interpolate between 2x2 sample points (bilinear interpolation)

  kLast = kLinear,
};

enum class ImageSampling {
  kNearestNeighbor,
  kLinear,
  kMipmapLinear,
  kCubic,
};

#ifndef ENABLE_SKITY
inline FilterMode ToClay(const SkFilterMode filter_mode) {
  return static_cast<FilterMode>(filter_mode);
}

inline SkFilterMode ToSk(const FilterMode filter_mode) {
  return static_cast<SkFilterMode>(filter_mode);
}

inline ImageSampling ToClay(const SkSamplingOptions& so) {
  if (so.useCubic) {
    return ImageSampling::kCubic;
  }
  if (so.filter == SkFilterMode::kLinear) {
    if (so.mipmap == SkMipmapMode::kLinear) {
      return ImageSampling::kMipmapLinear;
    }
    return ImageSampling::kLinear;
  }
  return ImageSampling::kNearestNeighbor;
}

inline SkSamplingOptions ToSk(ImageSampling sampling) {
  switch (sampling) {
    case ImageSampling::kCubic:
      return SkSamplingOptions(SkCubicResampler{1 / 3.0f, 1 / 3.0f});
    case ImageSampling::kLinear:
      return SkSamplingOptions(SkFilterMode::kLinear);
    case ImageSampling::kMipmapLinear:
      return SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear);
    case ImageSampling::kNearestNeighbor:
      return SkSamplingOptions(SkFilterMode::kNearest);
  }
}
#else
inline skity::SamplingOptions ToSk(ImageSampling sampling) {
  switch (sampling) {
    case ImageSampling::kCubic:
      FML_UNIMPLEMENTED();
      return {};
    case ImageSampling::kLinear:
      return skity::SamplingOptions(skity::FilterMode::kLinear,
                                    skity::MipmapMode::kNone);
    case ImageSampling::kMipmapLinear:
      return skity::SamplingOptions(skity::FilterMode::kLinear,
                                    skity::MipmapMode::kLinear);
    case ImageSampling::kNearestNeighbor:
      return skity::SamplingOptions(skity::FilterMode::kNearest,
                                    skity::MipmapMode::kNone);
  }
}

#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_GFX_STYLE_SAMPLING_OPTIONS_H_
