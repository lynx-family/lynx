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

  renderer.DispatchSyntheticPointerEvent("mousePressed", 10, 20, "left", 0, 0,
                                         0, 1);
  renderer.DispatchSyntheticPointerEvent("mouseMoved", 11, 25, "left", 0, 0, 0,
                                         0);
  renderer.DispatchSyntheticPointerEvent("mouseReleased", 11, 25, "left", 0, 0,
                                         0, 1);

  ASSERT_EQ(renderer.events.size(), 5u);
  EXPECT_EQ(renderer.events[0].phase, kClayPointerPhaseAdd);
  EXPECT_EQ(renderer.events[1].phase, kClayPointerPhaseDown);
  EXPECT_EQ(renderer.events[2].phase, kClayPointerPhaseMove);
  EXPECT_EQ(renderer.events[3].phase, kClayPointerPhaseUp);
  EXPECT_EQ(renderer.events[4].phase, kClayPointerPhaseRemove);
  EXPECT_EQ(renderer.events[0].buttons, 0);
  EXPECT_EQ(renderer.events[1].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(renderer.events[2].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(renderer.events[3].buttons, 0);
  EXPECT_EQ(renderer.events[4].buttons, 0);
  EXPECT_DOUBLE_EQ(renderer.events[0].x, 20.0);
  EXPECT_DOUBLE_EQ(renderer.events[0].y, 40.0);
  EXPECT_DOUBLE_EQ(renderer.events[2].x, 22.0);
  EXPECT_DOUBLE_EQ(renderer.events[2].y, 50.0);
  EXPECT_EQ(renderer.events[0].device, 0);
  for (const auto& event : renderer.events) {
    EXPECT_EQ(event.device, renderer.events[0].device);
    EXPECT_NE(event.timestamp, 0u);
  }
}

TEST(LynxUIRendererTest, RightDragRetainsPressedButtonAcrossMove) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.DispatchSyntheticPointerEvent("mousePressed", 1, 2, "right", 0, 0, 0,
                                         1);
  renderer.DispatchSyntheticPointerEvent("mouseMoved", 3, 4, "right", 0, 0, 0,
                                         0);
  renderer.DispatchSyntheticPointerEvent("mouseReleased", 3, 4, "right", 0, 0,
                                         0, 1);

  ASSERT_EQ(renderer.events.size(), 5u);
  EXPECT_EQ(renderer.events[0].buttons, 0);
  EXPECT_EQ(renderer.events[1].buttons, kClayPointerMouseButtonsMouseSecondary);
  EXPECT_EQ(renderer.events[2].buttons, kClayPointerMouseButtonsMouseSecondary);
  EXPECT_EQ(renderer.events[3].buttons, 0);
  EXPECT_EQ(renderer.events[4].buttons, 0);
}

TEST(LynxUIRendererTest, WheelProducesPreciseClayScroll) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 2.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.DispatchSyntheticPointerEvent("mouseWheel", 7, 8, "none", 1.5f,
                                         -2.5f, 0, 0);

  ASSERT_EQ(renderer.events.size(), 3u);
  EXPECT_EQ(renderer.events[0].phase, kClayPointerPhaseAdd);
  const auto& event = renderer.events[1];
  EXPECT_EQ(event.phase, kClayPointerPhaseHover);
  EXPECT_EQ(event.signal_kind, kClayPointerSignalKindScroll);
  EXPECT_DOUBLE_EQ(event.scroll_delta_x, 3.0);
  EXPECT_DOUBLE_EQ(event.scroll_delta_y, -5.0);
  EXPECT_EQ(event.is_precise_scroll, 1u);
  EXPECT_EQ(renderer.events[2].phase, kClayPointerPhaseRemove);
  EXPECT_EQ(renderer.events[0].signal_kind, kClayPointerSignalKindNone);
  EXPECT_EQ(renderer.events[2].signal_kind, kClayPointerSignalKindNone);
}

TEST(LynxUIRendererTest, MoveUsesButtonStateFromCaller) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.DispatchSyntheticPointerEvent("mouseMoved", 1, 2, "left", 0, 0, 0,
                                         0);
  renderer.DispatchSyntheticPointerEvent("mouseMoved", 3, 4, "none", 0, 0, 0,
                                         0);

  ASSERT_EQ(renderer.events.size(), 4u);
  EXPECT_EQ(renderer.events[0].phase, kClayPointerPhaseMove);
  EXPECT_EQ(renderer.events[0].buttons, kClayPointerMouseButtonsMousePrimary);
  EXPECT_EQ(renderer.events[1].phase, kClayPointerPhaseAdd);
  EXPECT_EQ(renderer.events[2].phase, kClayPointerPhaseHover);
  EXPECT_EQ(renderer.events[2].buttons, 0);
  EXPECT_EQ(renderer.events[3].phase, kClayPointerPhaseRemove);
}

TEST(LynxUIRendererTest, UnknownInputIsIgnored) {
  lynx_view_builder_t builder = {};
  builder.screen_size.pixel_ratio = 1.f;
  CapturingLynxUIRenderer renderer(&builder);

  renderer.DispatchSyntheticPointerEvent("unknown", 1, 2, "left", 0, 0, 0, 0);

  EXPECT_TRUE(renderer.events.empty());
}

}  // namespace
}  // namespace embedder
}  // namespace lynx
