// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Environment snapshot consumed by MediaQueryEvaluator when it answers
// "does this media query match?". Adding a new feature means adding a
// field here and a case in MediaQueryEvaluator::EvalFeature.

#ifndef CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_VALUES_H_
#define CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_VALUES_H_

#include <string>

namespace lynx {
namespace css {

enum class MediaOrientation {
  kUnknown,
  kPortrait,
  kLandscape,
};

// Three-valued flags. "kUnknown" means the feature is not supported on this
// surface, causing any query referencing it to fail to match.
enum class MediaTristate {
  kUnknown,
  kNone,
  kPresent,
};

// Maps to the `prefers-color-scheme` media feature.
enum class MediaPreferredColorScheme {
  kLight,
  kDark,
};

class MediaValues {
 public:
  MediaValues() = default;

  // ---- viewport --------------------------------------------------------
  double ViewportWidth() const { return viewport_width_; }
  double ViewportHeight() const { return viewport_height_; }
  void SetViewportWidth(double v) { viewport_width_ = v; }
  void SetViewportHeight(double v) { viewport_height_ = v; }

  // ---- density ---------------------------------------------------------
  double DevicePixelRatio() const { return device_pixel_ratio_; }
  void SetDevicePixelRatio(double v) { device_pixel_ratio_ = v; }

  // ---- font roots ------------------------------------------------------
  // Used when a feature value carries rem/em units.
  double RootFontSize() const { return root_font_size_; }
  double FontSize() const { return font_size_; }
  void SetRootFontSize(double v) { root_font_size_ = v; }
  void SetFontSize(double v) { font_size_ = v; }

  // ---- orientation -----------------------------------------------------
  MediaOrientation Orientation() const {
    if (orientation_ != MediaOrientation::kUnknown) return orientation_;
    // Derive from viewport size when the surface did not set it explicitly.
    if (viewport_width_ <= 0 || viewport_height_ <= 0) {
      return MediaOrientation::kUnknown;
    }
    return viewport_height_ >= viewport_width_ ? MediaOrientation::kPortrait
                                               : MediaOrientation::kLandscape;
  }
  void SetOrientation(MediaOrientation v) { orientation_ = v; }

  // ---- interaction -----------------------------------------------------
  MediaTristate Hover() const { return hover_; }
  MediaTristate Pointer() const { return pointer_; }
  void SetHover(MediaTristate v) { hover_ = v; }
  void SetPointer(MediaTristate v) { pointer_ = v; }

  // ---- user preferences ------------------------------------------------
  MediaPreferredColorScheme PreferredColorScheme() const {
    return preferred_color_scheme_;
  }
  void SetPreferredColorScheme(MediaPreferredColorScheme v) {
    preferred_color_scheme_ = v;
  }

  // ---- color -----------------------------------------------------------
  int ColorBitsPerComponent() const { return color_bits_per_component_; }
  void SetColorBitsPerComponent(int v) { color_bits_per_component_ = v; }

  // Convenience factory that fills in the pieces the renderer nearly always
  // has readily available.
  static MediaValues WithViewport(double width, double height,
                                  double device_pixel_ratio) {
    MediaValues v;
    v.viewport_width_ = width;
    v.viewport_height_ = height;
    v.device_pixel_ratio_ = device_pixel_ratio;
    return v;
  }

 private:
  double viewport_width_ = 0.0;
  double viewport_height_ = 0.0;
  double device_pixel_ratio_ = 1.0;
  double root_font_size_ = 16.0;
  double font_size_ = 16.0;
  MediaOrientation orientation_ = MediaOrientation::kUnknown;
  MediaTristate hover_ = MediaTristate::kUnknown;
  MediaTristate pointer_ = MediaTristate::kUnknown;
  MediaPreferredColorScheme preferred_color_scheme_ =
      MediaPreferredColorScheme::kLight;
  int color_bits_per_component_ = 8;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_VALUES_H_
