// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transforms/transform_operation.h"

#include <memory>

#include "base/include/log/logging.h"
#include "core/animation/transforms/decomposed_transform.h"
#include "core/animation/transforms/matrix44.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace transforms {

namespace {
bool IsOperationIdentity(const TransformOperation* operation) {
  return !operation || operation->IsIdentity();
}

float GetDefaultValue(TransformOperation::Type type) {
  switch (type) {
    case TransformOperation::Type::kScale: {
      return 1.0;
    }
    default: {
      return 0.0;
    }
  }
}

bool IsIdentityMatrix(const std::array<float, 16>& matrix) {
  static constexpr std::array<float, 16> identity_matrix = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  return matrix == identity_matrix;
}

// get the final length type of translateX and translateY
std::array<TransformOperation::LengthType, 2> GetTranslateLengthType(
    const TransformOperation* from, const TransformOperation* to) {
  DCHECK(from || to);
  TransformOperation::LengthType final_type_x =
      TransformOperation::LengthType::kLengthUnit;
  TransformOperation::LengthType final_type_y =
      TransformOperation::LengthType::kLengthUnit;

  if (IsOperationIdentity(from) && !IsOperationIdentity(to)) {
    final_type_x = to->translate.type.x == TransformOperation::kLengthPercentage
                       ? TransformOperation::kLengthPercentage
                       : TransformOperation::kLengthUnit;
    final_type_y = to->translate.type.y == TransformOperation::kLengthPercentage
                       ? TransformOperation::kLengthPercentage
                       : TransformOperation::kLengthUnit;
    ;
  } else if (!IsOperationIdentity(from) && IsOperationIdentity(to)) {
    final_type_x =
        from->translate.type.x == TransformOperation::kLengthPercentage
            ? TransformOperation::kLengthPercentage
            : TransformOperation::kLengthUnit;
    ;
    final_type_y =
        from->translate.type.y == TransformOperation::kLengthPercentage
            ? TransformOperation::kLengthPercentage
            : TransformOperation::kLengthUnit;
    ;
  } else if (!IsOperationIdentity(from) && !IsOperationIdentity(to)) {
    if (from->translate.type.x == to->translate.type.x &&
        from->translate.type.x == TransformOperation::kLengthPercentage) {
      final_type_x = TransformOperation::kLengthPercentage;
    } else {
      final_type_x = TransformOperation::kLengthUnit;
    }
    if (from->translate.type.y == to->translate.type.y &&
        from->translate.type.y == TransformOperation::kLengthPercentage) {
      final_type_y = TransformOperation::kLengthPercentage;
    } else {
      final_type_y = TransformOperation::kLengthUnit;
    }
  }
  return std::array<TransformOperation::LengthType, 2>{final_type_x,
                                                       final_type_y};
}

// Convert possible percentage or calc values to unit values
std::array<float, 3> GetPercentOrCalcTranslateValue(
    const TransformOperation* translate, tasm::Element* element) {
  DCHECK(element);
  if (IsOperationIdentity(translate)) {
    return std::array<float, 3>{0.0f, 0.0f, 0.0f};
  }
  float x =
      starlight::NLengthToLayoutUnit(translate->translate.value.x,
                                     starlight::LayoutUnit(element->width()))
          .ToFloat();
  float y =
      starlight::NLengthToLayoutUnit(translate->translate.value.y,
                                     starlight::LayoutUnit(element->height()))
          .ToFloat();
  float z = starlight::NLengthToLayoutUnit(translate->translate.value.z,
                                           starlight::LayoutUnit(0.0f))
                .ToFloat();
  return std::array<float, 3>{x, y, z};
};

static float BlendValue(float from, float to, float progress) {
  return from * (1 - progress) + to * progress;
}

}  // namespace
bool TransformOperation::IsIdentity() const {
  switch (type) {
    case TransformOperation::Type::kTranslate: {
      float default_value = GetDefaultValue(type);
      return translate.value.x.NumericLength().GetFixedPart() ==
                 default_value &&
             !translate.value.x.NumericLength().ContainsPercentage() &&
             translate.value.y.NumericLength().GetFixedPart() ==
                 default_value &&
             !translate.value.y.NumericLength().ContainsPercentage() &&
             translate.value.z.NumericLength().GetFixedPart() ==
                 default_value &&
             !translate.value.z.NumericLength().ContainsPercentage();
    }
    case TransformOperation::Type::kRotateX:
    case TransformOperation::Type::kRotateY:
    case TransformOperation::Type::kRotateZ: {
      return rotate.degree == GetDefaultValue(type);
    }
    case TransformOperation::Type::kScale: {
      return scale.x == GetDefaultValue(type) &&
             scale.y == GetDefaultValue(type);
    }
    case TransformOperation::Type::kSkew: {
      return skew.x == GetDefaultValue(type) && skew.y == GetDefaultValue(type);
    }
    case TransformOperation::Type::kMatrix:
    case TransformOperation::Type::kMatrix3d: {
      return IsIdentityMatrix(matrix.matrix_data);
    }
    default: {
      return true;
    }
  }
}

