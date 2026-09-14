// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_event_simulation_proxy.h"

#include <utility>

namespace lynx {
namespace embedder {

LynxViewEventSimulationProxy::LynxViewEventSimulationProxy(
    std::unique_ptr<LynxViewEventSimulationTarget> target)
    : target_(std::move(target)) {}

void LynxViewEventSimulationProxy::EmulateTouch(
    const std::string& event_type, int x, int y, const std::string& button,
    float delta_x, float delta_y, int modifiers, int click_count) {
  if (!target_) {
    return;
  }
  // Clay's touch pointer has no wheel channel; downgrade to mouse and flip
  // the delta sign to match Clay's convention.
  if (event_type == "mouseWheel") {
    target_->EmulateMouseSyntheticEvent(event_type, x, y, button, -delta_x,
                                        -delta_y, modifiers, click_count);
    return;
  }
  target_->EmulateTouchSyntheticEvent(event_type, x, y, button, delta_x,
                                      delta_y, modifiers, click_count);
}

void LynxViewEventSimulationProxy::EmulateMouse(
    const std::string& event_type, int x, int y, const std::string& button,
    float delta_x, float delta_y, int modifiers, int click_count) {
  if (target_) {
    target_->EmulateMouseSyntheticEvent(event_type, x, y, button, delta_x,
                                        delta_y, modifiers, click_count);
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

}  // namespace embedder
}  // namespace lynx
