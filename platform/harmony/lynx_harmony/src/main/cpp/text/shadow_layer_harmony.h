// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_SHADOW_LAYER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_SHADOW_LAYER_HARMONY_H_
#include <native_drawing/drawing_shadow_layer.h>

#include "base/include/fml/hash_combine.h"
namespace lynx {
namespace tasm {
namespace harmony {

struct ShadowData {
  float blur_radius = .0f;
  float x = .0f;
  float y = .0f;
  uint32_t color = 0xFF000000;

  bool operator==(const ShadowData& rhs) const {
    return std::tie(blur_radius, x, y, color) ==
           std::tie(rhs.blur_radius, rhs.x, rhs.y, rhs.color);
  }
};

class ShadowLayerHarmony {
 public:
  explicit ShadowLayerHarmony(const ShadowData& data)
      : ShadowLayerHarmony(data.blur_radius, data.x, data.y, data.color) {}

  ShadowLayerHarmony(float blur_radius, float x, float y, uint32_t color)
      : data_({blur_radius, x, y, color}) {
    shadow_layer_ = OH_Drawing_ShadowLayerCreate(blur_radius, x, y, color);
  }
  ShadowLayerHarmony(const ShadowLayerHarmony& other)
      : ShadowLayerHarmony(other.data_.blur_radius, other.data_.x,
                           other.data_.y, other.data_.color) {}

  ~ShadowLayerHarmony() { OH_Drawing_ShadowLayerDestroy(shadow_layer_); }

  OH_Drawing_ShadowLayer* ShadowLayer() const { return shadow_layer_; }

 private:
  ShadowData data_;
  OH_Drawing_ShadowLayer* shadow_layer_{nullptr};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

namespace std {
template <>
struct hash<lynx::tasm::harmony::ShadowData> {
  size_t operator()(
      const lynx::tasm::harmony::ShadowData& shadow_data) const noexcept {
    auto seed = lynx::fml::HashCombine();
    lynx::fml::HashCombineSeed(seed, shadow_data.blur_radius, shadow_data.x,
                               shadow_data.y, shadow_data.color);
    return seed;
  }
};
}  // namespace std

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_SHADOW_LAYER_HARMONY_H_