// Lazy bake matrix till we need matrix, to avoid getting invalid size of
// element if layout is not ready.
const Matrix44& TransformOperation::GetMatrix(tasm::Element* element) {
  if (matrix44) {
    return *matrix44;
  }
  Bake(element);
  return *matrix44;
}

void TransformOperation::Bake(tasm::Element* element) {
  matrix44 = std::make_optional<Matrix44>();
  switch (type) {
    case TransformOperation::Type::kTranslate: {
      std::array<float, 3> arr = GetPercentOrCalcTranslateValue(this, element);
      matrix44->preTranslate(arr[0], arr[1], arr[2]);
      break;
    }
    case TransformOperation::Type::kRotateX: {
      matrix44->setRotateAboutXAxis(rotate.degree);
      break;
    }
    case TransformOperation::Type::kRotateY: {
      matrix44->setRotateAboutYAxis(rotate.degree);
      break;
    }
    case TransformOperation::Type::kRotateZ: {
      matrix44->setRotateAboutZAxis(rotate.degree);
      break;
    }
    case TransformOperation::Type::kScale: {
      matrix44->preScale(scale.x, scale.y, 1);
      break;
    }
    case TransformOperation::Type::kSkew: {
      matrix44->Skew(skew.x, skew.y);
      break;
    }
    case TransformOperation::Type::kMatrix:
    case TransformOperation::Type::kMatrix3d: {
      matrix44->Matrix(matrix.matrix_data);
    }
    default: {
      break;
    }
  }
}

TransformOperation ComposeTransform(
    const DecomposedTransform& decomposed_transform) {
  TransformOperation result;
  result.type = TransformOperation::kMatrix3d;
  Matrix44 temp_matrix44;

  // perspective
  for (int i = 0; i < 3; ++i) {
    if (decomposed_transform.perspective[i] != 0) {
      temp_matrix44.setRC(3, i, decomposed_transform.perspective[i]);
    }
  }
  if (decomposed_transform.perspective[3] != 1) {
    temp_matrix44.setRC(3, 3, decomposed_transform.perspective[3]);
  }

  // translate
  temp_matrix44.preTranslate(decomposed_transform.translate[0],
                             decomposed_transform.translate[1],
                             decomposed_transform.translate[2]);

  // rotate
  temp_matrix44.preConcat(Matrix44(decomposed_transform.quaternion));

  // skew
  if (decomposed_transform.skew[0] || decomposed_transform.skew[1] ||
      decomposed_transform.skew[2])
    temp_matrix44.Skew(decomposed_transform.skew[0], 2);

  // scale
  temp_matrix44.preScale(decomposed_transform.scale[0],
                         decomposed_transform.scale[1],
                         decomposed_transform.scale[2]);

  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      result.matrix.matrix_data.at(4 * row + col) = temp_matrix44.rc(row, col);
    }
  }
  return result;
}

