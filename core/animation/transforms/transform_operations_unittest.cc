// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transforms/transform_operations.h"

#include <cmath>

#include "core/animation/transforms/decomposed_transform.h"
#include "core/animation/transforms/matrix44.h"
#include "core/animation/transforms/transform_operation.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/parser/transform_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace transforms {
namespace testing {
namespace {

void CompareMatrix44(const Matrix44& a, const Matrix44& b) {
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_FLOAT_EQ(a.rc(row, col), b.rc(row, col));
    }
  }
}

Matrix44 BuildPerspectiveMatrix(const DecomposedTransform& decomposed) {
  Matrix44 matrix;
  for (int i = 0; i < 4; i++) matrix.setRC(3, i, decomposed.perspective[i]);
  return matrix;
}

Matrix44 BuildTranslationMatrix(const DecomposedTransform& decomposed) {
  Matrix44 matrix;
  matrix.preTranslate(decomposed.translate[0], decomposed.translate[1],
                      decomposed.translate[2]);
  return matrix;
}

Matrix44 BuildRotationMatrix(const DecomposedTransform& decomposed) {
  Quaternion q = decomposed.quaternion;
  Matrix44 matrix(
      // Row 1.
      (1.0 - 2.0 * (q.y() * q.y() + q.z() * q.z())),
      (2.0 * (q.x() * q.y() - q.z() * q.w())),
      (2.0 * (q.x() * q.z() + q.y() * q.w())), 0,
      // Row 2.
      (2.0 * (q.x() * q.y() + q.z() * q.w())),
      (1.0 - 2.0 * (q.x() * q.x() + q.z() * q.z())),
      (2.0 * (q.y() * q.z() - q.x() * q.w())), 0,
      // Row 3.
      (2.0 * (q.x() * q.z() - q.y() * q.w())),
      (2.0 * (q.y() * q.z() + q.x() * q.w())),
      (1.0 - 2.0 * (q.x() * q.x() + q.y() * q.y())), 0,
      // row 4.
      0, 0, 0, 1);
  return matrix;
}

Matrix44 BuildSkewMatrix(const DecomposedTransform& decomposed) {
  Matrix44 matrix;

  Matrix44 temp;
  if (decomposed.skew[2]) {
    temp.setRC(1, 2, decomposed.skew[2]);
    matrix.preConcat(temp);
  }

  if (decomposed.skew[1]) {
    temp.setRC(1, 2, 0);
    temp.setRC(0, 2, decomposed.skew[1]);
    matrix.preConcat(temp);
  }

  if (decomposed.skew[0]) {
    temp.setRC(0, 2, 0);
    temp.setRC(0, 1, decomposed.skew[0]);
    matrix.preConcat(temp);
  }
  return matrix;
}

Matrix44 BuildScaleMatrix(const DecomposedTransform& decomposed) {
  Matrix44 matrix;
  matrix.preScale((decomposed.scale[0]), (decomposed.scale[1]),
                  (decomposed.scale[2]));
  return matrix;
}

Matrix44 ComposeTransform(DecomposedTransform& decomposed) {
  Matrix44 perspective = BuildPerspectiveMatrix(decomposed);
  Matrix44 translation = BuildTranslationMatrix(decomposed);
  Matrix44 rotation = BuildRotationMatrix(decomposed);
  Matrix44 skew = BuildSkewMatrix(decomposed);
  Matrix44 scale = BuildScaleMatrix(decomposed);

  Matrix44 matrix;
  matrix.preConcat(perspective);
  matrix.preConcat(translation);
  matrix.preConcat(rotation);
  matrix.preConcat(skew);
  matrix.preConcat(scale);
  return matrix;
}

