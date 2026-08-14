// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>

#include "clay/gfx/geometry/transform_operations_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

TEST(TransformOperationsUtilsTest, PrimitiveZOnlyAffectsSiblingOrder) {
  lynx::gfx::TransformOperations operations;
  operations.AppendTranslate({10.0f, lynx::gfx::LengthUnit::kNumber},
                             {20.0f, lynx::gfx::LengthUnit::kNumber},
                             {30.0f, lynx::gfx::LengthUnit::kNumber});

  const skity::Matrix matrix =
      ApplyTransform(operations, 500.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_FLOAT_EQ(matrix.Get(0, 3), 10.0f);
  EXPECT_FLOAT_EQ(matrix.Get(1, 3), 20.0f);
  EXPECT_FLOAT_EQ(matrix.Get(2, 3), 0.0f);
  EXPECT_FLOAT_EQ(matrix.Get(3, 2), -1.0f / 500.0f);
  EXPECT_FLOAT_EQ(GetTranslateZ(operations), 30.0f);
}

TEST(TransformOperationsUtilsTest, Matrix3dRemainsVisual) {
  std::array<double, 16> matrix_data = {1, 0, 0, 0, 0,  1,  0,  0,
                                        0, 0, 1, 0, 10, 20, 30, 1};
  lynx::gfx::TransformOperations operations;
  operations.AppendMatrix(lynx::gfx::TransformOperation::kMatrix3d,
                          matrix_data);

  const skity::Matrix matrix =
      ApplyTransform(operations, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_FLOAT_EQ(matrix.Get(0, 3), 10.0f);
  EXPECT_FLOAT_EQ(matrix.Get(1, 3), 20.0f);
  EXPECT_FLOAT_EQ(matrix.Get(2, 3), 30.0f);
  EXPECT_FLOAT_EQ(GetTranslateZ(operations), 0.0f);
}

}  // namespace
}  // namespace clay
