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
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "gfx/animation/animation_utils.h"
#include "gfx/animation/timing_function.h"
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
    return lynx::tasm::CSSValue();
  };

  std::vector<std::unique_ptr<gfx::Keyframe>> GetKeyframes() {
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
    return lynx::tasm::CSSValue();
  }

  std::vector<std::unique_ptr<gfx::Keyframe>> GetKeyframes() {
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

  void ResetManager(float layouts_unit_per_px) {
    ::lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, layouts_unit_per_px,
        kDefaultPhysicalPixelsPerLayoutUnit);
    if (!tasm_mediator) {
      tasm_mediator = std::make_shared<
          ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    }
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<::lynx::tasm::MockPaintingContext>(),
        tasm_mediator.get(), lynx_env_config);
    auto config = std::make_shared<::lynx::tasm::PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }

  void SetUp() override { ResetManager(kDefaultLayoutsUnitPerPx); }

  fml::RefPtr<lynx::tasm::Element> InitTestElement() {
    return manager->CreateFiberElement("view");
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
  auto sampling1 = gfx::ComputeKeyframedProgress(
      test_keyframes, nullptr, test_scaled_duration, test_time1);
  EXPECT_TRUE(sampling1.valid);
  EXPECT_EQ(test_time1, sampling1.effective_time);

  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func = starlight::TimingFunctionType::kEaseIn;
  auto test_timing_function = gfx::CubicBezierTimingFunction::CreatePreset(
      gfx::CubicBezierTimingFunction::EaseType::EASE_IN);
  auto sampling2 =
      gfx::ComputeKeyframedProgress(test_keyframes, test_timing_function.get(),
                                    test_scaled_duration, test_time1);
  EXPECT_TRUE(sampling2.valid);
  EXPECT_EQ(int64_t(315356734), sampling2.effective_time.ToNanoseconds());
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
  auto sampling = gfx::ComputeKeyframedProgress(
      test_keyframes, nullptr, test_scaled_duration, test_time1);
  EXPECT_TRUE(sampling.valid);
  EXPECT_EQ(sampling.progress, 0.5);
}

TEST_F(TransformAnimationCurveTest, TransformedKeyframeProgress2) {
  std::unique_ptr<MockKeyframedLayoutAnimationCurve> curve(
      MockKeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;

  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func = starlight::TimingFunctionType::kEaseIn;
  auto test_timing_function = gfx::CubicBezierTimingFunction::CreatePreset(
      gfx::CubicBezierTimingFunction::EaseType::EASE_IN);

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
  auto sampling = gfx::ComputeKeyframedProgress(
      test_keyframes, nullptr, test_scaled_duration, test_time1);
  EXPECT_TRUE(sampling.valid);
  EXPECT_EQ(sampling.progress, 0.31535673426536154);
}

TEST_F(TransformAnimationCurveTest, GetStyleInElement) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  std::unique_ptr<KeyframedOpacityAnimationCurve> curve(
      KeyframedOpacityAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame1 = gfx::FloatKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetValue(1.0f);
  EXPECT_EQ(test_frame1->IsEmpty(), false);
  auto w_id = lynx::tasm::CSSPropertyID::kPropertyIDTop;
  auto result_value2 = GetStyleInElement(w_id, test_element);
  EXPECT_EQ(result_value2, lynx::tasm::CSSValue());
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
  auto s1 = gfx::ComputeKeyframedProgress(keyframes, nullptr,
                                          curve->scaled_duration(), value1);
  auto s2 = gfx::ComputeKeyframedProgress(keyframes, nullptr,
                                          curve->scaled_duration(), value2);
  auto s3 = gfx::ComputeKeyframedProgress(keyframes, nullptr,
                                          curve->scaled_duration(), value3);
  auto s4 = gfx::ComputeKeyframedProgress(keyframes, nullptr,
                                          curve->scaled_duration(), value4);
  EXPECT_TRUE(s1.valid);
  EXPECT_TRUE(s2.valid);
  EXPECT_TRUE(s3.valid);
  EXPECT_TRUE(s4.valid);
  EXPECT_EQ(0u, s1.index);
  EXPECT_EQ(0u, s2.index);
  EXPECT_EQ(1u, s3.index);
  EXPECT_EQ(1u, s4.index);
}

TEST_F(TransformAnimationCurveTest, CreateTransformKeyframe) {
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_EQ(test_frame->timing_function(), nullptr);
  EXPECT_EQ(test_frame->Time(), fml::TimeDelta());
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
  bool set_success1 = test_frame->SetValue(id, raw_value1, test_element);
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
  bool set_success2 = test_frame->SetValue(id, raw_value2, test_element);
  EXPECT_EQ(set_success2, true);
  EXPECT_EQ(test_frame->IsEmpty(), false);
}

TEST_F(TransformAnimationCurveTest, SetValueWithoutElementDefersResolve) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;

  lynx::tasm::StyleMap output1;
  auto impl1 = lepus::Value("scale(0.1)");
  EXPECT_TRUE(lynx::tasm::UnitHandler::Process(id, impl1, output1, configs));
  ASSERT_TRUE(output1[id].IsArray());
  EXPECT_TRUE(test_frame->SetValue(id, output1[id], test_element));
  EXPECT_TRUE(test_frame->HasResolvedValue());

  lynx::tasm::StyleMap output2;
  auto impl2 = lepus::Value("scale(1.1)");
  EXPECT_TRUE(lynx::tasm::UnitHandler::Process(id, impl2, output2, configs));
  ASSERT_TRUE(output2[id].IsArray());
  EXPECT_TRUE(test_frame->SetValue(id, output2[id], nullptr));
  EXPECT_FALSE(test_frame->IsEmpty());
  EXPECT_FALSE(test_frame->HasResolvedValue());

  EXPECT_TRUE(test_frame->EnsureResolvedValue(id, test_element));
  ASSERT_TRUE(test_frame->HasResolvedValue());
  EXPECT_EQ(test_frame->ResolvedValue().size(), static_cast<size_t>(1));
}

