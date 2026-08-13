// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/geometry/transform_operation.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace gfx {
namespace testing {

namespace {

TransformOperation MakeTranslate(LengthValue x, LengthValue y,
                                 LengthValue z = {}) {
  TransformOperation operation;
  operation.type = TransformOperation::kTranslate;
  operation.translate = {x, y, z};
  return operation;
}

void ExpectTranslate(const TransformOperation& operation, LengthValue x,
                     LengthValue y, LengthValue z) {
  ASSERT_EQ(operation.type, TransformOperation::kTranslate);
  EXPECT_FLOAT_EQ(operation.translate.x.value, x.value);
  EXPECT_EQ(operation.translate.x.unit, x.unit);
  EXPECT_FLOAT_EQ(operation.translate.y.value, y.value);
  EXPECT_EQ(operation.translate.y.unit, y.unit);
  EXPECT_FLOAT_EQ(operation.translate.z.value, z.value);
  EXPECT_EQ(operation.translate.z.unit, z.unit);
}

void ExpectMatrixNear(const Matrix44& actual, const Matrix44& expected,
                      float tolerance = 0.0001f) {
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_NEAR(actual.rc(row, col), expected.rc(row, col), tolerance);
    }
  }
}

}  // namespace

TEST(TransformOperationTest, ResolvesPercentageWithReferenceSize) {
  TransformOperation operation = MakeTranslate({50.0f, LengthUnit::kPercent},
                                               {25.0f, LengthUnit::kPercent},
                                               {10.0f, LengthUnit::kNumber});

  const Matrix44 matrix = operation.GetMatrix(200.0f, 400.0f);
  EXPECT_FLOAT_EQ(matrix.rc(0, 3), 100.0f);
  EXPECT_FLOAT_EQ(matrix.rc(1, 3), 100.0f);
  EXPECT_FLOAT_EQ(matrix.rc(2, 3), 10.0f);
}

TEST(TransformOperationTest, RebuildsMatrixWhenReferenceSizeChanges) {
  TransformOperation operation =
      MakeTranslate({50.0f, LengthUnit::kPercent}, {0.0f, LengthUnit::kNumber});

  EXPECT_FLOAT_EQ(operation.GetMatrix(100.0f, 100.0f).rc(0, 3), 50.0f);
  EXPECT_FLOAT_EQ(operation.GetMatrix(240.0f, 100.0f).rc(0, 3), 120.0f);
}

TEST(TransformOperationTest, BlendsMatchingPercentagesWithoutResolving) {
  TransformOperation from = MakeTranslate({10.0f, LengthUnit::kPercent},
                                          {20.0f, LengthUnit::kPercent},
                                          {100.0f, LengthUnit::kNumber});
  TransformOperation to = MakeTranslate({50.0f, LengthUnit::kPercent},
                                        {60.0f, LengthUnit::kPercent},
                                        {20.0f, LengthUnit::kNumber});

  TransformOperation result;
  ASSERT_TRUE(TransformOperation::BlendTransformOperations(
      &from, &to, 0.2f, 100.0f, 200.0f, &result));
  ExpectTranslate(result, {18.0f, LengthUnit::kPercent},
                  {28.0f, LengthUnit::kPercent}, {84.0f, LengthUnit::kNumber});
}

TEST(TransformOperationTest, ResolvesMixedTranslateUnitsBeforeBlending) {
  TransformOperation from = MakeTranslate({20.0f, LengthUnit::kNumber},
                                          {25.0f, LengthUnit::kPercent});
  TransformOperation to = MakeTranslate({50.0f, LengthUnit::kPercent},
                                        {70.0f, LengthUnit::kNumber});

  TransformOperation result;
  ASSERT_TRUE(TransformOperation::BlendTransformOperations(
      &from, &to, 0.5f, 200.0f, 400.0f, &result));
  ExpectTranslate(result, {60.0f, LengthUnit::kNumber},
                  {85.0f, LengthUnit::kNumber}, {0.0f, LengthUnit::kNumber});
}

TEST(TransformOperationTest, BlendsIdentityWithOperation) {
  TransformOperation scale;
  scale.type = TransformOperation::kScale;
  scale.scale = {3.0f, 5.0f};

  TransformOperation result;
  ASSERT_TRUE(TransformOperation::BlendTransformOperations(
      nullptr, &scale, 0.25f, 0.0f, 0.0f, &result));
  EXPECT_EQ(result.type, TransformOperation::kScale);
  EXPECT_FLOAT_EQ(result.scale.x, 1.5f);
  EXPECT_FLOAT_EQ(result.scale.y, 2.0f);
}

TEST(TransformOperationTest, DecomposedTransformRoundTripsAll3dComponents) {
  DecomposedTransform decomposed;
  decomposed.translate[0] = 12.0f;
  decomposed.translate[1] = -8.0f;
  decomposed.translate[2] = 30.0f;
  decomposed.scale[0] = 1.5f;
  decomposed.scale[1] = 0.75f;
  decomposed.scale[2] = 2.25f;
  decomposed.skew[0] = 0.1f;
  decomposed.skew[1] = -0.15f;
  decomposed.skew[2] = 0.2f;
  decomposed.perspective[0] = 0.0003f;
  decomposed.perspective[1] = -0.0002f;
  decomposed.perspective[2] = -0.001f;
  // Keep the composed homogeneous matrix normalized to m44 == 1.
  decomposed.perspective[3] = 1.0248f;
  decomposed.quaternion =
      Quaternion(0.102597835, -0.20519567, 0.3077935, 0.9233805);

  const Matrix44 matrix =
      TransformOperation::FromDecomposedTransform(decomposed)
          .GetMatrix(0.0f, 0.0f);
  DecomposedTransform round_tripped;
  ASSERT_TRUE(DecomposeTransform(&round_tripped, matrix));
  const Matrix44 recomposed =
      TransformOperation::FromDecomposedTransform(round_tripped)
          .GetMatrix(0.0f, 0.0f);

  ExpectMatrixNear(recomposed, matrix, 0.001f);
}

TEST(Matrix44Test, InverseRecomputesPerspectiveType) {
  Matrix44 matrix;
  matrix.setRC(3, 2, -0.002f);

  Matrix44 inverse;
  ASSERT_TRUE(matrix.invert(&inverse));
  EXPECT_TRUE(inverse.HasPerspective());
  EXPECT_NEAR(inverse.rc(3, 2), 0.002f, 0.000001f);
}

}  // namespace testing
}  // namespace gfx
}  // namespace lynx
