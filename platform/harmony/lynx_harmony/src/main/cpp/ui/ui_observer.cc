// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_observer.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/ui_observer_callback.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIObserver::UIObserver(UIOwner* ui_owner)
    : ui_exposure_(std::make_shared<UIExposure>(this, ui_owner)),
      ui_intersection_observer_manager_(
          std::make_shared<UIIntersectionObserverManager>(this, ui_owner)) {}

void UIObserver::NotifyUILayout() {
  for (auto callback : ui_layout_observers_) {
    callback->PostTask();
  }
}

void UIObserver::NotifyUIScroll() {
  for (auto callback : ui_scroll_observers_) {
    callback->PostTask();
  }
}

void UIObserver::NotifyUIPropsChange() {
  for (auto callback : ui_props_change_observers_) {
    callback->PostTask();
  }
}

void UIObserver::AddUILayoutObserver(UIObserverCallback* callback) {
  ui_layout_observers_.insert(callback);
}

void UIObserver::RemoveUILayoutObserver(UIObserverCallback* callback) {
  ui_layout_observers_.erase(callback);
}

void UIObserver::AddUIScrollObserver(UIObserverCallback* callback) {
  ui_scroll_observers_.insert(callback);
}

void UIObserver::RemoveUIScrollObserver(UIObserverCallback* callback) {
  ui_scroll_observers_.erase(callback);
}

void UIObserver::AddUIPropsChangeObserver(UIObserverCallback* callback) {
  ui_props_change_observers_.insert(callback);
}

void UIObserver::RemoveUIPropsChangeObserver(UIObserverCallback* callback) {
  ui_props_change_observers_.erase(callback);
}

void UIObserver::AddUIToExposedMap(UIBase* ui, std::string unique_id,
                                   lepus::Value extra_data,
                                   bool is_custom_event) {
  ui_exposure_->AddUIToExposedMap(ui, std::move(unique_id),
                                  std::move(extra_data), is_custom_event);
}

void UIObserver::RemoveUIFromExposedMap(UIBase* ui, std::string unique_id) {
  ui_exposure_->RemoveUIFromExposedMap(ui, std::move(unique_id));
}

void UIObserver::TriggerExposureCheck() { ui_exposure_->ExecExposureCheck(); }

void UIObserver::StopExposure(const lepus::Value& options) {
  ui_exposure_->StopExposure(options);
}

void UIObserver::ResumeExposure() { ui_exposure_->ResumeExposure(); }

void UIObserver::SetObserverFrameRate(const lepus::Value& options) {
  ui_exposure_->SetObserverFrameRate(options);
}

void UIObserver::CreateUIIntersectionObserver(
    int intersection_observer_id, const std::string& js_component_id,
    const lepus::Value& options) {
  ui_intersection_observer_manager_->AddUIIntersectionObserver(
      ui_intersection_observer_manager_.get(), intersection_observer_id,
      js_component_id, options);
}

UIIntersectionObserver* UIObserver::GetUIIntersectionObserver(
    int intersection_observer_id) {
  return ui_intersection_observer_manager_->GetUIIntersectionObserver(
      intersection_observer_id);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
