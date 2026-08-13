// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/transforms/transform_operations_helper.h"

#include <array>

#include "core/renderer/starlight/types/nlength.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace transforms {
namespace testing {

TEST(TransformOperationsAdapterTest, ConvertsNLengthAtCoreBoundary) {
  starlight::TransformRawData translate;
  translate.type = starlight::TransformType::kTranslate3d;
  translate.p0 = starlight::NLength::MakePercentageNLength(50.0f);
  translate.p1 = starlight::NLength::MakeCalcNLength(10.0f, 25.0f);
  translate.p2 = starlight::NLength::MakeUnitNLength(30.0f);

  base::InlineVector<starlight::TransformRawData, 1> raw;
  raw.push_back(translate);
  gfx::TransformOperations operations =
      ConvertToGfxTransformOperations(raw, 200.0f, 400.0f);
  ASSERT_EQ(operations.size(), 1u);
  const auto& operation = operations.GetOperations()[0];
  EXPECT_FLOAT_EQ(operation.translate.x.value, 50.0f);
  EXPECT_EQ(operation.translate.x.unit, gfx::LengthUnit::kPercent);
  EXPECT_FLOAT_EQ(operation.translate.y.value, 110.0f);
  EXPECT_EQ(operation.translate.y.unit, gfx::LengthUnit::kNumber);
  EXPECT_FLOAT_EQ(operation.translate.z.value, 30.0f);
}

TEST(TransformOperationsAdapterTest, SerializesBlendedValueToCSSArray) {
  gfx::TransformOperations operations;
  operations.AppendTranslate({25.0f, gfx::LengthUnit::kPercent},
                             {40.0f, gfx::LengthUnit::kNumber},
                             {0.0f, gfx::LengthUnit::kNumber});

  tasm::CSSValue value = ConvertToCSSValue(operations, 1.0f);
  ASSERT_TRUE(value.IsArray());
  auto array = value.GetValue().Array();
  ASSERT_EQ(array->size(), 1u);
  auto translate = array->get(0).Array();
  EXPECT_EQ(translate->get(0).Number(),
            static_cast<int>(starlight::TransformType::kTranslate3d));
  EXPECT_FLOAT_EQ(translate->get(1).Number(), 25.0f);
  EXPECT_EQ(translate->get(2).Number(),
            static_cast<int>(tasm::CSSValuePattern::PERCENT));
  EXPECT_FLOAT_EQ(translate->get(3).Number(), 40.0f);
  EXPECT_EQ(translate->get(4).Number(),
            static_cast<int>(tasm::CSSValuePattern::NUMBER));
}

TEST(TransformOperationsAdapterTest, SerializesMatrixParameters) {
  gfx::TransformOperations operations;
  operations.AppendMatrix(
      gfx::TransformOperation::kMatrix,
      std::array<double, 16>{1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 1, 0, 5, 6, 0, 1});
  operations.AppendMatrix(gfx::TransformOperation::kMatrix3d,
                          std::array<double, 16>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                 11, 12, 13, 14, 15, 16});

  tasm::CSSValue value = ConvertToCSSValue(operations, 1.0f);
  ASSERT_TRUE(value.IsArray());
  auto array = value.GetValue().Array();
  ASSERT_EQ(array->size(), 2u);

  auto matrix = array->get(0).Array();
  ASSERT_EQ(matrix->size(), 7u);
  EXPECT_EQ(matrix->get(0).Number(),
            static_cast<int>(starlight::TransformType::kMatrix));
  const std::array<float, 6> expected_2d = {1, 2, 3, 4, 5, 6};
  for (size_t i = 0; i < expected_2d.size(); ++i) {
    EXPECT_FLOAT_EQ(matrix->get(i + 1).Number(), expected_2d[i]);
  }

  auto matrix3d = array->get(1).Array();
  ASSERT_EQ(matrix3d->size(), 17u);
  EXPECT_EQ(matrix3d->get(0).Number(),
            static_cast<int>(starlight::TransformType::kMatrix3d));
  for (size_t i = 0; i < 16; ++i) {
    EXPECT_FLOAT_EQ(matrix3d->get(i + 1).Number(), static_cast<float>(i + 1));
  }
}

