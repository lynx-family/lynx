// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_CONIC_GRADIENT_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_CONIC_GRADIENT_LAYER_H_

#include "core/renderer/css/css_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_gradient_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundConicGradientLayer : public BackgroundGradientLayer {
 public:
  explicit BackgroundConicGradientLayer(const lepus::Value& data);

  ~BackgroundConicGradientLayer() override = default;

  void OnSizeChange(float width, float height, float scale_density) override;
  void Draw(OH_Drawing_Canvas* canvas, OH_Drawing_Path* path) override;

 private:
  float angle_{0};
  bool need_rotation_{false};
  bool has_local_matrix_{false};
  PlatformLength center_x_length_;
  PlatformLength center_y_length_;
  float center_x_{0};
  float center_y_{0};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_CONIC_GRADIENT_LAYER_H_
