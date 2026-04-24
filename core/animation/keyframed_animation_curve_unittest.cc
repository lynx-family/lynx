// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public
#include "core/animation/keyframed_animation_curve.h"

#include <memory>

#include "core/animation/css_keyframe_manager.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "gfx/animation/animation_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx {
namespace animation {
namespace tasm {
namespace test {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class KeyframedAnimationCurveTest : public ::testing::Test {
 public:
  KeyframedAnimationCurveTest() {}
  ~KeyframedAnimationCurveTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator;
  fml::RefPtr<lynx::tasm::Element> element_;

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

  fml::RefPtr<::lynx::tasm::FiberElement> InitFiberElement() {
    manager->SetEnableNewAnimatorRadon(true);
    auto test_element = manager->CreateFiberView();
    test_element->SetAttribute(base::String("enable-new-animator"),
                               lepus::Value("true"));
    return test_element;
  }
};

// Tests that a layout animation with two keyframes works as expected.
TEST_F(KeyframedAnimationCurveTest, TwoLayoutKeyframe) {
  std::unique_ptr<KeyframedLayoutAnimationCurve> curve(
      KeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto test_frame1 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  test_frame1->css_value_ =
      ::lynx::tasm::CSSValue(2.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_frame2->css_value_ =
      ::lynx::tasm::CSSValue(4.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  EXPECT_FLOAT_EQ(2.f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(3.f, curve->GetValue(value2).AsNumber());
  EXPECT_FLOAT_EQ(4.f, curve->GetValue(value3).AsNumber());
}

// Tests that a layout animation with three keyframes works as expected.
TEST_F(KeyframedAnimationCurveTest, ThreeLayoutKeyframe) {
  std::unique_ptr<KeyframedLayoutAnimationCurve> curve(
      KeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto test_frame1 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  test_frame1->css_value_ =
      ::lynx::tasm::CSSValue(2.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_frame2->css_value_ =
      ::lynx::tasm::CSSValue(4.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  test_frame3->SetLayout(starlight::NLength::MakeUnitNLength(8.f));
  test_frame3->css_value_ =
      ::lynx::tasm::CSSValue(8.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame3));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);
  EXPECT_FLOAT_EQ(2.f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(3.f, curve->GetValue(value2).AsNumber());
  EXPECT_FLOAT_EQ(4.f, curve->GetValue(value3).AsNumber());
  EXPECT_FLOAT_EQ(6.f, curve->GetValue(value4).AsNumber());
  EXPECT_FLOAT_EQ(8.f, curve->GetValue(value5).AsNumber());
}

TEST_F(KeyframedAnimationCurveTest, LayoutAutoFallsBackToDiscreteValue) {
  std::unique_ptr<KeyframedLayoutAnimationCurve> curve(
      KeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto test_frame1 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_TRUE(test_frame1->SetValue(
      ::lynx::tasm::kPropertyIDLeft,
      ::lynx::tasm::CSSValue(starlight::LengthValueType::kAuto),
      test_element.get()));
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_frame2->css_value_ =
      ::lynx::tasm::CSSValue(4.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta start_value = fml::TimeDelta::FromSecondsF(0.0f);
  fml::TimeDelta value = fml::TimeDelta::FromSecondsF(0.5f);
  EXPECT_EQ(::lynx::tasm::CSSValue(starlight::LengthValueType::kAuto),
            curve->GetValue(start_value));
  EXPECT_FLOAT_EQ(4.f, curve->GetValue(value).AsNumber());
}

TEST_F(KeyframedAnimationCurveTest, LayoutFrIsRejected) {
  auto test_element = InitFiberElement();
  auto test_frame = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_FALSE(test_frame->SetValue(
      ::lynx::tasm::kPropertyIDWidth,
      ::lynx::tasm::CSSValue(1.f, ::lynx::tasm::CSSValuePattern::FR),
      test_element.get()));
}

// Tests that a layout animation with multiple keys at a given time works
// sanely.
TEST_F(KeyframedAnimationCurveTest, RepeatedLayoutKeyTimes) {
  std::unique_ptr<KeyframedLayoutAnimationCurve> curve(
      KeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto test_frame1 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_frame1->css_value_ =
      ::lynx::tasm::CSSValue(4.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_frame2->css_value_ =
      ::lynx::tasm::CSSValue(4.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame3->SetLayout(starlight::NLength::MakeUnitNLength(6.f));
  test_frame3->css_value_ =
      ::lynx::tasm::CSSValue(6.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame3));

  auto test_frame4 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  test_frame4->SetLayout(starlight::NLength::MakeUnitNLength(6.f));
  test_frame4->css_value_ =
      ::lynx::tasm::CSSValue(6.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame4));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);

  EXPECT_FLOAT_EQ(4.f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(4.f, curve->GetValue(value2).AsNumber());

  // There is a discontinuity at 1. Any value between 4 and 6 is valid.
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  float mid_value = curve->GetValue(value3).AsNumber();
  EXPECT_TRUE(mid_value >= 4 && mid_value <= 6);

  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);
  EXPECT_FLOAT_EQ(6.f, curve->GetValue(value4).AsNumber());
  EXPECT_FLOAT_EQ(6.f, curve->GetValue(value5).AsNumber());
}

// Tests that a opacity animation with two keyframes works as expected.
TEST_F(KeyframedAnimationCurveTest, TwoOpacityKeyframe) {
  std::unique_ptr<KeyframedOpacityAnimationCurve> curve(
      KeyframedOpacityAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame1 = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetOpacity(1.0f);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetOpacity(0.0f);
  curve->AddKeyframe(std::move(test_frame2));
  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  EXPECT_FLOAT_EQ(1.0f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(0.5f, curve->GetValue(value2).AsNumber());
  EXPECT_FLOAT_EQ(0.0f, curve->GetValue(value3).AsNumber());
}

TEST_F(KeyframedAnimationCurveTest, OpacityKeyframeRejectsNonNumberLikeLegacy) {
  auto test_element = InitFiberElement();

  auto rem_frame = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_FALSE(rem_frame->SetValue(
      ::lynx::tasm::kPropertyIDOpacity,
      ::lynx::tasm::CSSValue(2.f, ::lynx::tasm::CSSValuePattern::REM),
      test_element.get()));

  auto calc_frame = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_FALSE(calc_frame->SetValue(
      ::lynx::tasm::kPropertyIDOpacity,
      ::lynx::tasm::CSSValue("calc(1rem + 10px)",
                             ::lynx::tasm::CSSValuePattern::CALC,
                             ::lynx::tasm::CSSValueType::DEFAULT),
      test_element.get()));
}

// Tests that a opacity animation with three keyframes works as expected.
TEST_F(KeyframedAnimationCurveTest, ThreeOpacityKeyframe) {
  std::unique_ptr<KeyframedOpacityAnimationCurve> curve(
      KeyframedOpacityAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame1 = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetOpacity(1.0f);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetOpacity(0.4f);
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  test_frame3->SetOpacity(0.0f);
  curve->AddKeyframe(std::move(test_frame3));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);
  EXPECT_FLOAT_EQ(1.0f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(0.7f, curve->GetValue(value2).AsNumber());
  EXPECT_FLOAT_EQ(0.4f, curve->GetValue(value3).AsNumber());
  EXPECT_FLOAT_EQ(0.2f, curve->GetValue(value4).AsNumber());
  EXPECT_FLOAT_EQ(0.0f, curve->GetValue(value5).AsNumber());
}

// Tests that a layout animation with multiple keys at a given time works
// sanely.
TEST_F(KeyframedAnimationCurveTest, RepeatedOpacityKeyTimes) {
  std::unique_ptr<KeyframedOpacityAnimationCurve> curve(
      KeyframedOpacityAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame1 = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetOpacity(0.0f);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetOpacity(0.0f);
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame3->SetOpacity(1.0f);
  curve->AddKeyframe(std::move(test_frame3));

  auto test_frame4 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  test_frame4->SetOpacity(1.0f);
  curve->AddKeyframe(std::move(test_frame4));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);

  EXPECT_FLOAT_EQ(0.0f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(0.0f, curve->GetValue(value2).AsNumber());

  // There is a discontinuity at 1. Any value between 4 and 6 is valid.
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  float mid_value = curve->GetValue(value3).AsNumber();
  EXPECT_TRUE(mid_value >= 0.0f && mid_value <= 1.0f);

  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);
  EXPECT_FLOAT_EQ(1.0f, curve->GetValue(value4).AsNumber());
  EXPECT_FLOAT_EQ(1.0f, curve->GetValue(value5).AsNumber());
}

// Tests that a color animation with two keyframes works as expected.
TEST_F(KeyframedAnimationCurveTest, TwoColorKeyFrame) {
  std::unique_ptr<KeyframedColorAnimationCurve> curve(
      KeyframedColorAnimationCurve::Create(
          starlight::XAnimationColorInterpolationType::kSRGB));
  curve->type_ = AnimationCurve::CurveType::BGCOLOR;
  auto test_frame1 = ColorKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetColor(4294901760);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetColor(4278255360);
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  EXPECT_EQ(::lynx::tasm::CSSValue(4294901760u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value1));
  EXPECT_EQ(::lynx::tasm::CSSValue(4290427392u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value2));
  EXPECT_EQ(::lynx::tasm::CSSValue(4278255360u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value3));
}

// Tests that a color animation with three keyframes works as expected.
TEST_F(KeyframedAnimationCurveTest, ThreeColorKeyFrame) {
  std::unique_ptr<KeyframedColorAnimationCurve> curve(
      KeyframedColorAnimationCurve::Create(
          starlight::XAnimationColorInterpolationType::kSRGB));
  curve->type_ = AnimationCurve::CurveType::BGCOLOR;
  auto test_frame1 = ColorKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetColor(4294901760);  // ARGB(255, 255, 0, 0)
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetColor(4278255360);  // ARGB(255, 0, 255, 0)
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  test_frame3->SetColor(4278190335);  // ARGB(255, 0, 0, 255)
  curve->AddKeyframe(std::move(test_frame3));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);
  EXPECT_EQ(::lynx::tasm::CSSValue(4294901760u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value1));
  EXPECT_EQ(::lynx::tasm::CSSValue(4290427392u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value2));
  EXPECT_EQ(::lynx::tasm::CSSValue(4278255360u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value3));
  EXPECT_EQ(::lynx::tasm::CSSValue(4278237882u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value4));
  EXPECT_EQ(::lynx::tasm::CSSValue(4278190335u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value5));
}

// Tests that a color animation with multiple keys at a given time works
// sanely.
TEST_F(KeyframedAnimationCurveTest, RepeatedColorKeyFrame) {
  std::unique_ptr<KeyframedColorAnimationCurve> curve(
      KeyframedColorAnimationCurve::Create(
          starlight::XAnimationColorInterpolationType::kSRGB));
  curve->type_ = AnimationCurve::CurveType::BGCOLOR;
  auto test_frame1 = ColorKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetColor(4282384384);  // ARGB(255, 64, 0, 0)
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetColor(4282384384);  // ARGB(255, 64, 0, 0)
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame3->SetColor(4290772992);  // ARGB(255, 192, 0, 0)
  curve->AddKeyframe(std::move(test_frame3));

  auto test_frame4 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  test_frame4->SetColor(4290772992);  // ARGB(255, 192, 0, 0)
  curve->AddKeyframe(std::move(test_frame4));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);

  EXPECT_EQ(::lynx::tasm::CSSValue(4282384384u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value1));
  EXPECT_EQ(::lynx::tasm::CSSValue(4282384384u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value2));

  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.0f);

  auto mid_value = curve->GetValue(value3);
  EXPECT_EQ(::lynx::tasm::CSSValue(4290772992u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            mid_value);
  auto alpha_value = uint32_t(mid_value.AsNumber()) >> 24;
  EXPECT_EQ(255, alpha_value);
  auto red_value = uint32_t(mid_value.AsNumber()) << 8 >> 24;
  EXPECT_LE(64, red_value);
  EXPECT_GE(192, red_value);

  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);
  EXPECT_EQ(::lynx::tasm::CSSValue(4290772992u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value4));
  EXPECT_EQ(::lynx::tasm::CSSValue(4290772992u,
                                   ::lynx::tasm::CSSValuePattern::NUMBER),
            curve->GetValue(value5));
}

// Tests that the keyframes may be added out of order.
TEST_F(KeyframedAnimationCurveTest, UnsortedKeyframes) {
  std::unique_ptr<KeyframedLayoutAnimationCurve> curve(
      KeyframedLayoutAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::LEFT;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto test_frame1 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(2.f), nullptr);
  test_frame1->SetLayout(starlight::NLength::MakeUnitNLength(8.f));
  test_frame1->css_value_ =
      ::lynx::tasm::CSSValue(8.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame2->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  test_frame2->css_value_ =
      ::lynx::tasm::CSSValue(2.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame2));

  auto test_frame3 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(1.f), nullptr);
  test_frame3->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_frame3->css_value_ =
      ::lynx::tasm::CSSValue(4.f, ::lynx::tasm::CSSValuePattern::NUMBER);
  curve->AddKeyframe(std::move(test_frame3));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(0.5f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value4 = fml::TimeDelta::FromSecondsF(1.5f);
  fml::TimeDelta value5 = fml::TimeDelta::FromSecondsF(2.f);

  EXPECT_FLOAT_EQ(2.f, curve->GetValue(value1).AsNumber());
  EXPECT_FLOAT_EQ(3.f, curve->GetValue(value2).AsNumber());
  EXPECT_FLOAT_EQ(4.f, curve->GetValue(value3).AsNumber());
  EXPECT_FLOAT_EQ(6.f, curve->GetValue(value4).AsNumber());
  EXPECT_FLOAT_EQ(8.f, curve->GetValue(value5).AsNumber());
}

TEST_F(KeyframedAnimationCurveTest, TransformedKeyframeProgress) {
  {
    std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
    auto test_frame1 = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetOpacity(1.0f);
    keyframes.emplace_back(std::move(test_frame1));

    auto test_frame2 =
        OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
    test_frame2->SetOpacity(0.0f);
    keyframes.emplace_back(std::move(test_frame2));
    auto s1 = gfx::ComputeKeyframedProgress(keyframes, nullptr, 0,
                                            fml::TimeDelta::FromSecondsF(1));
    EXPECT_TRUE(s1.valid);
    EXPECT_TRUE(s1.progress == 1.0);

    auto test_frame3 =
        OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
    test_frame3->SetOpacity(0.0f);
    keyframes.emplace_back(std::move(test_frame3));
    auto s2 = gfx::ComputeKeyframedProgress(keyframes, nullptr, 1,
                                            fml::TimeDelta::FromSecondsF(1));
    EXPECT_TRUE(s2.valid);
    EXPECT_EQ(1u, s2.index);
    EXPECT_TRUE(s2.progress == 1.0);
  }

  {
    std::vector<std::unique_ptr<gfx::Keyframe>> keyframes;
    auto test_frame1 = OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetOpacity(1.0f);
    keyframes.emplace_back(std::move(test_frame1));

    auto test_frame2 =
        OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
    test_frame2->SetOpacity(0.0f);
    keyframes.emplace_back(std::move(test_frame2));
    auto s3 = gfx::ComputeKeyframedProgress(keyframes, nullptr, 2,
                                            fml::TimeDelta::FromSecondsF(0.5));
    EXPECT_TRUE(s3.valid);
    EXPECT_TRUE(s3.progress == 0.25);

    auto test_frame3 =
        OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
    test_frame3->SetOpacity(0.0f);
    keyframes.emplace_back(std::move(test_frame3));
    auto s4 = gfx::ComputeKeyframedProgress(keyframes, nullptr, 2,
                                            fml::TimeDelta::FromSecondsF(2));
    EXPECT_TRUE(s4.valid);
    EXPECT_EQ(1u, s4.index);
    EXPECT_TRUE(s4.progress == 1.0);
  }
}

TEST_F(KeyframedAnimationCurveTest, MakeEmptyKeyframe) {
  auto test_curve1 = KeyframedLayoutAnimationCurve::Create();
  auto test_frame1 =
      LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  auto test_frame2 =
      test_curve1->KeyframedLayoutAnimationCurve::MakeEmptyKeyframe(
          fml::TimeDelta::FromSecondsF(2.0));
  EXPECT_EQ(test_frame1->Time(), test_frame2->Time());
  EXPECT_EQ(test_frame1->timing_function(), test_frame2->timing_function());

  auto test_curve2 = KeyframedOpacityAnimationCurve::Create();
  auto test_frame3 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  auto test_frame4 =
      test_curve2->KeyframedOpacityAnimationCurve::MakeEmptyKeyframe(
          fml::TimeDelta::FromSecondsF(2.0));
  EXPECT_EQ(test_frame3->Time(), test_frame4->Time());
  EXPECT_EQ(test_frame3->timing_function(), test_frame4->timing_function());

  auto test_curve3 = KeyframedColorAnimationCurve::Create(
      starlight::XAnimationColorInterpolationType::kSRGB);
  auto test_frame5 =
      ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(2.0), nullptr);
  auto test_frame6 =
      test_curve3->KeyframedColorAnimationCurve::MakeEmptyKeyframe(
          fml::TimeDelta::FromSecondsF(2.0));
  EXPECT_EQ(test_frame5->Time(), test_frame6->Time());
  EXPECT_EQ(test_frame5->timing_function(), test_frame6->timing_function());
}

TEST_F(KeyframedAnimationCurveTest, HandleCSSVariableValueIfNeed) {
  ::lynx::tasm::RadonNode node(nullptr, "my_tag", 0);
  manager->SetEnableNewAnimatorRadon(true);
  auto test_element = manager->CreateFiberElement("view");
  test_element->SetAttributeHolder(node.attribute_holder());
  auto test_value =
      ::lynx::tasm::CSSValue(0.8, ::lynx::tasm::CSSValuePattern::NUMBER);
  auto result_value_1 = HandleCSSVariableValueIfNeed(
      ::lynx::tasm::kPropertyIDOpacity, test_value, test_element.get());

  EXPECT_EQ(result_value_1, test_value);

  node.UpdateCSSVariable("--height", "20px");

  auto test_value_2 =
      ::lynx::tasm::CSSValue("calc({{--height}} + {{--height}})",
                             ::lynx::tasm::CSSValuePattern::STRING,
                             ::lynx::tasm::CSSValueType::VARIABLE);

  auto var_value = ::lynx::tasm::CSSValue("calc(20px + 20px)",
                                          ::lynx::tasm::CSSValuePattern::CALC,
                                          ::lynx::tasm::CSSValueType::VARIABLE);
  auto result_value_2 = HandleCSSVariableValueIfNeed(
      ::lynx::tasm::kPropertyIDLeft, test_value_2, test_element.get());
  EXPECT_EQ(result_value_2, var_value);
}

TEST_F(KeyframedAnimationCurveTest, LayoutUnitPatternInvalidatesResolvedValue) {
  auto test_frame = LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame->css_value_ =
      ::lynx::tasm::CSSValue(1.f, ::lynx::tasm::CSSValuePattern::REM);
  test_frame->SetLayout({20.f, gfx::LengthUnit::kNumber});
  EXPECT_TRUE(test_frame->HasResolvedValue());

  test_frame->NotifyUnitValuesUpdated(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::EM));
  EXPECT_TRUE(test_frame->HasResolvedValue());

  test_frame->NotifyUnitValuesUpdated(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));
  EXPECT_FALSE(test_frame->HasResolvedValue());
}

TEST_F(KeyframedAnimationCurveTest, FilterKeyframeStoresRawAndResolvesRem) {
  auto test_element = InitFiberElement();
  auto start_arr = lepus::CArray::Create();
  start_arr->emplace_back(static_cast<uint32_t>(starlight::FilterType::kBlur));
  start_arr->emplace_back(1.f);
  start_arr->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));
  auto end_arr = lepus::CArray::Create();
  end_arr->emplace_back(static_cast<uint32_t>(starlight::FilterType::kBlur));
  end_arr->emplace_back(3.f);
  end_arr->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));

  auto start_frame = FilterKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_TRUE(start_frame->SetValue(::lynx::tasm::kPropertyIDFilter,
                                    ::lynx::tasm::CSSValue(start_arr),
                                    test_element.get()));
  auto* start_frame_ptr = start_frame.get();
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM),
            start_frame->filter_.GetArray()->get(2).UInt32());
  EXPECT_FALSE(start_frame->HasResolvedValue());

