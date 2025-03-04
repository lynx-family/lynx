// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transforms/transform_operation.h"

#include <cmath>

#include "core/animation/transforms/matrix44.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
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

void CompareTransformOperation(const TransformOperation& a,
                               const TransformOperation& b) {
  EXPECT_EQ(a.type, b.type);
  switch (a.type) {
    case TransformOperation::Type::kTranslate: {
      EXPECT_EQ(a.translate.type.x, b.translate.type.x);
      EXPECT_EQ(a.translate.type.y, b.translate.type.y);
      EXPECT_EQ(a.translate.type.z, b.translate.type.z);
      EXPECT_FLOAT_EQ(a.translate.value.x.GetRawValue(),
                      b.translate.value.x.GetRawValue());
      EXPECT_FLOAT_EQ(a.translate.value.y.GetRawValue(),
                      b.translate.value.y.GetRawValue());
      EXPECT_FLOAT_EQ(a.translate.value.z.GetRawValue(),
                      b.translate.value.z.GetRawValue());
      break;
    }
    case TransformOperation::Type::kRotateX:
    case TransformOperation::Type::kRotateY:
    case TransformOperation::Type::kRotateZ: {
      EXPECT_FLOAT_EQ(a.rotate.degree, b.rotate.degree);
      break;
    }
    case TransformOperation::Type::kScale: {
      EXPECT_FLOAT_EQ(a.scale.x, b.scale.x);
      EXPECT_FLOAT_EQ(a.scale.y, b.scale.y);
      break;
    }
    case TransformOperation::Type::kSkew: {
      EXPECT_FLOAT_EQ(a.skew.x, b.skew.x);
      EXPECT_FLOAT_EQ(a.skew.y, b.skew.y);
      break;
    }
    default: {
      break;
    }
  }
}

}  // namespace

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class TransformOperationTest : public ::testing::Test {
 public:
  TransformOperationTest() {}
  ~TransformOperationTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator;

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
  }
};

TEST_F(TransformOperationTest, GetMatrix) {
  auto element = manager->CreateNode("view", nullptr);
  std::array<float, 4> arr = {0, 0, 0, 0};
  element->UpdateLayout(0.0, 0.0, 100.0, 200.0, arr, arr, arr, &arr, 0);

  TransformOperation translate_op;
  translate_op.type = TransformOperation::Type::kTranslate;
  translate_op.translate.type.x =
      TransformOperation::LengthType::kLengthPercentage;
  translate_op.translate.type.y =
      TransformOperation::LengthType::kLengthPercentage;
  translate_op.translate.type.z = TransformOperation::LengthType::kLengthUnit;
  translate_op.translate.value.x =
      starlight::NLength::MakePercentageNLength(20.0f);
  translate_op.translate.value.y =
      starlight::NLength::MakePercentageNLength(50.0f);
  translate_op.translate.value.z = starlight::NLength::MakeUnitNLength(100.0f);

  Matrix44 m0 = translate_op.GetMatrix(element.get());
  Matrix44 expected0;
  expected0.setRC(0, 3, 20);
  expected0.setRC(1, 3, 100);
  expected0.setRC(2, 3, 100);
  CompareMatrix44(expected0, m0);
}

