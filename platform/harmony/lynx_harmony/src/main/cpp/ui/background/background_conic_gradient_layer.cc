// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_conic_gradient_layer.h"

#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_point.h>

#include <algorithm>

#include "core/base/harmony/harmony_function_loader.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/utils/value_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/shader_effect.h"

namespace lynx {
namespace tasm {
namespace harmony {

BackgroundConicGradientLayer::~BackgroundConicGradientLayer() = default;

void BackgroundConicGradientLayer::OnSizeChange(float width, float height,
                                                float scale_density) {
  BackgroundGradientLayer::OnSizeChange(width, height, scale_density);
  center_x_ = center_x_length_.GetValue(width, scale_density);
  center_y_ = center_y_length_.GetValue(height, scale_density);

  OH_Drawing_Matrix* local_matrix{nullptr};
  if (ShaderEffect::SweepGradientWithLocalMatrixHandle() && need_rotation_) {
    has_local_matrix_ = true;
    local_matrix = OH_Drawing_MatrixCreate();
    OH_Drawing_MatrixPreRotate(local_matrix, angle_, center_x_, center_y_);
  }

  shader_effect_ = ShaderEffect::CreateConicGradientEffect(
      center_x_, center_y_, colors_.data(), positions_.data(), colors_.size(),
      CLAMP, local_matrix);
}
void BackgroundConicGradientLayer::Draw(OH_Drawing_Canvas* canvas,
                                        OH_Drawing_Path* path) {
  OH_Drawing_CanvasSave(canvas);
  if (!has_local_matrix_ && need_rotation_) {
    OH_Drawing_CanvasRotate(canvas, angle_, center_x_, center_y_);
  }
  BackgroundGradientLayer::Draw(canvas, path);
  OH_Drawing_CanvasRestore(canvas);
}

BackgroundConicGradientLayer::BackgroundConicGradientLayer(
    const lepus::Value& data) {
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  if (items->size() < 4) {
    return;
  }
  angle_ = static_cast<float>(items->get(0).Number()) - 90.f;
  need_rotation_ = base::FloatsNotEqual(angle_, 0.f);
  const auto& center = items->get(1).Array();
  center_x_length_ = PlatformLength(
      center->get(0), static_cast<PlatformLengthType>(center->get(1).Number()));
  center_y_length_ = PlatformLength(
      center->get(2), static_cast<PlatformLengthType>(center->get(3).Number()));
  SetColorAndStop(items->get(2), items->get(3));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