TransformOperation TransformOperation::BlendTransformOperations(
    const TransformOperation* from, const TransformOperation* to,
    float progress, tasm::Element* element) {
  DCHECK(from != nullptr || to != nullptr);
  DCHECK(element);
  if (IsOperationIdentity(from) && IsOperationIdentity(to)) {
    return TransformOperation();
  }
  TransformOperation operation;
  TransformOperation::Type transform_type =
      IsOperationIdentity(from) ? to->type : from->type;
  operation.type = transform_type;
  switch (transform_type) {
    case TransformOperation::Type::kTranslate: {
      float from_x = IsOperationIdentity(from)
                         ? 0.0f
                         : from->translate.value.x.GetRawValue();
      float from_y = IsOperationIdentity(from)
                         ? 0.0f
                         : from->translate.value.y.GetRawValue();
      float from_z = IsOperationIdentity(from)
                         ? 0.0f
                         : from->translate.value.z.GetRawValue();
      float to_x =
          IsOperationIdentity(to) ? 0.0f : to->translate.value.x.GetRawValue();
      float to_y =
          IsOperationIdentity(to) ? 0.0f : to->translate.value.y.GetRawValue();
      float to_z =
          IsOperationIdentity(to) ? 0.0f : to->translate.value.z.GetRawValue();
      std::array<TransformOperation::LengthType, 2> result_type_arr =
          GetTranslateLengthType(from, to);
      std::array<float, 3> from_value_arr =
          GetPercentOrCalcTranslateValue(from, element);
      std::array<float, 3> to_value_arr =
          GetPercentOrCalcTranslateValue(to, element);
      if (result_type_arr[0] !=
          TransformOperation::LengthType::kLengthPercentage) {
        from_x = from_value_arr[0];
        to_x = to_value_arr[0];
      }
      if (result_type_arr[1] !=
          TransformOperation::LengthType::kLengthPercentage) {
        from_y = from_value_arr[1];
        to_y = to_value_arr[1];
      }

      operation.translate.type.x = result_type_arr[0];
      operation.translate.value.x =
          result_type_arr[0] == kLengthUnit
              ? starlight::NLength::MakeUnitNLength(
                    BlendValue(from_x, to_x, progress))
              : starlight::NLength::MakePercentageNLength(
                    BlendValue(from_x, to_x, progress));

      operation.translate.type.y = result_type_arr[1];
      operation.translate.value.y =
          result_type_arr[1] == kLengthUnit
              ? starlight::NLength::MakeUnitNLength(
                    BlendValue(from_y, to_y, progress))
              : starlight::NLength::MakePercentageNLength(
                    BlendValue(from_y, to_y, progress));

      operation.translate.type.z = TransformOperation::LengthType::kLengthUnit;
      operation.translate.value.z = starlight::NLength::MakeUnitNLength(
          BlendValue(from_z, to_z, progress));
      return operation;
    }
    case TransformOperation::Type::kRotateX:
    case TransformOperation::Type::kRotateY:
    case TransformOperation::Type::kRotateZ: {
      float from_angle = IsOperationIdentity(from) ? 0.0f : from->rotate.degree;
      float to_angle = IsOperationIdentity(to) ? 0.0f : to->rotate.degree;
      operation.rotate.degree = BlendValue(from_angle, to_angle, progress);
      return operation;
    }
    case TransformOperation::Type::kScale: {
      float from_x = IsOperationIdentity(from) ? 1.0f : from->scale.x;
      float from_y = IsOperationIdentity(from) ? 1.0f : from->scale.y;
      float to_x = IsOperationIdentity(to) ? 1.0f : to->scale.x;
      float to_y = IsOperationIdentity(to) ? 1.0f : to->scale.y;
      operation.scale.x = BlendValue(from_x, to_x, progress);
      operation.scale.y = BlendValue(from_y, to_y, progress);
      return operation;
    }
    case TransformOperation::Type::kSkew: {
      float from_x = IsOperationIdentity(from) ? 0.0f : from->skew.x;
      float from_y = IsOperationIdentity(from) ? 0.0f : from->skew.y;
      float to_x = IsOperationIdentity(to) ? 0.0f : to->skew.x;
      float to_y = IsOperationIdentity(to) ? 0.0f : to->skew.y;
      operation.skew.x = BlendValue(from_x, to_x, progress);
      operation.skew.y = BlendValue(from_y, to_y, progress);
      return operation;
    }
    case TransformOperation::Type::kMatrix:
    case TransformOperation::Type::kMatrix3d: {
      std::unique_ptr<DecomposedTransform> decomposed_transform_from =
          std::make_unique<DecomposedTransform>();
      Matrix44 from_matrix = Matrix44();
      if (!IsOperationIdentity(from)) {
        from_matrix.Matrix(from->matrix.matrix_data);
      }
      DecomposeTransform(decomposed_transform_from.get(), from_matrix);

      std::unique_ptr<DecomposedTransform> decomposed_transform_to =
          std::make_unique<DecomposedTransform>();
      Matrix44 to_matrix = Matrix44();
      if (!IsOperationIdentity(to)) {
        to_matrix.Matrix(to->matrix.matrix_data);
      }
      DecomposeTransform(decomposed_transform_to.get(), to_matrix);
      auto decomposed_transform_blended = BlendDecomposedTransforms(
          *decomposed_transform_from, *decomposed_transform_to, progress);
      operation = ComposeTransform(decomposed_transform_blended);
    }
    default: {
      return operation;
    }
  }
}

bool TransformOperation::NotifyElementSizeUpdated() {
  if (type == TransformOperation::Type::kTranslate &&
      (translate.type.x == TransformOperation::LengthType::kLengthPercentage ||
       translate.type.y == TransformOperation::LengthType::kLengthPercentage ||
       translate.type.z == TransformOperation::LengthType::kLengthPercentage)) {
    matrix44 = std::optional<Matrix44>();
    return true;
  }
  return false;
}

bool TransformOperation::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern pattern_type) {
  if (type == TransformOperation::Type::kTranslate) {
    if (unit_type_0_ == pattern_type || unit_type_1_ == pattern_type ||
        unit_type_2_ == pattern_type) {
      matrix44 = std::optional<Matrix44>();
      return true;
    }
  }
  return false;
}

}  // namespace transforms
}  // namespace lynx