TEST_F(TransformOperationTest, BlendTransformOperations) {
  auto element = manager->CreateNode("view", nullptr);
  std::array<float, 4> arr = {0, 0, 0, 0};
  element->UpdateLayout(0.0, 0.0, 100.0, 200.0, arr, arr, arr, &arr, 0);

  {
    TransformOperation transform0;
    transform0.type = TransformOperation::Type::kTranslate;
    transform0.translate.type.x =
        TransformOperation::LengthType::kLengthPercentage;
    transform0.translate.type.y =
        TransformOperation::LengthType::kLengthPercentage;
    transform0.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    transform0.translate.value.x =
        starlight::NLength::MakePercentageNLength(20.0f);
    transform0.translate.value.y =
        starlight::NLength::MakePercentageNLength(50.0f);
    transform0.translate.value.z = starlight::NLength::MakeUnitNLength(100.0f);

    TransformOperation transform1;
    transform1.type = TransformOperation::Type::kTranslate;
    transform1.translate.type.x =
        TransformOperation::LengthType::kLengthPercentage;
    transform1.translate.type.y =
        TransformOperation::LengthType::kLengthPercentage;
    transform1.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    transform1.translate.value.x =
        starlight::NLength::MakePercentageNLength(30.0f);
    transform1.translate.value.y =
        starlight::NLength::MakePercentageNLength(70.0f);
    transform1.translate.value.z = starlight::NLength::MakeUnitNLength(20.0f);

    TransformOperation expected;
    expected.type = TransformOperation::Type::kTranslate;
    expected.translate.type.x =
        TransformOperation::LengthType::kLengthPercentage;
    expected.translate.type.y =
        TransformOperation::LengthType::kLengthPercentage;
    expected.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    expected.translate.value.x =
        starlight::NLength::MakePercentageNLength(22.0f);
    expected.translate.value.y =
        starlight::NLength::MakePercentageNLength(54.0f);
    expected.translate.value.z = starlight::NLength::MakeUnitNLength(84.0f);

    TransformOperation result = TransformOperation::BlendTransformOperations(
        &transform0, &transform1, 0.2, element.get());
    CompareTransformOperation(expected, result);
  }

  {
    TransformOperation transform0;
    transform0.type = TransformOperation::Type::kTranslate;
    transform0.translate.type.x = TransformOperation::LengthType::kLengthUnit;
    transform0.translate.type.y =
        TransformOperation::LengthType::kLengthPercentage;
    transform0.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    transform0.translate.value.x = starlight::NLength::MakeUnitNLength(20.0f);
    transform0.translate.value.y =
        starlight::NLength::MakePercentageNLength(50.0f);
    transform0.translate.value.z = starlight::NLength::MakeUnitNLength(100.0f);

    TransformOperation transform1;
    transform1.type = TransformOperation::Type::kTranslate;
    transform1.translate.type.x = TransformOperation::LengthType::kLengthUnit;
    transform1.translate.type.y = TransformOperation::LengthType::kLengthUnit;
    transform1.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    transform1.translate.value.x = starlight::NLength::MakeUnitNLength(30.0f);
    transform1.translate.value.y = starlight::NLength::MakeUnitNLength(70.0f);
    transform1.translate.value.z = starlight::NLength::MakeUnitNLength(20.0f);

    TransformOperation expected;
    expected.type = TransformOperation::Type::kTranslate;
    expected.translate.type.x = TransformOperation::LengthType::kLengthUnit;
    expected.translate.type.y = TransformOperation::LengthType::kLengthUnit;
    expected.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    expected.translate.value.x = starlight::NLength::MakeUnitNLength(24.0f);
    expected.translate.value.y = starlight::NLength::MakeUnitNLength(88.0f);
    expected.translate.value.z = starlight::NLength::MakeUnitNLength(68.0f);

    TransformOperation result = TransformOperation::BlendTransformOperations(
        &transform0, &transform1, 0.4, element.get());
    CompareTransformOperation(expected, result);
  }

  {
    TransformOperation transform0;
    transform0.type = TransformOperation::Type::kTranslate;
    transform0.translate.type.x = TransformOperation::LengthType::kLengthUnit;
    transform0.translate.type.y =
        TransformOperation::LengthType::kLengthPercentage;
    transform0.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    transform0.translate.value.x = starlight::NLength::MakeUnitNLength(20.0f);
    transform0.translate.value.y =
        starlight::NLength::MakePercentageNLength(50.0f);
    transform0.translate.value.z = starlight::NLength::MakeUnitNLength(100.0f);

    TransformOperation transform1;

    TransformOperation expected;
    expected.type = TransformOperation::Type::kTranslate;
    expected.translate.type.x = TransformOperation::LengthType::kLengthUnit;
    expected.translate.type.y =
        TransformOperation::LengthType::kLengthPercentage;
    expected.translate.type.z = TransformOperation::LengthType::kLengthUnit;
    expected.translate.value.x = starlight::NLength::MakeUnitNLength(8.0f);
    expected.translate.value.y =
        starlight::NLength::MakePercentageNLength(20.0f);
    expected.translate.value.z = starlight::NLength::MakeUnitNLength(40.0f);

    TransformOperation result = TransformOperation::BlendTransformOperations(
        &transform0, &transform1, 0.6, element.get());
    CompareTransformOperation(expected, result);
  }
  {
    TransformOperation transform0;
    transform0.type = TransformOperation::Type::kScale;
    transform0.scale.x = 5;
    transform0.scale.y = 0.5;

    TransformOperation transform1;

    TransformOperation expected;
    expected.type = TransformOperation::Type::kScale;
    expected.scale.x = 1.8;
    expected.scale.y = 0.9;

    TransformOperation result = TransformOperation::BlendTransformOperations(
        &transform0, &transform1, 0.8, element.get());
    CompareTransformOperation(expected, result);
  }
}

}  // namespace testing
}  // namespace transforms
}  // namespace lynx
