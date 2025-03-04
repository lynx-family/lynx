// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/utils/cubic_bezier.h"

#include <algorithm>
#include <cmath>
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

class CubicBezierTest : public ::testing::Test {
 public:
  CubicBezierTest() {}
  ~CubicBezierTest() override {}
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

TEST_F(CubicBezierTest, Create) {
  auto test_cubic = CubicBezier(0.0, 0.5, 1.0, 0.5);

  double epsilon = 1e-7;
  bool flag = abs(test_cubic.GetX1() - 0.0) < epsilon &&
              abs(test_cubic.GetY1() - 0.5) < epsilon &&
              abs(test_cubic.GetX2() - 1.0) < epsilon &&
              abs(test_cubic.GetY2() - 0.5) < epsilon;
  EXPECT_TRUE(flag);
}

TEST_F(CubicBezierTest, GetDefaultEpsilon) {
  double epsilon = 1e-7;
  bool flag = abs(CubicBezier::GetDefaultEpsilon() - 1e-7) < epsilon;
  EXPECT_TRUE(flag);
}

TEST_F(CubicBezierTest, SolveCurveX) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;

  double result1 = test_cubic.SolveCurveX(0.2, 1e-7);
  bool flag1 = abs(result1 - 0.3269672002845937) - epsilon;
  EXPECT_TRUE(flag1);

  double result2 = test_cubic.SolveCurveX(0.5, 1e-7);
  bool flag2 = abs(result2 - 0.6995724416969088) - epsilon;
  EXPECT_TRUE(flag2);

  double result3 = test_cubic.SolveCurveX(0.8, 1e-7);
  bool flag3 = abs(result3 - 0.90190872048750559) - epsilon;
  EXPECT_TRUE(flag3);

  double result4 = test_cubic.SolveCurveX(1.0, 1e-7);
  bool flag4 = abs(result4 - 1.0) - epsilon;
  EXPECT_TRUE(flag4);
}

TEST_F(CubicBezierTest, Solve) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;

  double result1 = test_cubic.Solve(0.2);
  bool flag1 = abs(result1 - 0.2952443342678806) - epsilon;
  EXPECT_TRUE(flag1);

  double result2 = test_cubic.Solve(0.5);
  bool flag2 = abs(result2 - 0.80240339105984371) - epsilon;
  EXPECT_TRUE(flag2);

  double result3 = test_cubic.Solve(0.8);
  bool flag3 = abs(result3 - 0.97562537385835967) - epsilon;
  EXPECT_TRUE(flag3);

  double result4 = test_cubic.Solve(1.0);
  bool flag4 = abs(result4 - 1.0) - epsilon;
  EXPECT_TRUE(flag4);
}

TEST_F(CubicBezierTest, SampleCurveX) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;
  double result = test_cubic.SampleCurveX(0.5);
  bool flag = abs(result - 0.3125) < epsilon;
  EXPECT_TRUE(flag);
}

TEST_F(CubicBezierTest, SampleCurveY) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;
  double result = test_cubic.SampleCurveY(0.5);
  bool flag = abs(result - 0.53750000000000009) < epsilon;
  EXPECT_TRUE(flag);
}

TEST_F(CubicBezierTest, SampleCurveDerivativeX) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;
  double result = test_cubic.SampleCurveDerivativeX(0.5);
  bool flag = abs(result - 0.75) < epsilon;
  EXPECT_TRUE(flag);
}

TEST_F(CubicBezierTest, SampleCurveDerivativeY) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;
  double result = test_cubic.SampleCurveDerivativeY(0.5);
  bool flag = abs(result - 1.425) < epsilon;
  EXPECT_TRUE(flag);
}

TEST_F(CubicBezierTest, SolveWithEpsilon) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;

  double result1 = test_cubic.SolveWithEpsilon(0.2, 1e-7);
  bool flag1 = abs(result1 - 0.2952443342678806) - epsilon;
  EXPECT_TRUE(flag1);

  double result2 = test_cubic.SolveWithEpsilon(0.5, 1e-7);
  bool flag2 = abs(result2 - 0.80240339105984371) - epsilon;
  EXPECT_TRUE(flag2);

  double result3 = test_cubic.SolveWithEpsilon(0.8, 1e-7);
  bool flag3 = abs(result3 - 0.97562537385835967) - epsilon;
  EXPECT_TRUE(flag3);

  double result4 = test_cubic.SolveWithEpsilon(1.0, 1e-7);
  bool flag4 = abs(result4 - 1.0) - epsilon;
  EXPECT_TRUE(flag4);
}

TEST_F(CubicBezierTest, SlopeWithEpsilon) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;

  double result1 = test_cubic.SlopeWithEpsilon(0.2, 1e-7);
  bool flag1 = abs(result1 - 2.2820580674389523) - epsilon;
  EXPECT_TRUE(flag1);

  double result2 = test_cubic.SlopeWithEpsilon(0.5, 1e-7);
  bool flag2 = abs(result2 - 0.99414243958918846) - epsilon;
  EXPECT_TRUE(flag2);

  double result3 = test_cubic.SlopeWithEpsilon(0.8, 1e-7);
  bool flag3 = abs(result3 - 0.26156898519284688) - epsilon;
  EXPECT_TRUE(flag3);

  double result4 = test_cubic.SlopeWithEpsilon(1.0, 1e-7);
  bool flag4 = abs(result4 - 0.0) - epsilon;
  EXPECT_TRUE(flag4);
}

TEST_F(CubicBezierTest, Slope) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-7;

  double result1 = test_cubic.Slope(0.2);
  bool flag1 = abs(result1 - 2.2820580674389523) - epsilon;
  EXPECT_TRUE(flag1);

  double result2 = test_cubic.Slope(0.5);
  bool flag2 = abs(result2 - 0.99414243958918846) - epsilon;
  EXPECT_TRUE(flag2);

  double result3 = test_cubic.Slope(0.8);
  bool flag3 = abs(result3 - 0.26156898519284688) - epsilon;
  EXPECT_TRUE(flag3);

  double result4 = test_cubic.Slope(1.0);
  bool flag4 = abs(result4 - 0.0) - epsilon;
  EXPECT_TRUE(flag4);
}

TEST_F(CubicBezierTest, Get) {
  auto test_cubic = CubicBezier(0.25, 0.1, 0.25, 1.0);

  double epsilon = 1e-6;
  bool flag1 = abs(test_cubic.GetX1() - 0.25) < epsilon;
  bool flag2 = abs(test_cubic.GetY1() - 0.1) < epsilon;
  bool flag3 = abs(test_cubic.GetX2() - 0.25) < epsilon;
  bool flag4 = abs(test_cubic.GetY2() - 1.0) < epsilon;
  EXPECT_TRUE(flag1 && flag2 && flag3 && flag4);
}

}  // namespace testing
}  // namespace tasm
}  // namespace animation
}  // namespace lynx
