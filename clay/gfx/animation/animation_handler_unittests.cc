// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "clay/gfx/animation/animation_handler.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

namespace {
class MockAnimationFrameCallback
    : public AnimationHandler::AnimationFrameCallback {
 public:
  MOCK_METHOD(bool, DoAnimationFrame, (int64_t frame_time, bool update_values),
              (override));
};

class LifecycleAnimationFrameCallback
    : public AnimationHandler::AnimationFrameCallback {
 public:
  explicit LifecycleAnimationFrameCallback(int64_t next_lifecycle_time)
      : next_lifecycle_time_(next_lifecycle_time) {}

  MOCK_METHOD(bool, DoAnimationFrame, (int64_t frame_time, bool update_values),
              (override));

  bool ShouldReceiveAnimationFrame(int64_t current_time,
                                   int64_t* next_lifecycle_time) override {
    last_current_time_ = current_time;
    if (next_lifecycle_time) {
      *next_lifecycle_time = next_lifecycle_time_;
    }
    return false;
  }

  int64_t last_current_time() const { return last_current_time_; }

 private:
  int64_t next_lifecycle_time_;
  int64_t last_current_time_ = -1;
};

}  // namespace

using ::testing::InSequence;
using ::testing::Return;

TEST(AnimationHandlerTest, NoActiveAnimation) {
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  int64_t frame_time = 0;
  for (size_t i = 0; i < 10; i++) {
    frame_time += 16;
    handler->DoAnimationFrame(frame_time);
    EXPECT_EQ(handler->GetLastAnimationFrameTime(), frame_time);
  }

  EXPECT_EQ(handler->GetAnimationCount(), 0);
}

TEST(AnimationHandlerTest, AnimationFrameCallback) {
  MockAnimationFrameCallback anim1;
  MockAnimationFrameCallback anim2;

  int64_t frame_time = 0;
  {
    InSequence s;

    for (int i = 1; i <= 10; i++) {
      frame_time += 16;
      EXPECT_CALL(anim1, DoAnimationFrame(frame_time, true))
          .Times(1)
          .RetiresOnSaturation();
    }
  }

  frame_time = 0;
  {
    InSequence s;

    for (int i = 1; i <= 10; i++) {
      frame_time += 16;
      if (i >= 6) {
        EXPECT_CALL(anim2, DoAnimationFrame(frame_time, true))
            .Times(1)
            .RetiresOnSaturation();
      }
    }
  }

  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  EXPECT_EQ(handler->GetAnimationCount(), 0);
  handler->AddAnimationFrameCallback(&anim1, 0);
  EXPECT_EQ(handler->GetAnimationCount(), 1);

  frame_time = 0;
  for (size_t i = 1; i <= 10; i++) {
    if (i == 6) {
      handler->AddAnimationFrameCallback(&anim2, 0);
      EXPECT_EQ(handler->GetAnimationCount(), 2);
    }
    frame_time += 16;
    handler->DoAnimationFrame(frame_time);
  }
  EXPECT_EQ(handler->GetAnimationCount(), 2);
  handler->RemoveCallback(&anim1);
  handler->RemoveCallback(&anim2);
  EXPECT_EQ(handler->GetAnimationCount(), 0);
}

TEST(AnimationHandlerTest, DelayIsAnchoredOnFirstFrameTime) {
  MockAnimationFrameCallback anim;
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();

  handler->AddAnimationFrameCallback(&anim, 1000);

  EXPECT_CALL(anim, DoAnimationFrame(::testing::_, ::testing::_)).Times(0);
  handler->DoAnimationFrame(10000);
  handler->DoAnimationFrame(10999);
  ::testing::Mock::VerifyAndClearExpectations(&anim);

  EXPECT_CALL(anim, DoAnimationFrame(11000, true)).WillOnce(Return(false));
  handler->DoAnimationFrame(11000);
}

TEST(AnimationHandlerTest, LifecycleCallbackUsesFrameTime) {
  LifecycleAnimationFrameCallback anim(100);
  std::vector<int64_t> scheduled_delays;
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  handler->SetAnimationCallback([&scheduled_delays](int64_t delay) {
    scheduled_delays.push_back(delay);
  });
  handler->AddAnimationFrameCallback(&anim, 0);

  EXPECT_CALL(anim, DoAnimationFrame(::testing::_, ::testing::_)).Times(0);
  handler->DoAnimationFrame(90);
  ASSERT_FALSE(scheduled_delays.empty());
  EXPECT_EQ(scheduled_delays.back(), 10);
  EXPECT_EQ(anim.last_current_time(), 90);
  ::testing::Mock::VerifyAndClearExpectations(&anim);

  EXPECT_CALL(anim, DoAnimationFrame(110, false)).WillOnce(Return(false));
  handler->DoAnimationFrame(110);
}

