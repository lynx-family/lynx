// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_PAINT_IMAGE_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_PAINT_IMAGE_H_

#include <cstdint>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/value/base_string.h"

namespace lynx::tasm {

enum class ImageFitMode : int32_t {
  kScaleToFill = 0,
  kAspectFit = 1,
  kAspectFill = 2,
  kCenter = 3,
};

struct ImagePaintInfo {
  ImageFitMode mode{ImageFitMode::kScaleToFill};
  base::String blur_radius;

  bool operator==(const ImagePaintInfo& other) const {
    return mode == other.mode && blur_radius == other.blur_radius;
  }

  bool operator!=(const ImagePaintInfo& other) const {
    return !(*this == other);
  }
};

class PaintImage : public fml::RefCountedThreadSafe<PaintImage> {
 public:
  explicit PaintImage(int32_t image_key) : image_key_(image_key) {}
  virtual ~PaintImage() = default;
  int32_t image_key_;
};
}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PAINT_IMAGE_H_
