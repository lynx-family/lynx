// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transform_animation_curve.h"

#include <memory>

#include "base/include/log/logging.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/transforms/transform_operations.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace animation {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class MockKeyframedLayoutAnimationCurve : public LayoutAnimationCurve {
 public:
  MockKeyframedLayoutAnimationCurve() = default;
  ~MockKeyframedLayoutAnimationCurve() override = default;

  static std::unique_ptr<MockKeyframedLayoutAnimationCurve> Create() {
    return std::make_unique<MockKeyframedLayoutAnimationCurve>();
  }

  lynx::tasm::CSSValue GetValue(fml::TimeDelta& t) const override {
    return lynx::tasm::CSSValue::Empty();
  };

  std::vector<std::unique_ptr<Keyframe>> GetKeyframes() {
    return std::move(keyframes_);
  }
};

class MockKeyframedTransformAnimationCurve : public TransformAnimationCurve {
 public:
  MockKeyframedTransformAnimationCurve() = default;
  ~MockKeyframedTransformAnimationCurve() override = default;

  static std::unique_ptr<MockKeyframedTransformAnimationCurve> Create() {
    return std::make_unique<MockKeyframedTransformAnimationCurve>();
  }

  lynx::tasm::CSSValue GetValue(fml::TimeDelta& t) const override {
    return lynx::tasm::CSSValue::Empty();
  }

  std::vector<std::unique_ptr<Keyframe>> GetKeyframes() {
    return std::move(keyframes_);
  }
};

class TransformAnimationCurveTest : public ::testing::Test {
 public:
  TransformAnimationCurveTest() {}
  ~TransformAnimationCurveTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator;

  void SetUp() override {
    ::lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<::lynx::tasm::MockPaintingContext>(),
        tasm_mediator.get(), lynx_env_config);
    auto config = std::make_shared<::lynx::tasm::PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }

  fml::RefPtr<lynx::tasm::Element> InitTestElement() {
    return manager->CreateNode("view", nullptr);
  }
};

