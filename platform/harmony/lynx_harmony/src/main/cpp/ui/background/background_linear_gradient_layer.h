// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_LINEAR_GRADIENT_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_LINEAR_GRADIENT_LAYER_H_

#include "core/base/lynx_export.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_gradient_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundLinearGradientLayer : public BackgroundGradientLayer {
 public:
  enum class DirectionType : unsigned {
    kTop = 1,
    kBottom,
    kLeft,
    kRight,
    kTopRight,
    kTopLeft,
    kBottomRight,
    kBottomLeft,
    kAngel,
  };

  LYNX_EXPORT explicit BackgroundLinearGradientLayer(const lepus::Value& data);

  LYNX_EXPORT ~BackgroundLinearGradientLayer() override;

  LYNX_EXPORT void OnSizeChange(float width, float height,
                                float scale_density) override;

 private:
  float angle_{0};
  DirectionType direction_type_{DirectionType::kAngel};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_LINEAR_GRADIENT_LAYER_H_