TEST_F(TransformAnimationCurveTest, ReportsDynamicDependenciesForRouting) {
  auto test_element_ptr = InitTestElement();
  auto* test_element = test_element_ptr.get();
  const auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;

  const auto has_dynamic_dependencies = [&](const char* transform) {
    lynx::tasm::StyleMap output;
    EXPECT_TRUE(lynx::tasm::UnitHandler::Process(id, lepus::Value(transform),
                                                 output, configs));
    auto keyframe = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
    EXPECT_TRUE(keyframe->SetValue(id, output[id], test_element));
    return keyframe->HasDynamicDependencies();
  };

  EXPECT_FALSE(has_dynamic_dependencies("translateX(100px) scale(2)"));
  EXPECT_FALSE(has_dynamic_dependencies("translateX(50%)"));
  EXPECT_TRUE(has_dynamic_dependencies("translateX(calc(50% + 10px))"));
  EXPECT_TRUE(has_dynamic_dependencies("translateX(1rem)"));
}

TEST_F(TransformAnimationCurveTest, NotificationsReResolveRawCSSValue) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  test_element->UpdateLayout(0, 0, 100, 100, {0}, {0}, {0}, nullptr, 0);
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;
  lynx::tasm::StyleMap output;

  EXPECT_TRUE(lynx::tasm::UnitHandler::Process(
      id, lepus::Value("translateX(calc(10px + 50%))"), output, configs));
  ASSERT_TRUE(output[id].IsArray());
  ASSERT_TRUE(test_frame->SetValue(id, output[id], test_element));
  ASSERT_TRUE(test_frame->HasResolvedValue());
  ASSERT_EQ(test_frame->ResolvedValue().size(), 1u);
  EXPECT_FLOAT_EQ(
      test_frame->ResolvedValue().GetOperations()[0].translate.x.Resolve(
          100.0f),
      60.0f);

  test_element->UpdateLayout(0, 0, 200, 100, {0}, {0}, {0}, nullptr, 0);
  test_frame->NotifyElementSizeUpdated();
  EXPECT_FALSE(test_frame->HasResolvedValue());
  EXPECT_TRUE(test_frame->EnsureResolvedValue(id, test_element));
  ASSERT_TRUE(test_frame->HasResolvedValue());
  EXPECT_FLOAT_EQ(
      test_frame->ResolvedValue().GetOperations()[0].translate.x.Resolve(
          200.0f),
      110.0f);

  test_frame->NotifyUnitValuesUpdated(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT));
  EXPECT_FALSE(test_frame->HasResolvedValue());
  EXPECT_TRUE(test_frame->EnsureResolvedValue(id, test_element));
  ASSERT_TRUE(test_frame->HasResolvedValue());
  EXPECT_FLOAT_EQ(
      test_frame->ResolvedValue().GetOperations()[0].translate.x.Resolve(
          200.0f),
      110.0f);
}

TEST_F(TransformAnimationCurveTest,
       NotificationsKeepSizeIndependentValuesResolved) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;
  lynx::tasm::StyleMap output;

  ASSERT_TRUE(lynx::tasm::UnitHandler::Process(
      id, lepus::Value("scale(2) translateX(50%)"), output, configs));
  ASSERT_TRUE(test_frame->SetValue(id, output[id], test_element));
  ASSERT_TRUE(test_frame->HasResolvedValue());

  test_frame->NotifyElementSizeUpdated();
  EXPECT_TRUE(test_frame->HasResolvedValue());
  test_frame->NotifyUnitValuesUpdated(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::REM));
  EXPECT_TRUE(test_frame->HasResolvedValue());
}

