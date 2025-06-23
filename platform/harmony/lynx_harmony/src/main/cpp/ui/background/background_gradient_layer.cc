// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_gradient_layer.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_shader_effect.h>

#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
bool BackgroundGradientLayer::IsReady() { return true; }

void BackgroundGradientLayer::Draw(OH_Drawing_Canvas* canvas,
                                   OH_Drawing_Path* path) {
  if (!shader_effect_) {
    return;
  }
  OH_Drawing_BrushSetShaderEffect(brush_,
                                  shader_effect_->HarmonyShaderEffect());
  OH_Drawing_CanvasAttachBrush(canvas, brush_);
  if (path) {
    OH_Drawing_CanvasDrawPath(canvas, path);
  } else {
    OH_Drawing_CanvasDrawRect(canvas, rect_);
  }
  OH_Drawing_CanvasDetachBrush(canvas);
}

BackgroundGradientLayer::~BackgroundGradientLayer() {
  if (brush_) {
    OH_Drawing_BrushDestroy(brush_);
  }
  if (rect_) {
    OH_Drawing_RectDestroy(rect_);
  }
}

void BackgroundGradientLayer::OnSizeChange(float width, float height,
                                           float scale_density) {
  if (width_ != width || height != height_) {
    if (!brush_) {
      brush_ = OH_Drawing_BrushCreate();
      OH_Drawing_BrushSetAntiAlias(brush_, true);
    }
    width_ = width;
    height_ = height;
    if (rect_) {
      OH_Drawing_RectDestroy(rect_);
      rect_ = nullptr;
    }
    rect_ = OH_Drawing_RectCreate(0, 0, width, height);
  }
}

void BackgroundGradientLayer::SetColorAndStop(const lepus::Value& colors,
                                              const lepus::Value& stops) {
  if (!colors.IsArray() || !stops.IsArray()) {
    return;
  }
  auto stops_array = stops.Array();
  auto colors_array = colors.Array();
  if (stops_array->size() != 0 && stops_array->size() != colors_array->size()) {
    return;
  }
  colors_.clear();
  positions_.clear();
  bool need_default_stops = stops_array->size() == 0;
  auto length = colors_array->size();
  float posScale = 1.0f / (length - 1);
  for (size_t i = 0; i < length; ++i) {
    colors_.emplace_back(static_cast<uint32_t>(colors_array->get(i).Number()));
    if (need_default_stops) {
      positions_.push_back(i == length - 1 ? 1 : i * posScale);
    } else {
      positions_.emplace_back(
          static_cast<float>(stops_array->get(i).Number() / 100.f));
    }
  }
}

}  // namespace harmony

}  // namespace tasm

}  // namespace lynx