static void CheckProgress(float progress, const Matrix44& from_matrix,
                          const Matrix44& to_matrix,
                          TransformOperations& from_transform,
                          TransformOperations& to_transform) {
  DecomposedTransform decomposed_from = DecomposedTransform();
  DecomposedTransform decomposed_to = DecomposedTransform();
  DecomposeTransform(&decomposed_from, from_matrix);
  DecomposeTransform(&decomposed_to, to_matrix);
  DecomposedTransform blended =
      BlendDecomposedTransforms(decomposed_to, decomposed_from, progress);
  Matrix44 matrix0 = ComposeTransform(blended);
  Matrix44 matrix1 =
      to_transform.Blend(from_transform, progress).ApplyRemaining(0);
  CompareMatrix44(matrix0, matrix1);
}

std::vector<std::unique_ptr<TransformOperations>> GetIdentityOperations(
    lynx::tasm::ElementManager* manager) {
  auto element = manager->CreateNode("view", nullptr);
  std::vector<std::unique_ptr<TransformOperations>> operations;
  std::unique_ptr<TransformOperations> to_add(
      std::make_unique<TransformOperations>(element.get()));
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  auto zero_unit_nlength = starlight::NLength::MakeUnitNLength(0.0f);
  to_add->AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  to_add->AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendScale(1, 1);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendScale(1, 1);
  to_add->AppendScale(1, 1);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendSkew(0, 0);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendSkew(0, 0);
  to_add->AppendSkew(0, 0);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendRotate(TransformOperation::Type::kRotateX, 0);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendRotate(TransformOperation::Type::kRotateY, 0);
  operations.push_back(std::move(to_add));

  to_add = std::make_unique<TransformOperations>(element.get());
  to_add->AppendRotate(TransformOperation::Type::kRotateZ, 0);
  operations.push_back(std::move(to_add));

  return operations;
}

}  // namespace

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
class TransformOperationsTest : public ::testing::Test {
 public:
  TransformOperationsTest() {}
  ~TransformOperationsTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator;
  fml::RefPtr<lynx::tasm::Element> element;

  void SetUp() override {
    tasm::LynxEnvConfig lynx_env_config(kWidth, kHeight,
                                        kDefaultLayoutsUnitPerPx,
                                        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<tasm::MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<tasm::PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
    element = manager->CreateNode("view", nullptr);
  }
};

TEST_F(TransformOperationsTest, MatchingPrefixSameLength) {
  TransformOperations translates(element.get());
  auto zero_unit_nlength = starlight::NLength::MakeUnitNLength(0.0f);
  auto one_unit_nlength = starlight::NLength::MakeUnitNLength(1.0f);
  auto two_unit_nlength = starlight::NLength::MakeUnitNLength(2.0f);
  translates.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);

  TransformOperations skews(element.get());
  skews.AppendSkew(0, 2);
  skews.AppendSkew(0, 2);
  skews.AppendSkew(0, 2);

  TransformOperations translates2(element.get());
  translates2.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      two_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates2.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      two_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates2.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      two_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);

  TransformOperations mixed(element.get());
  mixed.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      two_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  mixed.AppendScale(2, 1);
  mixed.AppendSkew(0, 2);

  TransformOperations translates3 = translates2;

  EXPECT_EQ(0UL, translates.MatchingPrefixLength(skews));
  EXPECT_EQ(3UL, translates.MatchingPrefixLength(translates2));
  EXPECT_EQ(3UL, translates.MatchingPrefixLength(translates3));
  EXPECT_EQ(1UL, translates.MatchingPrefixLength(mixed));
}

