// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/clay_input_event_target.h"

#include <initializer_list>
#include <limits>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

using devtool::input::Pointer;
using devtool::input::PointerEvent;
using devtool::input::PointerEventType;
using devtool::input::PointerSourceType;

PointerEvent Event(PointerEventType type, float x = 10.25f, float y = 20.5f,
                   int id = 7) {
  PointerEvent event;
  event.type = type;
  event.source_type = PointerSourceType::kMouse;
  event.action_pointer_id = id;
  event.timestamp_us = 123456;
  event.buttons =
      type == PointerEventType::kDown || type == PointerEventType::kMove
          ? devtool::input::kPrimaryButton
          : devtool::input::kNoButton;
  Pointer pointer;
  pointer.id = id;
  pointer.x = x;
  pointer.y = y;
  event.pointers.push_back(pointer);
  return event;
}

Pointer TouchPointer(int id, float x, float y) {
  Pointer pointer;
  pointer.id = id;
  pointer.x = x;
  pointer.y = y;
  return pointer;
}

PointerEvent TouchEvent(PointerEventType type,
                        std::initializer_list<Pointer> pointers,
                        int action_pointer_id) {
  PointerEvent event;
  event.type = type;
  event.source_type = PointerSourceType::kTouch;
  event.action_pointer_id = action_pointer_id;
  event.timestamp_us = 123456;
  event.buttons = type == PointerEventType::kDown
                      ? devtool::input::kPrimaryButton
                      : devtool::input::kNoButton;
  event.pointers.assign(pointers);
  return event;
}

class ClayInputEventTargetTest : public ::testing::Test {
 protected:
  ClayInputViewport viewport_{100, 200, 2};
  std::vector<ClayPointerEvent> events_;
  ClayInputEventTarget target_{
      [this]() { return viewport_; },
      [this](const ClayPointerEvent& event) { events_.push_back(event); }};
};

TEST_F(ClayInputEventTargetTest, AdvertisesMouseAndTouchForValidViewport) {
  auto capabilities = target_.GetPointerCapabilities();
  EXPECT_EQ(capabilities.default_source_type, PointerSourceType::kMouse);
  EXPECT_TRUE(capabilities.supports_mouse);
  EXPECT_TRUE(capabilities.supports_touch);
  EXPECT_EQ(capabilities.max_touch_points, 2);
  viewport_.pixel_ratio = 0;
  EXPECT_FALSE(target_.GetPointerCapabilities().supports_mouse);
  EXPECT_FALSE(target_.GetPointerCapabilities().supports_touch);
  viewport_ = {100, 0, 1};
  EXPECT_FALSE(target_.GetPointerCapabilities().supports_mouse);
  viewport_ = {100, 200, std::numeric_limits<float>::infinity()};
  EXPECT_FALSE(target_.GetPointerCapabilities().supports_mouse);
}

