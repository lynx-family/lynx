// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>

#include "clay/flow/embedded_views.h"
#include "clay/fml/logging.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(EmbeddedViewParams, GetBoundingRectAfterMutationsWithNoMutations) {
  MutatorsStack stack;
  skity::Matrix matrix;

  EmbeddedViewParams params(matrix, skity::Vec2(1, 1), stack);
  const skity::Rect& rect = params.finalBoundingRect();
  ASSERT_TRUE(SkScalarNearlyEqual(rect.X(), 0));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Y(), 0));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Width(), 1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Height(), 1));
}

TEST(EmbeddedViewParams, GetBoundingRectAfterMutationsWithScale) {
  MutatorsStack stack;
  skity::Matrix matrix = skity::Matrix::Scale(2, 2);
  stack.PushTransform(matrix);

  EmbeddedViewParams params(matrix, skity::Vec2(1, 1), stack);
  const skity::Rect& rect = params.finalBoundingRect();
  ASSERT_TRUE(SkScalarNearlyEqual(rect.X(), 0));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Y(), 0));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Width(), 2));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Height(), 2));
}

TEST(EmbeddedViewParams, GetBoundingRectAfterMutationsWithTranslate) {
  MutatorsStack stack;
  skity::Matrix matrix = skity::Matrix::Translate(1, 1);
  stack.PushTransform(matrix);

  EmbeddedViewParams params(matrix, skity::Vec2(1, 1), stack);
  const skity::Rect& rect = params.finalBoundingRect();
  ASSERT_TRUE(SkScalarNearlyEqual(rect.X(), 1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Y(), 1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Width(), 1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Height(), 1));
}

TEST(EmbeddedViewParams, GetBoundingRectAfterMutationsWithRotation90) {
  MutatorsStack stack;
  skity::Matrix matrix = skity::Matrix::RotateDeg(90);
  stack.PushTransform(matrix);

  EmbeddedViewParams params(matrix, skity::Vec2(1, 1), stack);
  const skity::Rect& rect = params.finalBoundingRect();

  ASSERT_TRUE(SkScalarNearlyEqual(rect.X(), -1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Y(), 0));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Width(), 1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Height(), 1));
}

TEST(EmbeddedViewParams, GetBoundingRectAfterMutationsWithRotation45) {
  MutatorsStack stack;
  skity::Matrix matrix = skity::Matrix::RotateDeg(45);
  stack.PushTransform(matrix);

  EmbeddedViewParams params(matrix, skity::Vec2(1, 1), stack);
  const skity::Rect& rect = params.finalBoundingRect();
  ASSERT_TRUE(SkScalarNearlyEqual(rect.X(), -sqrt(2) / 2));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Y(), 0));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Width(), sqrt(2)));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Height(), sqrt(2)));
}

TEST(EmbeddedViewParams,
     GetBoundingRectAfterMutationsWithTranslateScaleAndRotation) {
  skity::Matrix matrix = skity::Matrix::Translate(2, 2);
  matrix.PreScale(3, 3);
  matrix.PreRotate(90);

  MutatorsStack stack;
  stack.PushTransform(matrix);

  EmbeddedViewParams params(matrix, skity::Vec2(1, 1), stack);
  const skity::Rect& rect = params.finalBoundingRect();
  ASSERT_TRUE(SkScalarNearlyEqual(rect.X(), -1));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Y(), 2));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Width(), 3));
  ASSERT_TRUE(SkScalarNearlyEqual(rect.Height(), 3));
}

}  // namespace testing
}  // namespace clay
