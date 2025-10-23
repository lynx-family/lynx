// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/gfx/animation/animation_handler.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

namespace {
class MockAnimationFrameCallback
    : public AnimationHandler::AnimationFrameCallback {
 public:
  MOCK_METHOD(bool, DoAnimationFrame, (int64_t frame_time), (override));
};
}  // namespace

using ::testing::InSequence;

TEST(AnimationHandlerTest, NoActiveAnimation) {
  std::unique_ptr<AnimationHandler> handler =
      std::make_unique<AnimationHandler>();
  int64_t frame_time = 0;
  for (size_t i = 0; i < 10; i++) {
    frame_time += 16;
    handler->DoAnimationFrame(frame_time);
    EXPECT_EQ(handler->GetCurrentAnimationTime(), frame_time);
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
      EXPECT_CALL(anim1, DoAnimationFrame(frame_time))
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
        EXPECT_CALL(anim2, DoAnimationFrame(frame_time))
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

}  // namespace testing
}  // namespace clay