  auto end_frame =
      FilterKeyframe::Create(fml::TimeDelta::FromSecondsF(1.f), nullptr);
  EXPECT_TRUE(end_frame->SetValue(::lynx::tasm::kPropertyIDFilter,
                                  ::lynx::tasm::CSSValue(end_arr),
                                  test_element.get()));
  auto* end_frame_ptr = end_frame.get();
  EXPECT_FALSE(end_frame->HasResolvedValue());

  auto curve = KeyframedFilterAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::FILTER;
  curve->SetElement(test_element.get());
  curve->AddKeyframe(std::move(start_frame));
  curve->AddKeyframe(std::move(end_frame));

  fml::TimeDelta time = fml::TimeDelta::FromSecondsF(0.5f);
  auto normalized = curve->GetValue(time).GetArray();
  const auto& context =
      CSSKeyframeManager::GetLengthContext(test_element.get());
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 2.f,
                  normalized->get(1).Double());
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::NUMBER),
            normalized->get(2).UInt32());
  EXPECT_TRUE(start_frame_ptr->HasResolvedValue());
  EXPECT_EQ(gfx::UnitTag::kNumber, start_frame_ptr->ResolvedValue().unit);
  EXPECT_TRUE(end_frame_ptr->HasResolvedValue());
}

TEST_F(KeyframedAnimationCurveTest, TransformOriginKeyframeResolvesRem) {
  auto test_element = InitFiberElement();
  auto start_arr = lepus::CArray::Create();
  start_arr->emplace_back(1.f);
  start_arr->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));
  start_arr->emplace_back(50.f);
  start_arr->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::PERCENT));
  auto end_arr = lepus::CArray::Create();
  end_arr->emplace_back(3.f);
  end_arr->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));
  end_arr->emplace_back(100.f);
  end_arr->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::PERCENT));

  auto start_frame = TransformOriginKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_TRUE(start_frame->SetValue(::lynx::tasm::kPropertyIDTransformOrigin,
                                    ::lynx::tasm::CSSValue(start_arr),
                                    test_element.get()));
  auto* start_frame_ptr = start_frame.get();
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM),
            start_frame->transform_origin_.GetArray()->get(1).UInt32());
  EXPECT_FALSE(start_frame->HasResolvedValue());

  auto end_frame = TransformOriginKeyframe::Create(
      fml::TimeDelta::FromSecondsF(1.f), nullptr);
  EXPECT_TRUE(end_frame->SetValue(::lynx::tasm::kPropertyIDTransformOrigin,
                                  ::lynx::tasm::CSSValue(end_arr),
                                  test_element.get()));
  auto* end_frame_ptr = end_frame.get();
  EXPECT_FALSE(end_frame->HasResolvedValue());

  auto curve = KeyframedTransformOriginAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::TRANSFORM_ORIGIN;
  curve->SetElement(test_element.get());
  curve->AddKeyframe(std::move(start_frame));
  curve->AddKeyframe(std::move(end_frame));

  fml::TimeDelta time = fml::TimeDelta::FromSecondsF(0.5f);
  auto normalized = curve->GetValue(time).GetArray();
  const auto& context =
      CSSKeyframeManager::GetLengthContext(test_element.get());
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 2.f,
                  normalized->get(0).Double());
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::NUMBER),
            normalized->get(1).UInt32());
  EXPECT_FLOAT_EQ(75.f, normalized->get(2).Double());
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::PERCENT),
            normalized->get(3).UInt32());
  EXPECT_TRUE(start_frame_ptr->HasResolvedValue());
  EXPECT_EQ(gfx::UnitTag::kNumber, start_frame_ptr->ResolvedValue().x.tag);
  EXPECT_EQ(gfx::UnitTag::kPercent, start_frame_ptr->ResolvedValue().y.tag);
  EXPECT_TRUE(end_frame_ptr->HasResolvedValue());
}

