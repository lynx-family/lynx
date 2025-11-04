// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/basic_shape.h"

#include <cmath>

#include "base/include/float_comparison.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/svg_path_utils.h"

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
static constexpr const uint32_t kRawParamsLenInsetRect = 9;
static constexpr const uint32_t kRawParamsLenInsetRound = 25;
static constexpr const uint32_t kRawParamsLenInsetSuperEllipse = 27;
static constexpr const uint32_t kRawIndexInsetSuperEllipseExponentX = 9;
static constexpr const uint32_t kRawIndexInsetSuperEllipseExponentY = 10;
static constexpr const uint32_t kInsetRoundRadiusOffset = 9;
static constexpr const uint32_t kInsetSuperEllipseRadiusOffset = 11;

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
            .GetValue(view_width) *
        density_;
    float ry =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseRadiusY),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseRadiusYUnit).Number()))
            .GetValue(view_height) *
        density_;
    float cx =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseCenterX),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseCenterXUnit).Number()))
            .GetValue(view_width) *
        density_;
    float cy =
        PlatformLength(
            shape_data_->get(kRawIndexSuperEllipseCenterY),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexSuperEllipseCenterYUnit).Number()))
            .GetValue(view_height) *
        density_;
    path_string_ =
        SuperEllipseToSvgPath(rx, ry, cx, cy, exponents_x, exponents_y);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kCircle) {
    if (len != kRawParamsLenCircle) {
      return;
    }
    float parent_percent =
        std::sqrt(view_width * view_width + view_height * view_height) /
        std::sqrt(2);
    float radius =
        PlatformLength(
            shape_data_->get(kRawIndexCircleRadius),
            static_cast<PlatformLengthType>(
                shape_data_->get(kRawIndexCircleRadiusUnit).Number()))
            .GetValue(parent_percent) *
        density_;
    float cx = PlatformLength(
                   shape_data_->get(kRawIndexCircleCenterX),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexCircleCenterXUnit).Number()))
                   .GetValue(view_width) *
               density_;
    float cy = PlatformLength(
                   shape_data_->get(kRawIndexCircleCenterY),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexCircleCenterYUnit).Number()))
                   .GetValue(view_height) *
               density_;
    path_string_ = CircleToSvgPath(radius, cx, cy);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kEllipse) {
    if (len != kRawParamsLenEllipse) {
      return;
    }
    float rx = PlatformLength(
                   shape_data_->get(kRawIndexEllipseRadiusX),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseRadiusXUnit).Number()))
                   .GetValue(view_width) *
               density_;
    float ry = PlatformLength(
                   shape_data_->get(kRawIndexEllipseRadiusY),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseRadiusYUnit).Number()))
                   .GetValue(view_height) *
               density_;
    float cx = PlatformLength(
                   shape_data_->get(kRawIndexEllipseCenterX),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseCenterXUnit).Number()))
                   .GetValue(view_width) *
               density_;
    float cy = PlatformLength(
                   shape_data_->get(kRawIndexEllipseCenterY),
                   static_cast<PlatformLengthType>(
                       shape_data_->get(kRawIndexEllipseCenterYUnit).Number()))
                   .GetValue(view_height) *
               density_;
    path_string_ = EllipseToSvgPath(rx, ry, cx, cy);
  } else if (basic_shape_type_ == starlight::BasicShapeType::kInset) {
    std::vector<PlatformLength> box{};
    for (int i = 0; i < 4; ++i) {
      box.emplace_back(shape_data_->get(2 * i + 1).Number(),
                       static_cast<PlatformLengthType>(
                           shape_data_->get(2 * i + 2).Number()));
    }
    float top = box[0].GetValue(view_height) * density_;
    float right = box[1].GetValue(view_width) * density_;
    float bottom = box[2].GetValue(view_height) * density_;
    float left = box[3].GetValue(view_width) * density_;
    float v_inset = top + bottom;
    float h_inset = left + right;
    if (v_inset != 0 && base::FloatsLarger(v_inset, view_height * density_)) {
      float s = view_height * density_ / v_inset;
      top *= s;
      bottom *= s;
    }
    if (h_inset != 0 && base::FloatsLarger(h_inset, view_width * density_)) {
      float s = view_width * density_ / h_inset;
      left *= s;
      right *= s;
    }

    float inset_right = view_width * density_ - right;
    float inset_bottom = view_height * density_ - bottom;
    std::ostringstream oss;
    if (len == kRawParamsLenInsetRect) {
      SvgPathUtils::MoveTo(oss, left, top);
      SvgPathUtils::LineTo(oss, inset_right, top);
      SvgPathUtils::LineTo(oss, inset_right, inset_bottom);
      SvgPathUtils::LineTo(oss, left, inset_bottom);
      SvgPathUtils::Close(oss);
    } else {
      std::vector<PlatformLength> radius{};
      int radius_offset = len == kRawParamsLenInsetRound
                              ? kInsetRoundRadiusOffset
                              : kInsetSuperEllipseRadiusOffset;
      for (int i = 0; i < 4; i++) {
        radius.emplace_back(
            shape_data_->get(4 * i + radius_offset).Number(),
            static_cast<PlatformLengthType>(
                shape_data_->get(4 * i + radius_offset + 1).Number()));
        radius.emplace_back(
            shape_data_->get(4 * i + radius_offset + 2).Number(),
            static_cast<PlatformLengthType>(
                shape_data_->get(4 * i + radius_offset + 3).Number()));
      }
      float rx1 = radius[0].GetValue(view_width) * density_;
      float ry1 = radius[1].GetValue(view_height) * density_;
      float rx2 = radius[2].GetValue(view_width) * density_;
      float ry2 = radius[3].GetValue(view_height) * density_;
      float rx3 = radius[4].GetValue(view_width) * density_;
      float ry3 = radius[5].GetValue(view_height) * density_;
      float rx4 = radius[6].GetValue(view_width) * density_;
      float ry4 = radius[7].GetValue(view_height) * density_;
      if (len == kRawParamsLenInsetRound) {
        SvgPathUtils::MoveTo(oss, left + rx1, top);
        SvgPathUtils::LineTo(oss, inset_right - rx2, top);
        SvgPathUtils::ArcTo(oss, rx2, ry2, inset_right - 2 * rx2, top,
                            inset_right, top + 2 * ry2, -90, 90);
        SvgPathUtils::LineTo(oss, inset_right, inset_bottom - ry3);
        SvgPathUtils::ArcTo(oss, rx3, ry3, inset_right - 2 * rx3,
                            inset_bottom - 2 * ry3, inset_right, inset_bottom,
                            0, 90);
        SvgPathUtils::LineTo(oss, left + rx4, inset_bottom);
        SvgPathUtils::ArcTo(oss, rx4, ry4, left, inset_bottom - 2 * ry4,
                            left + 2 * rx4, inset_bottom, 90, 90);
        SvgPathUtils::LineTo(oss, left, top + ry1);
        SvgPathUtils::ArcTo(oss, rx1, ry1, left + rx1, top);
        SvgPathUtils::Close(oss);
      } else if (len == kRawParamsLenInsetSuperEllipse) {
        float exponents_x = static_cast<float>(
            shape_data_->get(kRawIndexInsetSuperEllipseExponentX).Number());
        float exponents_y = static_cast<float>(
            shape_data_->get(kRawIndexInsetSuperEllipseExponentY).Number());
        float rx = rx3;
        float ry = ry3;
        float cx = inset_right - rx;
        float cy = inset_bottom - ry;
        AddLameCurveToPath(oss, rx, ry, cx, cy, exponents_x, exponents_y, 1);
        rx = rx4;
        ry = ry4;
        cx = left + rx;
        cy = inset_bottom - ry;
        AddLameCurveToPath(oss, rx, ry, cx, cy, exponents_x, exponents_y, 2);
        rx = rx1;
        ry = ry1;
        cx = left + rx;
        cy = top + ry;
        AddLameCurveToPath(oss, rx, ry, cx, cy, exponents_x, exponents_y, 3);
        rx = rx2;
        ry = ry2;
        cx = inset_right - rx;
        cy = top + ry;
        AddLameCurveToPath(oss, rx, ry, cx, cy, exponents_x, exponents_y, 4);
        SvgPathUtils::Close(oss);
      }
    }
    path_string_ = oss.str();
    // TODO(chengjunnan)For now, SVG path strings are used universally for shape
    // clipping to ensure consistency.However, certain clipping styles may be
    // optimized by leveraging native shape capabilities provided by HarmonyOS.
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
