// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/basic_shape.h"

#include "base/include/float_comparison.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr const uint32_t kRawParamsLenPath = 2;
static constexpr const uint32_t kRawParamsLenCircle = 7;
static constexpr const uint32_t kRawParamsLenEllipse = 9;
static constexpr const uint32_t kRawParamsLenSuperEllipse = 11;
static constexpr const uint32_t kRawIndexPathData = 1;
static constexpr const uint32_t kRawIndexCircleRadius = 1;
static constexpr const uint32_t kRawIndexCircleRadiusUnit = 2;
static constexpr const uint32_t kRawIndexCircleCenterX = 3;
static constexpr const uint32_t kRawIndexCircleCenterXUnit = 4;
static constexpr const uint32_t kRawIndexCircleCenterY = 5;
static constexpr const uint32_t kRawIndexCircleCenterYUnit = 6;

static constexpr const uint32_t kRawIndexEllipseRadiusX = 1;
static constexpr const uint32_t kRawIndexEllipseRadiusXUnit = 2;
static constexpr const uint32_t kRawIndexEllipseRadiusY = 3;
static constexpr const uint32_t kRawIndexEllipseRadiusYUnit = 4;
static constexpr const uint32_t kRawIndexEllipseCenterX = 5;
static constexpr const uint32_t kRawIndexEllipseCenterXUnit = 6;
static constexpr const uint32_t kRawIndexEllipseCenterY = 7;
static constexpr const uint32_t kRawIndexEllipseCenterYUnit = 8;

static constexpr const uint32_t kRawIndexSuperEllipseRadiusX = 1;
static constexpr const uint32_t kRawIndexSuperEllipseRadiusXUnit = 2;
static constexpr const uint32_t kRawIndexSuperEllipseRadiusY = 3;
static constexpr const uint32_t kRawIndexSuperEllipseRadiusYUnit = 4;
static constexpr const uint32_t kRawIndexSuperEllipseExponentX = 5;
static constexpr const uint32_t kRawIndexSuperEllipseExponentY = 6;
static constexpr const uint32_t kRawIndexSuperEllipseCenterX = 7;
static constexpr const uint32_t kRawIndexSuperEllipseCenterXUnit = 8;
static constexpr const uint32_t kRawIndexSuperEllipseCenterY = 9;
static constexpr const uint32_t kRawIndexSuperEllipseCenterYUnit = 10;
static constexpr const uint32_t kIndexBasicShapeType = 0;

BasicShape::BasicShape(const lepus::Value& value, float density) {
  density_ = density;
  if (value.IsArray()) {
    shape_data_ = value.Array();
    basic_shape_type_ = static_cast<starlight::BasicShapeType>(
        shape_data_->get(kIndexBasicShapeType).Number());
  }
}

void BasicShape::ParsePathWithParentSize(float view_width, float view_height) {
  if (basic_shape_type_ == starlight::BasicShapeType::kUnknown ||
      !shape_data_) {
    return;
  }
  float width = view_width * density_;
  float height = view_height * density_;
  auto len = shape_data_->size();
  if (basic_shape_type_ == starlight::BasicShapeType::kPath) {
    if (len != kRawParamsLenPath) {
      return;
    }
    const auto& path_data = shape_data_->get(kRawIndexPathData).StdString();
    path_string_ = ScaleSvgPath(path_data);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kSuperEllipse) {
    if (len != kRawParamsLenSuperEllipse) {
      return;
    }
    float exponents_x = static_cast<float>(
        shape_data_->get(kRawIndexSuperEllipseExponentX).Number());
    float exponents_y = static_cast<float>(
        shape_data_->get(kRawIndexSuperEllipseExponentY).Number());
    if (exponents_x == 0 || exponents_y == 0) {
      return;
    }
    float rx =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseRadiusX),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseRadiusXUnit).Number()))
            .GetValue(width);
    float ry =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseRadiusY),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseRadiusYUnit).Number()))
            .GetValue(height);
    float cx =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseCenterX),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseCenterXUnit).Number()))
            .GetValue(width);
    float cy =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseCenterY),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseCenterYUnit).Number()))
            .GetValue(height);
    path_string_ =
        SuperEllipseToSvgPath(rx, ry, cx, cy, exponents_x, exponents_y);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kCircle) {
    if (len != kRawParamsLenCircle) {
      return;
    }
    float parent_percent =
        std::sqrt(width * width + height * height) / std::sqrt(2);
    float radius =
        PlatformLength(
            shape_data_->get(kRawIndexCircleRadius),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexCircleRadiusUnit).Number()))
            .GetValue(parent_percent);
    float cx = PlatformLength(
                   shape_data_->get(kRawIndexCircleCenterX),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexCircleCenterXUnit).Number()))
                   .GetValue(width);
    float cy = PlatformLength(
                   shape_data_->get(kRawIndexCircleCenterY),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexCircleCenterYUnit).Number()))
                   .GetValue(height);
    path_string_ = CircleToSvgPath(radius, cx, cy);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kEllipse) {
    if (len != kRawParamsLenEllipse) {
      return;
    }
    float rx = PlatformLength(
                   shape_data_->get(kRawIndexEllipseRadiusX),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseRadiusXUnit).Number()))
                   .GetValue(width);
    float ry = PlatformLength(
                   shape_data_->get(kRawIndexEllipseRadiusY),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseRadiusYUnit).Number()))
                   .GetValue(height);
    float cx = PlatformLength(
                   shape_data_->get(kRawIndexEllipseCenterX),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseCenterXUnit).Number()))
                   .GetValue(width);
    float cy = PlatformLength(
                   shape_data_->get(kRawIndexEllipseCenterY),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseCenterYUnit).Number()))
                   .GetValue(height);
    path_string_ = EllipseToSvgPath(rx, ry, cx, cy);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kInset) {
    // TODO(chengjunnan)impl it later.
  }
}

