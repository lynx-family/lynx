// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/transforms/transform_operations_helper.h"

#include <string>
#include <utility>

#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace transforms {

namespace {

constexpr gfx::LengthValue kZeroLength{};

gfx::LengthValue ConvertLength(const starlight::NLength& length,
                               float percentage_base) {
  if (length.IsPercent()) {
    return {length.GetRawValue(), gfx::LengthUnit::kPercent};
  }
  if (length.IsCalc()) {
    return {starlight::NLengthToLayoutUnit(
                length, starlight::LayoutUnit(percentage_base))
                .ToFloat(),
            gfx::LengthUnit::kNumber};
  }
  return {length.GetRawValue(), gfx::LengthUnit::kNumber};
}

gfx::TransformOperation::Type ConvertRotationType(
    starlight::TransformType type) {
  if (type == starlight::TransformType::kRotateX) {
    return gfx::TransformOperation::kRotateX;
  }
  if (type == starlight::TransformType::kRotateY) {
    return gfx::TransformOperation::kRotateY;
  }
  return gfx::TransformOperation::kRotateZ;
}

void AppendTransformOperation(gfx::TransformOperations& operations,
                              const starlight::TransformRawData& item,
                              float reference_width, float reference_height) {
  switch (item.type) {
    case starlight::TransformType::kTranslate:
    case starlight::TransformType::kTranslateX:
    case starlight::TransformType::kTranslateY:
    case starlight::TransformType::kTranslateZ:
    case starlight::TransformType::kTranslate3d: {
      const bool is_y = item.type == starlight::TransformType::kTranslateY;
      const bool is_z = item.type == starlight::TransformType::kTranslateZ;
      const bool is_3d = item.type == starlight::TransformType::kTranslate3d;
      const bool has_xy =
          item.type == starlight::TransformType::kTranslate || is_3d;
      gfx::LengthValue x = kZeroLength;
      gfx::LengthValue y = kZeroLength;
      gfx::LengthValue z = kZeroLength;
      if (is_y) {
        y = ConvertLength(item.p0, reference_height);
      } else if (is_z) {
        z = ConvertLength(item.p0, 0.0f);
      } else {
        x = ConvertLength(item.p0, reference_width);
        if (has_xy) {
          y = ConvertLength(item.p1, reference_height);
        }
        if (is_3d) {
          z = ConvertLength(item.p2, 0.0f);
        }
      }
      operations.AppendTranslate(x, y, z);
      break;
    }
    case starlight::TransformType::kRotate:
    case starlight::TransformType::kRotateX:
    case starlight::TransformType::kRotateY:
    case starlight::TransformType::kRotateZ:
      operations.AppendRotate(ConvertRotationType(item.type),
                              item.p0.GetRawValue());
      break;
    case starlight::TransformType::kScaleX:
    case starlight::TransformType::kScale:
    case starlight::TransformType::kScaleY: {
      const bool is_y = item.type == starlight::TransformType::kScaleY;
      const float value = item.p0.GetRawValue();
      const float x = is_y ? 1.0f : value;
      float y = 1.0f;
      if (item.type == starlight::TransformType::kScale) {
        y = item.p1.GetRawValue();
      } else if (is_y) {
        y = value;
      }
      operations.AppendScale(x, y);
      break;
    }
    case starlight::TransformType::kSkewX:
    case starlight::TransformType::kSkew:
    case starlight::TransformType::kSkewY: {
      const bool is_y = item.type == starlight::TransformType::kSkewY;
      const float value = item.p0.GetRawValue();
      const float x = is_y ? 0.0f : value;
      float y = 0.0f;
      if (item.type == starlight::TransformType::kSkew) {
        y = item.p1.GetRawValue();
      } else if (is_y) {
        y = value;
      }
      operations.AppendSkew(x, y);
      break;
    }
    case starlight::TransformType::kMatrix:
    case starlight::TransformType::kMatrix3d: {
      const auto type = item.type == starlight::TransformType::kMatrix
                            ? gfx::TransformOperation::kMatrix
                            : gfx::TransformOperation::kMatrix3d;
      operations.AppendMatrix(type, item.matrix);
      break;
    }
    default:
      break;
  }
}

std::optional<starlight::TransformType> ConvertToRawTransformType(
    gfx::TransformOperation::Type type) {
  switch (type) {
    case gfx::TransformOperation::kTranslate:
      return starlight::TransformType::kTranslate3d;
    case gfx::TransformOperation::kRotateX:
      return starlight::TransformType::kRotateX;
    case gfx::TransformOperation::kRotateY:
      return starlight::TransformType::kRotateY;
    case gfx::TransformOperation::kRotateZ:
      return starlight::TransformType::kRotateZ;
    case gfx::TransformOperation::kScale:
      return starlight::TransformType::kScale;
    case gfx::TransformOperation::kSkew:
      return starlight::TransformType::kSkew;
    case gfx::TransformOperation::kMatrix:
      return starlight::TransformType::kMatrix;
    case gfx::TransformOperation::kMatrix3d:
      return starlight::TransformType::kMatrix3d;
    default:
      return std::nullopt;
  }
}

void AppendCSSLength(lepus::CArray& item, const gfx::LengthValue& length) {
  item.emplace_back(length.value);
  item.emplace_back(static_cast<int>(length.unit == gfx::LengthUnit::kPercent
                                         ? tasm::CSSValuePattern::PERCENT
                                         : tasm::CSSValuePattern::NUMBER));
}

std::string Get2DRepresentation(const gfx::Matrix44& matrix,
                                float layouts_unit_per_px) {
  return "matrix(" + tasm::CSSDecoder::NumberToString(matrix.rc(0, 0)) + ", " +
         tasm::CSSDecoder::NumberToString(matrix.rc(1, 0)) + ", " +
         tasm::CSSDecoder::NumberToString(matrix.rc(0, 1)) + ", " +
         tasm::CSSDecoder::NumberToString(matrix.rc(1, 1)) + ", " +
         tasm::CSSDecoder::NumberToString(matrix.rc(0, 3) /
                                          layouts_unit_per_px) +
         ", " +
         tasm::CSSDecoder::NumberToString(matrix.rc(1, 3) /
                                          layouts_unit_per_px) +
         ")";
}

std::string Get3DRepresentation(const gfx::Matrix44& matrix,
                                float layouts_unit_per_px) {
  std::string result = "matrix3d(";
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      float value = matrix.rc(row, col);
      if (col == 3 && row < 3) {
        value /= layouts_unit_per_px;
      }
      result += tasm::CSSDecoder::NumberToString(value);
      if (col != 3 || row != 3) {
        result += ", ";
      }
    }
  }
  return result + ")";
}

}  // namespace

