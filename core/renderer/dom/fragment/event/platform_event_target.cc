// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target.h"

#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"

namespace lynx {
namespace tasm {

fml::RefPtr<PlatformEventTarget> PlatformEventTarget::HitTest(float point[2]) {
  fml::RefPtr<PlatformEventTarget> target = nullptr;
  const auto& children = ChildrenTargets();
  int children_size = static_cast<int>(children.size());
  int sibling_target_idx = 0;
  float child_point[2] = {0.f};
  float target_point[2] = {point[0], point[1]};
  // find hit_target by traversing the sibling nodes in reverse.
  for (int i = children_size - 1; i >= 0; --i) {
    const auto& child = children[i];
    if (!child->ShouldHitTest()) {
      continue;
    }
    child->GetPointInTarget(child_point, fml::RefPtr<PlatformEventTarget>(this),
                            point);
    if (child->ContainsPoint(child_point)) {
      memcpy(target_point, child_point, sizeof(float) * 2);
      sibling_target_idx = i + 1;
      target = child;
      break;
    }
  }

  auto hit_target = target ? target->HitTest(target_point)
                           : fml::RefPtr<PlatformEventTarget>(this);
  // when a node has pointer-events:none set, the hit_target needs to be found
  // again.
  if (!hit_target ||
      hit_target->PointerEvents() == LynxPointerEventsValue::kNone) {
    for (int i = sibling_target_idx; i < children_size; ++i) {
      auto sibling_target = children[i];
      if (!sibling_target || !sibling_target->ShouldHitTest()) {
        continue;
      }
      float sibling_point[2] = {0.f};
      sibling_target->GetPointInTarget(
          sibling_point, fml::RefPtr<PlatformEventTarget>(this), point);
      if (!sibling_target->ContainsPoint(sibling_point)) {
        continue;
      }
      hit_target = sibling_target->HitTest(sibling_point);
      if (hit_target) {
        break;
      }
    }
  }
  return hit_target ? hit_target : fml::RefPtr<PlatformEventTarget>(this);
}

bool PlatformEventTarget::ShouldHitTest() const { return true; }

void PlatformEventTarget::GetPointInTarget(
    float target_point[2], fml::RefPtr<PlatformEventTarget> parent_target,
    float point[2]) {
  target_helper_->ConvertPointFromAncestorToDescendant(
      target_point, parent_target, fml::RefPtr<PlatformEventTarget>(this),
      point);
}

bool PlatformEventTarget::ContainsPoint(float point[2]) const {
  float x = point[0];
  float y = point[1];
  if (x >= 0.f && x <= Width() && y >= 0.f && y <= Height()) {
    return true;
  }
  return false;
}

void PlatformEventTarget::OnResponseChain() {}

void PlatformEventTarget::OffResponseChain() {}

bool PlatformEventTarget::IsOnResponseChain() const { return false; }

void PlatformEventTarget::OnFocusChange(bool has_focus,
                                        bool is_focus_transition) {}

bool PlatformEventTarget::Focusable() const { return true; }

void PlatformEventTarget::OnPseudoStatusChanged(
    LynxPseudoStatus pre_status, LynxPseudoStatus current_status) {}

LynxPseudoStatus PlatformEventTarget::GetPseudoStatus() const {
  return LynxPseudoStatus::kNone;
}

bool PlatformEventTarget::TouchPseudoPropagation() const { return true; }

const std::vector<std::string>& PlatformEventTarget::EventSet() const {
  return event_set_;
}

bool PlatformEventTarget::EventThrough(float point[2]) const { return false; }

bool PlatformEventTarget::IgnoreFocus() const { return false; }

LynxPointerEventsValue PlatformEventTarget::PointerEvents() const {
  return LynxPointerEventsValue::kAuto;
}

bool PlatformEventTarget::BlockNativeEvent(float point[2]) const {
  return false;
}

LynxConsumeSlideDirection PlatformEventTarget::ConsumeSlideEvent() const {
  return LynxConsumeSlideDirection::kNone;
}

}  // namespace tasm
}  // namespace lynx
