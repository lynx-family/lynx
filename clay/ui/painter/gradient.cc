// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/gradient.h"

#include <optional>
#include <string>

#include "clay/fml/logging.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/painter/gradient_factory.h"

namespace clay {

namespace utils = attribute_utils;

namespace {
// These values are defined in Lynx
constexpr int kPositionTypeNumber = 2;
constexpr int kPositionTypePercent = 11;

template <typename T>
void SetColorAndStop(const clay::Value::Array& array, T& gradient) {
  // colors:
  const auto& color_array = utils::GetArray(array[1]);
  for (size_t j = 0; j < color_array.size(); j++) {
    gradient.colors[j] = utils::GetUint(color_array[j]);
  }
  // positions (maybe empty):
  const auto& position_array = utils::GetArray(array[2]);
  for (size_t j = 0; j < position_array.size(); j++) {
    float value = utils::GetNum(position_array[j]) / 100;
    auto type = kPositionTypePercent;
    if (type == kPositionTypeNumber) {
      gradient.position_types[j] = ClayGradientPositionType::kNumber;
    } else {
      FML_DCHECK(type == kPositionTypePercent);
      gradient.position_types[j] = ClayGradientPositionType::kPercent;
    }
    gradient.positions[j] = value;
  }
  if (position_array.size() > 0) {
    FML_DCHECK(position_array.size() == color_array.size());
  }
}

}  // namespace

Gradient::Gradient() : type_(GradientType::kNotSet) {}

// static
std::optional<Gradient> Gradient::CreateLinear(
    const clay::Value::Array& linear_array) {
  FML_DCHECK(linear_array.size() > 0);
  ClayLinearGradient gradient;
  // degree:
  gradient.degree = utils::GetDouble(linear_array[0]);
  std::vector<unsigned int> colors(utils::GetArray(linear_array[1]).size());
  std::vector<float> positions(utils::GetArray(linear_array[2]).size());
  std::vector<ClayGradientPositionType> types(
      utils::GetArray(linear_array[2]).size());
  gradient.colors = colors.data();
  gradient.colors_length = colors.size();
  gradient.positions = positions.data();
  gradient.positions_length = positions.size();
  gradient.position_types = types.data();
  SetColorAndStop(linear_array, gradient);
  return CreateLinear(gradient);
}

// static
std::optional<Gradient> Gradient::CreateRadial(
    const clay::Value::Array& radial_array) {
  FML_DCHECK(radial_array.size() > 0);
  ClayRadialGradient gradient;
  auto& shape_size_position = utils::GetArray(radial_array[0]);
  // shape type and size:
  gradient.shape_type = static_cast<ClayRadialGradientShapeType>(
      utils::GetInt(shape_size_position[0]));
  gradient.shape_size = static_cast<ClayRadialGradientSizeType>(
      utils::GetInt(shape_size_position[1]));
  // [x-position-type, x-position, y-position-type y-position]:
  gradient.center_x = static_cast<ClayRadialGradientCenterType>(
      utils::GetInt(shape_size_position[2]));
  gradient.center_x_value =
      static_cast<float>(utils::GetDouble(shape_size_position[3]));
  gradient.center_y = static_cast<ClayRadialGradientCenterType>(
      utils::GetInt(shape_size_position[4]));
  gradient.center_y_value =
      static_cast<float>(utils::GetDouble(shape_size_position[5]));
  // For length value: [x_pattern, x_value, y_pattern, y_value]
  if (gradient.shape_size == ClayRadialGradientSizeType::kLength) {
    gradient.length_x =
        static_cast<float>(utils::GetDouble(shape_size_position[10]));
    gradient.length_x_unit = static_cast<ClayPlatformLengthUnit>(
        utils::GetInt(shape_size_position[11]));
    gradient.length_y =
        static_cast<float>(utils::GetDouble(shape_size_position[12]));
    gradient.length_y_unit = static_cast<ClayPlatformLengthUnit>(
        utils::GetInt(shape_size_position[13]));
  }
  // colors:
  std::vector<unsigned int> colors(utils::GetArray(radial_array[1]).size());
  // positions
  std::vector<float> positions(utils::GetArray(radial_array[2]).size());
  std::vector<ClayGradientPositionType> types(
      utils::GetArray(radial_array[2]).size());
  gradient.colors = colors.data();
  gradient.colors_length = colors.size();
  gradient.positions = positions.data();
  gradient.positions_length = positions.size();
  gradient.position_types = types.data();
  SetColorAndStop(radial_array, gradient);
  return CreateRadial(gradient);
}

/**
 parameter convention is as following
 [
   start_angle,
   [x, x_is_percent, y, y_is_percent],
   [color, color, ...],
   [[start_angle, end_angle], [start_angle, end_angle], ...]
 ]
 */
// static
std::optional<Gradient> Gradient::CreateConic(
    const clay::Value::Array& conic_array) {
  FML_DCHECK(conic_array.size() > 0);
  ClayConicGradient gradient;
  gradient.start_angle = utils::GetDouble(conic_array[0]);
  auto& center_array = utils::GetArray(conic_array[1]);
  auto& colors_array = utils::GetArray(conic_array[2]);
  auto& angle_pair_array = utils::GetArray(conic_array[3]);
  gradient.center_x = static_cast<float>(utils::GetDouble(center_array[0]));
  gradient.x_is_percent = static_cast<bool>(utils::GetInt(center_array[1]));
  gradient.center_y = static_cast<float>(utils::GetDouble(center_array[2]));
  gradient.y_is_percent = static_cast<bool>(utils::GetInt(center_array[3]));
  // Flatten angle-pair array to angle pair, and also transform color array.
  FML_DCHECK(colors_array.size() == angle_pair_array.size());
  std::vector<unsigned int> colors(colors_array.size() * 2);
  std::vector<float> positions(angle_pair_array.size() * 2);
  std::vector<ClayGradientPositionType> position_types(angle_pair_array.size() *
                                                       2);
  gradient.colors_length = colors.size();
  gradient.positions_length = positions.size();
  gradient.colors = colors.data();
  gradient.positions = positions.data();
  gradient.position_types = position_types.data();
  for (size_t i = 0; i < colors_array.size(); i++) {
    gradient.colors[i * 2] = utils::GetUint(colors_array[i]);
    gradient.colors[i * 2 + 1] = utils::GetUint(colors_array[i]);
  }
  for (size_t i = 0; i < angle_pair_array.size(); i++) {
    auto& angles = utils::GetArray(angle_pair_array[i]);
    FML_DCHECK(angles.size() == 2);
    gradient.positions[i * 2] = utils::GetDouble(angles[0]);
    gradient.positions[i * 2 + 1] = utils::GetDouble(angles[1]);
    gradient.position_types[i * 2] = ClayGradientPositionType::kNumber;
    gradient.position_types[i * 2 + 1] = ClayGradientPositionType::kNumber;
  }
  return CreateConic(gradient);
}

// static
std::optional<Gradient> Gradient::Create(std::string raw_data) {
  return GradientFactory::Create(std::move(raw_data));
}

// static
std::optional<Gradient> Gradient::CreateLinear(
    const ClayLinearGradient& gradient_data) {
  return GradientFactory::CreateLinear(gradient_data);
}

// static
std::optional<Gradient> Gradient::CreateRadial(
    const ClayRadialGradient& gradient_data) {
  return GradientFactory::CreateRadial(gradient_data);
}

// static
std::optional<Gradient> Gradient::CreateConic(
    const ClayConicGradient& gradient_data) {
  return GradientFactory::CreateConic(gradient_data);
}

bool Gradient::operator==(const Gradient& oth) const {
  return raw_data_ == oth.raw_data_;
}

bool Gradient::operator!=(const Gradient& oth) const {
  return raw_data_ != oth.raw_data_;
}

}  // namespace clay
