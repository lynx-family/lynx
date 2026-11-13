// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
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

float PlatformEventTarget::DistanceToTouchTarget(const float point[2]) const {
  if (minimum_touch_target_px_ < 0.f || minimum_touch_target_height_px_ < 0.f ||
      !ShouldHitTest() || PointerEvents() == LynxPointerEventsValue::kNone) {
    return std::numeric_limits<float>::infinity();
  }
  // Minimum target sizes are physical pixels; geometry uses platform units.
  const float scale = target_helper_->GetPhysicalPixelsPerLayoutUnit();
  const float padding_x =
      std::max(0.f, minimum_touch_target_px_ / scale - Width()) / 2;
  const float padding_y =
      std::max(0.f, minimum_touch_target_height_px_ / scale - Height()) / 2;
  if (point[0] < -padding_x || point[0] >= Width() + padding_x ||
      point[1] < -padding_y || point[1] >= Height() + padding_y) {
    return std::numeric_limits<float>::infinity();
  }
  const float dx = std::max({0.f, -point[0], point[0] - Width()});
  const float dy = std::max({0.f, -point[1], point[1] - Height()});
  return dx * dx + dy * dy;
}

PlatformEventTarget::HitTestResult PlatformEventTarget::HitTestInternal(
    float point[2], HitTestMode mode, bool visit_children) {
  const bool event_targets_only = mode == HitTestMode::kRegisteredTargets;
  HitTestResult result;
  if (visit_children) {
    const auto& children = ChildrenTargets();
    // Traverse siblings in reverse paint order, as for ordinary FLR input.
    for (auto child = children.rbegin(); child != children.rend(); ++child) {
      if (!(*child)->ShouldHitTest()) {
        continue;
      }
      float local[2];
      if (!(*child)->GetPointInTarget(
              local, fml::RefPtr<PlatformEventTarget>(this), point)) {
        continue;
      }
      const bool inside = (*child)->ContainsPoint(local);
      if (!inside && (!event_targets_only ||
                      !std::isfinite((*child)->DistanceToTouchTarget(local)))) {
        continue;
      }
      auto hit = (*child)->HitTestInternal(local, mode, inside);
      if (hit.targets.empty()) {
        continue;
      }
      if (result.targets.empty() || (hit.direct && !result.direct) ||
          (!hit.direct && !result.direct && hit.distance < result.distance)) {
        result = std::move(hit);
      } else if (result.direct && result.share && hit.direct) {
        for (auto& target : hit.targets) {
          result.targets.emplace_back(std::move(target));
        }
        result.share = hit.share;
      }
      // Expanded targets must not obscure a lower sibling's direct hit.
      if (result.direct && !result.share) {
        break;
      }
    }
  }
  const float distance = event_targets_only
                             ? DistanceToTouchTarget(point)
                             : std::numeric_limits<float>::infinity();
  const bool direct = point[0] >= 0.f && point[0] < Width() &&
                      point[1] >= 0.f && point[1] < Height();
  if (!result.targets.empty()) {
    if (std::isfinite(distance)) {
      // An eligible ancestor also receives this branch. Its sharing policy
      // remains effective when separate handlers occupy separate Elements.
      result.share = result.share || share_touch_target_;
      if (direct) {
        result.direct = true;
        result.distance = 0.f;
      }
    }
    return result;
  }
  if (std::isfinite(distance)) {
    result.targets.emplace_back(this);
    result.distance = distance;
    result.direct = direct;
    result.share = share_touch_target_;
  } else if (!event_targets_only && !is_layout_only_ &&
             PointerEvents() != LynxPointerEventsValue::kNone) {
    // Preserve the ordinary traversal root fallback, including overflow boxes.
    result.targets.emplace_back(this);
    result.direct = true;
  }
  return result;
}

fml::RefPtr<PlatformEventTarget> PlatformEventTarget::HitTest(
    float point[2], HitTestMode mode, HitTestTargets* targets) {
  auto result = HitTestInternal(point, mode, true);
  if (targets) {
    targets->clear();
    for (const auto& target : result.targets) {
      targets->emplace_back(target->Sign());
    }
  }
  return result.targets.empty() ? nullptr : result.targets.front();
}

bool PlatformEventTarget::ShouldHitTest() const {
  return user_interaction_enabled_;
}

bool PlatformEventTarget::GetPointInTarget(
    float target_point[2], fml::RefPtr<PlatformEventTarget> parent_target,
    float point[2]) {
  return target_helper_->ConvertPointFromAncestorToDescendant(
      target_point, parent_target, fml::RefPtr<PlatformEventTarget>(this),
      point);
}

void PlatformEventTarget::SetHitSlop(EventRegion hit_slop) {
  NormalizeEventRegionUnits(&hit_slop, 1);
  *hit_slop_ = std::move(hit_slop);
}

void PlatformEventTarget::SetEventThroughActiveRegions(
    std::vector<EventRegion> regions) {
  if (regions.empty()) {
    event_through_active_regions_.reset();
    return;
  }
  NormalizeEventRegionUnits(regions.data(), regions.size());
  *event_through_active_regions_ = std::move(regions);
}

void PlatformEventTarget::SetBlockNativeEventAreas(
    std::vector<EventRegion> areas) {
  if (areas.empty()) {
    block_native_event_areas_.reset();
    return;
  }
  NormalizeEventRegionUnits(areas.data(), areas.size());
  *block_native_event_areas_ = std::move(areas);
}

