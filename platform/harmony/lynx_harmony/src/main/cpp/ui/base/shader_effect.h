// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_SHADER_EFFECT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_SHADER_EFFECT_H_

#include <native_drawing/drawing_shader_effect.h>
#include <native_drawing/drawing_types.h>

#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace tasm {
namespace harmony {

class ShaderEffect : public fml::RefCountedThreadSafeStorage {
 public:
  static fml::RefPtr<ShaderEffect> CreateLinearGradientEffect(
      float start_x, float start_y, float end_x, float end_y,
      const uint32_t* colors, const float* pos, uint32_t size,
      OH_Drawing_TileMode);
  static fml::RefPtr<ShaderEffect> CreateRadialGradientEffect(
      float center_x, float center_y, float radius, const uint32_t* colors,
      const float* pos, uint32_t size, OH_Drawing_TileMode, OH_Drawing_Matrix*);
  OH_Drawing_ShaderEffect* HarmonyShaderEffect() const {
    return shader_effect_;
  }
  ShaderEffect();
  ~ShaderEffect() override;

 protected:
  void ReleaseSelf() const override;

 private:
  OH_Drawing_ShaderEffect* shader_effect_{nullptr};
  OH_Drawing_Point* start_point_{nullptr};
  OH_Drawing_Point* end_point_{nullptr};
  OH_Drawing_Matrix* gradient_transform_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_SHADER_EFFECT_H_
