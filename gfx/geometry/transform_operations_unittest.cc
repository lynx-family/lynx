// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/geometry/transform_operations.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace gfx {
namespace testing {

namespace {

constexpr float kReferenceWidth = 200.0f;
constexpr float kReferenceHeight = 400.0f;
constexpr LengthValue kZero{0.0f, LengthUnit::kNumber};

void ExpectMatrixNear(const Matrix44& actual, const Matrix44& expected) {
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_NEAR(actual.rc(row, col), expected.rc(row, col), 0.0001f);
    }
  }
}

}  // namespace

TEST(TransformOperationsTest, MatchingPrefixLength) {
  TransformOperations left;
  left.AppendTranslate({1.0f, LengthUnit::kNumber}, kZero, kZero);
  left.AppendScale(2.0f, 2.0f);

  TransformOperations same_prefix;
  same_prefix.AppendTranslate({3.0f, LengthUnit::kNumber}, kZero, kZero);
  same_prefix.AppendScale(4.0f, 4.0f);
  same_prefix.AppendRotate(TransformOperation::kRotateZ, 30.0f);
  EXPECT_EQ(left.MatchingPrefixLength(same_prefix), 3u);

  TransformOperations different;
  different.AppendTranslate({3.0f, LengthUnit::kNumber}, kZero, kZero);
  different.AppendSkew(10.0f, 20.0f);
  EXPECT_EQ(left.MatchingPrefixLength(different), 1u);
}

TEST(TransformOperationsTest, EmptyListMatchesIdentityOperations) {
  TransformOperations empty;

  TransformOperations translated;
  translated.AppendTranslate(kZero, kZero, kZero);
  EXPECT_EQ(translated.MatchingPrefixLength(empty), translated.size());

  TransformOperations scaled;
  scaled.AppendScale(1.0f, 1.0f);
  EXPECT_EQ(scaled.MatchingPrefixLength(empty), scaled.size());

  TransformOperations rotated;
  rotated.AppendRotate(TransformOperation::kRotateX, 0.0f);
  EXPECT_EQ(rotated.MatchingPrefixLength(empty), rotated.size());
}

TEST(TransformOperationsTest, ApplyPreservesOperationOrder) {
  TransformOperations operations;
  operations.AppendScale(2.0f, 3.0f);
  operations.AppendTranslate({10.0f, LengthUnit::kNumber},
                             {20.0f, LengthUnit::kNumber}, kZero);

  Matrix44 expected;
  expected.preScale(2.0f, 3.0f, 1.0f);
  Matrix44 translation;
  translation.preTranslate(10.0f, 20.0f, 0.0f);
  expected.preConcat(translation);

  ExpectMatrixNear(
      operations.ApplyRemaining(0, kReferenceWidth, kReferenceHeight),
      expected);
}

TEST(TransformOperationsTest, BlendMatchingOperations) {
  TransformOperations from;
  from.AppendTranslate({10.0f, LengthUnit::kPercent}, kZero, kZero);
  from.AppendScale(1.0f, 2.0f);

  TransformOperations to;
  to.AppendTranslate({50.0f, LengthUnit::kPercent}, kZero, kZero);
  to.AppendScale(3.0f, 4.0f);

  TransformOperations result =
      to.Blend(from, 0.25f, kReferenceWidth, kReferenceHeight);
  ASSERT_EQ(result.size(), 2u);
  EXPECT_FLOAT_EQ(result.GetOperations()[0].translate.x.value, 20.0f);
  EXPECT_EQ(result.GetOperations()[0].translate.x.unit, LengthUnit::kPercent);
  EXPECT_FLOAT_EQ(result.GetOperations()[1].scale.x, 1.5f);
  EXPECT_FLOAT_EQ(result.GetOperations()[1].scale.y, 2.5f);
}

TEST(TransformOperationsTest, BlendSupportsExtrapolation) {
  TransformOperations from;
  from.AppendScale(2.0f, 4.0f);

  TransformOperations to;
  to.AppendScale(4.0f, 8.0f);

  TransformOperations before =
      to.Blend(from, -1.0f, kReferenceWidth, kReferenceHeight);
  EXPECT_FLOAT_EQ(before.GetOperations()[0].scale.x, 0.0f);
  EXPECT_FLOAT_EQ(before.GetOperations()[0].scale.y, 0.0f);

  TransformOperations after =
      to.Blend(from, 2.0f, kReferenceWidth, kReferenceHeight);
  EXPECT_FLOAT_EQ(after.GetOperations()[0].scale.x, 6.0f);
  EXPECT_FLOAT_EQ(after.GetOperations()[0].scale.y, 12.0f);
}

