// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>

#include "clay/gfx/style/color_unittests_helper.h"
#include "clay/ui/painter/gradient.h"
#include "clay/ui/painter/gradient_factory.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(GradientTest, CreateLinearGradient) {
  std::string raw_data("linear-gradient(45deg, blue, red)");
  std::optional<Gradient> gradient = Gradient::Create(raw_data);
  EXPECT_TRUE(gradient.has_value());
  EXPECT_EQ(gradient->Type(), GradientType::kLinear);
  EXPECT_EQ(gradient->Direction(), LinearGradientDirection::kAngle);
  EXPECT_DOUBLE_EQ(gradient->Angle(), 45.0 * M_PI / 180.0);

  {
    const auto& pos_colors = gradient->PositionColors();
    EXPECT_EQ(pos_colors.size(), 2u);
    EXPECT_EQ(pos_colors[0].first.value, 0.f);
    EXPECT_EQ_RGBO(pos_colors[0].second, 0, 0, 255, 1.0);

    EXPECT_EQ(pos_colors[1].first.value, 1.f);
    EXPECT_EQ_RGBO(pos_colors[1].second, 255, 0, 0, 1.0);
  }

  EXPECT_EQ(raw_data, gradient->RawData());

  // Invalid Case
  raw_data = "linear-gradient(45deg)";
  gradient = Gradient::Create(raw_data);
  EXPECT_FALSE(gradient.has_value());

  raw_data = "linear-gradient(to top, black, cyan)";
  gradient = Gradient::Create(raw_data);
  EXPECT_TRUE(gradient.has_value());
  EXPECT_EQ(gradient->Type(), GradientType::kLinear);
  EXPECT_EQ(gradient->Direction(), LinearGradientDirection::kToTop);

  raw_data = "linear-gradient(tolefttop, black, cyan)";
  gradient = Gradient::Create(raw_data);
  EXPECT_TRUE(gradient.has_value());
  EXPECT_EQ(gradient->Type(), GradientType::kLinear);
  EXPECT_EQ(gradient->Direction(), LinearGradientDirection::kToTopLeft);

  // No direction
  raw_data = "linear-gradient(black, cyan)";
  gradient = Gradient::Create(raw_data);
  EXPECT_TRUE(gradient.has_value());
  EXPECT_EQ(gradient->Type(), GradientType::kLinear);
  EXPECT_EQ(gradient->Direction(), LinearGradientDirection::kToBottom);
  EXPECT_EQ(gradient->PositionColors().size(), 2u);

  // More than 2 colors.
  raw_data = "linear-gradient(135deg, red, red 60%, blue)";
  gradient = Gradient::Create(raw_data);
  EXPECT_TRUE(gradient.has_value());
  {
    const auto& pos_colors = gradient->PositionColors();
    EXPECT_EQ(pos_colors.size(), 3u);
    EXPECT_EQ(pos_colors[0].first.value, 0.f);
    EXPECT_EQ(pos_colors[1].first.value, 0.6f);
    EXPECT_EQ(pos_colors[2].first.value, 1.f);
  }

  // Interpolate pos.
  raw_data =
      "linear-gradient(135deg, white, white, white 20%, red, red, red, red "
      "60%, blue, blue, blue, blue)";
  gradient = Gradient::Create(raw_data);
  EXPECT_TRUE(gradient.has_value());
  {
    const auto& pos_colors = gradient->PositionColors();
    EXPECT_EQ(pos_colors.size(), 11u);
    EXPECT_FLOAT_EQ(pos_colors[0].first.value, 0.f);
    EXPECT_FLOAT_EQ(pos_colors[1].first.value, 0.1f);
    EXPECT_FLOAT_EQ(pos_colors[2].first.value, 0.2f);
    EXPECT_FLOAT_EQ(pos_colors[3].first.value, 0.3f);
    EXPECT_FLOAT_EQ(pos_colors[4].first.value, 0.4f);
    EXPECT_FLOAT_EQ(pos_colors[5].first.value, 0.5f);
    EXPECT_FLOAT_EQ(pos_colors[6].first.value, 0.6f);
    EXPECT_FLOAT_EQ(pos_colors[7].first.value, 0.7f);
    EXPECT_FLOAT_EQ(pos_colors[8].first.value, 0.8f);
    EXPECT_FLOAT_EQ(pos_colors[9].first.value, 0.9f);
    EXPECT_FLOAT_EQ(pos_colors[10].first.value, 1.f);
  }
}

