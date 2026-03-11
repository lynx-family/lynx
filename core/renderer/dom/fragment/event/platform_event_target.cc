// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target.h"

#include <cstring>
#include <utility>

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

bool PlatformEventTarget::ShouldHitTest() const {
  return user_interaction_enabled_;
}

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

void PlatformEventTarget::GetOrUpdateTargetScreenRect(
    std::unordered_map<int32_t, CommonAncestorRect>& common_ancestor_rect_map,
    const fml::RefPtr<PlatformEventTarget>& target, float out_rect[4],
    float root_view_origin_on_screen[2]) const {
  out_rect[0] = 0;
  out_rect[1] = 0;
  out_rect[2] = target->Width();
  out_rect[3] = target->Height();

  const int32_t sign = target->Sign();
  auto it = common_ancestor_rect_map.find(sign);
  if (it != common_ancestor_rect_map.end()) {
    if (it->second.rect_updated) {
      std::memcpy(out_rect, it->second.rect, sizeof(float) * 4);
      return;
    }
    target_helper_->ConvertRectFromTargetToRootTarget(out_rect, target,
                                                      out_rect);
    target_helper_->OffsetRect(out_rect, root_view_origin_on_screen);
    std::memcpy(it->second.rect, out_rect, sizeof(float) * 4);
    it->second.rect_updated = true;
    return;
  }

  target_helper_->ConvertRectFromTargetToRootTarget(out_rect, target, out_rect);
  target_helper_->OffsetRect(out_rect, root_view_origin_on_screen);
  CommonAncestorRect rect;
  rect.target_count = 1;
  rect.rect_updated = true;
  std::memcpy(rect.rect, out_rect, sizeof(float) * 4);
  common_ancestor_rect_map.emplace(sign, std::move(rect));
}

bool PlatformEventTarget::IsVisibleForExposure(
    std::unordered_map<int32_t, CommonAncestorRect>& common_ancestor_rect_map,
    float root_view_origin_on_screen[2], const float window_rect[4]) const {
  if (Width() == 0.f || Height() == 0.f) {
    return false;
  }

  auto self =
      fml::RefPtr<PlatformEventTarget>(const_cast<PlatformEventTarget*>(this));

  float parent_rect[4] = {0};
  std::vector<fml::RefPtr<PlatformEventTarget>> parent_array;
  auto current = self;
  while (current != nullptr && current->ParentTarget() != current) {
    if (!current->IsVisible()) {
      return false;
    }
    if (current->IsOverlayContent()) {
      break;
    }
    if (current->EnableExposureUIClip() == LynxEventPropStatus::kEnable ||
        (current->EnableExposureUIClip() == LynxEventPropStatus::kUndefined &&
         current->IsScrollable()) ||
        current->IsRoot()) {
      parent_array.push_back(current);
      GetOrUpdateTargetScreenRect(common_ancestor_rect_map, current,
                                  parent_rect, root_view_origin_on_screen);
    }
    current = current->ParentTarget();
  }

  float root_rect[4] = {0};
  std::memcpy(root_rect, parent_rect, sizeof(float) * 4);

  float target_rect[4] = {0, 0, Width(), Height()};
  bool target_rect_calculated = false;
  for (const auto& parent : parent_array) {
    GetOrUpdateTargetScreenRect(common_ancestor_rect_map, parent, parent_rect,
                                root_view_origin_on_screen);

    if (!target_rect_calculated) {
      target_helper_->ConvertRectFromDescendantToAncestor(target_rect, self,
                                                          parent, target_rect);
      target_helper_->OffsetRect(target_rect, parent_rect);
      GetExposureTargetRect(target_rect);
      target_rect_calculated = true;
    }

    if (!target_helper_->CheckViewportIntersectWithRatio(
            target_rect, parent_rect, exposure_area_ratio_)) {
      return false;
    }
  }

  float window_rect_local[4] = {window_rect[0], window_rect[1], window_rect[2],
                                window_rect[3]};
  GetExposureWindowRect(window_rect_local);
  bool is_root_intersect_with_window =
      target_helper_->CheckViewportIntersectWithRatio(root_rect,
                                                      window_rect_local, 0);
  bool is_target_intersect_with_window =
      target_helper_->CheckViewportIntersectWithRatio(
          target_rect, window_rect_local, exposure_area_ratio_);
  return is_target_intersect_with_window && is_root_intersect_with_window;
}

void PlatformEventTarget::GetExposureTargetRect(float rect[4]) const {
  float device_pixel_ratio = target_helper_->GetDevicePixelRatio();
  rect[0] -= exposure_ui_margin_left_ * device_pixel_ratio;
  rect[1] -= exposure_ui_margin_top_ * device_pixel_ratio;
  rect[2] += exposure_ui_margin_right_ * device_pixel_ratio;
  rect[3] += exposure_ui_margin_bottom_ * device_pixel_ratio;
}

void PlatformEventTarget::GetExposureWindowRect(float rect[4]) const {
  float device_pixel_ratio = target_helper_->GetDevicePixelRatio();
  rect[0] -= exposure_screen_margin_left_ * device_pixel_ratio;
  rect[1] -= exposure_screen_margin_top_ * device_pixel_ratio;
  rect[2] += exposure_screen_margin_right_ * device_pixel_ratio;
  rect[3] += exposure_screen_margin_bottom_ * device_pixel_ratio;
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