TEST_F(KeyframedAnimationCurveTest, BackgroundPositionKeyframeResolvesUnits) {
  auto test_element = InitFiberElement();
  const auto& context =
      CSSKeyframeManager::GetLengthContext(test_element.get());
  auto start_position = lepus::CArray::Create();
  start_position->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));
  start_position->emplace_back(1.f);
  start_position->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM));
  start_position->emplace_back(2.f);
  auto end_position = lepus::CArray::Create();
  end_position->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::PX));
  end_position->emplace_back(context.root_node_font_size_ * 3.f /
                             context.layouts_unit_per_px_);
  end_position->emplace_back(
      static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::PX));
  end_position->emplace_back(context.root_node_font_size_ * 4.f /
                             context.layouts_unit_per_px_);

  auto start_outer = lepus::CArray::Create();
  start_outer->emplace_back(std::move(start_position));
  auto end_outer = lepus::CArray::Create();
  end_outer->emplace_back(std::move(end_position));

  auto start_frame =
      BackgroundPositionKeyframe::Create(fml::TimeDelta(), nullptr);
  EXPECT_TRUE(start_frame->SetValue(::lynx::tasm::kPropertyIDBackgroundPosition,
                                    ::lynx::tasm::CSSValue(start_outer),
                                    test_element.get()));
  auto* start_frame_ptr = start_frame.get();
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::REM),
            start_frame->background_position_.GetArray()
                ->get(0)
                .Array()
                ->get(0)
                .UInt32());
  EXPECT_FALSE(start_frame->HasResolvedValue());

  auto end_frame = BackgroundPositionKeyframe::Create(
      fml::TimeDelta::FromSecondsF(1.f), nullptr);
  EXPECT_TRUE(end_frame->SetValue(::lynx::tasm::kPropertyIDBackgroundPosition,
                                  ::lynx::tasm::CSSValue(end_outer),
                                  test_element.get()));
  auto* end_frame_ptr = end_frame.get();
  EXPECT_FALSE(end_frame->HasResolvedValue());

  auto curve = KeyframedBackgroundPositionAnimationCurve::Create();
  curve->type_ = AnimationCurve::CurveType::BACKGROUND_POSITION;
  curve->SetElement(test_element.get());
  curve->AddKeyframe(std::move(start_frame));
  curve->AddKeyframe(std::move(end_frame));

  fml::TimeDelta time = fml::TimeDelta::FromSecondsF(0.5f);
  auto output = curve->GetValue(time);
  auto output_array = output.GetArray();
  auto normalized = output_array->get(0).Array();
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::NUMBER),
            normalized->get(0).UInt32());
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 2.f,
                  normalized->get(1).Double());
  EXPECT_EQ(static_cast<uint32_t>(::lynx::tasm::CSSValuePattern::NUMBER),
            normalized->get(2).UInt32());
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 3.f,
                  normalized->get(3).Double());
  EXPECT_TRUE(start_frame_ptr->HasResolvedValue());
  EXPECT_EQ(gfx::UnitTag::kNumber, start_frame_ptr->ResolvedValue().x.tag);
  EXPECT_FLOAT_EQ(context.root_node_font_size_,
                  start_frame_ptr->ResolvedValue().x.value);
  EXPECT_EQ(gfx::UnitTag::kNumber, start_frame_ptr->ResolvedValue().y.tag);
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 2.f,
                  start_frame_ptr->ResolvedValue().y.value);
  EXPECT_TRUE(end_frame_ptr->HasResolvedValue());
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 3.f,
                  end_frame_ptr->ResolvedValue().x.value);
  EXPECT_FLOAT_EQ(context.root_node_font_size_ * 4.f,
                  end_frame_ptr->ResolvedValue().y.value);
}

