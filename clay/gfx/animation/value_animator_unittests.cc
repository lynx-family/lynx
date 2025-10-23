// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

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

}  // namespace testing
}  // namespace clay