TEST(TransformOperationsAdapterTest, SerializesMatrixTranslationInCssPixels) {
  gfx::TransformOperations operations;
  operations.AppendMatrix(
      gfx::TransformOperation::kMatrix,
      std::array<double, 16>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 20, 40, 0, 1});
  operations.AppendMatrix(gfx::TransformOperation::kMatrix3d,
                          std::array<double, 16>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                                                 1, 0, 20, 40, 60, 1});

  auto array = ConvertToCSSValue(operations, 2.0f).GetValue().Array();
  auto matrix = array->get(0).Array();
  EXPECT_FLOAT_EQ(matrix->get(5).Number(), 10.0f);
  EXPECT_FLOAT_EQ(matrix->get(6).Number(), 20.0f);

  auto matrix3d = array->get(1).Array();
  EXPECT_FLOAT_EQ(matrix3d->get(13).Number(), 10.0f);
  EXPECT_FLOAT_EQ(matrix3d->get(14).Number(), 20.0f);
  EXPECT_FLOAT_EQ(matrix3d->get(15).Number(), 30.0f);
}

TEST(TransformOperationsAdapterTest, ConvertsEveryRawOperationType) {
  const starlight::NLength one = starlight::NLength::MakeUnitNLength(1.0f);
  const starlight::NLength two = starlight::NLength::MakeUnitNLength(2.0f);

  base::InlineVector<starlight::TransformRawData, 1> raw;
  const auto append = [&](starlight::TransformType type) {
    starlight::TransformRawData item;
    item.type = type;
    item.p0 = one;
    item.p1 = two;
    item.p2 = one;
    raw.push_back(item);
  };
  append(starlight::TransformType::kTranslate);
  append(starlight::TransformType::kTranslateX);
  append(starlight::TransformType::kTranslateY);
  append(starlight::TransformType::kTranslateZ);
  append(starlight::TransformType::kTranslate3d);
  append(starlight::TransformType::kRotate);
  append(starlight::TransformType::kRotateX);
  append(starlight::TransformType::kRotateY);
  append(starlight::TransformType::kRotateZ);
  append(starlight::TransformType::kScale);
  append(starlight::TransformType::kScaleX);
  append(starlight::TransformType::kScaleY);
  append(starlight::TransformType::kSkew);
  append(starlight::TransformType::kSkewX);
  append(starlight::TransformType::kSkewY);
  append(starlight::TransformType::kMatrix);
  append(starlight::TransformType::kMatrix3d);

  gfx::TransformOperations operations =
      ConvertToGfxTransformOperations(raw, 0.0f, 0.0f);
  ASSERT_EQ(operations.size(), raw.size());

  const std::array<gfx::TransformOperation::Type, 17> expected_types = {
      gfx::TransformOperation::kTranslate, gfx::TransformOperation::kTranslate,
      gfx::TransformOperation::kTranslate, gfx::TransformOperation::kTranslate,
      gfx::TransformOperation::kTranslate, gfx::TransformOperation::kRotateZ,
      gfx::TransformOperation::kRotateX,   gfx::TransformOperation::kRotateY,
      gfx::TransformOperation::kRotateZ,   gfx::TransformOperation::kScale,
      gfx::TransformOperation::kScale,     gfx::TransformOperation::kScale,
      gfx::TransformOperation::kSkew,      gfx::TransformOperation::kSkew,
      gfx::TransformOperation::kSkew,      gfx::TransformOperation::kMatrix,
      gfx::TransformOperation::kMatrix3d,
  };
  for (size_t i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(operations.GetOperations()[i].type, expected_types[i]);
  }

  EXPECT_FLOAT_EQ(operations.GetOperations()[1].translate.x.value, 1.0f);
  EXPECT_FLOAT_EQ(operations.GetOperations()[1].translate.y.value, 0.0f);
  EXPECT_FLOAT_EQ(operations.GetOperations()[10].scale.y, 1.0f);
  EXPECT_FLOAT_EQ(operations.GetOperations()[11].scale.x, 1.0f);
  EXPECT_FLOAT_EQ(operations.GetOperations()[13].skew.y, 0.0f);
  EXPECT_FLOAT_EQ(operations.GetOperations()[14].skew.x, 0.0f);
}

}  // namespace testing
}  // namespace transforms
}  // namespace lynx
