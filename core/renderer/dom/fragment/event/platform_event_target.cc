// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target.h"

#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"

namespace lynx {
namespace tasm {

fml::RefPtr<PlatformEventTarget> PlatformEventTarget::HitTest(float point[2]) {
  return nullptr;
}

bool PlatformEventTarget::ShouldHitTest() { return true; }

void PlatformEventTarget::GetPointInTarget(
    float target_point[2], fml::RefPtr<PlatformEventTarget> parent_target,
    float point[2]) {}

bool PlatformEventTarget::ContainsPoint(float point[2]) { return false; }

void PlatformEventTarget::OnResponseChain() {}

void PlatformEventTarget::OffResponseChain() {}

bool PlatformEventTarget::IsOnResponseChain() { return false; }

void PlatformEventTarget::OnFocusChange(bool has_focus,
                                        bool is_focus_transition) {}

bool PlatformEventTarget::Focusable() { return true; }

void PlatformEventTarget::OnPseudoStatusChanged(
    LynxPseudoStatus pre_status, LynxPseudoStatus current_status) {}

LynxPseudoStatus PlatformEventTarget::GetPseudoStatus() {
  return LynxPseudoStatus::kNone;
}

bool PlatformEventTarget::TouchPseudoPropagation() { return true; }

std::vector<std::string> PlatformEventTarget::EventSet() { return {}; }

bool PlatformEventTarget::BlockNativeEvent(float point[2]) { return false; }

bool PlatformEventTarget::EventThrough(float point[2]) { return false; }

bool PlatformEventTarget::IgnoreFocus() { return false; }

LynxConsumeSlideDirection PlatformEventTarget::ConsumeSlideEvent() {
  return LynxConsumeSlideDirection::kNone;
}

LynxPointerEventsValue PlatformEventTarget::PointerEvents() {
  return LynxPointerEventsValue::kAuto;
}

}  // namespace tasm
}  // namespace lynx