TEST_F(ClayInputEventTargetTest, ScalesFractionalViewCoordinatesOnce) {
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  ASSERT_TRUE(
      target_.InjectPointerEvent(Event(PointerEventType::kMove, 11.5, 25)));
  ASSERT_TRUE(
      target_.InjectPointerEvent(Event(PointerEventType::kUp, 11.5, 25)));
  ASSERT_EQ(events_.size(), 4u);
  EXPECT_EQ(events_[0].phase, kClayPointerPhaseAdd);
  EXPECT_EQ(events_[1].phase, kClayPointerPhaseDown);
  EXPECT_EQ(events_[2].phase, kClayPointerPhaseMove);
  EXPECT_EQ(events_[3].phase, kClayPointerPhaseUp);
  EXPECT_DOUBLE_EQ(events_[1].x, 20.5);
  EXPECT_DOUBLE_EQ(events_[1].y, 41);
  EXPECT_DOUBLE_EQ(events_[2].x, 23);
  EXPECT_DOUBLE_EQ(events_[2].y, 50);
  EXPECT_EQ(events_[1].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(events_[2].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(events_[3].buttons, 0);
  for (const auto& event : events_) {
    EXPECT_EQ(event.struct_size, sizeof(ClayPointerEvent));
    EXPECT_EQ(event.device_kind, kClayPointerDeviceKindMouse);
    EXPECT_EQ(event.device, events_[0].device);
    EXPECT_LT(event.device, 0);
    EXPECT_EQ(event.timestamp, 123456u);
  }
}

TEST_F(ClayInputEventTargetTest, RepeatedTapsRetainMouseDevice) {
  for (int id : {7, 8}) {
    ASSERT_TRUE(
        target_.InjectPointerEvent(Event(PointerEventType::kDown, 10, 20, id)));
    ASSERT_TRUE(
        target_.InjectPointerEvent(Event(PointerEventType::kUp, 10, 20, id)));
  }
  ASSERT_EQ(events_.size(), 5u);
  EXPECT_EQ(events_[3].phase, kClayPointerPhaseDown);
  EXPECT_EQ(events_[4].phase, kClayPointerPhaseUp);
  target_.Invalidate();
  ASSERT_EQ(events_.size(), 6u);
  EXPECT_EQ(events_.back().phase, kClayPointerPhaseRemove);
}

TEST_F(ClayInputEventTargetTest,
       CancelUsesLastPositionAndNeverReleasesAsClick) {
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  ASSERT_TRUE(target_.InjectPointerEvent(Event(
      PointerEventType::kCancel, std::numeric_limits<float>::quiet_NaN(), -1)));
  ASSERT_EQ(events_.size(), 4u);
  EXPECT_EQ(events_[2].phase, kClayPointerPhaseCancel);
  EXPECT_DOUBLE_EQ(events_[2].x, 20.5);
  EXPECT_DOUBLE_EQ(events_[2].y, 41);
  EXPECT_EQ(events_[3].phase, kClayPointerPhaseRemove);
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  EXPECT_EQ(events_[4].phase, kClayPointerPhaseAdd);
}

TEST_F(ClayInputEventTargetTest, InjectsTwoTouchPointers) {
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown, {TouchPointer(10, 20, 50)}, 10)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown,
                 {TouchPointer(10, 20, 50), TouchPointer(11, 80, 50)}, 11)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kMove,
                 {TouchPointer(10, 10, 50), TouchPointer(11, 90, 50)}, 10)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kUp,
                 {TouchPointer(10, 10, 50), TouchPointer(11, 90, 50)}, 11)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kUp, {TouchPointer(10, 10, 50)}, 10)));

  ASSERT_EQ(events_.size(), 10u);
  const ClayPointerPhase expected_phases[] = {
      kClayPointerPhaseAdd,    kClayPointerPhaseDown,   kClayPointerPhaseAdd,
      kClayPointerPhaseDown,   kClayPointerPhaseMove,   kClayPointerPhaseMove,
      kClayPointerPhaseUp,     kClayPointerPhaseRemove, kClayPointerPhaseUp,
      kClayPointerPhaseRemove,
  };
  for (std::size_t index = 0; index < events_.size(); ++index) {
    EXPECT_EQ(events_[index].phase, expected_phases[index]);
    EXPECT_EQ(events_[index].device_kind, kClayPointerDeviceKindTouch);
    EXPECT_EQ(events_[index].buttons, 0);
  }
  EXPECT_EQ(events_[0].device, events_[1].device);
  EXPECT_EQ(events_[2].device, events_[3].device);
  EXPECT_NE(events_[0].device, events_[2].device);
  EXPECT_DOUBLE_EQ(events_[1].x, 40);
  EXPECT_DOUBLE_EQ(events_[3].x, 160);
  EXPECT_DOUBLE_EQ(events_[4].x, 20);
  EXPECT_DOUBLE_EQ(events_[5].x, 180);
  EXPECT_DOUBLE_EQ(events_[4].y, 100);
  EXPECT_DOUBLE_EQ(events_[5].y, 100);
}

TEST_F(ClayInputEventTargetTest, CancelsAllActiveTouchPointers) {
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown, {TouchPointer(10, 20, 50)}, 10)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown,
                 {TouchPointer(10, 20, 50), TouchPointer(11, 80, 50)}, 11)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kCancel,
                 {TouchPointer(10, 20, 50), TouchPointer(11, 80, 50)}, 10)));

  ASSERT_EQ(events_.size(), 8u);
  EXPECT_EQ(events_[4].phase, kClayPointerPhaseCancel);
  EXPECT_EQ(events_[5].phase, kClayPointerPhaseRemove);
  EXPECT_EQ(events_[6].phase, kClayPointerPhaseCancel);
  EXPECT_EQ(events_[7].phase, kClayPointerPhaseRemove);
  EXPECT_NE(events_[4].device, events_[6].device);
}