TEST_F(TransformAnimationCurveTest,
       UnitNotificationsInvalidateOnlyMatchingDependencies) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();
  auto test_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;
  lynx::tasm::StyleMap output;

  ASSERT_TRUE(lynx::tasm::UnitHandler::Process(
      id, lepus::Value("translateX(1rem)"), output, configs));
  ASSERT_TRUE(test_frame->SetValue(id, output[id], test_element));
  ASSERT_TRUE(test_frame->HasResolvedValue());

  test_frame->NotifyElementSizeUpdated();
  EXPECT_TRUE(test_frame->HasResolvedValue());
  test_frame->NotifyUnitValuesUpdated(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::EM));
  EXPECT_TRUE(test_frame->HasResolvedValue());
  test_frame->NotifyUnitValuesUpdated(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::REM));
  EXPECT_FALSE(test_frame->HasResolvedValue());
}

TEST_F(TransformAnimationCurveTest, CreateTransformAnimationCurve) {
  auto test_curve1 = KeyframedTransformAnimationCurve::Create();
  EXPECT_EQ(test_curve1->Type(), AnimationCurve::CurveType::UNSUPPORT);
  EXPECT_EQ(test_curve1->scaled_duration(), 1.0);
  EXPECT_EQ(test_curve1->timing_function(), nullptr);
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
  bool set_success1 = test_frame1->SetValue(id, raw_value1, test_element);
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
  bool set_success2 = test_frame2->SetValue(id, raw_value2, test_element);
  EXPECT_EQ(set_success2, true);
  EXPECT_EQ(test_frame2->IsEmpty(), false);
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta test_time1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta test_time2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta test_time3 = fml::TimeDelta::FromSecondsF(1.f);
  auto test_value1 = curve->GetValue(test_time1)
                         .GetValue()
                         .Array()
                         ->get(0)
                         .Array()
                         .strongify();
  EXPECT_EQ(test_value1->get(0).Number(),
            (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(test_value1->get(1).Number(), 0.1);

  auto test_value2 = curve->GetValue(test_time2)
                         .GetValue()
                         .Array()
                         ->get(0)
                         .Array()
                         .strongify();
  EXPECT_EQ(test_value2->get(0).Number(),
            (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(test_value2->get(1).Number(), 0.6);

  auto test_value3 = curve->GetValue(test_time3)
                         .GetValue()
                         .Array()
                         ->get(0)
                         .Array()
                         .strongify();
  EXPECT_EQ(test_value3->get(0).Number(),
            (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(test_value3->get(1).Number(), 1.1);
}

TEST_F(TransformAnimationCurveTest,
       ToOnlyTransformRepeatsFromStableUnderlyingStyle) {
  auto test_element_ptr = InitTestElement();
  auto* test_element = test_element_ptr.get();
  auto curve = KeyframedTransformAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::TRANSFORM;
  curve->SetElement(test_element);
  curve->SetUnderlyingValue(
      GetStyleInElement(lynx::tasm::kPropertyIDTransform, test_element));

  auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  auto start_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  curve->AddKeyframe(std::move(start_frame));

  lynx::tasm::StyleMap output;
  lynx::tasm::CSSParserConfigs configs;
  ASSERT_TRUE(lynx::tasm::UnitHandler::Process(
      id, lepus::Value("rotate(360deg)"), output, configs));
  auto end_frame =
      TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  ASSERT_TRUE(end_frame->SetValue(id, output[id], test_element));
  curve->AddKeyframe(std::move(end_frame));

  fml::TimeDelta first_iteration_time = fml::TimeDelta::FromSecondsF(0.5f);
  const auto first_iteration_value = curve->GetValue(first_iteration_time);

  // The legacy new-animator path writes the sampled value into the final
  // computed style. A missing keyframe must still read the underlying style on
  // the next iteration instead of treating this animated value as its new
  // starting point.
  ASSERT_TRUE(
      test_element->computed_css_style()->SetValue(id, first_iteration_value));
  fml::TimeDelta second_iteration_time = fml::TimeDelta::FromSecondsF(0.5f);
  EXPECT_EQ(first_iteration_value, curve->GetValue(second_iteration_time));
}

TEST_F(TransformAnimationCurveTest, InterpolatesNoneAsIdentity) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();

  auto curve = KeyframedTransformAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::TRANSFORM;
  curve->SetElement(test_element);

  const auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;

  lynx::tasm::StyleMap none_output;
  ASSERT_TRUE(lynx::tasm::UnitHandler::Process(id, lepus::Value("none"),
                                               none_output, configs));
  ASSERT_TRUE(none_output[id].IsArray());
  ASSERT_EQ(none_output[id].GetArray()->size(), 0u);
  auto none_frame = TransformKeyframe::Create(fml::TimeDelta(), nullptr);
  ASSERT_TRUE(none_frame->SetValue(id, none_output[id], test_element));
  curve->AddKeyframe(std::move(none_frame));

  lynx::tasm::StyleMap scale_output;
  ASSERT_TRUE(lynx::tasm::UnitHandler::Process(id, lepus::Value("scale(3)"),
                                               scale_output, configs));
  auto scale_frame =
      TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  ASSERT_TRUE(scale_frame->SetValue(id, scale_output[id], test_element));
  curve->AddKeyframe(std::move(scale_frame));

  auto midpoint_time = fml::TimeDelta::FromSecondsF(0.5);
  auto midpoint = curve->GetValue(midpoint_time)
                      .GetValue()
                      .Array()
                      ->get(0)
                      .Array()
                      .strongify();
  ASSERT_EQ(midpoint->size(), 3u);
  EXPECT_EQ(midpoint->get(0).Number(),
            static_cast<int>(starlight::TransformType::kScale));
  EXPECT_FLOAT_EQ(midpoint->get(1).Number(), 2.0);
  EXPECT_FLOAT_EQ(midpoint->get(2).Number(), 2.0);
}

TEST_F(TransformAnimationCurveTest,
       PreservesPercentageTranslateZStackingInterpolation) {
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();

  auto curve = KeyframedTransformAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::TRANSFORM;
  curve->SetElement(test_element);

  const auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;
  const auto append_keyframe = [&](double time, const char* transform) {
    lynx::tasm::StyleMap output;
    EXPECT_TRUE(lynx::tasm::UnitHandler::Process(id, lepus::Value(transform),
                                                 output, configs));
    auto keyframe =
        TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(time), nullptr);
    EXPECT_TRUE(keyframe->SetValue(id, output[id], test_element));
    curve->AddKeyframe(std::move(keyframe));
  };
  append_keyframe(0.0, "translateZ(100%)");
  append_keyframe(1.0, "translateZ(50%)");

  auto sample_time = fml::TimeDelta::FromSecondsF(0.5);
  auto result = curve->GetValue(sample_time).GetValue().Array();
  ASSERT_EQ(result->size(), 1u);
  auto translate = result->get(0).Array();
  ASSERT_EQ(translate->size(), 7u);
  EXPECT_EQ(translate->get(0).Number(),
            static_cast<int>(starlight::TransformType::kTranslate3d));
  EXPECT_FLOAT_EQ(translate->get(5).Number(), 75.0f);
  EXPECT_EQ(translate->get(6).Number(),
            static_cast<int>(lynx::tasm::CSSValuePattern::NUMBER));
}

TEST_F(TransformAnimationCurveTest,
       MatrixAnimationSerializesTranslationInCSSPixels) {
  ResetManager(2.0f);
  auto test_element_ptr = InitTestElement();
  auto test_element = test_element_ptr.get();

  auto curve = KeyframedTransformAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::TRANSFORM;
  curve->SetElement(test_element);

  const auto id = lynx::tasm::CSSPropertyID::kPropertyIDTransform;
  lynx::tasm::CSSParserConfigs configs;
  const auto append_keyframe = [&](double time, const char* transform) {
    lynx::tasm::StyleMap output;
    EXPECT_TRUE(lynx::tasm::UnitHandler::Process(id, lepus::Value(transform),
                                                 output, configs));
    auto keyframe =
        TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(time), nullptr);
    EXPECT_TRUE(keyframe->SetValue(id, output[id], test_element));
    curve->AddKeyframe(std::move(keyframe));
  };
  append_keyframe(0.0, "matrix(1, 0, 0, 1, 10, 0)");
  append_keyframe(1.0, "matrix(1, 0, 0, 1, 30, 0)");

  fml::TimeDelta sample_time = fml::TimeDelta::FromSecondsF(0.5);
  auto result = curve->GetValue(sample_time).GetValue().Array();
  ASSERT_EQ(result->size(), 1u);
  auto matrix = result->get(0).Array();
  ASSERT_EQ(matrix->size(), 17u);
  EXPECT_EQ(matrix->get(0).Number(),
            static_cast<int>(starlight::TransformType::kMatrix3d));
  EXPECT_FLOAT_EQ(matrix->get(13).Number(), 20.0f);
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