TEST(AnimationHandlerTest, RemovedCallbackInvalidatesLifecycleSchedule) {
  LifecycleAnimationFrameCallback anim(100);
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  handler->SetAnimationCallback([](int64_t delay) {});
  handler->AddAnimationFrameCallback(&anim, 0);

  EXPECT_CALL(anim, DoAnimationFrame(::testing::_, ::testing::_)).Times(0);
  handler->DoAnimationFrame(90);
  const uint64_t schedule_id = handler->GetLifecycleScheduleId();
  EXPECT_TRUE(handler->IsLifecycleScheduleCurrent(schedule_id));
  EXPECT_TRUE(handler->IsLifecycleCallbackDue(100, schedule_id));

  handler->RemoveCallback(&anim);
  EXPECT_FALSE(handler->IsLifecycleScheduleCurrent(schedule_id));
  EXPECT_FALSE(handler->IsLifecycleCallbackDue(100, schedule_id));
  EXPECT_FALSE(
      handler->IsLifecycleScheduleCurrent(handler->GetLifecycleScheduleId()));
}

TEST(AnimationHandlerTest, RemovingEarlierCallbackReschedulesRemainingOne) {
  const int64_t current_time =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  const int64_t earlier_time = current_time + 1000;
  const int64_t later_time = current_time + 2000;
  LifecycleAnimationFrameCallback earlier_anim(earlier_time);
  LifecycleAnimationFrameCallback later_anim(later_time);
  std::vector<int64_t> scheduled_delays;
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  handler->SetAnimationCallback([&scheduled_delays](int64_t delay) {
    scheduled_delays.push_back(delay);
  });
  handler->AddAnimationFrameCallback(&earlier_anim, 0);
  handler->AddAnimationFrameCallback(&later_anim, 0);

  handler->DoAnimationFrame(current_time);
  ASSERT_EQ(scheduled_delays.size(), 1U);
  EXPECT_EQ(scheduled_delays.back(), 1000);
  const uint64_t old_schedule_id = handler->GetLifecycleScheduleId();

  handler->RemoveCallback(&earlier_anim);
  ASSERT_EQ(scheduled_delays.size(), 2U);
  EXPECT_GT(scheduled_delays.back(), 0);
  EXPECT_LE(scheduled_delays.back(), 2000);
  const uint64_t new_schedule_id = handler->GetLifecycleScheduleId();

  EXPECT_NE(new_schedule_id, old_schedule_id);
  EXPECT_FALSE(handler->IsLifecycleScheduleCurrent(old_schedule_id));
  EXPECT_TRUE(handler->IsLifecycleScheduleCurrent(new_schedule_id));
  EXPECT_TRUE(handler->IsLifecycleCallbackDue(later_time, new_schedule_id));
}

TEST(AnimationHandlerTest, EarlierDeadlineReplacesLifecycleSchedule) {
  LifecycleAnimationFrameCallback later_anim(200);
  LifecycleAnimationFrameCallback earlier_anim(150);
  std::vector<int64_t> scheduled_delays;
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  handler->SetAnimationCallback([&scheduled_delays](int64_t delay) {
    scheduled_delays.push_back(delay);
  });
  handler->AddAnimationFrameCallback(&later_anim, 0);

  handler->DoAnimationFrame(100);
  ASSERT_EQ(scheduled_delays.size(), 1U);
  EXPECT_EQ(scheduled_delays.back(), 100);
  const uint64_t old_schedule_id = handler->GetLifecycleScheduleId();

  handler->AddAnimationFrameCallback(&earlier_anim, 0);
  handler->ScheduleLifecycleCallback(100);
  ASSERT_EQ(scheduled_delays.size(), 2U);
  EXPECT_EQ(scheduled_delays.back(), 50);
  const uint64_t new_schedule_id = handler->GetLifecycleScheduleId();

  EXPECT_NE(new_schedule_id, old_schedule_id);
  EXPECT_FALSE(handler->IsLifecycleScheduleCurrent(old_schedule_id));
  EXPECT_FALSE(handler->IsLifecycleCallbackDue(200, old_schedule_id));
  EXPECT_TRUE(handler->IsLifecycleScheduleCurrent(new_schedule_id));
  EXPECT_FALSE(handler->IsLifecycleCallbackDue(149, new_schedule_id));
  EXPECT_TRUE(handler->IsLifecycleCallbackDue(150, new_schedule_id));
}

}  // namespace testing
}  // namespace clay
