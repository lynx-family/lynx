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
  enum Change : uint32_t {
    kModeChanged = 1u << 0,
    kBlurRadiusChanged = 1u << 1,
    kAutoSizeChanged = 1u << 2,
    kDownsamplingChanged = 1u << 3,
    // Bit 4 is reserved to keep the remaining flags aligned with Android.
    kPrefetchWidthChanged = 1u << 5,
    kPrefetchHeightChanged = 1u << 6,
    kPlaceholderChanged = 1u << 7,
    kTintColorChanged = 1u << 8,
    kCapInsetsChanged = 1u << 9,
    kCapInsetsScaleChanged = 1u << 10,
    kSkipRedirectionChanged = 1u << 11,
    kAutoplayChanged = 1u << 12,
    kLoopCountChanged = 1u << 13,
    kAllChanged = ((1u << 14) - 1) & ~(1u << 4),
  };

  ImageFitMode mode{ImageFitMode::kScaleToFill};
  base::String blur_radius;
  bool auto_size{false};
  bool is_background{false};
  bool downsampling{false};
  base::String request_priority;
  base::String prefetch_width;
  base::String prefetch_height;
  base::String placeholder;
  base::String tint_color;
  base::String cap_insets;
  float cap_insets_scale{1.f};
  bool skip_redirection{false};
  bool autoplay{true};
  int32_t loop_count{0};

  // Compare snapshots; platforms decide which changes require a new request.
  // Priority only affects future requests and does not trigger an update.
  uint32_t Diff(const ImagePaintInfo& old) const {
    uint32_t changes = 0;
    if (mode != old.mode) changes |= kModeChanged;
    if (blur_radius != old.blur_radius) changes |= kBlurRadiusChanged;
    if (auto_size != old.auto_size) changes |= kAutoSizeChanged;
    if (downsampling != old.downsampling) changes |= kDownsamplingChanged;
    if (prefetch_width != old.prefetch_width) changes |= kPrefetchWidthChanged;
    if (prefetch_height != old.prefetch_height)
      changes |= kPrefetchHeightChanged;
    if (placeholder != old.placeholder) changes |= kPlaceholderChanged;
    if (tint_color != old.tint_color) changes |= kTintColorChanged;
    if (cap_insets != old.cap_insets) changes |= kCapInsetsChanged;
    if (cap_insets_scale != old.cap_insets_scale)
      changes |= kCapInsetsScaleChanged;
    if (skip_redirection != old.skip_redirection)
      changes |= kSkipRedirectionChanged;
    if (autoplay != old.autoplay) changes |= kAutoplayChanged;
    if (loop_count != old.loop_count) changes |= kLoopCountChanged;
    return changes;
  }

  bool operator==(const ImagePaintInfo& other) const {
    // Priority applies to future requests; changing it alone must not reload.
    return mode == other.mode && blur_radius == other.blur_radius &&
           auto_size == other.auto_size &&
           is_background == other.is_background &&
           downsampling == other.downsampling &&
           prefetch_width == other.prefetch_width &&
           prefetch_height == other.prefetch_height &&
           placeholder == other.placeholder && tint_color == other.tint_color &&
           cap_insets == other.cap_insets &&
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
  // Opt in only when the loader can reuse its request across layout changes.
  virtual bool CanReuseOnResize() const { return false; }
  int32_t image_key_;
};
}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PAINT_IMAGE_H_