TEST_F(KeyframedAnimationCurveTest, FilterInterPolateTest) {
  std::unique_ptr<KeyframedFilterAnimationCurve> curve(
      KeyframedFilterAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::FILTER;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto start_arr = lepus::CArray::Create();
  start_arr->emplace_back(static_cast<uint32_t>(starlight::FilterType::kBlur));
  start_arr->emplace_back(20.f);
  start_arr->emplace_back(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PX));
  auto start_value = ::lynx::tasm::CSSValue(std::move(start_arr));

  auto end_arr = lepus::CArray::Create();
  end_arr->emplace_back(static_cast<uint32_t>(starlight::FilterType::kBlur));
  end_arr->emplace_back(60.f);
  end_arr->emplace_back(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PX));
  auto end_value = ::lynx::tasm::CSSValue(std::move(end_arr));

  auto test_frame1 = FilterKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetFilter(start_value);
  test_frame1->is_empty_ = false;
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      FilterKeyframe::Create(fml::TimeDelta::FromSecondsF(2.f), nullptr);
  test_frame2->SetFilter(end_value);
  test_frame2->is_empty_ = false;
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(2.f);

  auto result_1 = curve->GetValue(value1).GetArray();
  auto result_2 = curve->GetValue(value2).GetArray();
  auto result_3 = curve->GetValue(value3).GetArray();

  EXPECT_FLOAT_EQ(20.f, result_1.get()->get(1).Double());
  EXPECT_FLOAT_EQ(40.f, result_2.get()->get(1).Double());
  EXPECT_FLOAT_EQ(60.f, result_3.get()->get(1).Double());
}

