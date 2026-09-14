// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "platform/embedder/lynx_ui_renderer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

class CapturingLynxUIRenderer : public LynxUIRenderer {
 public:
  explicit CapturingLynxUIRenderer(lynx_view_builder_t* builder)
      : LynxUIRenderer(builder) {}

  void SetParent(NativeWindow parent) override {}
  NativeWindow GetNativeWindow() override { return nullptr; }
  void OnEnterForeground() override {}
  void OnEnterBackground() override {}
  tasm::UIDelegate* GetUIDelegate() override { return nullptr; }
  void RegisterIMEHandler(void* handler, void* opaque) override {}

  std::vector<ClayPointerEvent> events;

 private:
  void SendPointerEvent(const ClayPointerEvent& event) override {
    events.push_back(event);
  }
};

TEST(LynxUIRendererTest, DragProducesClayPointerSequence) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 2.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("mousePressed", 10, 20, "left", 0, 0, 0,
                                      1);
  renderer.EmulateMouseSyntheticEvent("mouseMoved", 11, 25, "left", 0, 0, 0, 0);
  renderer.EmulateMouseSyntheticEvent("mouseReleased", 11, 25, "left", 0, 0, 0,
                                      1);

  ASSERT_EQ(renderer.events.size(), 3u);
  EXPECT_EQ(renderer.events[0].phase, kClayPointerPhaseDown);
  EXPECT_EQ(renderer.events[1].phase, kClayPointerPhaseMove);
  EXPECT_EQ(renderer.events[2].phase, kClayPointerPhaseUp);
  EXPECT_EQ(renderer.events[0].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(renderer.events[1].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_DOUBLE_EQ(renderer.events[0].x, 20.0);
  EXPECT_DOUBLE_EQ(renderer.events[0].y, 40.0);
  EXPECT_DOUBLE_EQ(renderer.events[2].x, 22.0);
  EXPECT_DOUBLE_EQ(renderer.events[2].y, 50.0);
  for (const auto& event : renderer.events) {
    EXPECT_EQ(event.struct_size, sizeof(ClayPointerEvent));
    EXPECT_EQ(event.device, renderer.events[0].device);
    EXPECT_EQ(event.device_kind, kClayPointerDeviceKindMouse);
    EXPECT_NE(event.timestamp, 0u);
  }
}

TEST(LynxUIRendererTest, RightDragRetainsPressedButtonAcrossMove) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("mousePressed", 1, 2, "right", 0, 0, 0,
                                      1);
  renderer.EmulateMouseSyntheticEvent("mouseMoved", 3, 4, "right", 0, 0, 0, 0);
  renderer.EmulateMouseSyntheticEvent("mouseReleased", 3, 4, "right", 0, 0, 0,
                                      1);

  ASSERT_EQ(renderer.events.size(), 3u);
  EXPECT_EQ(renderer.events[0].buttons, kClayPointerMouseButtonsMouseSecondary);
  EXPECT_EQ(renderer.events[1].buttons, kClayPointerMouseButtonsMouseSecondary);
}

TEST(LynxUIRendererTest, WheelProducesPreciseClayScroll) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 2.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("mouseWheel", 7, 8, "none", 1.5f, -2.5f,
                                      0, 0);

  ASSERT_EQ(renderer.events.size(), 1u);
  const auto& event = renderer.events[0];
  EXPECT_EQ(event.phase, kClayPointerPhaseHover);
  EXPECT_EQ(event.signal_kind, kClayPointerSignalKindScroll);
  EXPECT_DOUBLE_EQ(event.x, 14.0);
  EXPECT_DOUBLE_EQ(event.y, 16.0);
  EXPECT_DOUBLE_EQ(event.scroll_delta_x, 3.0);
  EXPECT_DOUBLE_EQ(event.scroll_delta_y, -5.0);
  EXPECT_EQ(event.is_precise_scroll, 1u);
}

TEST(LynxUIRendererTest, MoveUsesButtonStateFromCaller) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("mousePressed", 1, 2, "left", 0, 0, 0, 1);
  renderer.EmulateMouseSyntheticEvent("mouseMoved", 2, 3, "left", 0, 0, 0, 0);
  renderer.EmulateMouseSyntheticEvent("mouseReleased", 2, 3, "left", 0, 0, 0,
                                      1);
  renderer.EmulateMouseSyntheticEvent("mouseMoved", 3, 4, "none", 0, 0, 0, 0);

  ASSERT_EQ(renderer.events.size(), 4u);
  EXPECT_EQ(renderer.events[1].phase, kClayPointerPhaseMove);
  EXPECT_EQ(renderer.events[1].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(renderer.events[3].phase, kClayPointerPhaseHover);
  EXPECT_EQ(renderer.events[3].buttons, 0);
}

