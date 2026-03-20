// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target_exposure.h"

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/event/custom_event.h"
#include "core/renderer/dom/fragment/event/platform_event_target.h"
#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx::tasm {

PlatformEventTargetExposure::ExposureTargetDetail::ExposureTargetDetail(
    const fml::RefPtr<PlatformEventTarget>& target, std::string unique_id,
    bool is_custom_event, bool is_global_event, bool intercept_global_event)
    : target_(target),
      unique_id_(std::move(unique_id)),
      is_custom_event_(is_custom_event),
      is_global_event_(is_global_event),
      intercept_global_event_(intercept_global_event) {}

lepus::Value PlatformEventTargetExposure::ExposureTargetDetail::ExposedParams()
    const {
  BASE_STATIC_STRING_DECL(kSign, "sign");
  BASE_STATIC_STRING_DECL(kExposureID, "exposure-id");
  BASE_STATIC_STRING_DECL(kExposureScene, "exposure-scene");
  BASE_STATIC_STRING_DECL(kDataset, "dataset");

  auto dict = lepus::Dictionary::Create();
  dict->SetValue(kSign, target_->Sign());
  dict->SetValue(kExposureID, target_->ExposureId());
  dict->SetValue(kExposureScene, target_->ExposureScene());
  auto dataset = target_->Dataset();
  dict->SetValue(kDataset, dataset.IsEmpty()
                               ? lepus::Value(lepus::Dictionary::Create())
                               : dataset);
  return lepus::Value(dict);
}

bool PlatformEventTargetExposure::ExposureTargetDetail::operator==(
    const ExposureTargetDetail& other) const {
  return unique_id_ == other.UniqueId();
}

bool PlatformEventTargetExposure::ExposureTargetDetail::operator!=(
    const ExposureTargetDetail& other) const {
  return !(*this == other);
}

bool PlatformEventTargetExposure::ExposureTargetDetail::operator<(
    const ExposureTargetDetail& other) const {
  return unique_id_ < other.UniqueId();
}

void PlatformEventTargetExposure::SetIntervalMs(int interval_ms) {
  if (interval_ms <= 0 || interval_ms > 1000 || interval_ms_ == interval_ms) {
    return;
  }
  interval_ms_ = interval_ms;
}

void PlatformEventTargetExposure::AddExposureTarget(
    const fml::RefPtr<PlatformEventTarget>& target,
    const lepus::Value& option) {
  if (target == nullptr) {
    return;
  }
  const auto meta = ParseExposureTargetMeta(option);
  auto it = exposure_target_map_.find(meta.unique_id);
  if (it != exposure_target_map_.end()) {
    return;
  }
  AddCommonAncestorRectMap(target.get());
  exposure_target_map_.insert_or_assign(
      meta.unique_id,
      ExposureTargetDetail(target, meta.unique_id, meta.is_custom_event,
                           meta.is_global_event, meta.intercept_global_event));
  if (exposure_target_map_.size() == 1) {
    StartExposureCheck();
  }
}

void PlatformEventTargetExposure::RemoveExposureTarget(
    const fml::RefPtr<PlatformEventTarget>& target,
    const lepus::Value& option) {
  if (target == nullptr || exposure_target_map_.empty()) {
    return;
  }
  const auto meta = ParseExposureTargetMeta(option);
  auto it = exposure_target_map_.find(meta.unique_id);
  if (it == exposure_target_map_.end()) {
    return;
  }
  if (meta.is_custom_event) {
    it->second.SetIsCustomEvent(false);
  }
  if (meta.is_global_event) {
    it->second.SetIsGlobalEvent(false);
  }
  if (!it->second.IsCustomEvent() && !it->second.IsGlobalEvent()) {
    RemoveCommonAncestorRectMap(target.get());
    exposure_target_map_.erase(it);
  }
  if (exposure_target_map_.empty()) {
    StopExposureCheck();
  }
}

