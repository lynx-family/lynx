// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/transform.h"

#include <limits>

#include "core/base/harmony/napi_convert_helper.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace harmony {
Transform::Transform(const lepus::Value& value) {
  if (!value.IsArray() || value.Array()->size() == 0) {
    return;
  }
  const fml::RefPtr<lepus::CArray>& array = value.Array();
  for (size_t i = 0; i < value.Array()->size(); i++) {
    if (const lepus::Value& transform = array->get(i);
        transform.IsArray() && transform.Array()->size() >= 4) {
      const auto& transform_array = transform.Array();

      raw_.emplace_back((TransformRaw){
          .func_type = static_cast<starlight::TransformType>(
              transform_array->get(0).Number()),
          .params_ = {
              (PlatformLength){transform_array->get(1),
                               static_cast<PlatformLengthType>(
                                   transform_array->get(2).Number())},
              (PlatformLength){transform_array->get(3),
                               static_cast<PlatformLengthType>(
                                   transform_array->get(4).Number())},
              (PlatformLength){transform_array->get(5),
                               static_cast<PlatformLengthType>(
                                   transform_array->get(6).Number())}}});
    }
  }
}

transforms::Matrix44 Transform::GetTransformMatrix(
    float width, float height, float scaled_density,
    bool with_transform_origin) const {
  transforms::Matrix44 result;
  transforms::Matrix44 transform;
  transforms::Matrix44 transform_origin;
  transforms::Matrix44 transform_origin_reverse;
  if (with_transform_origin) {
    transform_origin.preTranslate(
        -transform_origin_.x.GetValue(width) * scaled_density,
        -transform_origin_.y.GetValue(height) * scaled_density, 0);
    transform_origin_reverse.preTranslate(
        transform_origin_.x.GetValue(width) * scaled_density,
        transform_origin_.y.GetValue(height) * scaled_density, 0);
  }
  for (const auto& [type, params] : raw_) {
    switch (type) {
      case starlight::TransformType::kRotateX:
        transform.setRotateAboutXAxis(params[0].AsNumber());
        break;
      case starlight::TransformType::kRotateY:
        transform.setRotateAboutYAxis(params[0].AsNumber());
        break;
      case starlight::TransformType::kRotate:
      case starlight::TransformType::kRotateZ:
        transform.setRotateAboutZAxis(params[0].AsNumber());
        break;
      case starlight::TransformType::kTranslate:
      case starlight::TransformType::kTranslate3d:
        transform.preTranslate(params[0].GetValue(width) * scaled_density,
                               params[1].GetValue(height) * scaled_density,
                               params[2].AsNumber());
        break;
      case starlight::TransformType::kTranslateX:
        transform.preTranslate(params[0].GetValue(width) * scaled_density, 0,
                               0);
        break;
      case starlight::TransformType::kTranslateY:
        transform.preTranslate(0, params[0].GetValue(height) * scaled_density,
                               0);
        break;
      case starlight::TransformType::kTranslateZ:
        transform.preTranslate(0, 0, params[0].AsNumber());
        break;
      case starlight::TransformType::kScale: {
        float scale_x = params[0].AsNumber();
        float scale_y = params[1].AsNumber();
        scale_x = std::fabs(scale_x) < std::numeric_limits<float>::epsilon()
                      ? .0f
                      : scale_x;
        scale_y = std::fabs(scale_y) < std::numeric_limits<float>::epsilon()
                      ? .0f
                      : scale_y;
        transform.preScale(scale_x, scale_y, 1);
        break;
      }
      case starlight::TransformType::kScaleX: {
        float scale = params[0].AsNumber();
        scale = std::fabs(scale) < std::numeric_limits<float>::epsilon()
                    ? .0f
                    : scale;
        transform.preScale(scale, 1, 1);
        break;
      }
      case starlight::TransformType::kScaleY: {
        float scale = params[0].AsNumber();
        scale = std::fabs(scale) < std::numeric_limits<float>::epsilon()
                    ? .0f
                    : scale;
        transform.preScale(1, scale, 1);
        break;
      }
      case starlight::TransformType::kSkew:
        transform.Skew(params[0].AsNumber(), params[1].AsNumber());
        break;
      case starlight::TransformType::kSkewX:
        transform.Skew(params[0].AsNumber(), 0);
        break;
      case starlight::TransformType::kSkewY:
        transform.Skew(0, params[0].AsNumber());
        break;
      default:
        break;
    }
    result.preConcat(transform);
    transform.setIdentity();
  }

  if (with_transform_origin) {
    result.preConcat(transform_origin);
    transform_origin_reverse.preConcat(result);
    return transform_origin_reverse;
  }
  return result;
}

void Transform::SetTransformOrigin(const TransformOrigin& transform_origin) {
  transform_origin_ = transform_origin;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
