// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_RADIAL_GRADIENT_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_RADIAL_GRADIENT_LAYER_H_

#include "core/renderer/css/css_utils.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_gradient_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {

class BackgroundRadialGradientLayer : public BackgroundGradientLayer {
  enum class RadialCenterType {
    kTop = (1 << 5),
    kRight,
    kBottom,
    kLeft,
    kCenter,
    kPercentage = 11,
  };

 public:
  explicit BackgroundRadialGradientLayer(const lepus::Value& data);
  ~BackgroundRadialGradientLayer() override;

 protected:
  void OnSizeChange(float width, float height, float scale_density) override;

 private:
  RadialGradientShapeType shape_{starlight::RadialGradientShapeType::kEllipse};
  RadialGradientSizeType shape_size_{
      starlight::RadialGradientSizeType::kFarthestCorner};
  RadialCenterType center_x_{RadialCenterType::kCenter};
  RadialCenterType center_y_{RadialCenterType::kCenter};
  float center_x_value_{0.5f};
  float center_y_value_{0.5f};
  starlight::PlatformLengthUnit shape_size_x_unit_{
      starlight::PlatformLengthUnit::NUMBER};
  starlight::PlatformLengthUnit shape_size_y_unit_{
      starlight::PlatformLengthUnit::NUMBER};
  float shape_size_x_value_{0};
  float shape_size_y_value_{0};
  float point_x_{0.5f};
  float point_y_{0.5f};
  float radius_x_{0};
  float radius_y_{0};
  float aspect_ratio_{0.f};
  float CalculateValue(RadialCenterType type, float value, float base);
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_RADIAL_GRADIENT_LAYER_H_