void PlatformEventTargetExposure::ClearExposureTargetMap() {
  exposure_target_map_.clear();
  common_ancestor_rect_map_.clear();
}

void PlatformEventTargetExposure::StartExposureCheck() {
  if (scheduled_exposure_check_) {
    return;
  }
  scheduled_exposure_check_ = true;
  ScheduleNextExposureCheck();
}

void PlatformEventTargetExposure::StopExposureCheck() {
  if (!scheduled_exposure_check_) {
    return;
  }
  scheduled_exposure_check_ = false;
  SendEvent(visible_target_before_, "disexposure");
  visible_target_before_.clear();
  common_ancestor_rect_map_.clear();
  window_rect_valid_ = false;
}

void PlatformEventTargetExposure::AddCommonAncestorRectMap(
    PlatformEventTarget* target) {
  if (target == nullptr) {
    return;
  }
  auto current = target->ParentTarget();
  while (current != nullptr && current->ParentTarget() != current) {
    if (current->IsScrollable()) {
      int32_t sign = current->Sign();
      auto it = common_ancestor_rect_map_.find(sign);
      if (it != common_ancestor_rect_map_.end()) {
        it->second.target_count++;
      } else {
        CommonAncestorRect rect;
        rect.target_count = 1;
        rect.rect_updated = false;
        common_ancestor_rect_map_.emplace(sign, std::move(rect));
      }
    }
    current = current->ParentTarget();
  }
}

void PlatformEventTargetExposure::RemoveCommonAncestorRectMap(
    PlatformEventTarget* target) {
  if (target == nullptr) {
    return;
  }
  auto current = target->ParentTarget();
  while (current != nullptr && current->ParentTarget() != current) {
    auto it = common_ancestor_rect_map_.find(current->Sign());
    if (it != common_ancestor_rect_map_.end()) {
      it->second.target_count--;
      if (it->second.target_count <= 0) {
        common_ancestor_rect_map_.erase(it);
      }
    }
    current = current->ParentTarget();
  }
}

void PlatformEventTargetExposure::ResetCommonAncestorRectMap() {
  for (auto& it : common_ancestor_rect_map_) {
    it.second.rect_updated = false;
  }
}

void PlatformEventTargetExposure::ScheduleNextExposureCheck() {
  if (!scheduled_exposure_check_ || !task_runner_ ||
      exposure_target_map_.empty()) {
    return;
  }

  task_runner_->PostDelayedTask(
      [weak_this = weak_from_this()]() {
        auto self = weak_this.lock();
        if (!self || !self->scheduled_exposure_check_) {
          return;
        }
        self->DoExposureCheck();
        self->ScheduleNextExposureCheck();
      },
      fml::TimeDelta::FromMilliseconds(interval_ms_));
}

void PlatformEventTargetExposure::DoExposureCheck() {
  if (exposure_target_map_.empty()) {
    return;
  }
  bool did_reconstruct = false;
  auto root =
      platform_ref_->ReconstructEventTargetTreeRecursively(&did_reconstruct);
  if (!root) {
    if (!visible_target_before_.empty()) {
      SendEvent(visible_target_before_, "disexposure");
      visible_target_before_.clear();
    }
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, EXPOSURE_DO_EXPOSURE_CHECK);
  auto* helper = platform_ref_->GetEventTargetHelper();
  float root_view_origin_on_screen[2] = {0, 0};
  helper->GetRootViewLocationOnScreen(root_view_origin_on_screen);

  std::set<ExposureTargetDetail> visible_target_now;
  std::set<ExposureTargetDetail> appear_target_set;
  std::set<ExposureTargetDetail> disappear_target_set;

  if (did_reconstruct) {
    ResetCommonAncestorRectMap();
  }
  if (did_reconstruct || !window_rect_valid_) {
    float size[2] = {0};
    helper->GetScreenSize(size);
    window_rect_[0] = 0;
    window_rect_[1] = 0;
    window_rect_[2] = size[0];
    window_rect_[3] = size[1];
    window_rect_valid_ = true;
  }

  for (const auto& it : exposure_target_map_) {
    const auto& detail = it.second;
    const auto& target = detail.Target();
    if (target && target->IsVisibleForExposure(common_ancestor_rect_map_,
                                               root_view_origin_on_screen,
                                               window_rect_)) {
      visible_target_now.insert(detail);
    }
  }

  std::set_difference(
      visible_target_now.begin(), visible_target_now.end(),
      visible_target_before_.begin(), visible_target_before_.end(),
      std::inserter(appear_target_set, appear_target_set.begin()));
  std::set_difference(
      visible_target_before_.begin(), visible_target_before_.end(),
      visible_target_now.begin(), visible_target_now.end(),
      std::inserter(disappear_target_set, disappear_target_set.begin()));

  visible_target_before_ = std::move(visible_target_now);

  SendEvent(disappear_target_set, "disexposure");
  SendEvent(appear_target_set, "exposure");
}