TEST_F(TransformOperationsTest, MatchingPrefixDifferentLength) {
  auto zero_unit_nlength = starlight::NLength::MakeUnitNLength(0.0f);
  auto one_unit_nlength = starlight::NLength::MakeUnitNLength(1.0f);
  auto two_unit_nlength = starlight::NLength::MakeUnitNLength(2.0f);
  TransformOperations translates(element.get());
  translates.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);

  TransformOperations skews(element.get());
  skews.AppendSkew(2, 0);
  skews.AppendSkew(2, 0);

  TransformOperations translates2(element.get());
  translates2.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      two_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  translates2.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      two_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);

  TransformOperations none(element.get());

  EXPECT_EQ(0UL, translates.MatchingPrefixLength(skews));
  // Pad the length of the shorter list provided all previous operation-
  // pairs match per spec
  // (https://drafts.csswg.org/css-transforms/#interpolation-of-transforms).
  EXPECT_EQ(3UL, translates.MatchingPrefixLength(translates2));
  EXPECT_EQ(3UL, translates.MatchingPrefixLength(none));
}

TEST_F(TransformOperationsTest, MatchingPrefixLengthOrder) {
  auto zero_unit_nlength = starlight::NLength::MakeUnitNLength(0.0f);
  auto one_unit_nlength = starlight::NLength::MakeUnitNLength(1.0f);
  TransformOperations mix_order_identity(element.get());
  mix_order_identity.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  mix_order_identity.AppendScale(1, 1);
  mix_order_identity.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);

  TransformOperations mix_order_one(element.get());
  mix_order_one.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  mix_order_one.AppendScale(2, 1);
  mix_order_one.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);

  TransformOperations mix_order_two(element.get());
  mix_order_two.AppendTranslate(
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  mix_order_two.AppendTranslate(
      one_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit,
      zero_unit_nlength, TransformOperation::LengthType::kLengthUnit);
  mix_order_two.AppendScale(2, 1);

  EXPECT_EQ(3UL, mix_order_identity.MatchingPrefixLength(mix_order_one));
  EXPECT_EQ(1UL, mix_order_identity.MatchingPrefixLength(mix_order_two));
  EXPECT_EQ(1UL, mix_order_one.MatchingPrefixLength(mix_order_two));
}

TEST_F(TransformOperationsTest, NoneAlwaysMatches) {
  std::vector<std::unique_ptr<TransformOperations>> operations =
      GetIdentityOperations(manager.get());

  TransformOperations none_operation(element.get());
  for (const auto& operation : operations) {
    EXPECT_EQ(operation->size(),
              operation->MatchingPrefixLength(none_operation));
  }
}

TEST_F(TransformOperationsTest, ApplyOrder) {
  float sx = 2;
  float sy = 4;
  float sz = 1;

  float dx = 1;
  float dy = 2;
  float dz = 3;

  TransformOperations operations(element.get());
  operations.AppendScale(sx, sy);
  operations.AppendTranslate(starlight::NLength::MakeUnitNLength(dx),
                             TransformOperation::LengthType::kLengthUnit,
                             starlight::NLength::MakeUnitNLength(dy),
                             TransformOperation::LengthType::kLengthUnit,
                             starlight::NLength::MakeUnitNLength(dz),
                             TransformOperation::LengthType::kLengthUnit);

  Matrix44 expected_scale_matrix;
  expected_scale_matrix.preScale(sx, sy, sz);

  Matrix44 expected_translate_matrix;
  expected_translate_matrix.preTranslate(dx, dy, dz);

  Matrix44 expected_combined_matrix = expected_scale_matrix;
  expected_combined_matrix.preConcat(expected_translate_matrix);

  CompareMatrix44(expected_combined_matrix, operations.ApplyRemaining(0));
}

TEST_F(TransformOperationsTest, BlendProgress) {
  float sx = 2;
  float sy = 4;
  float sz = 1;
  TransformOperations operations_from(element.get());
  operations_from.AppendScale(sx, sy);

  Matrix44 matrix_from;
  matrix_from.preScale(sx, sy, sz);

  sx = 4;
  sy = 8;
  sz = 1;
  TransformOperations operations_to(element.get());
  operations_to.AppendScale(sx, sy);

  Matrix44 matrix_to;
  matrix_to.preScale(sx, sy, sz);

  CheckProgress(-1, matrix_from, matrix_to, operations_from, operations_to);
  CheckProgress(0, matrix_from, matrix_to, operations_from, operations_to);
  CheckProgress(0.25f, matrix_from, matrix_to, operations_from, operations_to);
  CheckProgress(0.5f, matrix_from, matrix_to, operations_from, operations_to);
  CheckProgress(1, matrix_from, matrix_to, operations_from, operations_to);
  CheckProgress(2, matrix_from, matrix_to, operations_from, operations_to);
}

