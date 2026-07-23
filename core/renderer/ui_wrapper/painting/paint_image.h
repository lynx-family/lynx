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
  bool auto_size{false};
  base::String placeholder;
  base::String tint_color;
  base::String cap_insets;
  float cap_insets_scale{1.f};
  bool skip_redirection{false};
  bool autoplay{true};
  int32_t loop_count{0};

  bool operator==(const ImagePaintInfo& other) const {
    return mode == other.mode && blur_radius == other.blur_radius &&
           auto_size == other.auto_size && placeholder == other.placeholder &&
           tint_color == other.tint_color && cap_insets == other.cap_insets &&
           cap_insets_scale == other.cap_insets_scale &&
           skip_redirection == other.skip_redirection &&
           autoplay == other.autoplay && loop_count == other.loop_count;
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
