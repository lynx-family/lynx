// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/timing_function.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace gfx {
namespace {

constexpr double kEpsilon = 1e-7;
constexpr double kLooseEpsilon = 0.0001;

class TimingFunctionTest : public ::testing::Test {
 protected:
  AnimationData InitAnimationData(TimingFunctionData timing_func) {
    AnimationData data;
    data.duration = 2000;
    data.delay = 0;
    data.timing_func = timing_func;
    data.iteration_count = 1;
    data.fill_mode = AnimationFillModeType::kBoth;
    data.direction = AnimationDirectionType::kNormal;
    data.play_state = AnimationPlayStateType::kRunning;
    return data;
  }
};

TEST_F(TimingFunctionTest, MakeTimingFunction1) {
  auto test_timing_function_data = TimingFunctionData();
  auto timing_function_0 = CreateTimingFunction(test_timing_function_data);
  auto type0 = timing_function_0->GetType();
  EXPECT_EQ(type0, TimingFunction::Type::LINEAR);

  auto test_func_type_1 = TimingFunctionType::kEaseIn;
  test_timing_function_data.timing_func = test_func_type_1;
  auto timing_function_1 = CreateTimingFunction(test_timing_function_data);
  auto type1 = timing_function_1->GetType();
  EXPECT_EQ(type1, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_2 = TimingFunctionType::kEaseOut;
  test_timing_function_data.timing_func = test_func_type_2;
  auto timing_function_2 = CreateTimingFunction(test_timing_function_data);
  auto type2 = timing_function_2->GetType();
  EXPECT_EQ(type2, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_3 = TimingFunctionType::kEaseInEaseOut;
  test_timing_function_data.timing_func = test_func_type_3;
  auto timing_function_3 = CreateTimingFunction(test_timing_function_data);
  auto type3 = timing_function_3->GetType();
  EXPECT_EQ(type3, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_4 = TimingFunctionType::kCubicBezier;
  test_timing_function_data.timing_func = test_func_type_4;
  auto timing_function_4 = CreateTimingFunction(test_timing_function_data);
  auto type4 = timing_function_4->GetType();
  EXPECT_EQ(type4, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_5 = TimingFunctionType::kSquareBezier;
  test_timing_function_data.timing_func = test_func_type_5;
  auto timing_function_5 = CreateTimingFunction(test_timing_function_data);
  auto type5 = timing_function_5->GetType();
  EXPECT_EQ(type5, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_6 = TimingFunctionType::kSteps;
  test_timing_function_data.timing_func = test_func_type_6;
  auto timing_function_6 = CreateTimingFunction(test_timing_function_data);
  auto type6 = timing_function_6->GetType();
  EXPECT_EQ(type6, TimingFunction::Type::STEPS);
}

TEST_F(TimingFunctionTest, MakeTimingFunction2) {
  auto test_timing_function_data = TimingFunctionData();
  auto test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_0 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type0 = timing_function_0->GetType();
  EXPECT_EQ(type0, TimingFunction::Type::LINEAR);

  auto test_func_type_1 = TimingFunctionType::kEaseIn;
  test_timing_function_data.timing_func = test_func_type_1;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_1 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type1 = timing_function_1->GetType();
  EXPECT_EQ(type1, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_2 = TimingFunctionType::kEaseOut;
  test_timing_function_data.timing_func = test_func_type_2;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_2 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type2 = timing_function_2->GetType();
  EXPECT_EQ(type2, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_3 = TimingFunctionType::kEaseInEaseOut;
  test_timing_function_data.timing_func = test_func_type_3;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_3 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type3 = timing_function_3->GetType();
  EXPECT_EQ(type3, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_4 = TimingFunctionType::kCubicBezier;
  test_timing_function_data.timing_func = test_func_type_4;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_4 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type4 = timing_function_4->GetType();
  EXPECT_EQ(type4, TimingFunction::Type::CUBIC_BEZIER);

  auto test_func_type_5 = TimingFunctionType::kSteps;
  test_timing_function_data.timing_func = test_func_type_5;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_5 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type5 = timing_function_5->GetType();
  EXPECT_EQ(type5, TimingFunction::Type::STEPS);

  auto test_func_type_6 = TimingFunctionType::kSquareBezier;
  test_timing_function_data.timing_func = test_func_type_6;
  test_animation_data = InitAnimationData(test_timing_function_data);
  auto timing_function_6 =
      CreateTimingFunction(test_animation_data.timing_func);
  auto type6 = timing_function_6->GetType();
  EXPECT_EQ(type6, TimingFunction::Type::CUBIC_BEZIER);
}

TEST_F(TimingFunctionTest, CreatePreset) {
  auto cubic_timing_function_0 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::CUSTOM);
  EXPECT_TRUE(cubic_timing_function_0 == nullptr);

  auto cubic_timing_function_1 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE);
  EXPECT_TRUE(cubic_timing_function_1 != nullptr);
  auto bezier1 = cubic_timing_function_1->bezier();
  EXPECT_NEAR(bezier1.GetX1(), 0.25, kEpsilon);
  EXPECT_NEAR(bezier1.GetY1(), 0.1, kEpsilon);
  EXPECT_NEAR(bezier1.GetX2(), 0.25, kEpsilon);
  EXPECT_NEAR(bezier1.GetY2(), 1.0, kEpsilon);

  auto cubic_timing_function_2 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE_IN);
  EXPECT_TRUE(cubic_timing_function_2 != nullptr);
  auto bezier2 = cubic_timing_function_2->bezier();
  EXPECT_NEAR(bezier2.GetX1(), 0.42, kEpsilon);
  EXPECT_NEAR(bezier2.GetY1(), 0.0, kEpsilon);
  EXPECT_NEAR(bezier2.GetX2(), 1.0, kEpsilon);
  EXPECT_NEAR(bezier2.GetY2(), 1.0, kEpsilon);

  auto cubic_timing_function_3 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE_OUT);
  EXPECT_TRUE(cubic_timing_function_3 != nullptr);
  auto bezier3 = cubic_timing_function_3->bezier();
  EXPECT_NEAR(bezier3.GetX1(), 0.0, kEpsilon);
  EXPECT_NEAR(bezier3.GetY1(), 0.0, kEpsilon);
  EXPECT_NEAR(bezier3.GetX2(), 0.58, kEpsilon);
  EXPECT_NEAR(bezier3.GetY2(), 1.0, kEpsilon);

  auto cubic_timing_function_4 = CubicBezierTimingFunction::CreatePreset(
      CubicBezierTimingFunction::EaseType::EASE_IN_OUT);
  EXPECT_TRUE(cubic_timing_function_4 != nullptr);
  auto bezier4 = cubic_timing_function_4->bezier();
  EXPECT_NEAR(bezier4.GetX1(), 0.42, kEpsilon);
  EXPECT_NEAR(bezier4.GetY1(), 0.0, kEpsilon);
  EXPECT_NEAR(bezier4.GetX2(), 0.58, kEpsilon);
  EXPECT_NEAR(bezier4.GetY2(), 1.0, kEpsilon);
}

TEST_F(TimingFunctionTest, CubicBezierTimingFunctionCreate) {
  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  auto bezier = test_cubic_timing_function->bezier();
  EXPECT_NEAR(bezier.GetX1(), 0.0, kEpsilon);
  EXPECT_NEAR(bezier.GetY1(), 0.25, kEpsilon);
  EXPECT_NEAR(bezier.GetX2(), 0.5, kEpsilon);
  EXPECT_NEAR(bezier.GetY2(), 1.0, kEpsilon);
  EXPECT_EQ(test_cubic_timing_function->ease_type(),
            CubicBezierTimingFunction::EaseType::CUSTOM);
  EXPECT_TRUE(test_cubic_timing_function->GetType() ==
              TimingFunction::Type::CUBIC_BEZIER);
}

TEST_F(TimingFunctionTest, SquareBezierTimingFunctionCreate) {
  auto test_square_timing_function =
      CubicBezierTimingFunction::CreateSquareBezier(1.0, 0.5);
  auto bezier = test_square_timing_function->bezier();
  EXPECT_NEAR(bezier.GetX1(), 2.0 / 3.0, kEpsilon);
  EXPECT_NEAR(bezier.GetY1(), 1.0 / 3.0, kEpsilon);
  EXPECT_NEAR(bezier.GetX2(), 1.0, kEpsilon);
  EXPECT_NEAR(bezier.GetY2(), 2.0 / 3.0, kEpsilon);
  EXPECT_EQ(test_square_timing_function->ease_type(),
            CubicBezierTimingFunction::EaseType::CUSTOM);
  EXPECT_EQ(test_square_timing_function->GetType(),
            TimingFunction::Type::CUBIC_BEZIER);
}

TEST_F(TimingFunctionTest, SquareBezierTimingFunctionDataCreate) {
  TimingFunctionData timing_function_data;
  timing_function_data.timing_func = TimingFunctionType::kSquareBezier;
  timing_function_data.x1 = 0.3;
  timing_function_data.y1 = 0.6;
  auto timing_function = CreateTimingFunction(timing_function_data);
  ASSERT_EQ(timing_function->GetType(), TimingFunction::Type::CUBIC_BEZIER);

  auto* cubic_timing_function =
      static_cast<CubicBezierTimingFunction*>(timing_function.get());
  auto bezier = cubic_timing_function->bezier();
  EXPECT_NEAR(bezier.GetX1(), 0.2, kEpsilon);
  EXPECT_NEAR(bezier.GetY1(), 0.4, kEpsilon);
  EXPECT_NEAR(bezier.GetX2(), 0.5333333333333333, kEpsilon);
  EXPECT_NEAR(bezier.GetY2(), 0.7333333333333334, kEpsilon);
}

TEST_F(TimingFunctionTest, LinearTimingFunctionCreate) {
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_TRUE(test_linear_timing_function->GetType() ==
              TimingFunction::Type::LINEAR);
}

TEST_F(TimingFunctionTest, StepsTimingFunctionCreate) {
  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, StepsType::kStart);
  EXPECT_EQ(test_steps_timing_function->steps(), 2);
  EXPECT_EQ(test_steps_timing_function->step_position(), StepsType::kStart);
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
      StepsTimingFunction::Create(2, StepsType::kStart);
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
      StepsTimingFunction::Create(2, StepsType::kStart);
  EXPECT_EQ(test_steps_timing_function->GetType(), TimingFunction::Type::STEPS);
}

TEST_F(TimingFunctionTest, GetValue) {
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_NEAR(test_linear_timing_function->GetValue(0.5), 0.5, kLooseEpsilon);

  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  EXPECT_NEAR(test_cubic_timing_function->GetValue(0.5), 0.7809, kLooseEpsilon);

  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, StepsType::kStart);
  EXPECT_NEAR(test_steps_timing_function->GetValue(0.5), 1.0, kLooseEpsilon);
}

TEST_F(TimingFunctionTest, Velocity) {
  auto test_linear_timing_function = LinearTimingFunction::Create();
  EXPECT_NEAR(test_linear_timing_function->Velocity(0.5), 0.0, kLooseEpsilon);

  auto test_cubic_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.25, 0.5, 1.0);
  EXPECT_NEAR(test_cubic_timing_function->Velocity(0.5), 0.8418, kLooseEpsilon);

  auto test_steps_timing_function =
      StepsTimingFunction::Create(2, StepsType::kStart);
  EXPECT_NEAR(test_steps_timing_function->Velocity(0.5), 0.0, kLooseEpsilon);
}

}  // namespace
}  // namespace gfx
}  // namespace lynx
