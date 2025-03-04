// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/utils/timing_function.h"

#include <memory>

#include "base/include/log/logging.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/transforms/transform_operations.h"
#include "core/animation/utils/cubic_bezier.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/style/css_type.h"
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

class TimingFunctionTest : public ::testing::Test {
 public:
  TimingFunctionTest() {}
  ~TimingFunctionTest() override {}
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

  fml::RefPtr<lynx::tasm::Element> InitTestElement() {
    return manager->CreateNode("view", nullptr);
  }

  starlight::AnimationData InitAnimationData(
      starlight::TimingFunctionData timing_func) {
    starlight::AnimationData data;
    data.name = base::String("test");
    data.duration = 2000;
    data.delay = 0;
    data.timing_func = timing_func;
    data.iteration_count = 1;
    data.fill_mode = starlight::AnimationFillModeType::kBoth;
    data.direction = starlight::AnimationDirectionType::kNormal;
    data.play_state = starlight::AnimationPlayStateType::kRunning;
    return data;
  }
};

TEST_F(TimingFunctionTest, MakeTimingFunction1) {
  auto test_timing_function_data = starlight::TimingFunctionData();
  auto timing_function_0 =
      TimingFunction::MakeTimingFunction(test_timing_function_data);
  auto type0 = timing_function_0->GetType();
  EXPECT_EQ(type0, TimingFunction::Type::LINEAR);

  auto test_func_type_1 = starlight::TimingFunctionType::kEaseIn;
  test_timing_function_data.timing_func = test_func_type_1;
  auto timing_function_1 =
      TimingFunction::MakeTimingFunction(test_timing_function_data);
  auto type1 = timing_function_1->GetType();
  EXPECT_EQ(type1, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_2 = starlight::TimingFunctionType::kEaseOut;
  test_timing_function_data.timing_func = test_func_type_2;
  auto timing_function_2 =
      TimingFunction::MakeTimingFunction(test_timing_function_data);
  auto type2 = timing_function_2->GetType();
  EXPECT_EQ(type2, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_3 = starlight::TimingFunctionType::kEaseInEaseOut;
  test_timing_function_data.timing_func = test_func_type_3;
  auto timing_function_3 =
      TimingFunction::MakeTimingFunction(test_timing_function_data);
  auto type3 = timing_function_3->GetType();
  EXPECT_EQ(type3, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_4 = starlight::TimingFunctionType::kCubicBezier;
  test_timing_function_data.timing_func = test_func_type_4;
  auto timing_function_4 =
      TimingFunction::MakeTimingFunction(test_timing_function_data);
  auto type4 = timing_function_4->GetType();
  EXPECT_EQ(type4, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_5 = starlight::TimingFunctionType::kSteps;
  test_timing_function_data.timing_func = test_func_type_5;
  auto timing_function_5 =
      TimingFunction::MakeTimingFunction(test_timing_function_data);
  auto type5 = timing_function_5->GetType();
  EXPECT_EQ(type5, TimingFunction::Type::STEPS);
}

TEST_F(TimingFunctionTest, MakeTimingFunction2) {
  auto test_timing_function_data = starlight::TimingFunctionData();
  auto test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_0 =
      TimingFunction::MakeTimingFunction(&test_animation_data);
  auto type0 = timing_function_0->GetType();
  EXPECT_EQ(type0, TimingFunction::Type::LINEAR);

  auto test_func_type_1 = starlight::TimingFunctionType::kEaseIn;
  test_timing_function_data.timing_func = test_func_type_1;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_1 =
      TimingFunction::MakeTimingFunction(&test_animation_data);
  auto type1 = timing_function_1->GetType();
  EXPECT_EQ(type1, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_2 = starlight::TimingFunctionType::kEaseOut;
  test_timing_function_data.timing_func = test_func_type_2;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_2 =
      TimingFunction::MakeTimingFunction(&test_animation_data);
  auto type2 = timing_function_2->GetType();
  EXPECT_EQ(type2, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_3 = starlight::TimingFunctionType::kEaseInEaseOut;
  test_timing_function_data.timing_func = test_func_type_3;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_3 =
      TimingFunction::MakeTimingFunction(&test_animation_data);
  auto type3 = timing_function_3->GetType();
  EXPECT_EQ(type3, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_4 = starlight::TimingFunctionType::kCubicBezier;
  test_timing_function_data.timing_func = test_func_type_4;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_4 =
      TimingFunction::MakeTimingFunction(&test_animation_data);
  auto type4 = timing_function_4->GetType();
  EXPECT_EQ(type4, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_5 = starlight::TimingFunctionType::kSteps;
  test_timing_function_data.timing_func = test_func_type_5;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_5 =
      TimingFunction::MakeTimingFunction(&test_animation_data);
  auto type5 = timing_function_5->GetType();
  EXPECT_EQ(type5, TimingFunction::Type::STEPS);
}

TEST_F(TimingFunctionTest, CreatePreset) {
  auto cubic_timing_function_0 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::CUSTOM);
  EXPECT_TRUE(cubic_timing_function_0 == nullptr);

  double epsilon = 1e-7;
  auto cubic_timing_function_1 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE);
  EXPECT_TRUE(cubic_timing_function_1 != nullptr);
  auto bezier1 = cubic_timing_function_1->bezier();
  bool flag1 = abs(bezier1.GetX1() - 0.25) < epsilon &&
               abs(bezier1.GetY1() - 0.1) < epsilon &&
               abs(bezier1.GetX2() - 0.25) < epsilon &&
               abs(bezier1.GetY2() - 1.0) < epsilon;
  EXPECT_TRUE(flag1);

  auto cubic_timing_function_2 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE_IN);
  EXPECT_TRUE(cubic_timing_function_2 != nullptr);
  auto bezier2 = cubic_timing_function_2->bezier();
  bool flag2 = abs(bezier2.GetX1() - 0.42) < epsilon &&
               abs(bezier2.GetY1() - 0.0) < epsilon &&
               abs(bezier2.GetX2() - 1.0) < epsilon &&
               abs(bezier2.GetY2() - 1.0) < epsilon;
  EXPECT_TRUE(flag2);

  auto cubic_timing_function_3 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE_OUT);
  EXPECT_TRUE(cubic_timing_function_3 != nullptr);
  auto bezier3 = cubic_timing_function_3->bezier();
  bool flag3 = abs(bezier3.GetX1() - 0.0) < epsilon &&
               abs(bezier3.GetY1() - 0.0) < epsilon &&
               abs(bezier3.GetX2() - 0.58) < epsilon &&
               abs(bezier3.GetY2() - 1.0) < epsilon;
  EXPECT_TRUE(flag3);

  auto cubic_timing_function_4 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE_IN_OUT);
  EXPECT_TRUE(cubic_timing_function_4 != nullptr);
  auto bezier4 = cubic_timing_function_4->bezier();
  bool flag4 = abs(bezier4.GetX1() - 0.42) < epsilon &&
               abs(bezier4.GetY1() - 0.0) < epsilon &&
               abs(bezier4.GetX2() - 0.58) < epsilon &&
               abs(bezier4.GetY2() - 1.0) < epsilon;
  EXPECT_TRUE(flag4);
}

TEST_F(TimingFunctionTest, CubicBezierTimingFunctionCreate) {
  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  double epsilon = 1e-7;
  auto bezier = test_cubic_timing_function->bezier();
  bool flag = abs(bezier.GetX1() - 0.0) < epsilon &&
              abs(bezier.GetY1() - 0.25) < epsilon &&
              abs(bezier.GetX2() - 0.5) < epsilon &&
              abs(bezier.GetY2() - 1.0) < epsilon;
  EXPECT_TRUE(flag);
  EXPECT_EQ(test_cubic_timing_function->ease_type(),
            CubicBezierTimingFunction::EaseType::CUSTOM);
  EXPECT_TRUE(test_cubic_timing_function->GetType() ==
              TimingFunction::Type::CUBIC_BEZIER);
}

TEST_F(TimingFunctionTest, LinearTimingFunctionCreate) {
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_TRUE(test_linear_timing_function->GetType() ==
              TimingFunction::Type::LINEAR);
}

TEST_F(TimingFunctionTest, StepsTimingFunctionCreate) {
  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, starlight::StepsType::kStart);
  EXPECT_EQ(test_steps_timing_function->steps(), 2);
  EXPECT_EQ(test_steps_timing_function->step_position(),
            starlight::StepsType::kStart);
  EXPECT_TRUE(test_steps_timing_function->GetType() ==
              TimingFunction::Type::STEPS);
}

TEST_F(TimingFunctionTest, Clone) {
  auto test_linear_timing_function = LinearTimingFunction::Create();
  auto clone_linear_timing_function = test_linear_timing_function->Clone();
  EXPECT_EQ(test_linear_timing_function->GetType(),
            clone_linear_timing_function->GetType());

  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  auto clone_cubic_timing_function = test_cubic_timing_function->Clone();
  EXPECT_EQ(test_cubic_timing_function->GetType(),
            clone_cubic_timing_function->GetType());

  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, starlight::StepsType::kStart);
  auto clone_steps_timing_function = test_steps_timing_function->Clone();
  EXPECT_EQ(test_steps_timing_function->GetType(),
            clone_steps_timing_function->GetType());
}

TEST_F(TimingFunctionTest, GetType) {
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_EQ(test_linear_timing_function->GetType(),
            TimingFunction::Type::LINEAR);

  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  EXPECT_EQ(test_cubic_timing_function->GetType(),
            TimingFunction::Type::CUBIC_BEZIER);

  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, starlight::StepsType::kStart);
  EXPECT_EQ(test_steps_timing_function->GetType(), TimingFunction::Type::STEPS);
}

TEST_F(TimingFunctionTest, GetValue) {
  double epsilon = 0.0001;
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_TRUE(abs(test_linear_timing_function->GetValue(0.5) - 0.5) < epsilon);

  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  EXPECT_TRUE(abs(test_cubic_timing_function->GetValue(0.5) - 0.7809) <
              epsilon);

  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, starlight::StepsType::kStart);
  EXPECT_TRUE(abs(test_steps_timing_function->GetValue(0.5) - 1.0) < epsilon);
}

TEST_F(TimingFunctionTest, Velocity) {
  double epsilon = 0.0001;
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_TRUE(abs(test_linear_timing_function->Velocity(0.5) - 0.0) < epsilon);

  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  EXPECT_TRUE(abs(test_cubic_timing_function->Velocity(0.5) - 0.8418) <
              epsilon);

  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, starlight::StepsType::kStart);
  EXPECT_TRUE(abs(test_steps_timing_function->Velocity(0.5) - 0.0) < epsilon);
}

}  // namespace testing
}  // namespace tasm
}  // namespace animation
}  // namespace lynx