gfx::TransformOperations ConvertToGfxTransformOperations(
    const base::Vector<starlight::TransformRawData>& transform_raw_data,
    float reference_width, float reference_height) {
  gfx::TransformOperations operations;
  for (const auto& item : transform_raw_data) {
    AppendTransformOperation(operations, item, reference_width,
                             reference_height);
  }
  return operations;
}

std::optional<gfx::TransformOperations> ResolveTransformOperations(
    const tasm::CSSValue& raw_data,
    const tasm::CssMeasureContext& measure_context,
    const tasm::CSSParserConfigs& parser_configs, float reference_width,
    float reference_height) {
  auto transform_data = base::make_flex_optional(
      base::InlineVector<starlight::TransformRawData, 1>());
  if (!starlight::CSSStyleUtils::ComputeTransform(
          raw_data, false, transform_data, measure_context, parser_configs)) {
    return std::nullopt;
  }
  return ConvertToGfxTransformOperations(*transform_data, reference_width,
                                         reference_height);
}

tasm::CSSValue ConvertToCSSValue(const gfx::TransformOperations& operations,
                                 float layouts_unit_per_px) {
  const auto to_css_pixels = [layouts_unit_per_px](float value) {
    return layouts_unit_per_px > 0.0f ? value / layouts_unit_per_px : value;
  };
  auto items = lepus::CArray::Create();
  for (const auto& operation : operations.GetOperations()) {
    const auto transform_type = ConvertToRawTransformType(operation.type);
    if (!transform_type) {
      continue;
    }

    auto item = lepus::CArray::Create();
    item->emplace_back(static_cast<int>(*transform_type));
    switch (operation.type) {
      case gfx::TransformOperation::kTranslate:
        AppendCSSLength(*item, operation.translate.x);
        AppendCSSLength(*item, operation.translate.y);
        AppendCSSLength(*item, operation.translate.z);
        break;
      case gfx::TransformOperation::kRotateX:
      case gfx::TransformOperation::kRotateY:
      case gfx::TransformOperation::kRotateZ:
        item->emplace_back(operation.rotate.degree);
        break;
      case gfx::TransformOperation::kScale:
        item->emplace_back(operation.scale.x);
        item->emplace_back(operation.scale.y);
        break;
      case gfx::TransformOperation::kSkew:
        item->emplace_back(operation.skew.x);
        item->emplace_back(operation.skew.y);
        break;
      case gfx::TransformOperation::kMatrix: {
        const gfx::Matrix44 matrix = operation.GetMatrix(0.0f, 0.0f);
        item->emplace_back(matrix.rc(0, 0));
        item->emplace_back(matrix.rc(1, 0));
        item->emplace_back(matrix.rc(0, 1));
        item->emplace_back(matrix.rc(1, 1));
        item->emplace_back(to_css_pixels(matrix.rc(0, 3)));
        item->emplace_back(to_css_pixels(matrix.rc(1, 3)));
        break;
      }
      case gfx::TransformOperation::kMatrix3d:
        for (size_t i = 0; i < operation.matrix.matrix_data.size(); ++i) {
          const float matrix_value = operation.matrix.matrix_data[i];
          item->emplace_back(i >= 12 && i <= 14 ? to_css_pixels(matrix_value)
                                                : matrix_value);
        }
        break;
      default:
        break;
    }
    items->emplace_back(std::move(item));
  }
  return tasm::CSSValue(std::move(items));
}

base::String ConvertToCSSText(const gfx::TransformOperations& operations,
                              float reference_width, float reference_height,
                              float layouts_unit_per_px) {
  const gfx::Matrix44 transform =
      operations.ApplyRemaining(0, reference_width, reference_height);
  return transform.Is2dTransform()
             ? Get2DRepresentation(transform, layouts_unit_per_px)
             : Get3DRepresentation(transform, layouts_unit_per_px);
}

}  // namespace transforms
}  // namespace lynx