TEST(TransformOperationsTest, BlendHandlesPaddedIdentityOperations) {
  TransformOperations from;
  from.AppendTranslate(kZero, kZero, kZero);
  from.AppendTranslate(kZero, kZero, kZero);

  TransformOperations to;
  to.AppendTranslate({100.0f, LengthUnit::kNumber}, kZero, kZero);

  TransformOperations result =
      to.Blend(from, 0.5f, kReferenceWidth, kReferenceHeight);
  ASSERT_EQ(result.size(), 2u);
  EXPECT_FLOAT_EQ(result.GetOperations()[0].translate.x.value, 50.0f);
  EXPECT_TRUE(result.GetOperations()[1].IsIdentity());
}

TEST(TransformOperationsTest, MatrixFallbackUsesCurrentReferenceSize) {
  TransformOperations from;
  from.AppendTranslate({50.0f, LengthUnit::kPercent}, kZero, kZero);

  TransformOperations to;
  to.AppendScale(2.0f, 2.0f);

  Matrix44 narrow =
      to.Blend(from, 0.5f, 100.0f, 100.0f).ApplyRemaining(0, 100.0f, 100.0f);
  Matrix44 wide =
      to.Blend(from, 0.5f, 300.0f, 100.0f).ApplyRemaining(0, 300.0f, 100.0f);
  EXPECT_NE(narrow.rc(0, 3), wide.rc(0, 3));
}

TEST(TransformOperationsTest, MatrixDataUsesColumnMajorOrder) {
  std::array<double, 16> raw_matrix = {1, 2,  3,  4,  5,  6,  7,  8,
                                       9, 10, 11, 12, 13, 14, 15, 16};
  TransformOperations operations;
  operations.AppendMatrix(TransformOperation::kMatrix3d, raw_matrix);

  const Matrix44 matrix = operations.ApplyRemaining(0, 0.0f, 0.0f);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_FLOAT_EQ(matrix.rc(row, col), raw_matrix[4 * col + row]);
    }
  }
}

TEST(TransformOperationsTest, MatrixFallbackPreservesThreeDimensionalValues) {
  std::array<double, 16> from_matrix = {1, 0, 0, 0,      0,  1,  0,  0,
                                        0, 0, 2, -0.002, 10, 20, 30, 1};
  std::array<double, 16> to_matrix = {2, 0, 0, 0,      0,  3,  0,  0,
                                      0, 0, 4, -0.004, 30, 40, 50, 1};
  TransformOperations from;
  from.AppendMatrix(TransformOperation::kMatrix3d, from_matrix);
  TransformOperations to;
  to.AppendMatrix(TransformOperation::kMatrix3d, to_matrix);

  TransformOperations result;
  ASSERT_TRUE(to.TryBlend(from, 0.5f, 0.0f, 0.0f, &result));
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result.GetOperations()[0].type, TransformOperation::kMatrix3d);

  const Matrix44 matrix = result.ApplyRemaining(0, 0.0f, 0.0f);
  EXPECT_NEAR(matrix.rc(0, 3), 20.0f, 0.0001f);
  EXPECT_NEAR(matrix.rc(1, 3), 30.0f, 0.0001f);
  EXPECT_NEAR(matrix.rc(2, 3), 40.0f, 0.0001f);
  EXPECT_NEAR(matrix.rc(2, 2), 3.0f, 0.0001f);
  EXPECT_NEAR(matrix.rc(3, 2), -0.003f, 0.0001f);
}

TEST(TransformOperationsTest, TryBlendReportsNonDecomposableMatrix) {
  std::array<double, 16> singular_matrix = {0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 1};
  TransformOperations from;
  from.AppendMatrix(TransformOperation::kMatrix3d, singular_matrix);
  TransformOperations to;
  to.AppendScale(2.0f, 2.0f);

  TransformOperations result;
  result.AppendTranslate({42.0f, LengthUnit::kNumber}, kZero, kZero);
  const TransformOperations original_result = result;
  EXPECT_FALSE(to.TryBlend(from, 0.25f, 0.0f, 0.0f, &result));
  EXPECT_TRUE(result.ApproximatelyEqual(original_result, 0.0f));
  EXPECT_TRUE(to.Blend(from, 0.25f, 0.0f, 0.0f).ApproximatelyEqual(from, 0.0f));
  EXPECT_TRUE(to.Blend(from, 0.75f, 0.0f, 0.0f).ApproximatelyEqual(to, 0.0f));
}

}  // namespace testing
}  // namespace gfx
}  // namespace lynx
