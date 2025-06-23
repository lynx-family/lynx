// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/shader_effect.h"

#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_point.h>
namespace lynx {
namespace tasm {
namespace harmony {
ShaderEffect::ShaderEffect() = default;

fml::RefPtr<ShaderEffect> ShaderEffect::CreateLinearGradientEffect(
    float start_x, float start_y, float end_x, float end_y,
    const uint32_t* colors, const float* pos, uint32_t size,
    OH_Drawing_TileMode mode) {
  auto effect = fml::MakeRefCounted<ShaderEffect>();
  effect->start_point_ = OH_Drawing_PointCreate(start_x, start_y);
  effect->end_point_ = OH_Drawing_PointCreate(end_x, end_y);
  effect->shader_effect_ = OH_Drawing_ShaderEffectCreateLinearGradient(
      effect->start_point_, effect->end_point_, colors, pos, size, mode);
  return effect;
}

fml::RefPtr<ShaderEffect> ShaderEffect::CreateRadialGradientEffect(
    float center_x, float center_y, float radius, const uint32_t* colors,
    const float* pos, uint32_t size, OH_Drawing_TileMode mode,
    OH_Drawing_Matrix* gradient_transform) {
  auto effect = fml::MakeRefCounted<ShaderEffect>();
  effect->gradient_transform_ = gradient_transform;
  OH_Drawing_Point2D center{center_x, center_y};
  effect->shader_effect_ =
      OH_Drawing_ShaderEffectCreateRadialGradientWithLocalMatrix(
          &center, radius, colors, pos, size, mode, gradient_transform);
  return effect;
}

ShaderEffect::~ShaderEffect() {
  if (shader_effect_) {
    OH_Drawing_ShaderEffectDestroy(shader_effect_);
    shader_effect_ = nullptr;
  }
  if (start_point_) {
    OH_Drawing_PointDestroy(start_point_);
    start_point_ = nullptr;
  }
  if (end_point_) {
    OH_Drawing_PointDestroy(end_point_);
    end_point_ = nullptr;
  }
  if (gradient_transform_) {
    OH_Drawing_MatrixDestroy(gradient_transform_);
    gradient_transform_ = nullptr;
  }
}

void ShaderEffect::ReleaseSelf() const { delete this; }
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