TEST_F(TransformAnimationCurveTest, TransformedAnimationTime) {
  std::unique_ptr<MockKeyframedLayoutAnimationCurve> curve(
      MockKeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_frame1 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  curve->AddKeyframe(std::move(test_frame2));

  auto test_keyframes = curve->GetKeyframes();
  auto test_scaled_duration = curve->scaled_duration();
  fml::TimeDelta test_time1 = fml::TimeDelta::FromSecondsF(0.5f);
  auto transformed_time1 = TransformedAnimationTime(
      std::move(test_keyframes), nullptr, test_scaled_duration, test_time1);
  EXPECT_EQ(test_time1, transformed_time1);

  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func = starlight::TimingFunctionType::kEaseIn;
  auto test_timing_function =
      TimingFunction::MakeTimingFunction(timing_function_data);
  auto transformed_time2 =
      TransformedAnimationTime(std::move(test_keyframes), test_timing_function,
                               test_scaled_duration, test_time1);
  EXPECT_EQ(int64_t(315356734), transformed_time2.ToNanoseconds());
}

TEST_F(TransformAnimationCurveTest, TransformedKeyframeProgress1) {
  std::unique_ptr<MockKeyframedLayoutAnimationCurve> curve(
      MockKeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_frame1 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  curve->AddKeyframe(std::move(test_frame2));

  auto test_keyframes = curve->GetKeyframes();
  auto test_scaled_duration = curve->scaled_duration();
  fml::TimeDelta test_time1 = fml::TimeDelta::FromSecondsF(0.5f);
  auto progress1 = TransformedKeyframeProgress(
      std::move(test_keyframes), test_scaled_duration, test_time1, 0);
  EXPECT_EQ(progress1, 0.5);
}

TEST_F(TransformAnimationCurveTest, TransformedKeyframeProgress2) {
  std::unique_ptr<MockKeyframedLayoutAnimationCurve> curve(
      MockKeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;

  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func = starlight::TimingFunctionType::kEaseIn;
  auto test_timing_function =
      TimingFunction::MakeTimingFunction(timing_function_data);

  auto test_frame1 =
      LayoutKeyframe::Create(fml::TimeDelta(), std::move(test_timing_function));
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  curve->AddKeyframe(std::move(test_frame2));

  auto test_keyframes = curve->GetKeyframes();
  auto test_scaled_duration = curve->scaled_duration();
  fml::TimeDelta test_time1 = fml::TimeDelta::FromSecondsF(0.5f);
  auto progress1 = TransformedKeyframeProgress(
      std::move(test_keyframes), test_scaled_duration, test_time1, 0);
  EXPECT_EQ(progress1, 0.31535673426536154);
}

TEST_F(TransformAnimationCurveTest, GetStyleInElement) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  std::unique_ptr<KeyframedOpacityAnimationCurve> curve(
      KeyframedOpacityAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame1 = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  auto raw_value = lynx::tasm::CSSValue(lepus_value(1.0f),
                                        lynx::tasm::CSSValuePattern::NUMBER);
  auto y_id = lynx::tasm::CSSPropertyID::kPropertyIDOpacity;
  auto value_pair1 = std::make_pair(y_id, raw_value);
  bool set_success1 = test_frame1->SetValue(value_pair1, test_element);
  EXPECT_EQ(set_success1, true);
  EXPECT_EQ(test_frame1->IsEmpty(), false);
  auto w_id = lynx::tasm::CSSPropertyID::kPropertyIDTop;
  auto result_value2 = GetStyleInElement(w_id, test_element);
  EXPECT_EQ(result_value2, lynx::tasm::CSSValue::Empty());
}

TEST_F(TransformAnimationCurveTest, GetActiveKeyframe) {
  //  std::unique_ptr<KeyframedTransformAnimationCurve> curve(
  //      KeyframedTransformAnimationCurve::Create());
  std::unique_ptr<MockKeyframedTransformAnimationCurve> curve(
      MockKeyframedTransformAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::TRANSFORM;

  auto test_frame1 = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  curve->AddKeyframe(std::move(test_frame3));

  auto keyframes = curve->GetKeyframes();
  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  EXPECT_EQ(0, GetActiveKeyframe(std::move(keyframes), curve->scaled_duration(),
                                 value1));
  EXPECT_EQ(0, GetActiveKeyframe(std::move(keyframes), curve->scaled_duration(),
                                 value2));
  EXPECT_EQ(1, GetActiveKeyframe(std::move(keyframes), curve->scaled_duration(),
                                 value3));
  EXPECT_EQ(1, GetActiveKeyframe(std::move(keyframes), curve->scaled_duration(),
                                 value4));
}

TEST_F(TransformAnimationCurveTest, CreateTransformKeyframe) {
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_EQ(test_frame->timing_function(), nullptr);
  EXPECT_EQ(test_frame->Time(), fml::TimeDelta());
}

TEST_F(TransformAnimationCurveTest, GetTransformKeyframeValueInElement) {
  auto element1 = manager->CreateNode("view1", nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::StyleMap output1;
  lynx::tasm::CSSParserConfigs configs;
  auto impl1 =
      lepus::Value("translate3D(1rem, 2rem, 3rem) scale(0.1) rotate(10deg)");
  bool ret1 = lynx::tasm::UnitHandler::Process(id, impl1, output1, configs);
  EXPECT_TRUE(ret1);
  EXPECT_FALSE(output1.empty());
  auto raw_value = output1[id];
  element1->ConsumeStyle(output1, nullptr);
  auto transform_in_element_1 =
      TransformKeyframe::GetTransformKeyframeValueInElement(element1.get());
  transforms::TransformOperations operations(element1.get(), raw_value);
  EXPECT_EQ(operations.size(), static_cast<size_t>(3));
  EXPECT_EQ(transform_in_element_1.size(), static_cast<size_t>(0));
}

TEST_F(TransformAnimationCurveTest, SetValue) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::StyleMap output;
  lynx::tasm::CSSParserConfigs configs;
  auto impl1 = lepus::Value("");
  lynx::tasm::UnitHandler::Process(id, impl1, output, configs);
  EXPECT_FALSE(output[id].IsArray());
  auto raw_value1 = output[id];
  auto test_value_pair1 = std::make_pair(id, raw_value1);
  bool set_success1 = test_frame->SetValue(test_value_pair1, test_element);
  EXPECT_EQ(set_success1, false);
  EXPECT_EQ(test_frame->IsEmpty(), true);

  auto impl2 =
      lepus::Value("translate3D(1rem, 2rem, 3rem) scale(0.1) rotate(10deg)");
  bool ret2 = lynx::tasm::UnitHandler::Process(id, impl2, output, configs);
  EXPECT_TRUE(ret2);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto raw_value2 = output[id];
  auto test_value_pair2 = std::make_pair(id, raw_value2);
  bool set_success2 = test_frame->SetValue(test_value_pair2, test_element);
  EXPECT_EQ(set_success2, true);
  EXPECT_EQ(test_frame->IsEmpty(), false);
}

TEST_F(TransformAnimationCurveTest, CreateTransformAnimationCurve) {
  auto test_curve1 = KeyframedTransformAnimationCurve::Create();
  EXPECT_EQ(test_curve1->Type(), AnimationCurve::CurveType::UNSUPPORT);
  EXPECT_EQ(test_curve1->scaled_duration(), 1.0);
  EXPECT_EQ(test_curve1->timing_function(), nullptr);
  EXPECT_EQ(test_curve1->get_keyframes_size(), 0);
}

TEST_F(TransformAnimationCurveTest, GetValue) {
  std::unique_ptr<KeyframedTransformAnimationCurve> curve(
      KeyframedTransformAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::TRANSFORM;
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  curve->SetElement(test_element);

  auto test_frame1 = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;
  auto impl1 = lepus::Value("scale(0.1)");
  lynx::tasm::StyleMap output1;
  bool ret1 = lynx::tasm::UnitHandler::Process(id, impl1, output1, configs);
  EXPECT_TRUE(ret1);
  EXPECT_FALSE(output1.empty());
  EXPECT_FALSE(output1.find(id) == output1.end());
  EXPECT_TRUE(output1[id].IsArray());
  auto raw_value1 = output1[id];
  auto test_value_pair1 = std::make_pair(id, raw_value1);
  bool set_success1 = test_frame1->SetValue(test_value_pair1, test_element);
  EXPECT_EQ(set_success1, true);
  EXPECT_EQ(test_frame1->IsEmpty(), false);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  auto impl2 = lepus::Value("scale(1.1)");
  lynx::tasm::StyleMap output2;
  bool ret2 = lynx::tasm::UnitHandler::Process(id, impl2, output2, configs);
  EXPECT_TRUE(ret2);
  EXPECT_FALSE(output2.empty());
  EXPECT_FALSE(output2.find(id) == output2.end());
  EXPECT_TRUE(output2[id].IsArray());
  auto raw_value2 = output2[id];
  auto test_value_pair2 = std::make_pair(id, raw_value2);
  bool set_success2 = test_frame2->SetValue(test_value_pair2, test_element);
  EXPECT_EQ(set_success2, true);
  EXPECT_EQ(test_frame2->IsEmpty(), false);
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta test_time1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta test_time2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta test_time3 = fml::TimeDelta::FromSecondsF(1.f);
  auto test_value1 =
      curve->GetValue(test_time1).GetValue().Array()->get(0).Array();
  EXPECT_EQ(test_value1->get(0).Number(),
            (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(test_value1->get(1).Number(), 0.1);

  auto test_value2 =
      curve->GetValue(test_time2).GetValue().Array()->get(0).Array();
  EXPECT_EQ(test_value2->get(0).Number(),
            (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(test_value2->get(1).Number(), 0.6);

  auto test_value3 =
      curve->GetValue(test_time3).GetValue().Array()->get(0).Array();
  EXPECT_EQ(test_value3->get(0).Number(),
            (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(test_value3->get(1).Number(), 1.1);
}

TEST_F(TransformAnimationCurveTest, MakeEmptyKeyframe) {
  auto test_curve = KeyframedTransformAnimationCurve::Create();
  auto test_frame1 =
      TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  auto test_frame2 =
      test_curve->KeyframedTransformAnimationCurve::MakeEmptyKeyframe(
          fml::TimeDelta::FromSecondsF(2.0));
  EXPECT_EQ(test_frame1->Time(), test_frame2->Time());
  EXPECT_EQ(test_frame1->timing_function(), test_frame2->timing_function());
}

}  // namespace testing
}  // namespace tasm
}  // namespace animation
}  // namespace lynx