TEST(LynxUIRendererTest, MouseHoverRetainsDeviceAcrossGestures) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("mouseMoved", 1, 2, "none", 0, 0, 0, 0);
  renderer.EmulateMouseSyntheticEvent("mouseMoved", 3, 4, "none", 0, 0, 0, 0);
  renderer.EmulateMouseSyntheticEvent("mousePressed", 3, 4, "left", 0, 0, 0, 1);
  renderer.EmulateMouseSyntheticEvent("mouseReleased", 3, 4, "left", 0, 0, 0,
                                      1);
  renderer.EmulateMouseSyntheticEvent("mouseWheel", 3, 4, "none", 0, 120, 0, 0);
  renderer.EmulateMouseSyntheticEvent("mouseMoved", 5, 6, "none", 0, 0, 0, 0);

  ASSERT_EQ(renderer.events.size(), 6u);
  for (const auto& event : renderer.events) {
    EXPECT_NE(event.phase, kClayPointerPhaseAdd);
    EXPECT_NE(event.phase, kClayPointerPhaseRemove);
    EXPECT_EQ(event.device, renderer.events[0].device);
    EXPECT_NE(event.device, 0);
  }
  for (const auto index : {0, 1, 5}) {
    EXPECT_EQ(renderer.events[index].phase, kClayPointerPhaseHover);
    EXPECT_EQ(renderer.events[index].buttons, 0);
  }
}

TEST(LynxUIRendererTest, UnknownEventTypeIsIgnored) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("unknown", 1, 2, "left", 0, 0, 0, 0);
  renderer.EmulateTouchSyntheticEvent("unknown", 1, 2, "left", 0, 0, 0, 0);

  EXPECT_TRUE(renderer.events.empty());
}

TEST(LynxUIRendererTest, TouchDragProducesTouchPointerSequence) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 2.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateTouchSyntheticEvent("mousePressed", 10, 20, "left", 0, 0, 0,
                                      1);
  renderer.EmulateTouchSyntheticEvent("mouseMoved", 11, 25, "left", 0, 0, 0, 0);
  renderer.EmulateTouchSyntheticEvent("mouseReleased", 11, 25, "left", 0, 0, 0,
                                      1);

  ASSERT_EQ(renderer.events.size(), 5u);
  EXPECT_EQ(renderer.events[0].phase, kClayPointerPhaseAdd);
  EXPECT_EQ(renderer.events[1].phase, kClayPointerPhaseDown);
  EXPECT_EQ(renderer.events[2].phase, kClayPointerPhaseMove);
  EXPECT_EQ(renderer.events[3].phase, kClayPointerPhaseUp);
  EXPECT_EQ(renderer.events[4].phase, kClayPointerPhaseRemove);
  for (const auto& event : renderer.events) {
    EXPECT_EQ(event.struct_size, sizeof(ClayPointerEvent));
    EXPECT_EQ(event.device, renderer.events[0].device);
    EXPECT_EQ(event.device_kind, kClayPointerDeviceKindTouch);
    EXPECT_EQ(event.buttons, 0);
    EXPECT_NE(event.timestamp, 0u);
  }
  EXPECT_DOUBLE_EQ(renderer.events[0].x, 20.0);
  EXPECT_DOUBLE_EQ(renderer.events[0].y, 40.0);
  for (const auto index : {2, 3, 4}) {
    EXPECT_DOUBLE_EQ(renderer.events[index].x, 22.0);
    EXPECT_DOUBLE_EQ(renderer.events[index].y, 50.0);
  }
}

TEST(LynxUIRendererTest, TouchHoverMoveHasNoLifecycleAndNoHover) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateTouchSyntheticEvent("mouseMoved", 3, 4, "none", 0, 0, 0, 0);

  ASSERT_EQ(renderer.events.size(), 1u);
  EXPECT_EQ(renderer.events[0].phase, kClayPointerPhaseMove);
  EXPECT_EQ(renderer.events[0].device_kind, kClayPointerDeviceKindTouch);
}

TEST(LynxUIRendererTest, TouchWheelIsIgnoredByRenderer) {
  // Renderer drops touch-wheel; the proxy layer downgrades it to mouse.
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 2.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateTouchSyntheticEvent("mouseWheel", 7, 8, "none", 1.5f, -2.5f,
                                      0, 0);

  EXPECT_TRUE(renderer.events.empty());
}

TEST(LynxUIRendererTest, TouchAndMouseUseDifferentDeviceIds) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.EmulateMouseSyntheticEvent("mousePressed", 1, 2, "left", 0, 0, 0, 1);
  renderer.EmulateTouchSyntheticEvent("mousePressed", 1, 2, "left", 0, 0, 0, 1);

  ASSERT_EQ(renderer.events.size(), 3u);
  EXPECT_EQ(renderer.events[0].device_kind, kClayPointerDeviceKindMouse);
  EXPECT_EQ(renderer.events[2].device_kind, kClayPointerDeviceKindTouch);
  EXPECT_NE(renderer.events[0].device, renderer.events[2].device);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx
