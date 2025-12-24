// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_EFFECT_PROCESSOR_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_EFFECT_PROCESSOR_H_
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_types.h>

#include <format>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "base/include/closure.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxImageEffectProcessor : public ImageProcessor {
 public:
  enum class ImageEffect {
    kNone = 0,
    kDropShadow,
    kCapInsets,
  };

  struct CommonViewParams {
    float view_width;
    float view_height;
    float padding_left;
    float padding_top;
    float padding_right;
    float padding_bottom;
    float scale_density;
    std::string ToString() const {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2);
      oss << "w:" << view_width << ","
          << "h:" << view_height << ","
          << "pl:" << padding_left << ","
          << "pt:" << padding_top << ","
          << "pr:" << padding_right << ","
          << "pb:" << padding_bottom << ","
          << "scale:" << scale_density;
      return oss.str();
    }
  };

  struct DropShadowParams {
    float radius;
    uint32_t color;
    float offset_x;
    float offset_y;
    CommonViewParams common_params;
    std::string ToString() const {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2);
      oss << "drop-shadow:{radius:" << radius << ","
          << "color:" << color << ","
          << "x:" << offset_x << ","
          << "y:" << offset_y << ",";
      return oss.str() + common_params.ToString() + "}";
    }
  };

  struct CapInsetParams {
    float cap_inset_left;
    float cap_inset_top;
    float cap_inset_right;
    float cap_inset_bottom;
    float cap_inset_scale;
    CommonViewParams common_params;
    std::string ToString() const {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2);
      oss << "cap-insets:{l:" << cap_inset_left << ","
          << "t:" << cap_inset_top << ","
          << "r:" << cap_inset_right << ","
          << "b:" << cap_inset_bottom << ","
          << "scale: " << cap_inset_scale << ",";
      return oss.str() + common_params.ToString() + "}";
    }
  };

  using EffectParams =
      std::variant<std::monostate, DropShadowParams, CapInsetParams>;
  LynxImageEffectProcessor()
      : effect_(ImageEffect::kNone), params_(std::monostate{}) {}

  LynxImageEffectProcessor(ImageEffect effect, EffectParams params)
      : effect_(effect), params_(std::move(params)) {}

  const ImageEffect& GetEffectType() const { return effect_; };

  OH_PixelmapNative* Process(OH_PixelmapNative* pixel_map) const override;

  std::string Info() const override;

 private:
  ImageEffect effect_;
  EffectParams params_;

  OH_PixelmapNative* CustomProcessor(const ImageEffect& effect,
                                     const EffectParams& params,
                                     OH_PixelmapNative* pixel_map) const;

  OH_PixelmapNative* ApplyDropShadowToBitmap(
      const DropShadowParams& params, OH_PixelmapNative* pixel_map) const;

  OH_PixelmapNative* ApplyCapInsetsToBitmap(const CapInsetParams& params,
                                            OH_PixelmapNative* pixel_map) const;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_EFFECT_PROCESSOR_H_