TEST_F(ClayInputEventTargetTest,
       RejectsMismatchedTouchSetsAndCancelsActivePointers) {
  EXPECT_FALSE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kMove, {TouchPointer(10, 20, 50)}, 10)));
  ASSERT_TRUE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown, {TouchPointer(10, 20, 50)}, 10)));
  EXPECT_FALSE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown, {TouchPointer(11, 80, 50)}, 11)));
  ASSERT_EQ(events_.size(), 4u);
  EXPECT_EQ(events_[2].phase, kClayPointerPhaseCancel);
  EXPECT_EQ(events_[3].phase, kClayPointerPhaseRemove);
}

TEST_F(ClayInputEventTargetTest, RejectsUnsupportedSourcesAndPayloads) {
  auto event = Event(PointerEventType::kDown);
  event.source_type = PointerSourceType::kDefault;
  EXPECT_FALSE(target_.InjectPointerEvent(event));
  event = Event(PointerEventType::kDown);
  event.pointers.push_back(event.pointers[0]);
  EXPECT_FALSE(target_.InjectPointerEvent(event));
  event = Event(PointerEventType::kDown);
  event.action_pointer_id = 8;
  EXPECT_FALSE(target_.InjectPointerEvent(event));
  event = Event(PointerEventType::kDown);
  event.modifiers = 2;
  EXPECT_FALSE(target_.InjectPointerEvent(event));
  event = Event(PointerEventType::kDown);
  event.buttons = devtool::input::kSecondaryButton;
  EXPECT_FALSE(target_.InjectPointerEvent(event));
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kScroll)));
  EXPECT_FALSE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown,
                 {TouchPointer(10, 10, 10), TouchPointer(11, 20, 10),
                  TouchPointer(12, 30, 10)},
                 12)));
  EXPECT_FALSE(target_.InjectPointerEvent(
      TouchEvent(PointerEventType::kDown,
                 {TouchPointer(10, 10, 10), TouchPointer(10, 20, 10)}, 10)));
  EXPECT_TRUE(events_.empty());
}

TEST_F(ClayInputEventTargetTest, RejectsNonFiniteAndOutOfViewCoordinates) {
  for (float x : {-1.f, 100.f, std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::quiet_NaN()}) {
    EXPECT_FALSE(
        target_.InjectPointerEvent(Event(PointerEventType::kDown, x, 0)));
  }
  EXPECT_FALSE(
      target_.InjectPointerEvent(Event(PointerEventType::kDown, 0, 200)));
  EXPECT_TRUE(events_.empty());
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown, 0, 0)));
  EXPECT_FALSE(
      target_.InjectPointerEvent(Event(PointerEventType::kMove, 101, 0)));
  ASSERT_EQ(events_.size(), 4u);
  EXPECT_EQ(events_[2].phase, kClayPointerPhaseCancel);
  EXPECT_EQ(events_[3].phase, kClayPointerPhaseRemove);
}

TEST_F(ClayInputEventTargetTest, RejectsInvalidTransitionsAndCancelsMismatch) {
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kMove)));
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kUp)));
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  EXPECT_EQ(events_[2].phase, kClayPointerPhaseCancel);
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  EXPECT_FALSE(
      target_.InjectPointerEvent(Event(PointerEventType::kUp, 10, 20, 8)));
  EXPECT_EQ(events_[6].phase, kClayPointerPhaseCancel);
}

TEST_F(ClayInputEventTargetTest, ViewportChangeCancelsBeforeUsingNewScale) {
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  viewport_ = {80, 120, 1.5};
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kUp)));
  ASSERT_EQ(events_.size(), 4u);
  EXPECT_EQ(events_[2].phase, kClayPointerPhaseCancel);
  EXPECT_DOUBLE_EQ(events_[2].x, 20.5);
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  EXPECT_DOUBLE_EQ(events_.back().x, 15.375);
}

TEST_F(ClayInputEventTargetTest,
       ResetAllowsNewGestureAndInvalidationDisablesTarget) {
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  target_.CancelActivePointers();
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kUp)));
  ASSERT_TRUE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  target_.Invalidate();
  const auto count = events_.size();
  target_.Invalidate();
  EXPECT_FALSE(target_.GetPointerCapabilities().supports_mouse);
  EXPECT_FALSE(target_.InjectPointerEvent(Event(PointerEventType::kDown)));
  EXPECT_EQ(events_.size(), count);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx
