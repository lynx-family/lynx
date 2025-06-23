// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_GRADIENT_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_GRADIENT_LAYER_H_

#include <native_drawing/drawing_shader_effect.h>

#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/shader_effect.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundGradientLayer : public BackgroundLayer {
 public:
  ~BackgroundGradientLayer() override;
  BackgroundGradientLayer() = default;
  void OnSizeChange(float width, float height, float scale_density) override;
  void Draw(OH_Drawing_Canvas* canvas, OH_Drawing_Path* path) override;
  const fml::RefPtr<ShaderEffect>& GetShaderEffect() const {
    return shader_effect_;
  }

 protected:
  fml::RefPtr<ShaderEffect> shader_effect_{nullptr};
  OH_Drawing_Brush* brush_{nullptr};
  std::vector<uint32_t> colors_;
  std::vector<float> positions_;
  OH_Drawing_Rect* rect_{nullptr};

  bool IsReady() override;

  void SetColorAndStop(const lepus::Value& colors, const lepus::Value& stops);
  bool IsGradient() override { return true; }
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_GRADIENT_LAYER_H_