TEST_F(TransformOperationsTest, BlendWhenTypesDoNotMatch) {
  float sx1 = 2;
  float sy1 = 4;
  float sz1 = 1;

  starlight::NLength dx1 = starlight::NLength::MakeUnitNLength(1.0f);
  starlight::NLength dy1 = starlight::NLength::MakeUnitNLength(2.0f);
  starlight::NLength dz1 = starlight::NLength::MakeUnitNLength(3.0f);

  float sx2 = 4;
  float sy2 = 8;
  float sz2 = 1;

  starlight::NLength dx2 = starlight::NLength::MakeUnitNLength(10.0f);
  starlight::NLength dy2 = starlight::NLength::MakeUnitNLength(20.0f);
  starlight::NLength dz2 = starlight::NLength::MakeUnitNLength(30.0f);

  TransformOperations operations_from(element.get());
  operations_from.AppendScale(sx1, sy1);
  operations_from.AppendTranslate(
      dx1, TransformOperation::LengthType::kLengthUnit, dy1,
      TransformOperation::LengthType::kLengthUnit, dz1,
      TransformOperation::LengthType::kLengthUnit);

  TransformOperations operations_to(element.get());
  operations_to.AppendTranslate(
      dx2, TransformOperation::LengthType::kLengthUnit, dy2,
      TransformOperation::LengthType::kLengthUnit, dz2,
      TransformOperation::LengthType::kLengthUnit);
  operations_to.AppendScale(sx2, sy2);

  Matrix44 from;
  from.preScale(sx1, sy1, sz1);
  from.preTranslate(dx1.GetRawValue(), dy1.GetRawValue(), dz1.GetRawValue());

  Matrix44 to;
  to.preTranslate(dx2.GetRawValue(), dy2.GetRawValue(), dz2.GetRawValue());
  to.preScale(sx2, sy2, sz2);

  float progress = 0.25f;
  CheckProgress(progress, from, to, operations_from, operations_to);
}

