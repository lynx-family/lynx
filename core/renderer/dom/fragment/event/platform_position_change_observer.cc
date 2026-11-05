// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_position_change_observer.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "base/include/fml/time/time_delta.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/event/custom_event.h"
#include "core/renderer/dom/fragment/event/platform_event_target.h"
#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx::tasm {

void PlatformPositionChangeObserver::UpdateTarget(int32_t sign, bool listens) {
  if (!listens) {
    targets_.erase(sign);
    return;
  }
  if (targets_.try_emplace(sign).second) {
    ScheduleRootViewCheck();
    RequestCheck();
  }
}

void PlatformPositionChangeObserver::Clear() { targets_.clear(); }

void PlatformPositionChangeObserver::RequestCheck() {
  if (targets_.empty() || check_pending_ || !task_runner_) {
    return;
  }
  check_pending_ = true;
  // The platform ref owns this observer and keeps it alive during callbacks.
  task_runner_->PostTask([this, weak_ref = platform_ref_->weak_from_this()]() {
    if (auto ref = weak_ref.lock()) {
      DispatchPositionChange();
    }
  });
}

void PlatformPositionChangeObserver::ScheduleRootViewCheck() {
  if (targets_.empty() || root_view_check_pending_ || !task_runner_) {
    return;
  }
  root_view_check_pending_ = true;
  // Share the exposure interval so both observers follow observerFrameRate.
  task_runner_->PostDelayedTask(
      [this, weak_ref = platform_ref_->weak_from_this()]() {
        if (auto ref = weak_ref.lock()) {
          root_view_check_pending_ = false;
          CheckRootViewLocation();
          ScheduleRootViewCheck();
        }
      },
      fml::TimeDelta::FromMilliseconds(platform_ref_->GetExposureIntervalMs()));
}

void PlatformPositionChangeObserver::CheckRootViewLocation() {
  if (targets_.empty()) {
    return;
  }
  float location[2] = {0.f, 0.f};
  if (!GetWindowOrigin(location)) {
    window_origin_lost_ = true;
    return;
  }
  if (window_origin_lost_ || location[0] != root_view_location_[0] ||
      location[1] != root_view_location_[1]) {
    RequestCheck();
  }
}

bool PlatformPositionChangeObserver::GetWindowOrigin(float origin[2]) {
  return platform_ref_->GetRootViewLocationInWindow(origin) &&
         std::isfinite(origin[0]) && std::isfinite(origin[1]);
}

void PlatformPositionChangeObserver::DispatchPositionChange() {
  check_pending_ = false;
  if (targets_.empty()) {
    return;
  }
  // Without a window, keep the last rects and wait for the host to attach.
  float origin[2] = {0.f, 0.f};
  if (!GetWindowOrigin(origin)) {
    window_origin_lost_ = true;
    return;
  }
  if (platform_ref_->EnsureEventTargetTree(kRootId) == nullptr) {
    return;
  }
  if (window_origin_lost_) {
    // Positions may have changed while detached; report them all again.
    window_origin_lost_ = false;
    for (auto& [sign, last] : targets_) {
      last.reset();
    }
  }
  root_view_location_ = {origin[0], origin[1]};

  auto* helper = platform_ref_->GetEventTargetHelper();
  // Independent roots (e.g. overlays) keep their own trees; refresh them too.
  const auto& active_root_ids = helper->GetActiveEventRootIds();
  base::Vector<int32_t> signs;
  signs.reserve(active_root_ids.size());
  for (const int32_t root_id : active_root_ids) {
    signs.push_back(root_id);
  }
  for (const int32_t root_id : signs) {
    platform_ref_->EnsureEventTargetTree(root_id);
  }

  BASE_STATIC_STRING_DECL(kId, "id");
  BASE_STATIC_STRING_DECL(kDataset, "dataset");
  BASE_STATIC_STRING_DECL(kLeft, "left");
  BASE_STATIC_STRING_DECL(kTop, "top");
  BASE_STATIC_STRING_DECL(kRight, "right");
  BASE_STATIC_STRING_DECL(kBottom, "bottom");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  BASE_STATIC_STRING_DECL(kWindowX, "windowX");
  BASE_STATIC_STRING_DECL(kWindowY, "windowY");

  const float unit = helper->GetLayoutsUnitPerPx();

  // Event callbacks can synchronously change listener registrations.
  signs.clear();
  signs.reserve(targets_.size());
  for (const auto& [sign, last] : targets_) {
    signs.push_back(sign);
  }
  for (const int32_t sign : signs) {
    auto cached = targets_.find(sign);
    if (cached == targets_.end()) {
      continue;
    }
    auto& last = cached->second;
    // Event bundles own registration; a target outside the tree is skipped.
    auto target = helper->GetEventTarget(sign);
    if (target == nullptr) {
      continue;
    }

    float rect[4] = {0.f, 0.f, target->Width(), target->Height()};
    if (!helper->ConvertRectFromTargetToPageRootTarget(rect, target, rect) ||
        !std::all_of(std::begin(rect), std::end(rect),
                     [](float value) { return std::isfinite(value); })) {
      continue;
    }
    const std::array<float, 4> window_rect = {
        rect[0] + origin[0], rect[1] + origin[1], rect[2] - rect[0],
        rect[3] - rect[1]};
    if (last && *last == window_rect) {
      continue;
    }
    last = window_rect;

    auto detail = lepus::Dictionary::Create();
    detail->SetValue(kId, target->IDSelector());
    const auto& dataset = target->Dataset();
    detail->SetValue(kDataset, dataset.IsEmpty()
                                   ? lepus::Value(lepus::Dictionary::Create())
                                   : dataset);
    detail->SetValue(kLeft, rect[0] / unit);
    detail->SetValue(kTop, rect[1] / unit);
    detail->SetValue(kRight, rect[2] / unit);
    detail->SetValue(kBottom, rect[3] / unit);
    detail->SetValue(kWidth, window_rect[2] / unit);
    detail->SetValue(kHeight, window_rect[3] / unit);
    detail->SetValue(kWindowX, window_rect[0] / unit);
    detail->SetValue(kWindowY, window_rect[1] / unit);
    platform_ref_->SendEvent(
        sign, fml::MakeRefCounted<event::CustomEvent>(
                  "positionchange", lepus::Value(std::move(detail)), "detail"));
  }
}

}  // namespace lynx::tasm
