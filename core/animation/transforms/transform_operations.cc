// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifdef OS_WIN
#define _USE_MATH_DEFINES
#endif

#include "core/animation/transforms/transform_operations.h"

#include <algorithm>
#include <cmath>
#include <utility>

// TODO(wujintian): Remove this include because the actual implementation has no
// relation with CSSKeyframeManager
#include "core/animation/css_keyframe_manager.h"  // nogncheck
#include "core/animation/transforms/decomposed_transform.h"
#include "core/animation/transforms/matrix44.h"
#include "core/animation/transforms/transform_operation.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace transforms {
static inline constexpr float RadToDeg(float rad) {
  return rad * 180.0f / M_PI;
}

// A standalone function using for initialize a transform operations with
// transform raw data. It will traverse each item in the raw data and append
// different transform operation to transform operations according to the item
// type.
void TransformOperations::InitializeTransformOperations(
    TransformOperations& transform_operations,
    std::vector<lynx::starlight::TransformRawData>& transform_raw_data) {
  for (auto& item : transform_raw_data) {
    auto zero_unit_nlength = starlight::NLength::MakeUnitNLength(0.0f);
    switch (item.type) {
      case starlight::TransformType::kTranslate: {
        transform_operations.AppendTranslate(
            item.p0,
            item.p0.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit,
            item.p1,
            item.p1.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit,
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
        transform_operations.AppendTranslateUnitType(item);
        break;
      }
      case starlight::TransformType::kTranslateX: {
        transform_operations.AppendTranslate(
            item.p0,
            item.p0.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit,
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
        transform_operations.AppendTranslateUnitType(item);
        break;
      }
      case starlight::TransformType::kTranslateY: {
        transform_operations.AppendTranslate(
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
            item.p0,
            item.p0.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit,
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
        transform_operations.AppendTranslateUnitType(item);
        break;
      }
      case starlight::TransformType::kTranslateZ: {
        transform_operations.AppendTranslate(
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
            zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
            item.p0,
            item.p0.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit);
        transform_operations.AppendTranslateUnitType(item);
        break;
      }
      case starlight::TransformType::kTranslate3d: {
        transform_operations.AppendTranslate(
            item.p0,
            item.p0.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit,
            item.p1,
            item.p1.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit,
            item.p2,
            item.p2.IsPercent()
                ? TransformOperation::LengthType::kLengthPercentage
                : TransformOperation::LengthType::kLengthUnit);
        transform_operations.AppendTranslateUnitType(item);
        break;
      }
      case starlight::TransformType::kRotateX: {
        transform_operations.AppendRotate(TransformOperation::Type::kRotateX,
                                          item.p0.GetRawValue());
        break;
      }
      case starlight::TransformType::kRotateY: {
        transform_operations.AppendRotate(TransformOperation::Type::kRotateY,
                                          item.p0.GetRawValue());
        break;
      }
      case starlight::TransformType::kRotate:
      case starlight::TransformType::kRotateZ: {
        transform_operations.AppendRotate(TransformOperation::Type::kRotateZ,
                                          item.p0.GetRawValue());
        break;
      }
      case starlight::TransformType::kScale: {
        transform_operations.AppendScale(item.p0.GetRawValue(),
                                         item.p1.GetRawValue());
        break;
      }
      case starlight::TransformType::kScaleX: {
        transform_operations.AppendScale(item.p0.GetRawValue(), 1.0f);
        break;
      }
      case starlight::TransformType::kScaleY: {
        transform_operations.AppendScale(1.0f, item.p0.GetRawValue());
        break;
      }
      case starlight::TransformType::kSkew: {
        transform_operations.AppendSkew(item.p0.GetRawValue(),
                                        item.p1.GetRawValue());
        break;
      }
      case starlight::TransformType::kSkewX: {
        transform_operations.AppendSkew(item.p0.GetRawValue(), 0.0f);
        break;
      }
      case starlight::TransformType::kSkewY: {
        transform_operations.AppendSkew(0.0f, item.p0.GetRawValue());
        break;
      }
      case starlight::TransformType::kMatrix:
      case starlight::TransformType::kMatrix3d: {
        transform_operations.AppendMatrix(item.type, item.matrix);
        break;
      }
      default: {
        break;
      }
    }
  }
}

TransformOperations::TransformOperations(tasm::Element* element)
    : element_(element) {}

// Construct a transform operations with transform data whose type is
// tasm::CSSValue. The transform data should be parsed by
// starlight::CSSStyleUtils::ComputeTransform before using it to initialize a
// transform operations.
TransformOperations::TransformOperations(tasm::Element* element,
                                         const tasm::CSSValue& raw_data)
    : element_(element) {
  std::optional<std::vector<starlight::TransformRawData>> transform_data =
      std::make_optional<std::vector<starlight::TransformRawData>>();
  if (!starlight::CSSStyleUtils::ComputeTransform(
          raw_data, false, transform_data,
          animation::CSSKeyframeManager::GetLengthContext(element),
          element->element_manager()->GetCSSParserConfigs())) {
    return;
  }
  InitializeTransformOperations(*this, *transform_data);
}

TransformOperations::TransformOperations(const TransformOperations& other) {
  operations_ = other.operations_;
  element_ = other.element_;
}

TransformOperations::~TransformOperations() = default;

TransformOperations& TransformOperations::operator=(
    const TransformOperations& other) {
  operations_ = other.operations_;
  element_ = other.element_;
  return *this;
}

Matrix44 TransformOperations::ApplyRemaining(size_t start) {
  Matrix44 to_return;
  for (size_t i = start; i < operations_.size(); i++) {
    to_return.preConcat(operations_[i].GetMatrix(element_));
  }
  return to_return;
}

TransformOperations TransformOperations::Blend(TransformOperations& from,
                                               float progress) {
  TransformOperations to_return(this->element_);
  if (!BlendInternal(from, progress, &to_return)) {
    // If the matrices cannot be blended, fallback to discrete animation logic.
    // See https://drafts.csswg.org/css-transforms/#matrix-interpolation
    to_return = progress < 0.5 ? from : *this;
  }
  return to_return;
}

size_t TransformOperations::MatchingPrefixLength(
    const TransformOperations& other) const {
  size_t num_operations =
      std::min(operations_.size(), other.operations_.size());
  for (size_t i = 0; i < num_operations; ++i) {
    if (operations_[i].type != other.operations_[i].type) {
      // Remaining operations in each operations list require matrix/matrix3d
      // interpolation.
      return i;
    }
    if (operations_[i].type == TransformOperation::kMatrix ||
        operations_[i].type == TransformOperation::kMatrix3d) {
      // The matched prefixes will interpolate with simple blend, but the matrix
      // transform_operation cannot be simply blended, so matrix that are equal
      // will not be treated as matched prefixes.
      return i;
    }
  }
  // If the operations match to the length of the shorter list, then pad its
  // length with the matching identity operations.
  // https://drafts.csswg.org/css-transforms/#transform-function-lists
  return std::max(operations_.size(), other.operations_.size());
}

bool TransformOperations::IsIdentity() const {
  for (auto& operation : operations_) {
    if (!operation.IsIdentity()) return false;
  }
  return true;
}

void TransformOperations::Append(const TransformOperation& operation) {
  operations_.push_back(operation);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendTranslate(
    starlight::NLength x_value, TransformOperation::LengthType x_type,
    starlight::NLength y_value, TransformOperation::LengthType y_type,
    starlight::NLength z_value, TransformOperation::LengthType z_type) {
  TransformOperation op;
  op.type = TransformOperation::Type::kTranslate;
  op.translate.type.x = x_type;
  op.translate.type.y = y_type;
  op.translate.type.z = z_type;
  op.translate.value.x = x_value;
  op.translate.value.y = y_value;
  op.translate.value.z = z_value;
  Append(op);
}

void TransformOperations::AppendTranslateUnitType(
    lynx::starlight::TransformRawData& raw_data) {
  operations_.front().unit_type_0_ = raw_data.unit_type0;
  operations_.front().unit_type_1_ = raw_data.unit_type1;
  operations_.front().unit_type_2_ = raw_data.unit_type2;
}

void TransformOperations::AppendRotate(TransformOperation::Type type,
                                       float degree) {
  TransformOperation op;
  op.type = type;
  op.rotate.degree = degree;
  Append(op);
}
void TransformOperations::AppendScale(float x, float y) {
  TransformOperation op;
  op.type = TransformOperation::Type::kScale;
  op.scale.x = x;
  op.scale.y = y;
  Append(op);
}
void TransformOperations::AppendSkew(float x, float y) {
  TransformOperation op;
  op.type = TransformOperation::Type::kSkew;
  op.skew.x = x;
  op.skew.y = y;
  Append(op);
}
void TransformOperations::AppendMatrix(
    starlight::TransformType type,
    const std::array<double, 16>& raw_matrix_data) {
  TransformOperation op;
  op.type = type == starlight::TransformType::kMatrix
                ? TransformOperation::kMatrix
                : TransformOperation::kMatrix3d;

  std::array<float, 16> temp_matrix;
  std::transform(raw_matrix_data.begin(), raw_matrix_data.end(),
                 temp_matrix.begin(),
                 [](double d) { return static_cast<float>(d); });
  op.matrix.matrix_data = temp_matrix;
  Append(op);
}

void TransformOperations::AppendDecomposedTransform(
    const DecomposedTransform& decomposed) {
  AppendTranslate(starlight::NLength::MakeUnitNLength(decomposed.translate[0]),
                  TransformOperation::LengthType::kLengthUnit,
                  starlight::NLength::MakeUnitNLength(decomposed.translate[1]),
                  TransformOperation::LengthType::kLengthUnit,
                  starlight::NLength::MakeUnitNLength(decomposed.translate[2]),
                  TransformOperation::LengthType::kLengthUnit);

  Euler euler = decomposed.quaternion.ConvertToEuler();
  AppendRotate(TransformOperation::Type::kRotateX, RadToDeg(euler.x));
  AppendRotate(TransformOperation::Type::kRotateY, RadToDeg(euler.y));
  AppendRotate(TransformOperation::Type::kRotateZ, RadToDeg(euler.z));

  AppendSkew(RadToDeg(atan(decomposed.skew[0])), 0);

  AppendScale(decomposed.scale[0], decomposed.scale[1]);
}

bool TransformOperations::BlendInternal(TransformOperations& from,
                                        float progress,
                                        TransformOperations* result) {
  bool from_identity = from.IsIdentity();
  bool to_identity = IsIdentity();
  if (from_identity && to_identity) return true;

  size_t matching_prefix_length = MatchingPrefixLength(from);
  size_t from_size = from_identity ? 0 : from.operations_.size();
  size_t to_size = to_identity ? 0 : operations_.size();
  size_t num_operations = std::max(from_size, to_size);

  for (size_t i = 0; i < matching_prefix_length; ++i) {
    TransformOperation blended = TransformOperation::BlendTransformOperations(
        i >= from_size ? nullptr : &from.operations_[i],
        i >= to_size ? nullptr : &operations_[i], progress, element_);
    result->Append(blended);
  }

  if (matching_prefix_length < num_operations) {
    if (!ComputeDecomposedTransform(matching_prefix_length) ||
        !from.ComputeDecomposedTransform(matching_prefix_length)) {
      return false;
    }
    DecomposedTransform matrix_transform = BlendDecomposedTransforms(
        *decomposed_transforms_[matching_prefix_length],
        *from.decomposed_transforms_[matching_prefix_length], progress);
    result->AppendDecomposedTransform(matrix_transform);
  }
  return true;
}

bool TransformOperations::ComputeDecomposedTransform(size_t start_offset) {
  auto it = decomposed_transforms_.find(start_offset);
  if (it == decomposed_transforms_.end()) {
    std::unique_ptr<DecomposedTransform> decomposed_transform =
        std::make_unique<DecomposedTransform>();
    Matrix44 transform = ApplyRemaining(start_offset);
    if (!DecomposeTransform(decomposed_transform.get(), transform)) {
      return false;
    }
    decomposed_transforms_[start_offset] = std::move(decomposed_transform);
  }
  return true;
}

void TransformOperations::NotifyElementSizeUpdated() {
  bool need_update = false;
  for (auto& op : operations_) {
    need_update = need_update || op.NotifyElementSizeUpdated();
  }
  if (need_update) {
    decomposed_transforms_.clear();
  }
}

void TransformOperations::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern type) {
  bool need_update = false;
  for (auto& op : operations_) {
    need_update = need_update || op.NotifyUnitValuesUpdatedToAnimation(type);
  }
  if (need_update) {
    decomposed_transforms_.clear();
    operations_.clear();
  }
}

// A method using for converting transform operations to transform raw data.
// Transform operations will be used for animation calculations. After the
// calculation is over, use this method to convert operations to raw data and
// update it on element.
tasm::CSSValue TransformOperations::ToTransformRawValue() {
  auto items = lepus::CArray::Create();
  for (auto& op : operations_) {
    switch (op.type) {
      case TransformOperation::Type::kTranslate: {
        auto item = lepus::CArray::Create();
        item->emplace_back(
            static_cast<int>(starlight::TransformType::kTranslate3d));
        item->emplace_back(op.translate.value.x.GetRawValue());
        item->emplace_back(static_cast<int>(
            op.translate.type.x ==
                    TransformOperation::LengthType::kLengthPercentage
                ? tasm::CSSValuePattern::PERCENT
                : tasm::CSSValuePattern::NUMBER));
        item->emplace_back(op.translate.value.y.GetRawValue());
        item->emplace_back(static_cast<int>(
            op.translate.type.y ==
                    TransformOperation::LengthType::kLengthPercentage
                ? tasm::CSSValuePattern::PERCENT
                : tasm::CSSValuePattern::NUMBER));
        item->emplace_back(op.translate.value.z.GetRawValue());
        item->emplace_back(static_cast<int>(
            op.translate.type.z ==
                    TransformOperation::LengthType::kLengthPercentage
                ? tasm::CSSValuePattern::PERCENT
                : tasm::CSSValuePattern::NUMBER));
        items->emplace_back(std::move(item));
        break;
      }
      case TransformOperation::Type::kRotateX: {
        auto item = lepus::CArray::Create();
        item->emplace_back(
            static_cast<int>(starlight::TransformType::kRotateX));
        item->emplace_back(op.rotate.degree);
        items->emplace_back(std::move(item));
        break;
      }
      case TransformOperation::Type::kRotateY: {
        auto item = lepus::CArray::Create();
        item->emplace_back(
            static_cast<int>(starlight::TransformType::kRotateY));
        item->emplace_back(op.rotate.degree);
        items->emplace_back(std::move(item));
        break;
      }
      case TransformOperation::Type::kRotateZ: {
        auto item = lepus::CArray::Create();
        item->emplace_back(
            static_cast<int>(starlight::TransformType::kRotateZ));
        item->emplace_back(op.rotate.degree);
        items->emplace_back(std::move(item));
        break;
      }
      case TransformOperation::Type::kScale: {
        auto item = lepus::CArray::Create();
        item->emplace_back(static_cast<int>(starlight::TransformType::kScale));
        item->emplace_back(op.scale.x);
        item->emplace_back(op.scale.y);
        items->emplace_back(std::move(item));
        break;
      }
      case TransformOperation::Type::kSkew: {
        auto item = lepus::CArray::Create();
        item->emplace_back(static_cast<int>(starlight::TransformType::kSkew));
        item->emplace_back(op.skew.x);
        item->emplace_back(op.skew.y);
        items->emplace_back(std::move(item));
        break;
      }
      case TransformOperation::Type::kMatrix:
      case TransformOperation::Type::kMatrix3d: {
        auto item = lepus::CArray::Create();
        item->emplace_back(
            static_cast<int>(starlight::TransformType::kMatrix3d));

        items->emplace_back(std::move(item));
      }
      default: {
        break;
      }
    }
  }
  return tasm::CSSValue(std::move(items));
}

}  // namespace transforms
}  // namespace lynx