void PlatformEventTarget::NormalizeEventRegionUnits(EventRegion* regions,
                                                    size_t region_count) {
  using Type = EventRegionSizeValue::Type;
  const float physical_pixels_per_layout_unit =
      target_helper_ ? target_helper_->GetPhysicalPixelsPerLayoutUnit() : 1.f;
  for (size_t i = 0; i < region_count; ++i) {
    for (auto& value : regions[i]) {
      switch (value.type) {
        case Type::kPhysicalPx:
          // ppx denotes physical pixels even when layout uses points or vp.
          value.value /= physical_pixels_per_layout_unit;
          value.type = Type::kLayoutUnit;
          break;
        case Type::kPlatformUnit:
          // Unitless strings use native coordinates, matching pointer events
          // and display-list geometry without any density conversion.
          value.type = Type::kLayoutUnit;
          break;
        default:
          // Keep CSS px, rpx and percentages for conversion at hit time so page
          // metrics and target-size changes also affect existing regions.
          // Normalized sides may be reused by partial hit-slop updates.
          break;
      }
    }
  }
}

bool PlatformEventTarget::ContainsPoint(float point[2]) {
  float x = point[0];
  float y = point[1];
  if (hit_test_regions_) {
    for (const auto& region : *hit_test_regions_) {
      if (x >= region.left && x <= region.right && y >= region.top &&
          y <= region.bottom) {
        return true;
      }
    }
    return false;
  }
  float left = 0.f;
  float top = 0.f;
  float right = Width();
  float bottom = Height();
  if (hit_slop_) {
    left = -ConvertEventRegionSizeValue((*hit_slop_)[0], true);
    top = -ConvertEventRegionSizeValue((*hit_slop_)[1], false);
    right = Width() + ConvertEventRegionSizeValue((*hit_slop_)[2], true);
    bottom = Height() + ConvertEventRegionSizeValue((*hit_slop_)[3], false);
  }
  if (x >= left && x <= right && y >= top && y <= bottom) {
    return true;
  }
  if (overflow_x_ && !overflow_y_ && (y < top || y > bottom)) {
    return false;
  }
  if (overflow_y_ && !overflow_x_ && (x < left || x > right)) {
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
         (current->RendererHostSign() == current->Sign() &&
          current->IsScrollContainer())) ||
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
  float layouts_unit_per_px = target_helper_->GetLayoutsUnitPerPx();
  rect[0] -= exposure_ui_margin_left_ * layouts_unit_per_px;
  rect[1] -= exposure_ui_margin_top_ * layouts_unit_per_px;
  rect[2] += exposure_ui_margin_right_ * layouts_unit_per_px;
  rect[3] += exposure_ui_margin_bottom_ * layouts_unit_per_px;
}

void PlatformEventTarget::GetExposureWindowRect(float rect[4]) const {
  float layouts_unit_per_px = target_helper_->GetLayoutsUnitPerPx();
  rect[0] -= exposure_screen_margin_left_ * layouts_unit_per_px;
  rect[1] -= exposure_screen_margin_top_ * layouts_unit_per_px;
  rect[2] += exposure_screen_margin_right_ * layouts_unit_per_px;
  rect[3] += exposure_screen_margin_bottom_ * layouts_unit_per_px;
}

void PlatformEventTarget::OnResponseChain() {}

void PlatformEventTarget::OffResponseChain() {}

bool PlatformEventTarget::IsOnResponseChain() const { return false; }

bool PlatformEventTarget::TouchPseudoPropagation() const {
  return touch_pseudo_propagation_;
}

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

  if (event_through_active_regions_) {
    is_event_through = HitEventRegions(*event_through_active_regions_, point)
                           ? is_event_through
                           : !is_event_through;
  }

  return is_event_through;
}

bool PlatformEventTarget::HitEventRegions(
    const std::vector<EventRegion>& regions, float point[2]) const {
  for (const auto& region : regions) {
    const float left = ConvertEventRegionSizeValue(region[0], true);
    const float top = ConvertEventRegionSizeValue(region[1], false);
    const float right = left + ConvertEventRegionSizeValue(region[2], true);
    const float bottom = top + ConvertEventRegionSizeValue(region[3], false);
    if (point[0] >= left && point[0] < right && point[1] >= top &&
        point[1] < bottom) {
      return true;
    }
  }
  return false;
}

float PlatformEventTarget::ConvertEventRegionSizeValue(
    const EventRegionSizeValue& value, bool is_horizontal) const {
  if (value.type == EventRegionSizeValue::Type::kPercentage) {
    return value.value * (is_horizontal ? Width() : Height());
  }
  if (value.type == EventRegionSizeValue::Type::kLayoutUnit) {
    return value.value;
  }
  if (value.type == EventRegionSizeValue::Type::kRpx) {
    // The UI-thread cache avoids platform queries in the hit-test path.
    return target_helper_ ? value.value * target_helper_->GetLayoutsUnitPerRpx()
                          : 0.f;
  }
  return target_helper_ ? value.value * target_helper_->GetLayoutsUnitPerPx()
                        : value.value;
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
  if (pointer_events_ != LynxPointerEventsValue::kUnset) {
    return pointer_events_;
  }
  auto parent = ParentTarget();
  return parent && parent.get() != this ? parent->PointerEvents()
                                        : LynxPointerEventsValue::kAuto;
}

bool PlatformEventTarget::BlockNativeEvent(float point[2]) const {
  return block_native_event_ ||
         (block_native_event_areas_ &&
          HitEventRegions(*block_native_event_areas_, point));
}

bool PlatformEventTarget::ConsumeSlideEvent(float angle) const {
  for (const auto& range : ConsumeSlideEventAngles()) {
    if (angle >= range[0] && angle <= range[1]) {
      return true;
    }
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
