// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_EFFECT_PROCESSOR_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_EFFECT_PROCESSOR_H_
#include <multimedia/image_framework/image/pixelmap_native.h>

#include <memory>
#include <utility>

#include "base/include/closure.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxImageEffectProcessor {
 public:
  enum class ImageEffect {
    kDropShadow,
    kBlur,
    kCapInsets,
    kNone,
  };

  struct CommonViewParams {
    float view_width;
    float view_height;
    float padding_left;
    float padding_top;
    float padding_right;
    float padding_bottom;
    float scale_density;
  };

  struct DropShadowParams {
    float radius;
    uint32_t color;
    float offset_x;
    float offset_y;
    CommonViewParams common_params;
  };

  struct CapInsetParams {
    float cap_inset_left;
    float cap_inset_top;
    float cap_inset_right;
    float cap_inset_bottom;
    float cap_inset_scale;
    CommonViewParams common_params;
  };

  struct BlurParams {
    uint32_t radius;
  };

  using EffectParams = std::variant<std::monostate, DropShadowParams,
                                    BlurParams, CapInsetParams>;
  LynxImageEffectProcessor()
      : effect_(ImageEffect::kNone), params_(std::monostate{}) {}

  LynxImageEffectProcessor(ImageEffect effect, EffectParams params)
      : effect_(effect), params_(std::move(params)) {}

  const ImageEffect& GetEffectType() const { return effect_; };

  OH_PixelmapNative* Process(OH_PixelmapNative* pixel_map) const;

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