void PlatformEventTargetExposure::SendEvent(
    const std::set<ExposureTargetDetail>& details,
    const std::string& event_name) const {
  if (details.empty()) {
    return;
  }

  for (const auto& detail : details) {
    if (detail.IsCustomEvent()) {
      const auto& target = detail.Target();
      if (!target) {
        continue;
      }
      const char* event_type = nullptr;
      PlatformEventName required_event = PlatformEventName::kUnknown;
      if (event_name == "exposure") {
        required_event = PlatformEventName::kUIAppear;
        event_type = "uiappear";
      }
      if (event_name == "disexposure") {
        required_event = PlatformEventName::kUIDisappear;
        event_type = "uidisappear";
      }

      const auto& event_set = target->EventSet();
      auto has_event = [&event_set](PlatformEventName name) -> bool {
        return std::find(event_set.begin(), event_set.end(), name) !=
               event_set.end();
      };

      if (event_name == "exposure" && has_event(required_event)) {
        auto event = fml::MakeRefCounted<event::CustomEvent>(
            event_type, detail.ExposedParams(), "detail");
        platform_ref_->SendEvent(target->Sign(), event);
      }
      if (event_name == "disexposure" && has_event(required_event)) {
        auto event = fml::MakeRefCounted<event::CustomEvent>(
            event_type, detail.ExposedParams(), "detail");
        platform_ref_->SendEvent(target->Sign(), event);
      }
    }
  }
}

ExposureTargetMeta PlatformEventTargetExposure::ParseExposureTargetMeta(
    const lepus::Value& option) {
  ExposureTargetMeta meta;
  if (!option.IsObject()) {
    return meta;
  }

  BASE_STATIC_STRING_DECL(kUniqueId, "unique-id");
  BASE_STATIC_STRING_DECL(kIsCustomEvent, "is-custom-event");
  BASE_STATIC_STRING_DECL(kIsGlobalEvent, "is-global-event");
  BASE_STATIC_STRING_DECL(kInterceptGlobalEvent, "intercept-global-event");

  auto v_unique_id = option.GetProperty(kUniqueId);
  if (v_unique_id.IsString()) {
    meta.unique_id = v_unique_id.StdString();
  }

  auto v_is_custom_event = option.GetProperty(kIsCustomEvent);
  if (v_is_custom_event.IsBool()) {
    meta.is_custom_event = v_is_custom_event.Bool();
  }

  auto v_is_global_event = option.GetProperty(kIsGlobalEvent);
  if (v_is_global_event.IsBool()) {
    meta.is_global_event = v_is_global_event.Bool();
  }

  auto v_intercept_global_event = option.GetProperty(kInterceptGlobalEvent);
  if (v_intercept_global_event.IsBool()) {
    meta.intercept_global_event = v_intercept_global_event.Bool();
  }

  return meta;
}

}  // namespace lynx::tasm