const std::string& BasicShape::PathString() { return path_string_; }

static inline bool IsArcPathCommand(const char c) {
  return c == 'A' || c == 'a';
}

static inline bool IsSimplePathCommand(const char c) {
  return c == 'M' || c == 'L' || c == 'H' || c == 'V' || c == 'C' || c == 'S' ||
         c == 'Q' || c == 'T' || c == 'Z' || c == 'm' || c == 'l' || c == 'h' ||
         c == 'v' || c == 'c' || c == 's' || c == 'q' || c == 't' || c == 'z';
}

static inline void ScaleNumber(std::ostringstream& oss,
                               const std::string& numStr, float scale) {
  float value = std::stof(numStr);
  value *= scale;
  oss << value;
  oss << " ";
}

const std::string BasicShape::ScaleSvgPath(const std::string& path) {
  std::ostringstream result;
  size_t i = 0;
  const size_t n = path.size();
  while (i < n) {
    if (IsSimplePathCommand(path[i])) {
      result << path[i] << " ";
      ++i;
    } else if (IsArcPathCommand(path[i])) {
      result << path[i] << " ";
      ++i;
      for (int paramIndex = 0; paramIndex < 7; ++paramIndex) {
        while (i < path.size() && (path[i] == ' ' || path[i] == ',')) {
          ++i;
        }
        std::string param;
        while (i < path.size() && !std::isspace(path[i]) && path[i] != ',') {
          param += path[i];
          ++i;
        }
        if (paramIndex == 0 || paramIndex == 1 || paramIndex == 5 ||
            paramIndex == 6) {
          ScaleNumber(result, param, density_);
        } else {
          result << param;
        }
        result << " ";
      }
    } else if (std::isdigit(path[i]) || path[i] == '-' || path[i] == '.') {
      size_t start = i;
      while (i < n && (std::isdigit(path[i]) || path[i] == '-' ||
                       path[i] == '.' || path[i] == 'e' || path[i] == 'E')) {
        ++i;
      }
      std::string numStr = path.substr(start, i - start);
      ScaleNumber(result, numStr, density_);
    } else if (std::isspace(path[i]) || path[i] == ',') {
      ++i;
    } else {
      result << path[i] << " ";
      ++i;
    }
  }
  return result.str();
}

const std::string BasicShape::CircleToSvgPath(float radius, float cx,
                                              float cy) {
  std::ostringstream path;
  path << "M " << cx + radius << "," << cy << " A " << radius << "," << radius
       << " 0 1,0 " << cx - radius << "," << cy << " A " << radius << ","
       << radius << " 0 1,0 " << cx + radius << "," << cy << " Z";
  return path.str();
}

const std::string BasicShape::EllipseToSvgPath(float rx, float ry, float cx,
                                               float cy) {
  std::ostringstream path;
  path << "M " << (cx + rx) << "," << cy << " ";
  path << "A " << rx << "," << ry << " 0 1,0 " << (cx - rx) << "," << cy << " ";
  path << "A " << rx << "," << ry << " 0 1,0 " << (cx + rx) << "," << cy << " ";
  path << "Z";
  return path.str();
}

const std::string BasicShape::SuperEllipseToSvgPath(float rx, float ry,
                                                    float cx, float cy,
                                                    float exponents_x,
                                                    float exponents_y) {
  std::ostringstream path;
  for (int32_t i = 1; i <= 4; i++) {
    AddLameCurveToPath(path, rx, ry, cx, cy, exponents_x, exponents_y, i);
  }
  path << "Z";
  return path.str();
}

void BasicShape::AddLameCurveToPath(std::ostringstream& path, float rx,
                                    float ry, float cx, float cy,
                                    float exponents_x, float exponents_y,
                                    int32_t quadrant) {
  double cos_i;
  double sin_i;
  double x;
  double y;
  float fx = (quadrant == 1 || quadrant == 4) ? 1 : -1;
  float fy = (quadrant == 1 || quadrant == 2) ? 1 : -1;
  for (float i = static_cast<float>(M_PI / 2 * (quadrant - 1));
       base::FloatsLarger((M_PI / 2 * quadrant), i); i += 0.01f) {
    cos_i = fx * std::cos(i);
    sin_i = fy * std::sin(i);
    x = fx * rx * std::pow(cos_i, 2 / exponents_x) + cx;
    y = fy * ry * std::pow(sin_i, 2 / exponents_y) + cy;
    if (i == 0) {
      path << "M " << x << "," << y << " ";
    } else {
      path << "L " << x << "," << y << " ";
    }
  }
}

}  // namespace harmony

}  // namespace tasm
}  // namespace lynx
