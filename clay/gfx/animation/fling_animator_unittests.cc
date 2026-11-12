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

TEST(FlingAnimatorTest, SplineDistanceAndDirectionRemainUnchanged) {
  for (float velocity : {-1000.f, 1000.f}) {
    AnimationHandler handler;
    TestFlingAnimator animator;
    animator.Initialize(&handler, velocity);
    const float coefficient = 9.80665f * 39.37f * 2.75f * 160.f * 0.015f;
    const float rate = std::log(0.75f) / std::log(0.9f);
    const float distance =
        coefficient *
        std::exp(rate / (rate - 1.f) * std::log(350.f / coefficient));
    EXPECT_NEAR(std::abs(animator.GetDistance()), distance, 0.001);
    EXPECT_FALSE(animator.DoAnimationFrame(1100));
    EXPECT_GT(animator.GetValue() * velocity, 0.f);
    EXPECT_GT(animator.GetCurrentVelocity() * velocity, 0.f);
  }
}

}  // namespace testing
}  // namespace clay
