// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_input_event_target.h"

#include <memory>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/devtool/harmony_devtool_coordinates.h"
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

input::PointerEvent MakeTwoPointerEvent(input::PointerEventType type,
                                        int32_t action_pointer_id,
                                        float first_x, float second_x) {
  auto event = MakePointerEvent(type, 5, first_x, 20.f);
  input::Pointer second_pointer;
  second_pointer.id = 6;
  second_pointer.x = second_x;
  second_pointer.y = 20.f;
  event.pointers.push_back(second_pointer);
  event.action_pointer_id = action_pointer_id;
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
  EXPECT_EQ(capabilities.max_touch_points, 2);

  injector->available = false;
  capabilities = target.GetPointerCapabilities();
  EXPECT_FALSE(capabilities.supports_touch);
}

TEST(HarmonyInputEventTargetTest, InjectsMultiPointerSequence) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());

  EXPECT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown, 5, 10.f, 20.f)));
  EXPECT_TRUE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kDown, 6, 10.f, 30.f)));
  EXPECT_TRUE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kMove, 5, 11.f, 31.f)));
  EXPECT_TRUE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kUp, 6, 12.f, 32.f)));
  EXPECT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kUp, 5, 12.f, 20.f)));

  ASSERT_EQ(injector->events.size(), 6u);
  EXPECT_EQ(injector->events[0].action, HarmonyTouchEventAction::kDown);
  EXPECT_EQ(injector->events[0].pointer_id, 0);
  EXPECT_EQ(injector->events[1].action, HarmonyTouchEventAction::kDown);
  EXPECT_EQ(injector->events[1].pointer_id, 1);
  EXPECT_EQ(injector->events[2].action, HarmonyTouchEventAction::kMove);
  EXPECT_EQ(injector->events[2].pointer_id, 0);
  EXPECT_EQ(injector->events[2].window_x, 22);
  EXPECT_EQ(injector->events[3].action, HarmonyTouchEventAction::kMove);
  EXPECT_EQ(injector->events[3].pointer_id, 1);
  EXPECT_EQ(injector->events[3].window_x, 62);
  EXPECT_EQ(injector->events[4].action, HarmonyTouchEventAction::kUp);
  EXPECT_EQ(injector->events[4].pointer_id, 1);
  EXPECT_EQ(injector->events[5].action, HarmonyTouchEventAction::kUp);
  EXPECT_EQ(injector->events[5].pointer_id, 0);
}

TEST(HarmonyInputEventTargetTest, CancelsAllActivePointers) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());

  ASSERT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown, 5)));
  ASSERT_TRUE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kDown, 6, 10.f, 30.f)));

  EXPECT_TRUE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kCancel, 5, 11.f, 31.f)));

  ASSERT_EQ(injector->events.size(), 4u);
  EXPECT_EQ(injector->events[2].action, HarmonyTouchEventAction::kCancel);
  EXPECT_EQ(injector->events[2].pointer_id, 0);
  EXPECT_EQ(injector->events[2].window_x, 22);
  EXPECT_EQ(injector->events[3].action, HarmonyTouchEventAction::kCancel);
  EXPECT_EQ(injector->events[3].pointer_id, 1);
  EXPECT_EQ(injector->events[3].window_x, 62);

  EXPECT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown, 7)));
  EXPECT_EQ(injector->events.back().pointer_id, 0);
}

TEST(HarmonyInputEventTargetTest, CancelsAllPointersWhenMoveInjectionFails) {
  auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
  HarmonyInputEventTarget target(injector);
  target.UpdateWindowInfo(MakeWindowInfo());

  ASSERT_TRUE(target.InjectPointerEvent(
      MakePointerEvent(input::PointerEventType::kDown, 5)));
  ASSERT_TRUE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kDown, 6, 10.f, 30.f)));
  injector->fail_on_index = 3;

  EXPECT_FALSE(target.InjectPointerEvent(
      MakeTwoPointerEvent(input::PointerEventType::kMove, 5, 11.f, 31.f)));

  ASSERT_EQ(injector->events.size(), 6u);
  EXPECT_EQ(injector->events[2].action, HarmonyTouchEventAction::kMove);
  EXPECT_EQ(injector->events[2].pointer_id, 0);
  EXPECT_EQ(injector->events[3].action, HarmonyTouchEventAction::kMove);
  EXPECT_EQ(injector->events[3].pointer_id, 1);
  EXPECT_EQ(injector->events[4].action, HarmonyTouchEventAction::kCancel);
  EXPECT_EQ(injector->events[4].pointer_id, 0);
  EXPECT_EQ(injector->events[5].action, HarmonyTouchEventAction::kCancel);
  EXPECT_EQ(injector->events[5].pointer_id, 1);
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

