// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_exposure.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/float_comparison.h"
#include "base/include/fml/task_runner.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_observer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIExposure::UIExposureDetail::UIExposureDetail(int ui_id, std::string unique_id,
                                               std::string exposure_id,
                                               std::string exposure_scene,
                                               lepus::Value extra_data,
                                               lepus::Value dataset,
                                               bool is_custom_event)
    : ui_id_(ui_id),
      unique_id_(std::move(unique_id)),
      exposure_id_(std::move(exposure_id)),
      exposure_scene_(std::move(exposure_scene)),
      extra_data_(std::move(extra_data)),
      dataset_(std::move(dataset)),
      is_custom_event_(is_custom_event) {}

int UIExposure::UIExposureDetail::ID() const { return ui_id_; }

bool UIExposure::UIExposureDetail::IsCustomEvent() const {
  return is_custom_event_;
}

lepus::Value UIExposure::UIExposureDetail::ExposedParams() const {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue(base::String("sign"), ui_id_);
  dict->SetValue(base::String("unique-id"), unique_id_);
  dict->SetValue(base::String("exposure-id"), exposure_id_);
  dict->SetValue(base::String("exposureID"), exposure_id_);
  dict->SetValue(base::String("exposure-scene"), exposure_scene_);
  dict->SetValue(base::String("exposureScene"), exposure_scene_);
  dict->SetValue(base::String("dataset"), dataset_);
  dict->SetValue(base::String("dataSet"), dataset_);
  return lepus::Value(dict);
}

bool UIExposure::UIExposureDetail::operator==(
    const UIExposure::UIExposureDetail& other) const {
  return unique_id_ == other.unique_id_ && ui_id_ == other.ui_id_ &&
         exposure_scene_ == other.exposure_scene_ &&
         exposure_id_ == other.exposure_id_;
}

bool UIExposure::UIExposureDetail::operator!=(
    const UIExposure::UIExposureDetail& other) const {
  return !(*this == other);
}

bool UIExposure::UIExposureDetail::operator<(
    const UIExposure::UIExposureDetail& other) const {
  return (unique_id_ < other.unique_id_) ||
         (unique_id_ == other.unique_id_ && ui_id_ < other.ui_id_) ||
         (unique_id_ == other.unique_id_ && ui_id_ == other.ui_id_ &&
          exposure_scene_ < other.exposure_scene_) ||
         (unique_id_ == other.unique_id_ && ui_id_ == other.ui_id_ &&
          exposure_scene_ == other.exposure_scene_ &&
          exposure_id_ < other.exposure_id_);
}

UIExposure::UIExposure(UIObserver* ui_observer, UIOwner* ui_owner)
    : ui_owner_(ui_owner), ui_observer_(ui_observer) {}

void UIExposure::RegisterExposureCheckCallBack() {
  ui_observer_->AddUILayoutObserver(this);
  ui_observer_->AddUIScrollObserver(this);
  ui_observer_->AddUIPropsChangeObserver(this);
  PostTask();
}

void UIExposure::UnregisterExposureCheckCallBack() {
  ui_observer_->RemoveUILayoutObserver(this);
  ui_observer_->RemoveUIScrollObserver(this);
  ui_observer_->RemoveUIPropsChangeObserver(this);
}

void UIExposure::AddUIToExposedMap(UIBase* ui, std::string unique_id,
                                   lepus::Value extra_data,
                                   bool is_custom_event) {
  AddCommonAncestorUI(ui);
  const auto& key = ui->ExposureUIKey(unique_id);
  exposed_ui_map_[key] = UIExposureDetail(
      ui->Sign(), std::move(unique_id), ui->ExposureID(), ui->ExposureScene(),
      std::move(extra_data), ui->Dataset(), is_custom_event);
  if (exposed_ui_map_.size() == 1) {
    RegisterExposureCheckCallBack();
  }
}

void UIExposure::RemoveUIFromExposedMap(UIBase* ui, std::string unique_id) {
  if (exposed_ui_map_.empty()) {
    return;
  }
  RemoveCommonAncestorUI(ui);
  exposed_ui_map_.erase(ui->ExposureUIKey(std::move(unique_id), false));
  if (exposed_ui_map_.empty()) {
    UnregisterExposureCheckCallBack();
  }
}

void UIExposure::StopExposure(const lepus::Value& options) {
  UnregisterExposureCheckCallBack();
  if (options.IsTable() && options.Table()->GetValue("sendEvent").Bool()) {
    SendEvent(ui_visible_before_, "disexposure");
    ui_visible_before_.clear();
  }
}

void UIExposure::ResumeExposure() { RegisterExposureCheckCallBack(); }

void UIExposure::ExecExposureCheck() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_EXPOSURE_EXEC);
  if (exposed_ui_map_.empty() || ui_owner_->Destroyed()) {
    return;
  }

  std::set<UIExposureDetail> ui_visible_now;
  std::set<UIExposureDetail> appear_ui_set;
  std::set<UIExposureDetail> disappear_ui_set;
  float offset_screen[2] = {0};
  ui_owner_->Root()->GetOffsetToScreen(offset_screen);
  for (auto& it : exposed_ui_map_) {
    auto ui = ui_owner_->FindUIBySign(it.second.ID());
    if (ui &&
        ui->IsVisibleForExposure(common_ancestor_ui_rect_map_, offset_screen)) {
      ui_visible_now.insert(it.second);
    }
  }
  ResetCommonAncestorUIRect();

  std::set_difference(ui_visible_now.begin(), ui_visible_now.end(),
                      ui_visible_before_.begin(), ui_visible_before_.end(),
                      std::inserter(appear_ui_set, appear_ui_set.begin()));
  std::set_difference(
      ui_visible_before_.begin(), ui_visible_before_.end(),
      ui_visible_now.begin(), ui_visible_now.end(),
      std::inserter(disappear_ui_set, disappear_ui_set.begin()));
  ui_visible_before_ = std::move(ui_visible_now);
  ui_visible_now.clear();

  SendEvent(disappear_ui_set, "disexposure");
  SendEvent(appear_ui_set, "exposure");
  disappear_ui_set.clear();
  appear_ui_set.clear();

  exposure_check_flag_ = false;
}

void UIExposure::PostTask() {
  if (exposure_check_flag_ || exposed_ui_map_.empty()) {
    return;
  }
  exposure_check_flag_ = true;

  ScheduleUIExposureCheck();
}

void UIExposure::SendEvent(const std::set<UIExposureDetail>& ui_set,
                           const std::string& event_name) const {
  if (ui_set.empty()) {
    return;
  }

  auto params = lepus::CArray::Create();

  for (auto& detail : ui_set) {
    auto ui = ui_owner_->FindUIBySign(detail.ID());
    if (!ui) {
      continue;
    }

    if (detail.IsCustomEvent()) {
      if (event_name == "exposure" && ui->HasAppearEvent()) {
        CustomEvent event{ui->Sign(), "uiappear", "detail",
                          detail.ExposedParams()};
        ui_owner_->SendEvent(event);
      }
      if (event_name == "disexposure" && ui->HasDisappearEvent()) {
        CustomEvent event{ui->Sign(), "uidisappear", "detail",
                          detail.ExposedParams()};
        ui_owner_->SendEvent(event);
      }
      continue;
    }

    if (!ui->ExposureID().empty()) {
      params->push_back(detail.ExposedParams());
    }
  }

  if (params->size() > 0) {
    auto global_event_params = lepus::CArray::Create();
    global_event_params->emplace_back(event_name);

    auto exposure_param_array = lepus::CArray::Create();
    exposure_param_array->emplace_back(std::move(params));
    global_event_params->emplace_back(exposure_param_array);
    ui_owner_->SendGlobalEvent(lepus::Value(std::move(global_event_params)));
  }
}

void OnVSync(const std::weak_ptr<UIExposure>& weak_this) {
  auto ui_exposure = weak_this.lock();
  if (!ui_exposure) {
    return;
  }
  ui_exposure->CheckOnUIThread();
}

void UIExposure::CheckOnUIThread() {
  if (time_interval_for_lynxview_check_ <= 0 && !exposure_check_flag_) {
    return;
  }

  // request next tick first
  ui_owner_->VSyncMonitor()->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<uintptr_t>(this),
      [weak_this = weak_from_this()](int64_t, int64_t) { OnVSync(weak_this); });

  // do Exec if needed
  auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
  int check_duration = time_interval_for_lynxview_check_ > 0
                           ? std::min(time_interval_for_lynxview_check_,
                                      time_interval_for_exposure_check_)
                           : time_interval_for_exposure_check_;
  // use the min duration as vsync check interval
  if (time_stamp - last_lynxview_check_time_ < check_duration) {
    return;
  }
  last_lynxview_check_time_ = time_stamp;

  if ((time_interval_for_lynxview_check_ > 0 && IsLynxViewChanged()) ||
      exposure_check_flag_) {
    ExecExposureCheck();
  }
}

void UIExposure::ScheduleUIExposureCheck() {
  // lynxview_check and exposure_check both need to request to new frame to
  // do checking
  if (time_interval_for_lynxview_check_ > 0 || exposure_check_flag_) {
    ui_owner_->VSyncMonitor()->ScheduleVSyncSecondaryCallback(
        reinterpret_cast<uintptr_t>(this),
        [weak_this = weak_from_this()](int64_t, int64_t) {
          OnVSync(weak_this);
        });
  }
}

bool UIExposure::IsLynxViewChanged() {
  if (exposed_ui_map_.empty() || ui_owner_->Destroyed()) {
    return false;
  }
  ArkUI_NodeHandle proxy_node = ui_owner_->Root()->GetProxyNode();
  float left =
      NodeManager::Instance().GetAttribute<float>(proxy_node, NODE_POSITION, 0);
  float top =
      NodeManager::Instance().GetAttribute<float>(proxy_node, NODE_POSITION, 1);
  float right = left + NodeManager::Instance().GetAttribute<float>(proxy_node,
                                                                   NODE_WIDTH);
  float bottom = top + NodeManager::Instance().GetAttribute<float>(proxy_node,
                                                                   NODE_HEIGHT);

  bool res = base::FloatsNotEqual(left, old_lynx_origin_rect_[0]) ||
             base::FloatsNotEqual(right, old_lynx_origin_rect_[2]) ||
             base::FloatsNotEqual(top, old_lynx_origin_rect_[1]) ||
             base::FloatsNotEqual(bottom, old_lynx_origin_rect_[3]);
  old_lynx_origin_rect_[0] = left;
  old_lynx_origin_rect_[1] = top;
  old_lynx_origin_rect_[2] = right;
  old_lynx_origin_rect_[3] = bottom;

  return res;
}

void UIExposure::SetObserverFrameRate(const lepus::Value& options) {
  if (!options.IsTable()) {
    return;
  }

  int value = options.Table()->GetValue("forExposureCheck").Number();
  if (0 < value && value <= 60) {
    time_interval_for_exposure_check_ = std::max(16, 1000 / value);
    ScheduleUIExposureCheck();
  }

  value = options.Table()->GetValue("forPageRect").Number();
  if (0 <= value && value <= 60) {
    time_interval_for_lynxview_check_ = value ? std::max(16, 1000 / value) : 0;
    if (time_interval_for_lynxview_check_) {
      ScheduleUIExposureCheck();
    }
  }
}

void UIExposure::AddCommonAncestorUI(UIBase* ui) {
  UIBase* current = ui->Parent();
  while (current != nullptr && current->Parent() != current) {
    if (current->IsScrollable()) {
      int sign = current->Sign();
      auto it = common_ancestor_ui_rect_map_.find(sign);
      if (it != common_ancestor_ui_rect_map_.end()) {
        it->second.ui_count++;
      } else {
        CommonAncestorUIRect ui_rect = {
            .ui_count = 1,
            .ui_rect_updated = false,
        };
        common_ancestor_ui_rect_map_.emplace(sign, std::move(ui_rect));
      }
    }
    current = current->Parent();
  }
}

void UIExposure::RemoveCommonAncestorUI(UIBase* ui) {
  UIBase* current = ui->Parent();
  while (current != nullptr && current->Parent() != current) {
    auto it = common_ancestor_ui_rect_map_.find(current->Sign());
    if (it != common_ancestor_ui_rect_map_.end()) {
      it->second.ui_count--;
      if (it->second.ui_count <= 0) {
        common_ancestor_ui_rect_map_.erase(it);
      }
    }
    current = current->Parent();
  }
}

void UIExposure::ResetCommonAncestorUIRect() {
  if (common_ancestor_ui_rect_map_.empty()) {
    return;
  }
  for (auto& it : common_ancestor_ui_rect_map_) {
    it.second.ui_rect_updated = false;
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
