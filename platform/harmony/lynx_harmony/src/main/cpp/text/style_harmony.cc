// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"

#include <native_drawing/drawing_brush.h>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_gradient_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {

void TextStyleHarmony::UpdateTextPaint(float width, float height,
                                       float scale_density) {
  if (stroke_width_ < 0 && !gradient_color_ && !shadow_layer_ &&
      !foreground_pen_ && !foreground_brush_) {
    return;
  }
  EnsureForegroundPenAndBrush();
  if (stroke_width_ >= 0) {
    uint32_t color = stroke_color_.value_or(color_);
    OH_Drawing_PenSetColor(foreground_pen_, color);
    OH_Drawing_PenSetWidth(foreground_pen_, stroke_width_);
    OH_Drawing_PenSetShadowLayer(
        foreground_pen_,
        shadow_layer_ ? shadow_layer_->ShadowLayer() : nullptr);
    OH_Drawing_SetTextStyleForegroundPen(text_style_, foreground_pen_);
  }

  // If has gradient color, use gradient shader effect.
  if (gradient_color_) {
    gradient_color_->OnSizeChange(width, height, scale_density);
    gradient_shader_effect_ = gradient_color_->GetShaderEffect();
    OH_Drawing_BrushSetShaderEffect(
        foreground_brush_, gradient_shader_effect_->HarmonyShaderEffect());
  } else {
    OH_Drawing_BrushSetColor(foreground_brush_, color_);
    gradient_shader_effect_ = nullptr;
  }
  OH_Drawing_BrushSetShadowLayer(
      foreground_brush_,
      shadow_layer_ ? shadow_layer_->ShadowLayer() : nullptr);
  OH_Drawing_SetTextStyleForegroundBrush(text_style_, foreground_brush_);
}

void TextStyleHarmony::SetGradientColor(
    std::unique_ptr<BackgroundGradientLayer> gradient) {
  gradient_color_ = std::move(gradient);
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
