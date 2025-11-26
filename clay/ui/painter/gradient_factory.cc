// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/gradient_factory.h"

#include <ctype.h>
#include <math.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "clay/fml/logging.h"
#include "clay/ui/common/utils/floating_comparison.h"

namespace clay {

namespace {

static constexpr char kLinearPrefix[] = "linear-gradient(";
static constexpr char kRadialPrefix[] = "radial-gradient(";
static const std::unordered_map<std::string, LinearGradientDirection>
    kDirectionMap = {
        {"top", LinearGradientDirection::kToTop},
        {"bottom", LinearGradientDirection::kToBottom},
        {"left", LinearGradientDirection::kToLeft},
        {"right", LinearGradientDirection::kToRight},
        {"topright", LinearGradientDirection::kToTopRight},
        {"righttop", LinearGradientDirection::kToTopRight},
        {"topleft", LinearGradientDirection::kToTopLeft},
        {"lefttop", LinearGradientDirection::kToTopLeft},
        {"bottomright", LinearGradientDirection::kToBottomRight},
        {"rightbottom", LinearGradientDirection::kToBottomRight},
        {"bottomleft", LinearGradientDirection::kToBottomLeft},
        {"leftbottom", LinearGradientDirection::kToBottomLeft},
};
static constexpr float kPositionNotSet = -2.f;

static const std::unordered_map<LinearGradientDirection, FloatPoint>
    kDirectionToStartPoints = {
        {LinearGradientDirection::kToTop, {0.f, -1.f}},
        {LinearGradientDirection::kToBottom, {0.f, 1.f}},
        {LinearGradientDirection::kToLeft, {-1.f, 0.f}},
        {LinearGradientDirection::kToRight, {1.f, 0.f}},
        {LinearGradientDirection::kToTopRight, {1.f, -1.f}},
        {LinearGradientDirection::kToTopLeft, {-1.f, -1.f}},
        {LinearGradientDirection::kToBottomRight, {1.f, 1.f}},
        {LinearGradientDirection::kToBottomLeft, {-1.f, 1.f}},
};

template <typename T>
void ConvertColorAndStops(Gradient::ColorAndStops* position_colors,
                          const T& gradient_data) {
  // invalid or empty positions:
  bool has_valid_positions = true;
  std::vector<GradientPosition> positions;
  if ((!gradient_data.positions) ||
      (gradient_data.positions && gradient_data.positions_length < 2) ||
      (gradient_data.positions &&
       gradient_data.positions_length != gradient_data.colors_length)) {
    has_valid_positions = false;

    // If given positions are invalid or empty, generate by ourself.
    // positions.size() should be same as |gradient_data.colors_length|.
    // push the first element:
    positions.push_back({0.f, GradientPositionType::kPercent});

    auto position = positions[0];
    float delta = 1.f / (gradient_data.colors_length - 1);
    // push the middle |gradient_data.colors_length - 2| elements:
    for (int i = 0; i < gradient_data.colors_length - 2; i++) {
      position.value += delta;
      positions.push_back(position);
    }
    // push the last element:
    positions.push_back({1.f, GradientPositionType::kPercent});
  }

  for (int i = 0; i < gradient_data.colors_length; i++) {
    GradientPosition position;
    if (has_valid_positions) {
      position.value = gradient_data.positions[i];
      position.type =
          static_cast<GradientPositionType>(gradient_data.position_types[i]);
    } else {
      position = positions[i];
    }
    position_colors->emplace_back(position, gradient_data.colors[i]);
  }
}

uint32_t LerpColor(uint32_t start_color, uint32_t end_color, float start_pos,
                   float end_pos, float current_pos) {
  float t = (current_pos - start_pos) / (end_pos - start_pos);
  return static_cast<uint32_t>(
      Color::Lerp(Color(start_color), Color(end_color), t));
}

void ClampColorAndStopListAtFront(std::vector<uint32_t>& colors,
                                  std::vector<float>& stops,
                                  uint32_t first_positive_index) {
  float prev_stop = stops[first_positive_index - 1];
  uint32_t prev_color = colors[first_positive_index - 1];

  float current_stop = stops[first_positive_index];
  uint32_t current_color = colors[first_positive_index];

  uint32_t result_color =
      LerpColor(prev_color, current_color, prev_stop, current_stop, 0.f);
  // update prev content
  stops[first_positive_index - 1] = 0.f;
  colors[first_positive_index - 1] = result_color;

  // remove all other negative stops and colors
  if (first_positive_index - 1 > 0) {
    stops.erase(stops.begin(), stops.begin() + first_positive_index - 1);
    colors.erase(colors.begin(), colors.begin() + first_positive_index - 1);
  }
}

void ClampColorAndStopListAtBack(std::vector<uint32_t>& colors,
                                 std::vector<float>& stops,
                                 uint32_t tail_position) {
  float prev_stop = stops[tail_position - 1];
  uint32_t prev_color = colors[tail_position - 1];

  float current_stop = stops[tail_position];
  uint32_t current_color = colors[tail_position];

  uint32_t result_color =
      LerpColor(prev_color, current_color, prev_stop, current_stop, 1.f);
  // update tail content
  stops[tail_position] = 1.f;
  colors[tail_position] = result_color;

  // remote all other greater than 100% stops and colors
  if (tail_position + 1 < stops.size()) {
    stops.erase(stops.begin() + tail_position + 1, stops.end());
    colors.erase(colors.begin() + tail_position + 1, colors.end());
  }
}

void ClampColorAndStopList(std::vector<uint32_t>& colors,
                           std::vector<float>& stops) {
  if (stops.size() < 2) {
    return;
  }
  bool clamp_front = stops.front() < 0.f;
  bool clamp_back = stops.back() > 1.f;

  if (!clamp_front && !clamp_back) {
    return;
  }

  if (clamp_front) {
    // find first positive position
    uint32_t first_positive_index = 0;
    auto result = std::find_if(stops.begin(), stops.end(),
                               [](float v) { return v >= 0.f; });
    if (result != stops.end()) {
      first_positive_index = static_cast<uint32_t>(result - stops.begin());
    }

    if (first_positive_index != 0) {
      ClampColorAndStopListAtFront(colors, stops, first_positive_index);
    }
  }

  if (clamp_back) {
    // find fist greater than 1.f position
    uint32_t tail_position = 0;
    auto result = std::find_if(stops.begin(), stops.end(),
                               [](float v) { return v >= 1.f; });
    if (result != stops.end()) {
      tail_position = static_cast<uint32_t>(result - stops.begin());
    }
    if (tail_position != 0) {
      ClampColorAndStopListAtBack(colors, stops, tail_position);
    }
  }
}

}  // namespace

// static
std::optional<Gradient> GradientFactory::CreateLinear(
    const ClayLinearGradient& gradient_data) {
  // invalid colors:
  if (!gradient_data.colors || gradient_data.colors_length < 2) {
    return std::nullopt;
  }

  std::optional<Gradient> res(std::in_place);
  res->type_ = GradientType::kLinear;
  res->direction_ = LinearGradientDirection::kAngle;
  res->angle_ = gradient_data.degree / 180.0 * M_PI;
  ConvertColorAndStops(&res->position_colors_, gradient_data);

  return res;
}

// static
std::optional<Gradient> GradientFactory::CreateRadial(
    const ClayRadialGradient& gradient_data) {
  // invalid colors:
  if (!gradient_data.colors || gradient_data.colors_length < 2) {
    return std::nullopt;
  }

  std::optional<Gradient> res(std::in_place);
  res->type_ = GradientType::kRadial;

  // shape
  res->radial_shape_ = static_cast<RadialShapeType>(gradient_data.shape_type);

  // center
  res->center_.center_x = static_cast<RadialCenterType>(gradient_data.center_x);
  res->center_.center_x_value = gradient_data.center_x_value;
  res->center_.center_y = static_cast<RadialCenterType>(gradient_data.center_y);
  res->center_.center_y_value = gradient_data.center_y_value;

  // extent: will be converted to radius later
  res->extent_ = static_cast<RadialGradientExtent>(gradient_data.shape_size);

  res->length_x_.type =
      static_cast<GradientLengthType>(gradient_data.length_x_unit);
  res->length_x_.value = gradient_data.length_x;
  res->length_y_.type =
      static_cast<GradientLengthType>(gradient_data.length_y_unit);
  res->length_y_.value = gradient_data.length_y;

  ConvertColorAndStops(&res->position_colors_, gradient_data);

  return res;
}

// static
std::optional<Gradient> GradientFactory::CreateConic(
    const ClayConicGradient& gradient_data) {
  if (!gradient_data.colors || gradient_data.colors_length < 2) {
    return std::nullopt;
  }

  std::optional<Gradient> res(std::in_place);
  res->type_ = GradientType::kConic;
  res->conic_center_x_.type = gradient_data.x_is_percent
                                  ? GradientPositionType::kPercent
                                  : GradientPositionType::kNumber;
  res->conic_center_x_.value = gradient_data.center_x;
  res->conic_center_y_.type = gradient_data.y_is_percent
                                  ? GradientPositionType::kPercent
                                  : GradientPositionType::kNumber;
  res->conic_center_y_.value = gradient_data.center_y;

  res->start_angle_ = gradient_data.start_angle;
  res->end_angle_ = gradient_data.end_angle;

  ConvertColorAndStops(&res->position_colors_, gradient_data);
  return res;
}

// static
std::optional<Gradient> GradientFactory::Create(std::string raw_data) {
  GradientType type;
  std::string args_str;
  if (lynx::base::BeginsWith(raw_data, kLinearPrefix)) {
    type = GradientType::kLinear;
    args_str = raw_data.substr(strlen(kLinearPrefix));
  } else if (lynx::base::BeginsWith(raw_data, kRadialPrefix)) {
    type = GradientType::kRadial;
    args_str = raw_data.substr(strlen(kRadialPrefix));
  }
  if (args_str.empty()) {
    return std::nullopt;
  }

  std::vector<std::string_view> args = ParseArgs(args_str);
  if (args.empty()) {
    return std::nullopt;
  }

  std::optional<Gradient> res(std::in_place);
  res->type_ = type;

  int color_start_idx = 0;
  ParseResult parse_res = ParseDirection(args[0], res.value());
  if (parse_res == ParseResult::kFailed) {
    return std::nullopt;
  } else if (parse_res == ParseResult::kSucceeded) {
    color_start_idx = 1;
  } else {  // NotFound
    color_start_idx = 0;
    res->direction_ = LinearGradientDirection::kToBottom;
  }

  parse_res = ParseColorPositions(args, color_start_idx, res.value());
  if (parse_res != ParseResult::kSucceeded) {
    return std::nullopt;
  }
  res->raw_data_ = std::move(raw_data);

  return res;
}

// static
std::vector<std::string_view> GradientFactory::ParseArgs(
    const std::string_view& args) {
  std::vector<std::string_view> res;
  // used to handle xxx-gradient(xxx, rgba(xxx), xxx)
  int left_parenthesis = 1;
  size_t substr_head = 0;
  size_t cur_idx = 0;

  const auto add_arg = [&args, &substr_head, &cur_idx, &res]() -> bool {
    if (substr_head >= cur_idx) {
      return false;
    }

    res.emplace_back(lynx::base::TrimToStringView(
        args.substr(substr_head, cur_idx - substr_head)));
    substr_head = cur_idx + 1;
    return true;
  };

  while (cur_idx < args.length()) {
    const char cur = args[cur_idx];
    switch (cur) {
      case '(':
        ++left_parenthesis;
        break;
      case ')':
        --left_parenthesis;
        if (left_parenthesis == 0) {
          if (!add_arg()) {
            return {};
          }
        }
        break;
      case ',':
        if (left_parenthesis <= 1) {
          if (!add_arg()) {
            return {};
          }
        }
        break;
      default:
        break;
    }
    cur_idx++;
  }
  return res;
}

// static
GradientFactory::ParseResult GradientFactory::ParseDirection(
    const std::string_view& arg, Gradient& gradient) {
  if (lynx::base::BeginsWith(arg, "to")) {
    // case 1: to xxx (xxx)
    // Note(Xietong): refer to iOS implementation. Tolerate string without
    // space (e.g. "tobottomleft")
    std::string dir = lynx::base::RemoveSpaces(arg.substr(strlen("to")));
    const auto itr = kDirectionMap.find(dir);
    if (itr == kDirectionMap.end()) {
      return ParseResult::kFailed;
    }
    gradient.direction_ = itr->second;
  } else if (lynx::base::EndsWith(arg, "deg")) {
    // case 2: xxx deg
    gradient.direction_ = LinearGradientDirection::kAngle;
    const std::string value_str(arg.substr(0, arg.length() - 3));
    double value;
    if (lynx::base::StringToDouble(value_str, value)) {
      gradient.angle_ = value / 180.0 * M_PI;
    } else {
      return ParseResult::kFailed;
    }
  } else if (lynx::base::EndsWith(arg, "rad")) {
    // case 3: xxx rad
    gradient.direction_ = LinearGradientDirection::kAngle;
    const std::string value_str(arg.substr(0, arg.length() - 3));
    double value;
    if (lynx::base::StringToDouble(value_str, value)) {
      gradient.angle_ = value;
    } else {
      return ParseResult::kFailed;
    }
  } else if (lynx::base::EndsWith(arg, "turn")) {
    // case 4: xxx turn
    gradient.direction_ = LinearGradientDirection::kAngle;
    const std::string value_str(arg.substr(0, arg.length() - 4));
    double value;
    if (lynx::base::StringToDouble(value_str, value)) {
      gradient.angle_ = value * 2.0 * M_PI;
    } else {
      return ParseResult::kFailed;
    }
  } else {
    return ParseResult::kNotFound;
  }
  return ParseResult::kSucceeded;
}

// static
GradientFactory::ParseResult GradientFactory::ParseColorPositions(
    const std::vector<std::string_view>& args, size_t start_idx,
    Gradient& gradient) {
  int parsed_color_count = 0;
  int not_set_pos_idx = -1;
  for (size_t idx = start_idx; idx < args.size(); ++idx) {
    std::string_view cur_str = args[idx];
    std::string_view prefix;
    size_t color_end = cur_str.find_first_of(')');
    if (color_end != std::string_view::npos) {
      prefix = cur_str.substr(0, color_end);
      cur_str = cur_str.substr(color_end);
    }
    std::vector<std::string_view> tokens =
        lynx::base::SplitToStringViews(cur_str, " ");
    FML_DCHECK(tokens.size() > 0);
    std::string color_str(prefix);
    color_str += tokens[0];
    Color to_parse;
    if (!Color::Parse(color_str, &to_parse)) {
      continue;
    }

    float position = kPositionNotSet;
    if (parsed_color_count == 0) {
      position = 0.f;
    }
    ++parsed_color_count;

    if (tokens.size() == 1 && idx != args.size() - 1) {
      gradient.position_colors_.emplace_back(
          GradientPosition{position, GradientPositionType::kPercent}, to_parse);
      if (position == kPositionNotSet && not_set_pos_idx == -1) {
        not_set_pos_idx = gradient.position_colors_.size() - 1;
      }
      continue;
    } else if (tokens.size() == 1 && idx == args.size() - 1) {
      // TODO(Xietong): confirm if this is right.
      position = 1.f;
    } else if (lynx::base::EndsWith(tokens[1], "%")) {
      if (!lynx::base::StringToFloat(
              std::string(tokens[1].substr(0, tokens[1].length() - 1)),
              position)) {
        return ParseResult::kFailed;
      }
      position = position / 100.f;
    } else {
      // NOTE: We do not support non-percentage values (e.g. 10px) here, as this
      // method is for internal use only.
      if (!lynx::base::StringToFloat(std::string(tokens[1]), position)) {
        return ParseResult::kFailed;
      }
    }
    gradient.position_colors_.emplace_back(
        GradientPosition{position, GradientPositionType::kPercent}, to_parse);

    // Interpolate the previous not set positions
    if (not_set_pos_idx != -1) {
      const float low =
          gradient.position_colors_[not_set_pos_idx - 1].first.value;
      const float high = position;
      const float num =
          static_cast<float>(parsed_color_count - 1 - not_set_pos_idx) + 1.f;
      for (int i = not_set_pos_idx; i < parsed_color_count - 1; ++i) {
        gradient.position_colors_[i].first.value =
            low + (high - low) / num * (i - not_set_pos_idx + 1);
      }
    }
    not_set_pos_idx = -1;
  }

  if (gradient.position_colors_.empty()) {
    return ParseResult::kNotFound;
  }

  return ParseResult::kSucceeded;
}

// Support both linear-gradient and radial-gradient
// static
std::shared_ptr<ColorSource> GradientFactory::CreateShader(
    const Gradient& gradient, const FloatRect& rect) {
  if (gradient.Type() == GradientType::kLinear) {
    return CreateLinearShader(gradient, rect);
  } else if (gradient.Type() == GradientType::kRadial) {
    return CreateRadialShader(gradient, rect);
  } else if (gradient.Type() == GradientType::kConic) {
    return CreateConicShader(gradient, rect);
  } else {
    // GradientType::kNoSet
    return std::shared_ptr<ColorSource>();
  }
}

// static
std::shared_ptr<ColorSource> GradientFactory::CreateLinearShader(
    const Gradient& gradient, const FloatRect& rect) {
  skity::Vec2 pts[2];
  ParseLinearGradientStartPoints(gradient, rect, pts);

  float total_length = sqrtf((pts[1].x - pts[0].x) * (pts[1].x - pts[0].x) +
                             (pts[1].y - pts[0].y) * (pts[1].y - pts[0].y));
  const Gradient::ColorAndStops& pos_to_colors = gradient.PositionColors();
  uint32_t stop_count = pos_to_colors.size();
  std::vector<float> positions(stop_count);
  std::vector<uint32_t> colors(stop_count);
  for (size_t i = 0; i < stop_count; i++) {
    auto item = pos_to_colors[i];
    switch (item.first.type) {
      case GradientPositionType::kNumber:
        positions[i] = item.first.value / total_length;
        break;
      case GradientPositionType::kPercent:
      default:
        positions[i] = item.first.value;
        break;
    }
    colors[i] = item.second.Value();
  }
  ClampColorAndStopList(colors, positions);

  static_assert(sizeof(Color) == sizeof(uint32_t));
  return ColorSource::MakeLinear(pts[0], pts[1], stop_count,
                                 reinterpret_cast<Color*>(colors.data()),
                                 positions.data(), TileMode::kClamp, nullptr);
}

// static
std::shared_ptr<ColorSource> GradientFactory::CreateRadialShader(
    const Gradient& gradient, const FloatRect& rect) {
  FloatPoint at;
  ParseRadialCenter(gradient, rect, at);
  skity::Vec2 sk_center = skity::Vec2(at.x(), at.y());
  double radius = 0;
  float width = rect.width();
  float height = rect.height();
  skity::Matrix matrix;
  ParseRadialRadiusAndMatrix(gradient, radius, width, height, at, matrix);
  float sk_radius = radius;
  const Gradient::ColorAndStops& pos_to_colors = gradient.PositionColors();
  uint32_t stop_count = pos_to_colors.size();
  std::vector<float> positions(stop_count);
  std::vector<uint32_t> colors(stop_count);
  for (size_t i = 0; i < stop_count; i++) {
    auto item = pos_to_colors[i];
    switch (item.first.type) {
      case GradientPositionType::kNumber:
        positions[i] = item.first.value / radius;
        break;
      case GradientPositionType::kPercent:
      default:
        positions[i] = item.first.value;
        break;
    }
    colors[i] = item.second.Value();
  }
  ClampColorAndStopList(colors, positions);

  static_assert(sizeof(Color) == sizeof(uint32_t));
  return ColorSource::MakeRadial(sk_center, sk_radius, stop_count,
                                 reinterpret_cast<Color*>(colors.data()),
                                 positions.data(), TileMode::kClamp, &matrix);
}

// static
std::shared_ptr<ColorSource> GradientFactory::CreateConicShader(
    const Gradient& gradient, const FloatRect& rect) {
  FloatPoint center;
  ParseConicCenter(gradient, rect, center);
  const Gradient::ColorAndStops& pos_to_colors = gradient.PositionColors();
  uint32_t stop_count = pos_to_colors.size();
  std::vector<float> positions(stop_count);
  std::vector<uint32_t> colors(stop_count);
  for (size_t i = 0; i < stop_count; i++) {
    auto& item = pos_to_colors[i];
    const GradientPosition& pos = item.first;
    const Color& color = item.second;
    switch (pos.type) {
      case GradientPositionType::kNumber:
        positions[i] = item.first.value / 360.0;
        break;
      case GradientPositionType::kPercent:
      default:
        positions[i] = item.first.value;
        break;
    }
    colors[i] = color.Value();
  }
  ClampColorAndStopList(colors, positions);
  skity::Vec2 sk_center = skity::Vec2(center.x(), center.y());
  // Skia and Skity default start at positive x axis, but web standard start at
  // negative y axis. Thus rotate counterclockwise by 90'' here to comply with
  // web standard.
  skity::Matrix mtx =
      skity::Matrix::RotateDeg(gradient.StartAngle() - 90.0, sk_center);

  static_assert(sizeof(Color) == sizeof(uint32_t));
  return ColorSource::MakeSweep(sk_center, 0, gradient.EndAngle(), stop_count,
                                reinterpret_cast<Color*>(colors.data()),
                                positions.data(), TileMode::kClamp, &mtx);
}

// static
void GradientFactory::ParseRadialRadiusAndMatrix(
    const Gradient& gradient, double& radius, const float& width,
    const float& height, const FloatPoint& at, skity::Matrix& matrix) {
  float center_x_val = at.x();
  float center_y_val = at.y();
  switch (gradient.Extent()) {
    case RadialGradientExtent::kClosestSide: {
      float x = std::min(center_x_val, width - center_x_val);
      float y = std::min(center_y_val, height - center_y_val);
      radius = std::min(x, y);
      if (gradient.radial_shape_ == RadialShapeType::kEllipse) {
        matrix.PostScale(x / radius, y / radius);
        matrix.PostTranslate(center_x_val - center_x_val * x / radius,
                             center_y_val - center_y_val * y / radius);
      }
      break;
    }
    case RadialGradientExtent::kClosestCorner: {
      float x = std::min(center_x_val, width - center_x_val);
      float y = std::min(center_y_val, height - center_y_val);
      if (gradient.radial_shape_ == RadialShapeType::kCircle) {
        radius = std::sqrt(x * x + y * y);
      } else {
        radius = std::min(x, y);
        matrix.PostScale(x / radius, y / radius);
        matrix.PostTranslate(center_x_val - center_x_val * x / radius,
                             center_y_val - center_y_val * y / radius);
        radius *= 1.41421356f;
      }
      break;
    }
    case RadialGradientExtent::kFarthestSide: {
      float x = std::max(center_x_val, width - center_x_val);
      float y = std::max(center_y_val, height - center_y_val);
      radius = std::max(x, y);
      if (gradient.radial_shape_ == RadialShapeType::kEllipse) {
        matrix.PostScale(x / radius, y / radius);
        matrix.PostTranslate(center_x_val - center_x_val * x / radius,
                             center_y_val - center_y_val * y / radius);
      }
      break;
    }
    case RadialGradientExtent::kFarthestCorner: {
      float x = std::max(center_x_val, width - center_x_val);
      float y = std::max(center_y_val, height - center_y_val);
      if (gradient.radial_shape_ == RadialShapeType::kCircle) {
        radius = std::sqrt(x * x + y * y);
      } else {
        radius = std::max(x, y);
        matrix.PostScale(x / radius, y / radius);
        matrix.PostTranslate(center_x_val - center_x_val * x / radius,
                             center_y_val - center_y_val * y / radius);
        radius *= 1.41421356f;
      }
      break;
    }
    case RadialGradientExtent::kLength: {
      float x = gradient.length_x_.value;
      float y = gradient.length_y_.value;
      if (gradient.length_x_.type == GradientLengthType::kPercent) {
        x *= width;
      }
      if (gradient.length_y_.type == GradientLengthType::kPercent) {
        y *= height;
      }
      radius = std::min(x, y);
      matrix.PostScale(x / radius, y / radius);
      matrix.PostTranslate(center_x_val - center_x_val * x / radius,
                           center_y_val - center_y_val * y / radius);
      break;
    }
  }
  if (RoughlyEqualToZero(radius)) {
    FML_DLOG(WARNING) << "Paint a radial-gradient with 0 radius.";
  }
}

void GradientFactory::ParseRadialCenter(const Gradient& gradient,
                                        const FloatRect& rect, FloatPoint& at) {
  float width = rect.width();
  float height = rect.height();
  auto calculate = [](RadialCenterType type, float value, float base) {
    type = static_cast<RadialCenterType>(-static_cast<int>(type));
    switch (type) {
      case RadialCenterType::BACKGROUND_POSITION_CENTER:
        return base * 0.5f;
      case RadialCenterType::BACKGROUND_POSITION_LEFT:
      case RadialCenterType::BACKGROUND_POSITION_TOP:
        return 0.f;
      case RadialCenterType::BACKGROUND_POSITION_RIGHT:
      case RadialCenterType::BACKGROUND_POSITION_BOTTOM:
        return base;
      case RadialCenterType::RADIAL_CENTER_TYPE_PERCENTAGE:
        return base * value / 100.f;
      default:
        return value;
    }
  };
  auto& center = gradient.Center();
  at.SetX(calculate(center.center_x, center.center_x_value, width));
  at.SetY(calculate(center.center_y, center.center_y_value, height));
}

// static
void GradientFactory::ParseConicCenter(const Gradient& gradient,
                                       const FloatRect& rect,
                                       FloatPoint& center) {
  float width = rect.width();
  float height = rect.height();
  auto& x = gradient.ConicCenterX();
  auto& y = gradient.ConicCenterY();
  center.SetX(x.type == GradientPositionType::kNumber ? x.value
                                                      : x.value * width);
  center.SetY(y.type == GradientPositionType::kNumber ? y.value
                                                      : y.value * height);
}

GradientFactory::ParseResult GradientFactory::ParseLinearGradientStartPoints(
    const Gradient& gradient, const FloatRect& rect, skity::Vec2* pts) {
  const float width = rect.width();
  const float height = rect.height();

  // If width/height is null, x/y can be infinite.
  if (width == 0 || height == 0) {
    FML_DLOG(ERROR) << "ParseLinearGradientStartPoints with empty width/height";
    pts[0] = skity::Vec2(rect.x(), rect.y());
    pts[1] = skity::Vec2(rect.x(), rect.y());
    return ParseResult::kFailed;
  }

  float angle = 0.f;

  auto direction_itr = kDirectionToStartPoints.find(gradient.Direction());
  if (direction_itr != kDirectionToStartPoints.end()) {
    const FloatPoint& start_point = direction_itr->second;
    angle = atan2(start_point.x() * height, -start_point.y() * width) -
            static_cast<float>(M_PI) * 2.f;
  } else if (gradient.Direction() == LinearGradientDirection::kAngle) {
    angle = gradient.Angle();
  }

  const float sin_val = sin(angle);
  const float cos_val = cos(angle);

  const float length = std::abs(sin_val * width) + std::abs(cos_val * height);
  const float x = sin_val * length / width;
  const float y = cos_val * length / height;
  const float halfWidth = width / 2.0;
  const float halfHeight = height / 2.0;
  pts[0] = skity::Vec2(rect.x() + halfWidth + -x * halfWidth,
                       rect.y() + halfHeight + y * halfHeight);
  pts[1] = skity::Vec2(rect.x() + halfWidth + x * halfWidth,
                       rect.y() + halfHeight - y * halfHeight);

  return ParseResult::kSucceeded;
}

}  // namespace clay