TEST_F(TransformOperationsTest,
       ConstructWithCSSValueRawDataAndToTransformRawValue) {
  auto id = tasm::CSSPropertyID::kPropertyIDTransform;
  tasm::StyleMap output;
  tasm::CSSParserConfigs configs;
  auto impl =
      lepus::Value("translate3D(1rem, 2rem, 3rem) scale(0.1) rotate(10deg)");
  bool ret = tasm::UnitHandler::Process(id, impl, output, configs);

  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto raw_value = output[id];

  {
    TransformOperations operations(element.get(), raw_value);
    EXPECT_EQ(operations.size(), static_cast<size_t>(3));
    auto result_value = operations.ToTransformRawValue();
    EXPECT_TRUE(result_value.IsArray());
    auto arr = result_value.GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(3));
    auto item = arr->get(0).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 14);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(3).Number(), 28);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(5).Number(), 42);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);

    item = arr->get(1).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 0.1);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 0.1);

    item = arr->get(2).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(2));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateZ);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 10);
  }

  impl = lepus::Value("scaleY(0.1) translate(100%, 1rem)");
  ret = tasm::UnitHandler::Process(id, impl, output, configs);

  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  raw_value = output[id];
  {
    TransformOperations operations(element.get(), raw_value);
    EXPECT_EQ(operations.size(), static_cast<size_t>(2));
    auto result_value = operations.ToTransformRawValue();
    EXPECT_TRUE(result_value.IsArray());
    auto arr = result_value.GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(2));
    auto item = arr->get(0).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 1);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 0.1);

    item = arr->get(1).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 100);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::PERCENT);
    EXPECT_EQ(item->get(3).Number(), 14);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(5).Number(), 0);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);
  }

  impl = lepus::Value("rotateY(36deg) translateY(50%)");
  ret = tasm::UnitHandler::Process(id, impl, output, configs);

  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  raw_value = output[id];
  {
    TransformOperations operations(element.get(), raw_value);
    EXPECT_EQ(operations.size(), static_cast<size_t>(2));
    auto result_value = operations.ToTransformRawValue();
    EXPECT_TRUE(result_value.IsArray());
    auto arr = result_value.GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(2));
    auto item = arr->get(0).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(2));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 36);

    item = arr->get(1).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 0);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(3).Number(), 50);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::PERCENT);
    EXPECT_EQ(item->get(5).Number(), 0);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);
  }

  impl = lepus::Value("aaa hahaha");
  output.clear();
  ret = tasm::UnitHandler::Process(id, impl, output, configs);

  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
  raw_value = tasm::CSSValue::Empty();
  {
    TransformOperations operations(element.get(), raw_value);
    EXPECT_EQ(operations.size(), static_cast<size_t>(0));
    auto result_value = operations.ToTransformRawValue();
    EXPECT_TRUE(result_value.IsArray());
    auto arr = result_value.GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(0));
  }

  impl = lepus::Value(
      "translateX(50%) translateY(1rem) translateZ(2rem) rotateX(36deg) "
      "rotateY(36deg) rotateZ(36deg) scaleX(0.5) scaleY(2) skewX(36deg) "
      "skewY(36deg) translate(3rem, 25%) skew(18deg, 72deg)");
  ret = tasm::UnitHandler::Process(id, impl, output, configs);

  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  raw_value = output[id];
  {
    TransformOperations operations(element.get(), raw_value);
    EXPECT_EQ(operations.size(), static_cast<size_t>(12));
    auto result_value = operations.ToTransformRawValue();
    EXPECT_TRUE(result_value.IsArray());
    auto arr = result_value.GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(12));

    auto item = arr->get(0).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 50);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::PERCENT);
    EXPECT_EQ(item->get(3).Number(), 0);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(5).Number(), 0);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);

    item = arr->get(1).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 0);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(3).Number(), 14);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(5).Number(), 0);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);

    item = arr->get(2).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 0);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(3).Number(), 0);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(5).Number(), 28);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);

    item = arr->get(3).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(2));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateX);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 36);

    item = arr->get(4).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(2));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 36);

    item = arr->get(5).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(2));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateZ);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 36);

    item = arr->get(6).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 0.5);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 1);

    item = arr->get(7).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 1);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 2);

    item = arr->get(8).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kSkew);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 36);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 0);

    item = arr->get(9).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kSkew);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 0);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 36);

    item = arr->get(10).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(7));
    EXPECT_EQ(item->get(0).Number(),
              (int)starlight::TransformType::kTranslate3d);
    EXPECT_EQ(item->get(1).Number(), 42);
    EXPECT_EQ(item->get(2).Number(), (int)tasm::CSSValuePattern::NUMBER);
    EXPECT_EQ(item->get(3).Number(), 25);
    EXPECT_EQ(item->get(4).Number(), (int)tasm::CSSValuePattern::PERCENT);
    EXPECT_EQ(item->get(5).Number(), 0);
    EXPECT_EQ(item->get(6).Number(), (int)tasm::CSSValuePattern::NUMBER);

    item = arr->get(11).Array();
    EXPECT_EQ(item->size(), static_cast<size_t>(3));
    EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kSkew);
    EXPECT_FLOAT_EQ(item->get(1).Number(), 18);
    EXPECT_FLOAT_EQ(item->get(2).Number(), 72);
  }
}

}  // namespace testing
}  // namespace transforms
}  // namespace lynx
