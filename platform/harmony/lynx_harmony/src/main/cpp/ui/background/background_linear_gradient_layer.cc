// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_linear_gradient_layer.h"

#include <native_drawing/drawing_point.h>

#include <algorithm>

#include "core/renderer/utils/value_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/shader_effect.h"

namespace lynx {
namespace tasm {
namespace harmony {

static inline float ToRadians(float degrees) { return degrees * M_PI / 180.0; }

void BackgroundLinearGradientLayer::OnSizeChange(float width, float height,
                                                 float scale_density) {
  BackgroundGradientLayer::OnSizeChange(width, height, scale_density);
  width_ = std::max(width_, 1.0f);
  height_ = std::max(height_, 1.0f);
  float left = 0;
  float top = 0;
  if (colors_.size() < 2) {
    shader_effect_ = nullptr;
  } else if (!positions_.empty() && positions_.size() != colors_.size()) {
    shader_effect_ = nullptr;
  } else {
    float start_x = 0;
    float start_y = 0;
    float end_x = 0;
    float end_y = 0;
    float mul =
        2.0f * width_ * height_ / (std::pow(width_, 2) + std::pow(height_, 2));
    if (direction_type_ == DirectionType::kTop) {
      start_x = left;
      start_y = top + height_;
      end_x = left;
      end_y = top;
    } else if (direction_type_ == DirectionType::kBottom) {
      start_x = left;
      start_y = top;
      end_x = left;
      end_y = top + height_;

    } else if (direction_type_ == DirectionType::kLeft) {
      start_x = left + width_;
      start_y = top;
      end_x = left;
      end_y = top;

    } else if (direction_type_ == DirectionType::kRight) {
      start_x = left;
      start_y = top;
      end_x = left + width_;
      end_y = top;
    } else if (direction_type_ == DirectionType::kTopRight) {
      start_x = left + width_ - height_ * mul;
      start_y = top + width_ * mul;
      end_x = left + width_;
      end_y = top;
    } else if (direction_type_ == DirectionType::kTopLeft) {
      start_x = left + height_ * mul;
      start_y = top + width_ * mul;
      end_x = left;
      end_y = top;
    } else if (direction_type_ == DirectionType::kBottomRight) {
      start_x = left;
      start_y = top;
      end_x = left + height_ * mul;
      end_y = top + width_ * mul;
    } else if (direction_type_ == DirectionType::kBottomLeft) {
      start_x = left + width_;
      start_y = top;
      end_x = left + width_ - height_ * mul;
      end_y = top + width_ * mul;
    } else {
      float center_x = width_ / 2.0f;
      float center_y = height_ / 2.0f;
      float m_x = 0;
      float m_y = 0;

      auto radial = ToRadians(angle_);
      float sin = std::sin(radial);
      float cos = std::cos(radial);
      float tan = std::tan(radial);
      if (sin >= 0 && cos >= 0) {
        m_x = width_;
      } else if (sin >= 0 && cos < 0) {
        m_x = width_;
        m_y = height_;
      } else if (sin < 0 && cos < 0) {
        m_y = height_;
      }
      start_x += left;
      start_y += top;
      end_x += left;
      end_y += top;
      center_x += left;
      center_y += top;
      m_x += left;
      m_y += top;
      float tmp = (center_y - m_y - tan * center_x + tan * m_x);
      end_x = center_x + sin * tmp / (sin * tan + cos);
      end_y = center_y - tmp / (tan * tan + 1);
      start_x = 2 * center_x - end_x;
      start_y = 2 * center_y - end_y;
    }
    shader_effect_ = ShaderEffect::CreateLinearGradientEffect(
        start_x, start_y, end_x, end_y, colors_.data(), positions_.data(),
        colors_.size(), CLAMP);
  }
}

BackgroundLinearGradientLayer::~BackgroundLinearGradientLayer() = default;

BackgroundLinearGradientLayer::BackgroundLinearGradientLayer(
    const lepus::Value& data) {
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  if (items->size() < 3) {
    return;
  }
  angle_ = static_cast<float>(items->get(0).Number());
  SetColorAndStop(items->get(1), items->get(2));
  direction_type_ = items->size() == 4
                        ? static_cast<DirectionType>(items->get(3).Number())
                        : DirectionType::kAngel;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
