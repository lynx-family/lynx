// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_GRADIENT_H_
#define CLAY_UI_PAINTER_GRADIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/style/color.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/public/value.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

enum class LinearGradientDirection {
  kNone = 0,
  kToTop,
  kToBottom,
  kToLeft,
  kToRight,
  kToTopRight,
  kToTopLeft,
  kToBottomRight,
  kToBottomLeft,
  kAngle,
};

// original enum RadialGradientExtent here.
// In order to be consistent with lynx, we change
// the enum order.
/*
enum class RadialGradientExtent {
  kFarthestCorner = 0,
  kClosestCorner,
  kFarthestSide,
  kClosestSide,
};
*/

enum class RadialGradientExtent {
  kFarthestCorner = 0,
  kFarthestSide,
  kClosestCorner,
  kClosestSide,
  kLength,
};

enum class RadialCenterType {
  BACKGROUND_POSITION_TOP = -(1 << 5),
  BACKGROUND_POSITION_RIGHT = -(1 << 5) - 1,
  BACKGROUND_POSITION_BOTTOM = -(1 << 5) - 2,
  BACKGROUND_POSITION_LEFT = -(1 << 5) - 3,
  BACKGROUND_POSITION_CENTER = -(1 << 5) - 4,
  RADIAL_CENTER_TYPE_PERCENTAGE = -11,  // according to lynx
};

enum class RadialShapeType {
  kEllipse = 0,
  kCircle,
};

struct RadialCenter {
  RadialCenterType center_x;
  RadialCenterType center_y;
  float center_x_value;
  float center_y_value;
};

enum class GradientPositionType {
  kPercent,
  kNumber,
};

struct GradientPosition {
  float value;
  GradientPositionType type;
};

enum class GradientLengthType {
  kNumber,
  kPercent,
};

struct GradientLength {
  float value;
  GradientLengthType type;
};

enum class GradientType {
  kNotSet = 0,
  kLinear,
  kRadial,
  kConic,
};

class GradientFactory;

class Gradient {
 public:
  using ColorAndStops = std::vector<std::pair<GradientPosition, Color>>;

  Gradient();

  static std::optional<Gradient> Create(std::string raw_data);

  // Linear Gradient
  static std::optional<Gradient> CreateLinear(
      const clay::Value::Array& linear_array);
  static std::optional<Gradient> CreateLinear(
      const ClayLinearGradient& gradient_data);
  // Radial Gradient
  static std::optional<Gradient> CreateRadial(
      const clay::Value::Array& radial_array);
  static std::optional<Gradient> CreateRadial(
      const ClayRadialGradient& gradient_data);
  // Conic Gradient
  static std::optional<Gradient> CreateConic(
      const clay::Value::Array& conic_array);
  static std::optional<Gradient> CreateConic(
      const ClayConicGradient& gradient_data);

  GradientType Type() const { return type_; }

  const ColorAndStops& PositionColors() const { return position_colors_; }

  // Only valid when type is linear.
  LinearGradientDirection Direction() const { return direction_; }
  double Angle() const { return angle_; }

  // Only valid when type is radial.
  const FloatPoint& At() const { return at_; }
  RadialGradientExtent Extent() const { return extent_; }

  const std::string& RawData() const { return raw_data_; }

  const RadialCenter& Center() const { return center_; }

  // Conic
  const GradientPosition& ConicCenterX() const { return conic_center_x_; }
  const GradientPosition& ConicCenterY() const { return conic_center_y_; }
  float StartAngle() const { return start_angle_; }
  float EndAngle() const { return end_angle_; }

  // NOTE: Only use Raw Data to compare.
  bool operator!=(const Gradient& oth) const;
  bool operator==(const Gradient& oth) const;

 private:
  friend class GradientFactory;
  FRIEND_TEST(GradientTest, DirectionToPoints);

  GradientType type_;
  ColorAndStops position_colors_;

  // Only valid when type is linear.
  LinearGradientDirection direction_;
  double angle_;

  // Only valid when type is radial.
  FloatPoint at_;
  RadialGradientExtent extent_;
  RadialCenter center_;
  RadialShapeType radial_shape_;
  GradientLength length_x_;
  GradientLength length_y_;

  // Conic
  GradientPosition conic_center_x_;
  GradientPosition conic_center_y_;
  float start_angle_;
  float end_angle_;

  std::string raw_data_;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_GRADIENT_H_
