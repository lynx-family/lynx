// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/animator.h"
#include "clay/ui/gesture_handler/handler/fling_scroller.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

class DummyAnimator : public Animator {
 public:
  void Start() override {}
  void Cancel() override {}
  void End() override {}
  int64_t GetStartDelay() override { return 0; }
  void SetStartDelay(int64_t) override {}
  void SetDuration(int64_t) override {}
  int64_t GetDuration() override { return 0; }
  void SetInterpolator(std::unique_ptr<Interpolator>) override {}
  bool IsRunning() override { return false; }
};

class FlingScrollerTest : public ::testing::Test {};

TEST_F(FlingScrollerTest, Start_SetsFlingStateAndInvokesCallback) {
  AnimationHandler animation_handler;
  FlingScroller scroller(&animation_handler);

  std::vector<uint8_t> states;
  scroller.Start(1000, 0,
                 [&](uint8_t s, float, float) { states.push_back(s); });

  for (int64_t t = 0; t <= 64; t += 16) {
    animation_handler.DoAnimationFrame(t);
  }

  EXPECT_FALSE(states.empty());
  EXPECT_TRUE(std::find(states.begin(), states.end(),
                        static_cast<uint8_t>(
                            FlingScroller::FlingState::FLING)) != states.end());
}

TEST_F(FlingScrollerTest, Stop_SetsIdle) {
  AnimationHandler animation_handler;
  FlingScroller scroller(&animation_handler);
  scroller.Start(1000, 0, [&](uint8_t, float, float) {});
  scroller.Stop();
  EXPECT_TRUE(scroller.IsIdle());
}

TEST_F(FlingScrollerTest, AnimationEnd_InvokesIdleCallback) {
  AnimationHandler animation_handler;
  FlingScroller scroller(&animation_handler);

  std::vector<uint8_t> states;
  scroller.Start(1000, 0,
                 [&](uint8_t s, float, float) { states.push_back(s); });

  DummyAnimator anim;
  scroller.OnAnimationEnd(anim);

  EXPECT_FALSE(states.empty());
  EXPECT_EQ(states.back(),
            static_cast<uint8_t>(FlingScroller::FlingState::IDLE));
}

TEST_F(FlingScrollerTest, AnimationCancel_InvokesIdleCallback) {
  AnimationHandler animation_handler;
  FlingScroller scroller(&animation_handler);

  std::vector<uint8_t> states;
  scroller.Start(1000, 0,
                 [&](uint8_t s, float, float) { states.push_back(s); });

  DummyAnimator anim;
  scroller.OnAnimationCancel(anim);

  EXPECT_FALSE(states.empty());
  EXPECT_EQ(states.back(),
            static_cast<uint8_t>(FlingScroller::FlingState::IDLE));
}
}  // namespace testing

}  // namespace clay
