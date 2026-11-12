// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>

#include "clay/gfx/animation/fling_animator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

class TestFlingAnimator : public FlingAnimator {
 public:
  void Initialize(AnimationHandler* handler, float velocity, float start = 0) {
    SetAnimationHandler(handler);
    SetFriction(1.f);
    SetStartValue(start);
    SetStartVelocity(velocity);
    FlingInitialize();
    Start();
    // Keep the production lifecycle, with a deterministic frame clock.
    start_time_ = last_frame_time_ = 1000;
  }
};

TEST(FlingAnimatorTest, IOSNormalDeceleration) {
  for (float velocity : {100.f, 1000.f, 5000.f, -100.f, -1000.f}) {
    AnimationHandler handler;
    TestFlingAnimator animator;
    animator.SetDevicePixelRatio(3.f);
    animator.Initialize(&handler, velocity, 100.f);
    EXPECT_NEAR(animator.GetDistance(), velocity * 0.49938064, 0.001);
    EXPECT_FALSE(animator.DoAnimationFrame(2000));
    EXPECT_NEAR(animator.GetCurrentVelocity(), velocity * 0.135, 0.001);
    EXPECT_NEAR(animator.GetValue(), 100.f + velocity * 0.431964256, 0.002);
  }
}

TEST(FlingAnimatorTest, IOSFrameCadenceDoesNotChangeTrajectory) {
  AnimationHandler handler;
  TestFlingAnimator frequent, delayed;
  frequent.Initialize(&handler, 1000.f);
  delayed.Initialize(&handler, 1000.f);
  for (int time = 1008; time < 2000; time += 8) {
    EXPECT_FALSE(frequent.DoAnimationFrame(time));
  }
  frequent.DoAnimationFrame(2000);
  delayed.DoAnimationFrame(2000);
  EXPECT_FLOAT_EQ(frequent.GetValue(), delayed.GetValue());
  EXPECT_FLOAT_EQ(frequent.GetCurrentVelocity(), delayed.GetCurrentVelocity());
}

TEST(FlingAnimatorTest, IOSStopsAtVisiblePixelToleranceWithoutEndpointJump) {
  for (float dpr : {1.f, 2.f, 3.f}) {
    AnimationHandler handler;
    TestFlingAnimator animator;
    animator.SetDevicePixelRatio(dpr);
    animator.Initialize(&handler, 50.f);
    const int stop_ms = static_cast<int>(
        std::ceil(std::log((20.0 / dpr) / 50.0) / std::log(0.135) * 1000));
    EXPECT_FALSE(animator.DoAnimationFrame(1000 + stop_ms - 1));
    const float before = animator.GetValue();
    EXPECT_TRUE(animator.DoAnimationFrame(1000 + stop_ms));
    EXPECT_LT(animator.GetCurrentVelocity(), 20.f / dpr);
    EXPECT_LT(animator.GetValue() - before, 0.03f);
    EXPECT_LT(animator.GetValue(), animator.GetDistance());
    EXPECT_EQ(handler.GetAnimationCount(), 0);
  }
}

TEST(FlingAnimatorTest, IOSBoundaryRetainsVelocityForBounce) {
  for (float direction : {-1.f, 1.f}) {
    AnimationHandler handler;
    TestFlingAnimator animator;
    animator.SetMinValue(-100.f);
    animator.SetMaxValue(100.f);
    animator.Initialize(&handler, direction * 1000.f);
    EXPECT_TRUE(animator.DoAnimationFrame(1200));
    EXPECT_FLOAT_EQ(animator.GetValue(), direction * 100.f);
    EXPECT_GT(animator.GetCurrentVelocity() * direction, 600.f);
  }
}

TEST(FlingAnimatorTest, IOSZeroVelocityAndReverseRestart) {
  AnimationHandler handler;
  TestFlingAnimator animator;
  animator.Initialize(&handler, 0.f, 20.f);
  EXPECT_TRUE(animator.DoAnimationFrame(1000));
  EXPECT_FLOAT_EQ(animator.GetDistance(), 0.f);
  EXPECT_FLOAT_EQ(animator.GetValue(), 20.f);
  animator.Initialize(&handler, 1000.f);
  animator.DoAnimationFrame(1100);
  animator.Cancel();
  animator.Initialize(&handler, -1000.f, 200.f);
  EXPECT_FALSE(animator.DoAnimationFrame(2000));
  EXPECT_NEAR(animator.GetValue(), -231.964256, 0.002);
  EXPECT_NEAR(animator.GetCurrentVelocity(), -135.f, 0.001);
}

}  // namespace testing
}  // namespace clay
