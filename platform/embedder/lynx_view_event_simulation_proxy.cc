// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_event_simulation_proxy.h"

#include <utility>

namespace lynx {
namespace embedder {

namespace {

// cspell:ignore rightmousedown rightmousemove rightmouseup
constexpr char kRightMouseDownEvent[] = "rightmousedown";
constexpr char kRightMouseMoveEvent[] = "rightmousemove";
constexpr char kRightMouseUpEvent[] = "rightmouseup";

}  // namespace

LynxViewEventSimulationProxy::LynxViewEventSimulationProxy(
    std::unique_ptr<LynxViewEventSimulationTarget> target)
    : target_(std::move(target)) {}

void LynxViewEventSimulationProxy::EmulateTouch(
    const std::string& event_type, int x, int y, const std::string& button,
    float delta_x, float delta_y, int modifiers, int click_count) {
  (void)modifiers;
  (void)click_count;
  if (!target_) {
    return;
  }
  if (event_type == kMouseWheel) {
    target_->EmulateMouseEvent("wheel", x, y, delta_x, delta_y);
    return;
  }
  if (button == kMouseRightButton || right_mouse_active_) {
    EmulateRightMouseEvent(event_type, x, y, delta_x, delta_y);
    return;
  }

  if (event_type == kMousePressed) {
    active_tag_ = target_->GetNodeForLocation(x, y);
    if (active_tag_ <= 0) {
      ResetTouchState();
      return;
    }
    touch_active_ = true;
    down_x_ = x;
    down_y_ = y;
    moved_beyond_tap_slop_ = false;
    target_->SendTouchEvent("touchstart", active_tag_, x, y);
    return;
  }

  if (!touch_active_ || active_tag_ <= 0) {
    return;
  }

  if (event_type == kMouseMoved) {
    UpdateTapState(x, y);
    target_->SendTouchEvent("touchmove", active_tag_, x, y);
    return;
  }

  if (event_type == kMouseReleased) {
    UpdateTapState(x, y);
    target_->SendTouchEvent("touchend", active_tag_, x, y);
    if (!moved_beyond_tap_slop_) {
      target_->SendTouchEvent("tap", active_tag_, x, y);
    }
    ResetTouchState();
  }
}

void LynxViewEventSimulationProxy::Focus(int node_id) {
  if (target_) {
    target_->Focus(node_id);
  }
}

void LynxViewEventSimulationProxy::InsertText(const std::string& text) {
  if (target_) {
    target_->InsertText(text);
  }
}

void LynxViewEventSimulationProxy::EmulateRightMouseEvent(
    const std::string& event_type, int x, int y, float delta_x, float delta_y) {
  if (event_type == kMousePressed) {
    right_mouse_active_ = true;
    target_->EmulateMouseEvent(kRightMouseDownEvent, x, y, delta_x, delta_y);
    return;
  }

  if (event_type == kMouseMoved) {
    target_->EmulateMouseEvent(
        right_mouse_active_ ? kRightMouseMoveEvent : "mousemove", x, y, delta_x,
        delta_y);
    return;
  }

  if (event_type == kMouseReleased) {
    target_->EmulateMouseEvent(kRightMouseUpEvent, x, y, delta_x, delta_y);
    right_mouse_active_ = false;
  }
}

void LynxViewEventSimulationProxy::UpdateTapState(int x, int y) {
  const int delta_x = x - down_x_;
  const int delta_y = y - down_y_;
  moved_beyond_tap_slop_ =
      moved_beyond_tap_slop_ ||
      (delta_x * delta_x + delta_y * delta_y) > (kTapSlopPx * kTapSlopPx);
}

void LynxViewEventSimulationProxy::ResetTouchState() {
  touch_active_ = false;
  moved_beyond_tap_slop_ = false;
  active_tag_ = 0;
  down_x_ = 0;
  down_y_ = 0;
}

}  // namespace embedder
}  // namespace lynx
