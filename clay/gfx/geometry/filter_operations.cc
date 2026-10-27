// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/filter_operations.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/transform_operation.h"

namespace clay {

namespace {
float linearInterpolate(float from, float to, float progress) {
  return from * (1 - progress) + to * progress;
}
void ColorMatrixSetIdentity(std::array<float, 20>& matrix) {
  matrix.fill(0.f);
  matrix[0] = matrix[6] = matrix[12] = matrix[18] = 1.f;
}

void ColorMatrixPreConcat(float result[20], const float mat_a[20],
                          const float mat_b[20]) {
  float tmp[20];
  float* target;

  if (mat_a == result || mat_b == result) {
    target = tmp;  // will memcpy answer when we're done into result
  } else {
    target = result;
  }

  int index = 0;
  for (int j = 0; j < 20; j += 5) {
    for (int i = 0; i < 4; i++) {
      target[index++] =
          mat_a[j + 0] * mat_b[i + 0] + mat_a[j + 1] * mat_b[i + 5] +
          mat_a[j + 2] * mat_b[i + 10] + mat_a[j + 3] * mat_b[i + 15];
    }
    target[index++] = mat_a[j + 0] * mat_b[4] + mat_a[j + 1] * mat_b[9] +
                      mat_a[j + 2] * mat_b[14] + mat_a[j + 3] * mat_b[19] +
                      mat_a[j + 4];
  }

  if (target != result) {
    std::copy_n(target, 20, result);
  }
}

}  // namespace

FilterOperation::FilterOperation(Type type, float value) : type(type) {
  switch (type) {
    case kTypeGrayscale:
    case kTypeBlur:
    case kTypeBrightness:
    case kTypeContrast:
    case kTypeSaturate:
    case kTypeHueRotate:
      this->value = value;
      break;
    case kTypeNone:
    case kTypeMatrix:
    case kTypeIdentity:
      break;
  }
}

FilterOperations::FilterOperations(const std::vector<FilterValue>& values) {
  for (auto& value : values) {
    FilterOperation op = FilterOperation(
        static_cast<FilterOperation::Type>(value.type), value.value);
    operations_.emplace_back(std::move(op));
  }
}

void FilterOperation::Bake() {
  ColorMatrixSetIdentity(color_matrix);
  switch (type) {
    case FilterOperation::kTypeGrayscale: {
      auto v = 1 - value;
      auto m01 = 0.2126f + 0.7874f * v;
      auto m02 = 0.7152f - 0.7152f * v;
      auto m10 = 0.2126f - 0.2126f * v;
      auto m11 = 0.7152f + 0.2848f * v;
      color_matrix = {m01,
                      m02,
                      1.f - (m01 + m02),
                      0.f,
                      0.f,
                      m10,
                      m11,
                      1.f - (m10 + m11),
                      0.f,
                      0.f,
                      m10,
                      m02,
                      1.f - (m10 + m02),
                      0.f,
                      0.f,
                      0.f,
                      0.f,
                      0.f,
                      1.f,
                      0.f};
      break;
    }
    case FilterOperation::kTypeBrightness: {
      if (value != 1) {
        // ref: cc/paint/render_surface_filters.cc
        // [0, +inf]
        color_matrix = {value, 0.f, 0.f,   0.f, 0.f, 0.f, value, 0.f, 0.f, 0.f,
                        0.f,   0.f, value, 0.f, 0.f, 0.f, 0.f,   0.f, 1.f, 0.f};
      }
      break;
    }
    case FilterOperation::kTypeContrast: {
      if (value != 1.0f) {
        // ref: cc/paint/render_surface_filters.cc
        auto v = -0.5f * value + 0.5f;
        color_matrix = {
            value, 0.f, 0.f,   0.f, v, 0.f, value, 0.f, 0.f, v,
            0.f,   0.f, value, 0.f, v, 0.f, 0.f,   0.f, 1.f, 0.f,
        };
      }
      break;
    }
    case FilterOperation::kTypeSaturate: {
      if (value != 1.0f) {
        auto m01 = 0.213f + 0.787f * value;
        auto m02 = 0.715f - 0.715f * value;
        auto m10 = 0.213f - 0.213f * value;
        auto m11 = 0.715f + 0.285f * value;
        color_matrix = {m01,
                        m02,
                        1.f - (m01 + m02),
                        0.f,
                        0.f,
                        m10,
                        m11,
                        1.f - (m10 + m11),
                        0.f,
                        0.f,
                        m10,
                        m02,
                        1.f - (m10 + m02),
                        0.f,
                        0.f,
                        0.f,
                        0.f,
                        0.f,
                        1.f,
                        0.f};
      }
      break;
    }
    case FilterOperation::kTypeHueRotate: {
      if (value != 0) {
        float rad = value * 3.1415926 / 180;
        float cos_hue = std::cosf(rad);
        float sin_hue = std::sinf(rad);
        color_matrix = {0.213f + cos_hue * 0.787f - sin_hue * 0.213f,
                        0.715f - cos_hue * 0.715f - sin_hue * 0.715f,
                        0.072f - cos_hue * 0.072f + sin_hue * 0.928f,
                        0.f,
                        0.f,
                        0.213f - cos_hue * 0.213f + sin_hue * 0.143f,
                        0.715f + cos_hue * 0.285f + sin_hue * 0.140f,
                        0.072f - cos_hue * 0.072f - sin_hue * 0.283f,
                        0.f,
                        0.f,
                        0.213f - cos_hue * 0.213f - sin_hue * 0.787f,
                        0.715f - cos_hue * 0.715f + sin_hue * 0.715f,
                        0.072f + cos_hue * 0.928f + sin_hue * 0.072f,
                        0.f,
                        0.f,
                        0.f,
                        0.f,
                        0.f,
                        1.f,
                        0.f};
      }
      break;
    }
    case FilterOperation::kTypeBlur: {
      break;
    }
    case FilterOperation::kTypeMatrix: {
      break;
    }
    default:
      break;
  }
}

void FilterOperation::MakeDefault(Type type) {
  switch (type) {
    case kTypeGrayscale:
    case kTypeHueRotate:
    case kTypeBlur:
      value = 0.f;
      break;
    case kTypeBrightness:
    case kTypeContrast:
    case kTypeSaturate:
      value = 1.f;
      break;
    case kTypeNone:
    case kTypeMatrix:
    case kTypeIdentity:
      break;
  }
}

bool FilterOperation::BlendOperations(const FilterOperation* from,
                                      const FilterOperation* to, float fraction,
                                      FilterOperation* result) {
  FilterOperation::Type interpolation_type = FilterOperation::kTypeIdentity;
  std::unique_ptr<FilterOperation> op = nullptr;
  if (from == nullptr) {
    FML_DCHECK(to);
    op = std::make_unique<FilterOperation>();
    op->MakeDefault(to->type);
    from = op.get();
    interpolation_type = to->type;
  } else if (to == nullptr) {
    FML_DCHECK(from);
    op = std::make_unique<FilterOperation>();
    op->MakeDefault(from->type);
    to = op.get();
    interpolation_type = from->type;
  } else {
    interpolation_type = from->type;
  }

  result->type = interpolation_type;
  result->value = linearInterpolate(from->value, to->value, fraction);
  result->Bake();
  return true;
}

std::array<float, 20> FilterOperations::Apply() const {
  std::array<float, 20> result;
  ColorMatrixSetIdentity(result);
  for (const auto& operation : operations_) {
    ColorMatrixPreConcat(result.data(), result.data(),
                         operation.color_matrix.data());
  }
  return result;
}

void FilterOperations::Grayscale(float value) {
  FilterOperation operation;
  operation.type = FilterOperation::kTypeGrayscale;
  operation.value = value;
  operation.Bake();
  operations_.emplace_back(std::move(operation));
}

void FilterOperations::Contrast(float value) {
  FilterOperation operation;
  operation.type = FilterOperation::kTypeContrast;
  operation.value = value;
  operation.Bake();
  operations_.emplace_back(std::move(operation));
}

void FilterOperations::Brightness(float value) {
  FilterOperation operation;
  operation.type = FilterOperation::kTypeBrightness;
  operation.value = value;
  operation.Bake();
  operations_.emplace_back(std::move(operation));
}
void FilterOperations::Saturate(float value) {
  FilterOperation operation;
  operation.type = FilterOperation::kTypeSaturate;
  operation.value = value;
  operation.Bake();
  operations_.emplace_back(std::move(operation));
}
void FilterOperations::HueRotate(float value) {
  FilterOperation operation;
  operation.type = FilterOperation::kTypeHueRotate;
  operation.value = value;
  operation.Bake();
  operations_.emplace_back(std::move(operation));
}

size_t FilterOperations::MatchingPrefixLength(
    const FilterOperations& other) const {
  size_t num_operations =
      std::min(operations_.size(), other.operations_.size());
  for (size_t i = 0; i < num_operations; ++i) {
    if (operations_[i].type != other.operations_[i].type) {
      // Remaining operations in each operations list require matrix/matrix3d
      // interpolation.
      return i;
    }
  }
  // If the operations match to the length of the shorter list, then pad its
  // length with the matching identity operations.
  // https://drafts.csswg.org/css-transforms/#transform-function-lists
  return std::max(operations_.size(), other.operations_.size());
}

void FilterOperations::Append(const FilterOperations& ops) {
  for (const auto& operation : ops.operations_) {
    operations_.emplace_back(operation);
  }
}

void FilterOperations::Append(const FilterOperation& operation) {
  operations_.push_back(operation);
}

FilterOperations FilterOperations::Blend(const FilterOperations& other,
                                         float fraction) const {
  FilterOperations to_return;
  if (!BlendInternal(other, fraction, &to_return)) {
    // If the matrices cannot be blended, fallback to discrete animation
    // logic. See
    // https://drafts.csswg.org/css-transforms/#matrix-interpolation
    to_return = fraction < 0.5 ? other : *this;
  }
  return to_return;
}
bool FilterOperations::BlendInternal(const FilterOperations& other,
                                     float fraction,
                                     FilterOperations* result) const {
  size_t matching_prefix_length = MatchingPrefixLength(other);
  size_t from_size = other.operations_.size();
  size_t to_size = operations_.size();
  for (size_t i = 0; i < matching_prefix_length; ++i) {
    FilterOperation blended;
    if (!FilterOperation::BlendOperations(
            i >= from_size ? nullptr : &other.operations_[i],
            i >= to_size ? nullptr : &operations_[i], fraction, &blended)) {
      return false;
    }
    if (blended.type == FilterOperation::kTypeBlur) {
      result->blur_radius_ = blended.value;
    } else {
      result->Append(blended);
    }
  }
  return true;
}

}  // namespace clay
