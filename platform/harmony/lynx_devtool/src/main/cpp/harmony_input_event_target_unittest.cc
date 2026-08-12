// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_input_event_target.h"

#include <memory>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace {

class RecordingHarmonyTouchEventInjector final
    : public HarmonyTouchEventInjector {
 public:
  bool IsAvailable() const override { return available; }

  bool Inject(const HarmonyTouchEvent& event) override {
    events.push_back(event);
    const size_t index = events.size() - 1;
    return fail_on_index < 0 || static_cast<int>(index) != fail_on_index;
  }

  bool available = true;
  int fail_on_index = -1;
  std::vector<HarmonyTouchEvent> events;
};

HarmonyInputWindowInfo MakeWindowInfo(int32_t window_id = 7) {
  HarmonyInputWindowInfo window_info;
  window_info.window_id = window_id;
  window_info.display_id = 3;
  window_info.left_px = 100;
  window_info.top_px = 200;
  window_info.width_px = 400;
  window_info.height_px = 300;
  window_info.pixel_ratio = 2.f;
  return window_info;
}

input::PointerEvent MakePointerEvent(input::PointerEventType type,
                                     int32_t pointer_id = 5, float x = 10.f,
                                     float y = 20.f) {
  input::PointerEvent event;
  event.source_type = input::PointerSourceType::kTouch;
  event.type = type;
  event.action_pointer_id = pointer_id;
  event.timestamp_us = 1234;
  input::Pointer pointer;
  pointer.id = pointer_id;
  pointer.x = x;
  pointer.y = y;
  event.pointers.push_back(pointer);
  return event;
}

TEST(HarmonyInputEventTargetTest, ReportsCapabilitiesForValidWindow) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);

  auto capabilities = target.GetPointerCapabilities();
  EXPECT_FALSE(capabilities.supports_touch);

  target.UpdateWindowInfo(MakeWindowInfo());
  capabilities = target.GetPointerCapabilities();
  EXPECT_EQ(capabilities.default_source_type, input::PointerSourceType::kTouch);
  EXPECT_TRUE(capabilities.supports_touch);
  EXPECT_FALSE(capabilities.supports_mouse);

  injector->available = false;
  capabilities = target.GetPointerCapabilities();
  EXPECT_FALSE(capabilities.supports_touch);
}

TEST(HarmonyInputEventTargetTest, ConvertsCssPixelsAndInjectsSequence) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());

  EXPECT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown)));
  EXPECT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kMove, 5, 11.f, 21.f)));
  EXPECT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kUp, 5, 12.f, 22.f)));

  ASSERT_EQ(injector->events.size(), 3u);
  EXPECT_EQ(injector->events[0].action, HarmonyTouchEventAction::kDown);
  EXPECT_EQ(injector->events[0].window_id, 7);
  EXPECT_EQ(injector->events[0].display_id, 3);
  EXPECT_EQ(injector->events[0].pointer_id, 0);
  EXPECT_EQ(injector->events[0].window_x, 20);
  EXPECT_EQ(injector->events[0].window_y, 40);
  EXPECT_EQ(injector->events[0].display_x, 120);
  EXPECT_EQ(injector->events[0].display_y, 240);
  EXPECT_EQ(injector->events[1].action, HarmonyTouchEventAction::kMove);
  EXPECT_EQ(injector->events[2].action, HarmonyTouchEventAction::kUp);
}

TEST(HarmonyInputEventTargetTest, CompletesInputProcessingSynchronously) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);

  bool callback_called = false;
  bool callback_result = false;
  target.WaitForInputProcessed([&](bool success) {
    callback_called = true;
    callback_result = success;
  });

  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(callback_result);
}

TEST(HarmonyInputEventTargetTest, RejectsUnsupportedAndOutOfBoundsEvents) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());

  auto event = MakePointerEvent(input::PointerEventType::kDown);
  event.source_type = input::PointerSourceType::kMouse;
  EXPECT_FALSE(target.InjectPointerEvent(event));

  event = MakePointerEvent(input::PointerEventType::kScroll);
  EXPECT_FALSE(target.InjectPointerEvent(event));

  event = MakePointerEvent(input::PointerEventType::kDown, 5, 200.f, 20.f);
  EXPECT_FALSE(target.InjectPointerEvent(event));
  EXPECT_TRUE(injector->events.empty());
}

TEST(HarmonyInputEventTargetTest, CancelsActivePointerWhenWindowChanges) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());
  ASSERT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown)));

  target.UpdateWindowInfo(MakeWindowInfo(8));

  ASSERT_EQ(injector->events.size(), 2u);
  EXPECT_EQ(injector->events[1].action, HarmonyTouchEventAction::kCancel);
  EXPECT_EQ(injector->events[1].window_id, 7);
  EXPECT_EQ(injector->events[1].window_x, 20);
  EXPECT_EQ(injector->events[1].window_y, 40);
  EXPECT_TRUE(target.GetPointerCapabilities().supports_touch);
}

TEST(HarmonyInputEventTargetTest, CancelsMismatchedPointerSequence) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());
  ASSERT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown)));

  EXPECT_FALSE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kMove, 6)));

  ASSERT_EQ(injector->events.size(), 2u);
  EXPECT_EQ(injector->events[1].action, HarmonyTouchEventAction::kCancel);
  EXPECT_EQ(injector->events[1].pointer_id, 0);
}

TEST(HarmonyInputEventTargetTest, CancelsWhenInjectionFails) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  injector->fail_on_index = 1;
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());
  ASSERT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown)));

  EXPECT_FALSE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kMove)));

  ASSERT_EQ(injector->events.size(), 3u);
  EXPECT_EQ(injector->events[1].action, HarmonyTouchEventAction::kMove);
  EXPECT_EQ(injector->events[2].action, HarmonyTouchEventAction::kCancel);
}

TEST(HarmonyInputEventTargetTest, InvalidateWindowCancelsAndDisablesTarget) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());
  ASSERT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown)));

  target.InvalidateWindow();

  ASSERT_EQ(injector->events.size(), 2u);
  EXPECT_EQ(injector->events[1].action, HarmonyTouchEventAction::kCancel);
  EXPECT_FALSE(target.GetPointerCapabilities().supports_touch);
}

}  // namespace
}  // namespace devtool
}  // namespace lynx
