// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "clay/gfx/animation/animator_listener_adapter.h"
#include "clay/gfx/animation/value_animator.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

namespace {
class MockAnimatorUpdateListener : public AnimatorUpdateListener {
 public:
  // NOLINTNEXTLINE
  MOCK_METHOD(void, OnAnimationUpdate, (ValueAnimator & animation), (override));
};

class ReentrantEndListener : public AnimatorListenerAdapter {
 public:
  explicit ReentrantEndListener(ValueAnimator* animator)
      : animator_(animator) {}

  void OnAnimationEnd(Animator& animation) override {
    end_count_++;
    animator_->End();
  }

  int end_count() const { return end_count_; }

 private:
  ValueAnimator* animator_;
  int end_count_ = 0;
};

class RecordingAnimatorListener : public AnimatorListenerAdapter {
 public:
  void OnAnimationStart(Animator& animation) override { start_count_++; }
  void OnAnimationEnd(Animator& animation) override { end_count_++; }

  int start_count() const { return start_count_; }
  int end_count() const { return end_count_; }

 private:
  int start_count_ = 0;
  int end_count_ = 0;
};

class RecordingFractionListener : public AnimatorUpdateListener {
 public:
  void OnAnimationUpdate(ValueAnimator& animation) override {
    update_count_++;
    fraction_ = animation.GetAnimatedFraction();
  }

  int update_count() const { return update_count_; }
  float fraction() const { return fraction_; }

 private:
  int update_count_ = 0;
  float fraction_ = -1.f;
};
}  // namespace

TEST(ValueAnimatorTest, AnimatorUpdateEvents) {
  MockAnimatorUpdateListener update_listener;
  EXPECT_CALL(update_listener, OnAnimationUpdate(::testing::_)).Times(6);

  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  EXPECT_EQ(handler->GetAnimationCount(), 0);

  ValueAnimator animator;
  animator.SetDuration(160);
  animator.AddUpdateListener(&update_listener);

  animator.SetAnimationHandler(handler.get());
  animator.Start();
  EXPECT_EQ(handler->GetAnimationCount(), 1);

  int64_t frame_time = 0;
  for (size_t i = 1; i <= 5; i++) {
    frame_time += 16;
    handler->DoAnimationFrame(frame_time);
  }

  EXPECT_EQ(handler->GetAnimationCount(), 1);
  animator.RemoveAllUpdateListeners();
  animator.End();
  EXPECT_EQ(handler->GetAnimationCount(), 0);
}

TEST(ValueAnimatorTest, ForwardsFillDoesNotRequestFrameAfterVisibleEnd) {
  MockAnimatorUpdateListener update_listener;
  EXPECT_CALL(update_listener, OnAnimationUpdate(::testing::_)).Times(4);

  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();

  ValueAnimator animator;
  animator.SetAnimationHandler(handler.get());
  animator.SetDuration(16);
  animator.SetFillMode(ValueAnimator::kForwards);
  animator.AddUpdateListener(&update_listener);
  animator.Start();

  EXPECT_EQ(handler->GetAnimationCount(), 1);
  handler->DoAnimationFrame(0);
  EXPECT_EQ(handler->GetAnimationCount(), 1);
  EXPECT_FALSE(handler->DoAnimationFrame(16));
  EXPECT_EQ(handler->GetAnimationCount(), 1);
  EXPECT_FALSE(handler->DoAnimationFrame(32));
  EXPECT_EQ(handler->GetAnimationCount(), 1);
}

TEST(ValueAnimatorTest, EndListenerFlagIsSetBeforeCallback) {
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();

  ValueAnimator animator;
  animator.SetAnimationHandler(handler.get());
  animator.SetDuration(16);
  ReentrantEndListener listener(&animator);
  animator.AddListener(&listener);

  animator.Start();
  handler->DoAnimationFrame(0);
  handler->DoAnimationFrame(16);

  EXPECT_EQ(listener.end_count(), 1);
}

TEST(ValueAnimatorTest, BackwardsFillDoesNotStartDuringDelay) {
  for (ValueAnimator::FillMode fill_mode :
       {ValueAnimator::kBackward, ValueAnimator::kBoth}) {
    SCOPED_TRACE(fill_mode);
    ValueAnimator animator;
    animator.SetDuration(100);
    animator.SetStartDelay(50);
    animator.SetFillMode(fill_mode);

    RecordingAnimatorListener lifecycle_listener;
    RecordingFractionListener fraction_listener;
    animator.AddListener(&lifecycle_listener);
    animator.AddUpdateListener(&fraction_listener);
    animator.Start();

    animator.DoAnimationFrame(1000);
    EXPECT_FALSE(animator.IsRunning());
    EXPECT_EQ(lifecycle_listener.start_count(), 0);
    EXPECT_EQ(fraction_listener.update_count(), 1);
    EXPECT_FLOAT_EQ(fraction_listener.fraction(), 0.f);

    animator.DoAnimationFrame(1049);
    EXPECT_FALSE(animator.IsRunning());
    EXPECT_EQ(lifecycle_listener.start_count(), 0);
    EXPECT_FLOAT_EQ(fraction_listener.fraction(), 0.f);

    animator.DoAnimationFrame(1050);
    EXPECT_TRUE(animator.IsRunning());
    EXPECT_EQ(lifecycle_listener.start_count(), 1);
    EXPECT_FLOAT_EQ(fraction_listener.fraction(), 0.f);

    animator.DoAnimationFrame(1150);
    EXPECT_EQ(lifecycle_listener.start_count(), 1);
    EXPECT_EQ(lifecycle_listener.end_count(), 1);
  }
}

TEST(ValueAnimatorTest, BackwardsFillDoesNotStartWhenValuesAreSkipped) {
  ValueAnimator animator;
  animator.SetDuration(100);
  animator.SetStartDelay(50);
  animator.SetFillMode(ValueAnimator::kBoth);

  RecordingAnimatorListener lifecycle_listener;
  animator.AddListener(&lifecycle_listener);
  animator.Start();

  animator.DoAnimationFrame(1000, false);
  EXPECT_FALSE(animator.IsRunning());
  EXPECT_EQ(lifecycle_listener.start_count(), 0);

  animator.DoAnimationFrame(1050, false);
  EXPECT_TRUE(animator.IsRunning());
  EXPECT_EQ(lifecycle_listener.start_count(), 1);

  animator.DoAnimationFrame(1150, false);
  EXPECT_EQ(lifecycle_listener.start_count(), 1);
  EXPECT_EQ(lifecycle_listener.end_count(), 1);
}

TEST(ValueAnimatorTest, LifecycleOnlyModeSkipsPerFrameValueUpdates) {
  AnimationHandler handler;
  std::vector<int64_t> requested_delays;
  handler.SetAnimationCallback([&requested_delays](int64_t delay) {
    requested_delays.push_back(delay);
  });

  ValueAnimator animator;
  animator.SetAnimationHandler(&handler);
  animator.SetDuration(100);
  animator.SetFrameUpdateMode(ValueAnimator::FrameUpdateMode::kLifecycleOnly);

  RecordingAnimatorListener lifecycle_listener;
  RecordingFractionListener fraction_listener;
  animator.AddListener(&lifecycle_listener);
  animator.AddUpdateListener(&fraction_listener);
  animator.Start();

  EXPECT_EQ(lifecycle_listener.start_count(), 1);
  EXPECT_EQ(fraction_listener.update_count(), 0);
  ASSERT_EQ(requested_delays.size(), 1u);
  EXPECT_EQ(requested_delays.front(), -1);

  EXPECT_FALSE(handler.DoAnimationFrame(1000));
  EXPECT_EQ(fraction_listener.update_count(), 0);
  ASSERT_EQ(requested_delays.size(), 2u);
  EXPECT_EQ(requested_delays.back(), 100);

  EXPECT_FALSE(handler.DoAnimationFrame(1050));
  EXPECT_EQ(fraction_listener.update_count(), 0);
  EXPECT_EQ(lifecycle_listener.end_count(), 0);

  EXPECT_FALSE(handler.DoAnimationFrame(1100));
  EXPECT_EQ(fraction_listener.update_count(), 0);
  EXPECT_EQ(lifecycle_listener.end_count(), 1);
}

}  // namespace testing
}  // namespace clay