TEST_F(KeyframedAnimationCurveTest, TransformOriginInterPolateTest) {
  std::unique_ptr<KeyframedTransformOriginAnimationCurve> curve(
      KeyframedTransformOriginAnimationCurve::Create());
  curve->type_ = AnimationCurve::CurveType::TRANSFORM_ORIGIN;
  auto test_element = InitFiberElement();
  curve->SetElement(test_element.get());

  auto start_arr = lepus::CArray::Create();
  start_arr->emplace_back(50);
  start_arr->emplace_back(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT));
  start_arr->emplace_back(50);
  start_arr->emplace_back(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT));

  auto end_arr = lepus::CArray::Create();
  end_arr->emplace_back(0);
  end_arr->emplace_back(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT));
  end_arr->emplace_back(0);
  end_arr->emplace_back(
      static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT));

  auto test_frame1 = TransformOriginKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->transform_origin_ = lynx::tasm::CSSValue(start_arr);
  test_frame1->is_empty_ = false;
  curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 = TransformOriginKeyframe::Create(
      fml::TimeDelta::FromSecondsF(2.f), nullptr);
  test_frame2->transform_origin_ = lynx::tasm::CSSValue(end_arr);
  test_frame2->is_empty_ = false;
  curve->AddKeyframe(std::move(test_frame2));

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(2.f);

  auto result_1 = curve->GetValue(value1).GetArray();
  auto result_2 = curve->GetValue(value2).GetArray();
  auto result_3 = curve->GetValue(value3).GetArray();

  EXPECT_FLOAT_EQ(50.f, result_1.get()->get(0).Number());
  EXPECT_FLOAT_EQ(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT),
                  result_1.get()->get(1).Number());
  EXPECT_FLOAT_EQ(50.f, result_1.get()->get(2).Number());
  EXPECT_FLOAT_EQ(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT),
                  result_1.get()->get(3).Number());

  EXPECT_FLOAT_EQ(25.f, result_2.get()->get(0).Number());
  EXPECT_FLOAT_EQ(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT),
                  result_2.get()->get(1).Number());
  EXPECT_FLOAT_EQ(25.f, result_2.get()->get(2).Number());
  EXPECT_FLOAT_EQ(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT),
                  result_2.get()->get(3).Number());

  EXPECT_FLOAT_EQ(0.f, result_3.get()->get(0).Number());
  EXPECT_FLOAT_EQ(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT),
                  result_3.get()->get(1).Number());
  EXPECT_FLOAT_EQ(0.f, result_3.get()->get(2).Number());
  EXPECT_FLOAT_EQ(static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT),
                  result_3.get()->get(3).Number());
}

TEST_F(KeyframedAnimationCurveTest, GetOnXAxisCurveTypeSet) {
  auto test_set = animation::GetOnXAxisCurveTypeSet();
  static const base::NoDestructor<std::unordered_set<AnimationCurve::CurveType>>
      base_set({AnimationCurve::CurveType::LEFT,
                AnimationCurve::CurveType::RIGHT,
                AnimationCurve::CurveType::WIDTH,
                AnimationCurve::CurveType::MAX_WIDTH,
                AnimationCurve::CurveType::MIN_WIDTH,
                AnimationCurve::CurveType::MARGIN_LEFT,
                AnimationCurve::CurveType::MARGIN_RIGHT,
                AnimationCurve::CurveType::PADDING_LEFT,
                AnimationCurve::CurveType::PADDING_RIGHT,
                AnimationCurve::CurveType::BORDER_LEFT_WIDTH,
                AnimationCurve::CurveType::BORDER_RIGHT_WIDTH});
  EXPECT_EQ(test_set, *base_set);
}

}  // namespace test
}  // namespace tasm
}  // namespace animation
}  // namespace lynx
