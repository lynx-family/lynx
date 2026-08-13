// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>

#include "clay/gfx/geometry/transform_value.h"
#include "clay/public/style_types.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

TransformRaw TranslateRaw(ClayTransformType type, float x, float y, float z) {
  TransformRaw raw{};
  raw.type = static_cast<int>(type);
  raw.values[0] = Length(x);
  raw.values[1] = Length(y);
  raw.values[2] = Length(z);
  return raw;
}

TEST(TransformValueTest, ResolveTranslate3dSeparatesStackingZ) {
  TransformValue result = ResolveTransform(
      {TranslateRaw(ClayTransformType::kTranslate3d, 10.0f, 20.0f, 30.0f)},
      100.0f, 200.0f);

  ASSERT_EQ(result.visual_operations.size(), 1u);
  const auto& operation = result.visual_operations.GetOperations().front();
  EXPECT_EQ(operation.type, lynx::gfx::TransformOperation::kTranslate);
  EXPECT_FLOAT_EQ(operation.translate.x.value, 10.0f);
  EXPECT_FLOAT_EQ(operation.translate.y.value, 20.0f);
  EXPECT_FLOAT_EQ(operation.translate.z.value, 0.0f);
  EXPECT_FLOAT_EQ(result.stacking_z, 30.0f);
}

TEST(TransformValueTest, ResolveTranslateZAccumulatesAndKeepsPlaceholders) {
  TransformValue result = ResolveTransform(
      {TranslateRaw(ClayTransformType::kTranslateZ, 4.0f, 0.0f, 0.0f),
       TranslateRaw(ClayTransformType::kTranslateZ, 6.0f, 0.0f, 0.0f)},
      0.0f, 0.0f);

  ASSERT_EQ(result.visual_operations.size(), 2u);
  for (const auto& operation : result.visual_operations.GetOperations()) {
    EXPECT_EQ(operation.type, lynx::gfx::TransformOperation::kTranslate);
    EXPECT_TRUE(operation.IsIdentity());
  }
  EXPECT_FLOAT_EQ(result.stacking_z, 10.0f);
}

TEST(TransformValueTest, ResolveGfxPercentagesAgainstContentSize) {
  lynx::gfx::TransformOperations operations;
  operations.AppendTranslate({50.0f, lynx::gfx::LengthUnit::kPercent},
                             {25.0f, lynx::gfx::LengthUnit::kPercent},
                             {7.0f, lynx::gfx::LengthUnit::kNumber});

  TransformValue result = ResolveTransform(operations, 200.0f, 400.0f);

  ASSERT_EQ(result.visual_operations.size(), 1u);
  const auto& translate =
      result.visual_operations.GetOperations().front().translate;
  EXPECT_EQ(translate.x.unit, lynx::gfx::LengthUnit::kNumber);
  EXPECT_EQ(translate.y.unit, lynx::gfx::LengthUnit::kNumber);
  EXPECT_FLOAT_EQ(translate.x.value, 100.0f);
  EXPECT_FLOAT_EQ(translate.y.value, 100.0f);
  EXPECT_FLOAT_EQ(translate.z.value, 0.0f);
  EXPECT_FLOAT_EQ(result.stacking_z, 7.0f);
}

TEST(TransformValueTest, Matrix3dKeepsCompleteVisualMatrix) {
  std::array<double, 16> raw_matrix = {1, 2,  3,  4,  5,  6,  7,  8,
                                       9, 10, 11, 12, 13, 14, 15, 16};
  lynx::gfx::TransformOperations operations;
  operations.AppendMatrix(lynx::gfx::TransformOperation::kMatrix3d, raw_matrix);

  const skity::Matrix matrix =
      ToSkityMatrix(operations.ApplyRemaining(0, 0.0f, 0.0f));
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      EXPECT_FLOAT_EQ(matrix.Get(row, column), raw_matrix[4 * column + row]);
    }
  }
}

TEST(TransformValueTest, ApplyTransformDoesNotAccumulatePerspective) {
  lynx::gfx::TransformOperations operations;
  operations.AppendTranslate({10.0f, lynx::gfx::LengthUnit::kNumber},
                             {20.0f, lynx::gfx::LengthUnit::kNumber},
                             {0.0f, lynx::gfx::LengthUnit::kNumber});

  const skity::Matrix first =
      ApplyTransform(operations, 500.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  const skity::Matrix second =
      ApplyTransform(operations, 500.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_EQ(first, second);
  EXPECT_FLOAT_EQ(first.Get(0, 3), 10.0f);
  EXPECT_FLOAT_EQ(first.Get(1, 3), 20.0f);
  EXPECT_FLOAT_EQ(first.Get(3, 2), -1.0f / 500.0f);
}

TEST(TransformValueTest, BlendFailureUsesOneDiscreteEndpointForBothValues) {
  std::array<double, 16> singular_matrix = {0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 1};
  TransformValue from;
  from.visual_operations.AppendMatrix(lynx::gfx::TransformOperation::kMatrix3d,
                                      singular_matrix);
  from.stacking_z = 10.0f;
  TransformValue to;
  to.visual_operations.AppendScale(2.0f, 2.0f);
  to.stacking_z = 20.0f;

  TransformValue before = to.Blend(from, 0.25f);
  EXPECT_TRUE(before.visual_operations.ApproximatelyEqual(
      from.visual_operations, 0.0f));
  EXPECT_FLOAT_EQ(before.stacking_z, from.stacking_z);

  TransformValue after = to.Blend(from, 0.75f);
  EXPECT_TRUE(
      after.visual_operations.ApproximatelyEqual(to.visual_operations, 0.0f));
  EXPECT_FLOAT_EQ(after.stacking_z, to.stacking_z);
}

}  // namespace
}  // namespace clay
