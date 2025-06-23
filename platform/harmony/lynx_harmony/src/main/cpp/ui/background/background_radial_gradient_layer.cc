// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_radial_gradient_layer.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_matrix.h>

#include <algorithm>

#include "base/include/float_comparison.h"

namespace lynx {
namespace tasm {
namespace harmony {
BackgroundRadialGradientLayer::BackgroundRadialGradientLayer(
    const lynx::lepus::Value& data) {
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  if (items->size() != 3) {
    return;
  }
  auto shape_size_position = items->get(0).Array();
  shape_ = static_cast<RadialGradientShapeType>(
      shape_size_position->get(0).Number());
  shape_size_ =
      static_cast<RadialGradientSizeType>(shape_size_position->get(1).Number());
  center_x_ =
      static_cast<RadialCenterType>(shape_size_position->get(2).Number());
  center_x_value_ = static_cast<float>(shape_size_position->get(3).Number());
  center_y_ =
      static_cast<RadialCenterType>(shape_size_position->get(4).Number());
  center_y_value_ = static_cast<float>(shape_size_position->get(5).Number());
  if (shape_size_ == RadialGradientSizeType::kLength) {
    shape_size_x_value_ = shape_size_position->get(10).Number();
    shape_size_x_unit_ = static_cast<starlight::PlatformLengthUnit>(
        shape_size_position->get(11).Number());
    shape_size_y_value_ = shape_size_position->get(12).Number();
    shape_size_y_unit_ = static_cast<starlight::PlatformLengthUnit>(
        shape_size_position->get(13).Number());
  }
  SetColorAndStop(items->get(1), items->get(2));
}

void BackgroundRadialGradientLayer::OnSizeChange(float width, float height,
                                                 float scale_density) {
  BackgroundGradientLayer::OnSizeChange(width, height, scale_density);
  point_x_ = CalculateValue(center_x_, center_x_value_, width_);
  point_y_ = CalculateValue(center_y_, center_y_value_, height_);

  if (shape_size_ == starlight::RadialGradientSizeType::kLength) {
    radius_x_ = shape_size_x_unit_ == starlight::PlatformLengthUnit::PERCENTAGE
                    ? width_ * shape_size_x_value_
                    : shape_size_x_value_ * scale_density;
    radius_y_ = shape_size_y_unit_ == starlight::PlatformLengthUnit::PERCENTAGE
                    ? height_ * shape_size_y_value_
                    : shape_size_y_value_ * scale_density;
  } else {
    auto radius = lynx::tasm::GetRadialGradientRadius(
        static_cast<lynx::starlight::RadialGradientShapeType>(shape_),
        static_cast<lynx::starlight::RadialGradientSizeType>(shape_size_),
        point_x_, point_y_, width_, height_);
    radius_x_ = radius.first;
    radius_y_ = radius.second;
  }
  if (colors_.size() < 2) {
    shader_effect_ = nullptr;
  } else if (!positions_.empty() && positions_.size() != colors_.size()) {
    shader_effect_ = nullptr;
  } else {
    bool has_zero = radius_x_ == 0 || radius_y_ == 0;
    aspect_ratio_ = has_zero ? 1 : radius_x_ / radius_y_;
    OH_Drawing_Matrix* radial_matrix{nullptr};
    if (base::FloatsNotEqual(aspect_ratio_, 1.0f)) {
      radial_matrix = OH_Drawing_MatrixCreate();
      OH_Drawing_MatrixScale(radial_matrix, 1, 1 / aspect_ratio_, point_x_,
                             point_y_);
    }
    shader_effect_ = ShaderEffect::CreateRadialGradientEffect(
        point_x_, point_y_, std::max(radius_x_, 1.f), colors_.data(),
        positions_.data(), colors_.size(), CLAMP, radial_matrix);
  }
}

float BackgroundRadialGradientLayer::CalculateValue(RadialCenterType type,
                                                    float value, float base) {
  switch (type) {
    case RadialCenterType::kCenter:
      return base * 0.5f;
    case RadialCenterType::kLeft:
    case RadialCenterType::kTop:
      return 0.f;
    case RadialCenterType::kRight:
    case RadialCenterType::kBottom:
      return base;
    case RadialCenterType::kPercentage:
      return base * value / 100.f;
    default:
      return value;
  }
}

BackgroundRadialGradientLayer::~BackgroundRadialGradientLayer() = default;
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