TEST(HarmonyInputEventTargetTest, BoxModelCoordinatesMatchHitTestingAndInput) {
  for (float density : {1.f, 2.f, 3.f}) {
    SCOPED_TRACE(density);
    const tasm::harmony::HarmonyDevToolCoordinates coordinates(density);
    // A 40 x 30 logical box at (15, 20) inside a root at window (10, 25).
    std::vector<float> quad = {15, 20, 55, 20, 55, 50, 15, 50};
    for (auto& value : quad) {
      value *= density;
    }
    const float root_x_px = 10 * density;
    const float root_y_px = 25 * density;
    coordinates.QuadToWindow(quad, root_x_px, root_y_px);
    EXPECT_EQ(quad, (std::vector<float>{25, 45, 65, 45, 65, 75, 25, 75}));
    EXPECT_FLOAT_EQ(quad[2] - quad[0], 40);
    EXPECT_FLOAT_EQ(quad[5] - quad[1], 30);

    // Both screencast modes must select the same point in the root.
    float fullscreen_point[2] = {quad[0], quad[1]};
    coordinates.WindowToRoot(fullscreen_point, root_x_px, root_y_px);
    std::vector<float> root_quad(8, 0.f);
    coordinates.QuadToWindow(root_quad, root_x_px, root_y_px);
    const float lynxview_x = quad[0] - root_quad[0];
    const float lynxview_y = quad[1] - root_quad[1];
    EXPECT_FLOAT_EQ(fullscreen_point[0], 15);
    EXPECT_FLOAT_EQ(fullscreen_point[1], 20);
    EXPECT_FLOAT_EQ(lynxview_x, fullscreen_point[0]);
    EXPECT_FLOAT_EQ(lynxview_y, fullscreen_point[1]);

    // A fullscreen box-model point can be passed directly to synthetic input.
    auto injector = std::make_shared<RecordingHarmonyTouchEventInjector>();
    HarmonyInputEventTarget target(injector);
    auto window = MakeWindowInfo();
    window.pixel_ratio = density;
    target.UpdateWindowInfo(window);
    ASSERT_TRUE(target.InjectPointerEvent(
        MakePointerEvent(input::PointerEventType::kDown, 5, quad[0], quad[1])));
    ASSERT_EQ(injector->events.size(), 1u);
    const auto& event = injector->events[0];
    EXPECT_EQ(event.window_x, 25 * density);
    EXPECT_EQ(event.window_y, 45 * density);
    EXPECT_EQ(event.display_x, 100 + 25 * density);
    EXPECT_EQ(event.display_y, 200 + 45 * density);

    // Emulated input converts to physical pixels only at native injection.
    EXPECT_FLOAT_EQ(coordinates.LogicalToPhysical(quad[0]), event.window_x);
    EXPECT_FLOAT_EQ(coordinates.LogicalToPhysical(quad[1]), event.window_y);
    const float root_screen_x = coordinates.PhysicalToLogical(100 + root_x_px);
    const float root_screen_y = coordinates.PhysicalToLogical(200 + root_y_px);
    EXPECT_FLOAT_EQ(coordinates.LogicalToPhysical(root_screen_x + lynxview_x),
                    event.display_x);
    EXPECT_FLOAT_EQ(coordinates.LogicalToPhysical(root_screen_y + lynxview_y),
                    event.display_y);

    // Viewport metadata and the box quad share the same scale.
    const float logical_width = coordinates.PhysicalToLogical(window.width_px);
    EXPECT_FLOAT_EQ(quad[0] / logical_width,
                    static_cast<float>(event.window_x) / window.width_px);
  }
}

TEST(HarmonyInputEventTargetTest, BoxModelPreservesTransformedCorners) {
  const tasm::harmony::HarmonyDevToolCoordinates coordinates(1.25f);
  std::vector<float> quad = {-5, 2.5f, 7.5f, 15, -5, 27.5f, -17.5f, 15};
  coordinates.QuadToWindow(quad, 3, 7);
  const std::vector<float> expected = {-1.6f, 7.6f,  8.4f,   17.6f,
                                       -1.6f, 27.6f, -11.6f, 17.6f};
  ASSERT_EQ(quad.size(), expected.size());
  for (size_t i = 0; i < quad.size(); ++i) {
    EXPECT_FLOAT_EQ(quad[i], expected[i]);
  }
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
