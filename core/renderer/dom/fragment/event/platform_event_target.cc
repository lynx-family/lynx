// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"

namespace lynx {
namespace tasm {

float PlatformEventTarget::ScrollOffsetX() {
  UpdateScrollOffsetIfNeeded();
  return scroll_offset_x_;
}

float PlatformEventTarget::ScrollOffsetY() {
  UpdateScrollOffsetIfNeeded();
  return scroll_offset_y_;
}

void PlatformEventTarget::RefreshScrollOffset() {
  if (!is_scroll_container_) {
    return;
  }
  scroll_offset_updated_ = false;
  UpdateScrollOffsetIfNeeded();
}

void PlatformEventTarget::SetTransform(const float transform[16]) {
  if (transform == nullptr) {
    transform_.reset();
    return;
  }
  std::array<float, 16> matrix;
  std::copy_n(transform, matrix.size(), matrix.begin());
  (*transform_).Matrix(matrix);
}

void PlatformEventTarget::UpdateScrollOffsetIfNeeded() {
  if (scroll_offset_updated_ || !is_scroll_container_) {
    return;
  }
  scroll_offset_updated_ = true;
  if (target_helper_ == nullptr) {
    return;
  }
  float offset[2] = {scroll_offset_x_, scroll_offset_y_};
  target_helper_->GetPlatformRendererScrollOffset(sign_, offset);
  scroll_offset_x_ = offset[0];
  scroll_offset_y_ = offset[1];
}

fml::RefPtr<PlatformEventTarget> PlatformEventTarget::HitTest(float point[2]) {
  fml::RefPtr<PlatformEventTarget> target = nullptr;
  const auto& children = ChildrenTargets();
  int children_size = static_cast<int>(children.size());
  int target_idx = -1;
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
      target_idx = i;
      target = child;
      break;
    }
  }

  auto hit_target =
      target ? target->HitTest(target_point)
             : (is_layout_only_ ? nullptr
                                : fml::RefPtr<PlatformEventTarget>(this));
  // when a node has pointer-events:none set, the hit_target needs to be found
  // again.
  if (!hit_target ||
      hit_target->PointerEvents() == LynxPointerEventsValue::kNone) {
    hit_target = nullptr;
    for (int i = target_idx - 1; i >= 0; --i) {
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
      if (hit_target &&
          hit_target->PointerEvents() != LynxPointerEventsValue::kNone) {
        break;
      }
      hit_target = nullptr;
    }
  }
  if (hit_target) {
    return hit_target;
  }
  return is_layout_only_ ? nullptr : fml::RefPtr<PlatformEventTarget>(this);
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

bool PlatformEventTarget::ContainsPoint(float point[2]) {
  float x = point[0];
  float y = point[1];
  if (x >= 0.f && x <= Width() && y >= 0.f && y <= Height()) {
    return true;
  }
  if (overflow_x_ && !overflow_y_ && (y < 0.f || y > Height())) {
    return false;
  }
  if (overflow_y_ && !overflow_x_ && (x < 0.f || x > Width())) {
    return false;
  }
  if (overflow_x_ || overflow_y_) {
    auto hit_target = HitTest(point);
    return hit_target != nullptr && hit_target.get() != this;
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

void PlatformEventTarget::OnPseudoStatusChanged(
    LynxPseudoStatus pre_status, LynxPseudoStatus current_status) {}

LynxPseudoStatus PlatformEventTarget::GetPseudoStatus() const {
  return LynxPseudoStatus::kNone;
}

bool PlatformEventTarget::TouchPseudoPropagation() const { return true; }

bool PlatformEventTarget::EventThrough(
    float point[2], const PlatformEventThroughConfig& config) const {
  if (events_pass_through_ == LynxEventPropStatus::kEnable) {
    return true;
  }
  return EventThroughInternal(point, config);
}

bool PlatformEventTarget::EventThroughInternal(
    float point[2], const PlatformEventThroughConfig& config) const {
  bool is_event_through = false;
  if (event_through_ == LynxEventPropStatus::kEnable) {
    is_event_through = true;
  } else if (event_through_ == LynxEventPropStatus::kDisable) {
    is_event_through = false;
  } else {
    auto parent = ParentTarget();
    if (parent != nullptr && parent.get() != this &&
        (config.enable_event_through_inherit_from_page ||
         !parent->IsPageRoot())) {
      float parent_point[2] = {point[0], point[1]};
      if (target_helper_ != nullptr) {
        auto self = fml::RefPtr<PlatformEventTarget>(
            const_cast<PlatformEventTarget*>(this));
        target_helper_->ConvertPointFromDescendantToAncestor(parent_point, self,
                                                             parent, point);
      }
      is_event_through = parent->EventThroughInternal(parent_point, config);
    }
  }

  // PageConfig.enableEventThrough is a fallback for the page root. A
  // descendant can only observe it when inheritance is allowed to reach the
  // page, and an explicit value on the descendant still takes precedence.
  if (!is_event_through && IsPageRoot()) {
    is_event_through = config.enable_event_through;
  }

  if (!event_through_active_regions_.empty()) {
    is_event_through = HitEventThroughActiveRegions(point) ? is_event_through
                                                           : !is_event_through;
  }

  return is_event_through;
}

bool PlatformEventTarget::HitEventThroughActiveRegions(float point[2]) const {
  for (const auto& region : event_through_active_regions_) {
    const float left = ConvertEventThroughSizeValue(region[0], true);
    const float top = ConvertEventThroughSizeValue(region[1], false);
    const float right = left + ConvertEventThroughSizeValue(region[2], true);
    const float bottom = top + ConvertEventThroughSizeValue(region[3], false);
    if (point[0] >= left && point[0] < right && point[1] >= top &&
        point[1] < bottom) {
      return true;
    }
  }
  return false;
}

float PlatformEventTarget::ConvertEventThroughSizeValue(
    const EventThroughSizeValue& value, bool is_horizontal) const {
  if (value.type == EventThroughSizeValue::Type::kPercentage) {
    return value.value * (is_horizontal ? Width() : Height());
  }
  return value.value;
}

bool PlatformEventTarget::IgnoreFocus() const {
  if (ignore_focus_ == LynxEventPropStatus::kEnable) {
    return true;
  }
  if (ignore_focus_ == LynxEventPropStatus::kDisable) {
    return false;
  }

  // An event root does not inherit ignore-focus from another event root. A
  // descendant may only inherit from a parent in the same event target tree.
  if (IsRoot()) {
    return false;
  }
  auto parent = ParentTarget();
  if (parent == nullptr || parent.get() == this ||
      parent->RootId() != RootId()) {
    return false;
  }
  return parent->IgnoreFocus();
}

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
