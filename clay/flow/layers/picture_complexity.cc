// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_complexity.h"

#include "clay/flow/layers/picture_complexity_gl.h"
#include "clay/flow/layers/picture_complexity_metal.h"

namespace clay {

PictureNaiveComplexityCalculator* PictureNaiveComplexityCalculator::instance_ =
    nullptr;

PictureComplexityCalculator* PictureNaiveComplexityCalculator::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new PictureNaiveComplexityCalculator();
  }
  return instance_;
}

PictureComplexityCalculator* PictureComplexityCalculator::GetForBackend(
    clay::GrGpuBackendType backend) {
  switch (backend) {
    case clay::GrGpuBackendType::kMetal:
      return PictureMetalComplexityCalculator::GetInstance();
    case clay::GrGpuBackendType::kOpenGL:
      return PictureGLComplexityCalculator::GetInstance();
    default:
      return PictureNaiveComplexityCalculator::GetInstance();
  }
}

PictureComplexityCalculator* PictureComplexityCalculator::GetForSoftware() {
#ifndef ENABLE_SKITY
  return PictureNaiveComplexityCalculator::GetInstance();
#else
  return PictureComplexityCalculatorSkity::GetInstance();
#endif  // ENABLE_SKITY
}

}  // namespace clay
