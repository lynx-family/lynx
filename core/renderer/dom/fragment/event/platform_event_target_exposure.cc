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
    int32_t id, std::string unique_id, std::string exposure_id,
    std::string exposure_scene, lepus::Value dataset)
    : id_(id),
      unique_id_(std::move(unique_id)),
      exposure_id_(std::move(exposure_id)),
      exposure_scene_(std::move(exposure_scene)),
      dataset_(std::move(dataset)) {}

int32_t PlatformEventTargetExposure::ExposureTargetDetail::ID() const {
  return id_;
}

lepus::Value PlatformEventTargetExposure::ExposureTargetDetail::ExposedParams()
    const {
  BASE_STATIC_STRING_DECL(kSign, "sign");
  BASE_STATIC_STRING_DECL(kUniqueID, "unique-id");
  BASE_STATIC_STRING_DECL(kExposureID, "exposure-id");
  BASE_STATIC_STRING_DECL(kExposureScene, "exposure-scene");
  BASE_STATIC_STRING_DECL(kDataset, "dataset");

  auto dict = lepus::Dictionary::Create();
  dict->SetValue(kSign, id_);
  dict->SetValue(kUniqueID, unique_id_);
  dict->SetValue(kExposureID, exposure_id_);
  dict->SetValue(kExposureScene, exposure_scene_);
  dict->SetValue(kDataset, dataset_);
  return lepus::Value(dict);
}

bool PlatformEventTargetExposure::ExposureTargetDetail::operator==(
    const ExposureTargetDetail& other) const {
  return unique_id_ == other.unique_id_ && id_ == other.id_ &&
         exposure_scene_ == other.exposure_scene_ &&
         exposure_id_ == other.exposure_id_;
}

bool PlatformEventTargetExposure::ExposureTargetDetail::operator!=(
    const ExposureTargetDetail& other) const {
  return !(*this == other);
}

bool PlatformEventTargetExposure::ExposureTargetDetail::operator<(
    const ExposureTargetDetail& other) const {
  return (unique_id_ < other.unique_id_) ||
         (unique_id_ == other.unique_id_ && id_ < other.id_) ||
         (unique_id_ == other.unique_id_ && id_ == other.id_ &&
          exposure_scene_ < other.exposure_scene_) ||
         (unique_id_ == other.unique_id_ && id_ == other.id_ &&
          exposure_scene_ == other.exposure_scene_ &&
          exposure_id_ < other.exposure_id_);
}

void PlatformEventTargetExposure::SetTaskRunner(
    const fml::RefPtr<fml::TaskRunner>& task_runner) {
  task_runner_ = task_runner;
  if (task_runner_ && scheduled_exposure_check_) {
    ScheduleNextExposureCheck();
  }
}

void PlatformEventTargetExposure::SetIntervalMs(int interval_ms) {
  if (interval_ms <= 0 || interval_ms > 1000 || interval_ms_ == interval_ms) {
    return;
  }
  interval_ms_ = interval_ms;
}

void PlatformEventTargetExposure::AddExposureTarget(
    int32_t id, const std::string& unique_id, const std::string& exposure_id,
    const std::string& exposure_scene, const lepus::Value& dataset) {
  AddCommonAncestorRectMap(id);
  const auto key =
      BuildExposureUniqueKey(id, unique_id, exposure_id, exposure_scene);
  exposure_target_map_[key] =
      ExposureTargetDetail(id, unique_id, exposure_id, exposure_scene, dataset);
  if (exposure_target_map_.size() == 1) {
    StartExposureCheck();
  }
}

void PlatformEventTargetExposure::RemoveExposureTarget(
    int32_t id, const std::string& unique_id, const std::string& exposure_id,
    const std::string& exposure_scene) {
  if (exposure_target_map_.empty()) {
    return;
  }
  RemoveCommonAncestorRectMap(id);
  exposure_target_map_.erase(
      BuildExposureUniqueKey(id, unique_id, exposure_id, exposure_scene));
  if (exposure_target_map_.empty()) {
    StopExposureCheck();
  }
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

void PlatformEventTargetExposure::AddCommonAncestorRectMap(int32_t id) {
  auto* helper =
      platform_ref_ ? platform_ref_->GetEventTargetHelper() : nullptr;
  if (!helper) {
    return;
  }
  auto target = helper->GetEventTarget(id);
  if (!target) {
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

void PlatformEventTargetExposure::RemoveCommonAncestorRectMap(int32_t id) {
  auto* helper =
      platform_ref_ ? platform_ref_->GetEventTargetHelper() : nullptr;
  if (!helper) {
    return;
  }
  auto target = helper->GetEventTarget(id);
  if (!target) {
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
    auto target = helper ? helper->GetEventTarget(detail.ID()) : nullptr;
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
    const std::set<ExposureTargetDetail>& targets,
    const std::string& event_name) const {
  if (targets.empty()) {
    return;
  }

  for (const auto& detail : targets) {
    auto* helper = platform_ref_->GetEventTargetHelper();
    auto target = helper ? helper->GetEventTarget(detail.ID()) : nullptr;
    static const base::Vector<PlatformEventName> empty_event_set;
    const auto& event_set = target ? target->EventSet() : empty_event_set;

    PlatformEventName required_event = PlatformEventName::kUnknown;
    const char* event_type = nullptr;
    if (event_name == "exposure") {
      required_event = PlatformEventName::kUIAppear;
      event_type = "uiappear";
    }
    if (event_name == "disexposure") {
      required_event = PlatformEventName::kUIDisappear;
      event_type = "uidisappear";
    }

    auto has_event = [&event_set](PlatformEventName name) -> bool {
      return std::find(event_set.begin(), event_set.end(), name) !=
             event_set.end();
    };

    if (event_name == "exposure" && has_event(required_event)) {
      auto event = fml::MakeRefCounted<event::CustomEvent>(
          event_type, detail.ExposedParams(), "detail");
      platform_ref_->SendEvent(detail.ID(), event);
    }
    if (event_name == "disexposure" && has_event(required_event)) {
      auto event = fml::MakeRefCounted<event::CustomEvent>(
          event_type, detail.ExposedParams(), "detail");
      platform_ref_->SendEvent(detail.ID(), event);
    }
  }
}

std::string PlatformEventTargetExposure::BuildExposureUniqueKey(
    int32_t id, const std::string& unique_id, const std::string& exposure_id,
    const std::string& exposure_scene) const {
  if (!unique_id.empty()) {
    return unique_id + "_" + exposure_scene + "_" + exposure_id;
  }
  return exposure_scene + "_" + exposure_id + "_" + std::to_string(id);
}

}  // namespace lynx::tasm