TEST(GradientTest, CreateLinearGradientFromArray) {
  clay::Value::Array colors;
  colors.emplace_back(0xffff0000u);
  colors.emplace_back(0xff0000ffu);
  clay::Value::Array positions;
  positions.emplace_back(0.f);
  positions.emplace_back(100.f);
  clay::Value::Array linear_array;
  linear_array.emplace_back(45.f);
  linear_array.emplace_back(std::move(colors));
  linear_array.emplace_back(std::move(positions));
  linear_array.emplace_back(
      static_cast<int>(LinearGradientDirection::kToTopRight));
  auto parsed_gradient = Gradient::CreateLinear(linear_array);
  ASSERT_TRUE(parsed_gradient.has_value());
  EXPECT_EQ(parsed_gradient->Direction(), LinearGradientDirection::kToTopRight);

  clay::Value::Array legacy_colors;
  legacy_colors.emplace_back(0xffff0000u);
  legacy_colors.emplace_back(0xff0000ffu);
  clay::Value::Array legacy_positions;
  legacy_positions.emplace_back(0.f);
  legacy_positions.emplace_back(100.f);
  clay::Value::Array legacy_linear_array;
  legacy_linear_array.emplace_back(45.f);
  legacy_linear_array.emplace_back(std::move(legacy_colors));
  legacy_linear_array.emplace_back(std::move(legacy_positions));
  auto legacy_gradient = Gradient::CreateLinear(legacy_linear_array);
  ASSERT_TRUE(legacy_gradient.has_value());
  EXPECT_EQ(legacy_gradient->Direction(), LinearGradientDirection::kAngle);
}

TEST(GradientTest, DirectionToPoints) {
  Gradient gradient;
  gradient.type_ = GradientType::kLinear;

  gradient.direction_ = LinearGradientDirection::kToTop;
  skity::Vec2 pts[2];
  GradientFactory::ParseLinearGradientStartPoints(
      gradient, FloatRect(50.f, 100.f, 20, 40.f), pts);
  EXPECT_FLOAT_EQ(pts[0].x, 60.f);
  EXPECT_FLOAT_EQ(pts[0].y, 140.f);
  EXPECT_FLOAT_EQ(pts[1].x, 60.f);
  EXPECT_FLOAT_EQ(pts[1].y, 100.f);

  gradient.direction_ = LinearGradientDirection::kToBottom;
  GradientFactory::ParseLinearGradientStartPoints(
      gradient, FloatRect(50.f, 100.f, 20, 40.f), pts);
  EXPECT_FLOAT_EQ(pts[1].x, 60.f);
  EXPECT_FLOAT_EQ(pts[1].y, 140.f);
  EXPECT_FLOAT_EQ(pts[0].x, 60.f);
  EXPECT_FLOAT_EQ(pts[0].y, 100.f);

  gradient.direction_ = LinearGradientDirection::kToLeft;
  GradientFactory::ParseLinearGradientStartPoints(
      gradient, FloatRect(50.f, 100.f, 20, 40.f), pts);
  EXPECT_FLOAT_EQ(pts[0].x, 70.f);
  EXPECT_FLOAT_EQ(pts[0].y, 120.f);
  EXPECT_FLOAT_EQ(pts[1].x, 50.f);
  EXPECT_FLOAT_EQ(pts[1].y, 120.f);

  gradient.direction_ = LinearGradientDirection::kToTopRight;
  GradientFactory::ParseLinearGradientStartPoints(
      gradient, FloatRect(50.f, 100.f, 20, 40.f), pts);
  EXPECT_FLOAT_EQ(pts[0].x, 44.f);
  EXPECT_FLOAT_EQ(pts[0].y, 128.f);
  EXPECT_FLOAT_EQ(pts[1].x, 76.f);
  EXPECT_FLOAT_EQ(pts[1].y, 112.f);
}

}  // namespace clay
