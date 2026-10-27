// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/lut_interpolator.h"

#include <algorithm>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/type_evaluator.h"

namespace clay {

LUTInterpolator::LUTInterpolator(std::unique_ptr<float[]> values, size_t size)
    : values_(std::move(values)), size_(size) {}

LUTInterpolator::~LUTInterpolator() = default;

std::unique_ptr<LUTInterpolator> LUTInterpolator::Create(
    std::unique_ptr<float[]> values, size_t size) {
  return std::make_unique<LUTInterpolator>(std::move(values), size);
}

std::unique_ptr<Interpolator> LUTInterpolator::Clone() {
  std::unique_ptr<float[]> values = std::make_unique<float[]>(size_);
  for (size_t i = 0; i < size_; i++) {
    values[i] = values_[i];
  }
  return LUTInterpolator::Create(std::move(values), size_);
}

float LUTInterpolator::Interpolate(float input) {
  // lut position should only be at the end of the table when input is 1f.
  float lut_pos = input * (size_ - 1);
  if (lut_pos >= (size_ - 1)) {
    return values_[size_ - 1];
  }

  float part, weight;
  weight = modff(lut_pos, &part);

  int i1 = static_cast<int>(part);
  int i2 = std::min(i1 + 1, static_cast<int>(size_) - 1);

  FML_DCHECK(i1 < 0 || i2 < 0) << "negatives in interpolation!";

  float v1 = values_[i1];
  float v2 = values_[i2];

  return TypeEvaluator<float>::Evaluate(weight, v1, v2);
}

}  // namespace clay
