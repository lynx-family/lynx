// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_intersection_observer.h"

#include <memory>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_observer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIIntersectionObserverTarget::UIIntersectionObserverEntry::
    UIIntersectionObserverEntry(std::string target_id_selector,
                                float intersection_rect[4],
                                float target_rect[4], float ref_rect[4],
                                long intersection_time)
    : target_id_selector_(std::move(target_id_selector)),
      intersection_time_(intersection_time) {
  std::memcpy(intersection_rect_, intersection_rect, 4 * sizeof(float));
  std::memcpy(target_rect_, target_rect, 4 * sizeof(float));
  std::memcpy(ref_rect_, ref_rect, 4 * sizeof(float));
  UpdateIntersectionRatio();
}

float UIIntersectionObserverTarget::UIIntersectionObserverEntry::
    IntersectionRatio() {
  return intersection_ratio_;
}

void UIIntersectionObserverTarget::UIIntersectionObserverEntry::
    UpdateIntersectionRatio() {
  float intersection_area = (intersection_rect_[2] - intersection_rect_[0]) *
                            (intersection_rect_[3] - intersection_rect_[1]);
  float target_area =
      (target_rect_[2] - target_rect_[0]) * (target_rect_[3] - target_rect_[1]);
  if (base::FloatsLarger(intersection_area, 0.f) &&
      base::FloatsLarger(target_area, 0.f)) {
    float ratio = intersection_area / target_area;
    intersection_ratio_ = base::FloatsLarger(ratio, 1.f) ? 1.f : ratio;
  } else {
    intersection_ratio_ = 0.f;
  }
  is_intersecting_ = base::FloatsNotEqual(intersection_ratio_, 0.f);
}

lepus::Value
UIIntersectionObserverTarget::UIIntersectionObserverEntry::GetRectDictionary(
    float rect[4]) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("left", rect[0]);
  dict->SetValue("top", rect[1]);
  dict->SetValue("right", rect[2]);
  dict->SetValue("bottom", rect[3]);
  return lepus::Value(dict);
}

void UIIntersectionObserverTarget::UIIntersectionObserverEntry::
    GetIntersectionRect(float rect[4]) {
  memcpy(rect, intersection_rect_, 4 * sizeof(float));
}

bool IsRectEqualZero(float rect[4]) {
  return base::FloatsEqualPrecise(rect[0], 0.f) &&
         base::FloatsEqualPrecise(rect[1], 0.f) &&
         base::FloatsEqualPrecise(rect[2], 0.f) &&
         base::FloatsEqualPrecise(rect[3], 0.f);
}

bool GetRectIntersection(float rect1[4], float rect2[4],
                         float intersection[4]) {
  if (IsRectEqualZero(rect1) || IsRectEqualZero(rect2)) {
    memset(intersection, 0.f, 4 * sizeof(float));
    return false;
  }

  float left = fmax(rect1[0], rect2[0]);
  float top = fmax(rect1[1], rect2[1]);
  float right = fmin(rect1[2], rect2[2]);
  float bottom = fmin(rect1[3], rect2[3]);
  if (base::FloatsLarger(right, left) && base::FloatsLarger(bottom, top)) {
    intersection[0] = left;
    intersection[1] = top;
    intersection[2] = right;
    intersection[3] = bottom;
    return true;
  } else {
    memset(intersection, 0.f, 4 * sizeof(float));
    return false;
  }
}

lepus::Value UIIntersectionObserverTarget::UIIntersectionObserverEntry::
    IntersectionParams() {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("observerId", target_id_selector_);
  dict->SetValue("isIntersecting", is_intersecting_);
  dict->SetValue("intersectionRatio", intersection_ratio_);
  dict->SetValue("intersectionRect", GetRectDictionary(intersection_rect_));
  dict->SetValue("boundingClientRect", GetRectDictionary(target_rect_));
  dict->SetValue("relativeRect", GetRectDictionary(ref_rect_));
  dict->SetValue("time", intersection_time_);
  return lepus::Value(dict);
}

UIIntersectionObserverTarget::UIIntersectionObserverTarget(int target_id,
                                                           int js_callback_id)
    : target_id_(target_id), js_callback_id_(js_callback_id) {}

int UIIntersectionObserverTarget::TargetID() { return target_id_; }

int UIIntersectionObserverTarget::JSCallbackID() { return js_callback_id_; }

void UIIntersectionObserverTarget::SetUIIntersectionObserverEntry(
    std::unique_ptr<UIIntersectionObserverEntry> intersection_entry) {
  intersection_entry_ = std::move(intersection_entry);
}

UIIntersectionObserverTarget::UIIntersectionObserverEntry*
UIIntersectionObserverTarget::IntersectionEntry() {
  return intersection_entry_.get();
}

UIIntersectionObserver::UIIntersectionObserver(
    UIIntersectionObserverManager* intersection_observer_manager,
    UIOwner* ui_owner, int intersection_observer_id, int js_component_id,
    const lepus::Value& options)
    : intersection_observer_manager_(intersection_observer_manager),
      ui_owner_(ui_owner),
      intersection_observer_id_(intersection_observer_id),
      js_component_id_(js_component_id) {
  if (options.IsTable() || options.IsJSTable()) {
    float initial_intersection_ratio = 0.f;
    std::vector<float> intersection_ratio_thresholds;
    tasm::ForEachLepusValue(
        options, [&initial_intersection_ratio, &intersection_ratio_thresholds](
                     const auto& key, const auto& val) {
          if (key.StdString() == "initialRatio") {
            initial_intersection_ratio = val.Number();
          } else if (key.StdString() == "thresholds") {
            if (val.IsArrayOrJSArray()) {
              tasm::ForEachLepusValue(
                  val, [&intersection_ratio_thresholds](const auto& index,
                                                        const auto& threshold) {
                    if (threshold.IsNumber()) {
                      float num = threshold.Number();
                      if (base::FloatsLargerOrEqual(num, 0.f) &&
                          base::FloatsLargerOrEqual(1.f, num)) {
                        intersection_ratio_thresholds.push_back(num);
                      }
                    }
                  });
            }
          }
        });
    if (intersection_ratio_thresholds.size() == 0) {
      intersection_ratio_thresholds.push_back(0.f);
    }
    initial_intersection_ratio_ =
        base::FloatsLargerOrEqual(initial_intersection_ratio, 0.f) &&
                base::FloatsLargerOrEqual(1.f, initial_intersection_ratio)
            ? initial_intersection_ratio
            : 0.f;
    intersection_ratio_thresholds_ = std::move(intersection_ratio_thresholds);
  }
}

void UIIntersectionObserver::RelativeTo(const std::string& ref_id_selector,
                                        const lepus::Value& options) {
  if (ref_id_selector.find("#") != 0) {
    return;
  }
  auto id_selector = ref_id_selector.substr(1);
  auto component =
      ui_owner_->FindLynxUIByComponentId(std::to_string(js_component_id_));
  if (component == nullptr) {
    return;
  }
  auto ui = component->FindViewById(id_selector, false);
  if (ui == nullptr) {
    ui = ui_owner_->FindUIByIdSelector(id_selector);
  }
  ref_id_ = ui != nullptr ? ui->Sign() : ui_owner_->Root()->Sign();
  ParseUIMargin(options);
}

void UIIntersectionObserver::RelativeToViewport(const lepus::Value& options) {
  ref_id_ = ui_owner_->Root()->Sign();
  ParseUIMargin(options);
}

void UIIntersectionObserver::RelativeToScreen(const lepus::Value& options) {
  ref_id_ = -1;
  ParseUIMargin(options);
}

// support id selector and uid
void UIIntersectionObserver::Observe(const std::string& selector,
                                     int js_callback_id) {
  UIBase* ui = nullptr;
  if (selector.find('#') == 0) {
    auto id_selector = selector.substr(1);
    auto component =
        ui_owner_->FindLynxUIByComponentId(std::to_string(js_component_id_));
    if (component == nullptr) {
      return;
    }
    ui = component->FindViewById(id_selector, false);
    if (ui == nullptr) {
      ui = ui_owner_->FindUIByIdSelector(id_selector);
    }
  } else {
    int uid;
    if (base::StringToInt(selector, &uid)) {
      ui = ui_owner_->FindUIBySign(static_cast<int>(uid));
    } else {
      LOGE("IntersectionObserver observe failed because uid is invalid");
      return;
    }
  }

  int target_id = ui != nullptr ? ui->Sign() : -1;
  if (target_id != -1) {
    intersection_observe_targets_.push_back(
        UIIntersectionObserverTarget(target_id, js_callback_id));
    CheckIntersectionWithTarget(&intersection_observe_targets_.back(), true);
  }
}

void UIIntersectionObserver::Disconnect() {
  intersection_observe_targets_.clear();
  intersection_observer_manager_->RemoveUIIntersectionObserver(
      intersection_observer_id_);
}

void UIIntersectionObserver::CheckIntersection() {
  if (intersection_observe_targets_.empty()) {
    return;
  }
  for (auto& target : intersection_observe_targets_) {
    CheckIntersectionWithTarget(&target, false);
  }
}

void UIIntersectionObserver::ParseUIMargin(const lepus::Value& options) {
  if (!options.IsTable() && !options.IsJSTable()) {
    return;
  }
  float ui_margin_top = 0.f;
  float ui_margin_right = 0.f;
  float ui_margin_bottom = 0.f;
  float ui_margin_left = 0.f;
  tasm::ForEachLepusValue(
      options,
      [observer = this, &ui_margin_top, &ui_margin_right, &ui_margin_bottom,
       &ui_margin_left](const auto& key, const auto& val) {
        if (key.StdString() == "top") {
          ui_margin_top = observer->GetUIMarginVP(val);
        } else if (key.StdString() == "right") {
          ui_margin_right = observer->GetUIMarginVP(val);
        } else if (key.StdString() == "bottom") {
          ui_margin_bottom = observer->GetUIMarginVP(val);
        } else if (key.StdString() == "left") {
          ui_margin_left = observer->GetUIMarginVP(val);
        }
      });
  ui_margin_top_ =
      base::FloatsNotEqual(ui_margin_top, 0.f) ? ui_margin_top : ui_margin_top_;
  ui_margin_right_ = base::FloatsNotEqual(ui_margin_right, 0.f)
                         ? ui_margin_right
                         : ui_margin_right_;
  ui_margin_bottom_ = base::FloatsNotEqual(ui_margin_bottom, 0.f)
                          ? ui_margin_bottom
                          : ui_margin_bottom_;
  ui_margin_left_ = base::FloatsNotEqual(ui_margin_left, 0.f) ? ui_margin_left
                                                              : ui_margin_left_;

  if (ref_id_ != -1) {
    auto ui = ui_owner_->FindUIBySign(ref_id_);
    if (ui == nullptr) {
      return;
    }
    ui->GetBoundingClientRect(ref_rect_, true);
  } else {
    float size[2] = {0.f};
    intersection_observer_manager_->ScreenSize(size);
    ref_rect_[0] = 0.f;
    ref_rect_[1] = 0.f;
    ref_rect_[2] = size[0];
    ref_rect_[3] = size[1];
  }
  ref_rect_[0] -= ui_margin_left_;
  ref_rect_[1] -= ui_margin_top_;
  ref_rect_[2] -= ui_margin_right_;
  ref_rect_[3] -= ui_margin_bottom_;
}

float UIIntersectionObserver::GetUIMarginVP(const lepus::Value& value) {
  if (value.IsNumber()) {
    return value.Number();
  } else if (value.IsString()) {
    float screen_size[2] = {0};
    ui_owner_->Context()->ScreenSize(screen_size);
    return LynxUnitUtils::ToVPFromUnitValue(
        value.StdString(), screen_size[0],
        ui_owner_->Context()->DevicePixelRatio(), 5.f);
  }
  return 0.f;
}

void UIIntersectionObserver::CheckIntersectionWithTarget(
    UIIntersectionObserverTarget* target, bool is_initial_check) {
  if (target == nullptr || ui_owner_->Destroyed()) {
    return;
  }
  auto ui = ui_owner_->FindUIBySign(target->TargetID());
  if (ui == nullptr) {
    return;
  }

  float target_rect[4] = {0.f};
  float intersection_rect[4] = {0.f};
  auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
  ui->GetBoundingClientRect(target_rect, true);
  ComputeTargetInteraction(ui, target_rect, intersection_rect);
  std::unique_ptr<UIIntersectionObserverTarget::UIIntersectionObserverEntry>
      new_entry = std::make_unique<
          UIIntersectionObserverTarget::UIIntersectionObserverEntry>(
          ui->IdSelector(), intersection_rect, target_rect, ref_rect_,
          time_stamp);

  bool trigger_js_callback = false;
  if (is_initial_check) {
    trigger_js_callback =
        new_entry->IntersectionRatio() >= initial_intersection_ratio_;
  } else {
    trigger_js_callback = HasCrossIntersectionRatioThreshold(
        target->IntersectionEntry(), new_entry.get());
  }
  target->SetUIIntersectionObserverEntry(std::move(new_entry));
  if (trigger_js_callback) {
    intersection_observer_manager_->CallJSIntersectionObserver(
        intersection_observer_id_, target->JSCallbackID(),
        target->IntersectionEntry()->IntersectionParams());
  }
}

void UIIntersectionObserver::ComputeTargetInteraction(
    UIBase* target, float target_rect[4], float intersection_rect[4]) {
  if (target == nullptr || !target->IsVisible() || ui_owner_->Destroyed()) {
    return;
  }

  auto current = target->Parent();
  memcpy(intersection_rect, target_rect, 4 * sizeof(float));
  while (current && current->Parent() != current) {
    if (!current->IsVisible()) {
      memset(intersection_rect, 0.f, 4 * sizeof(float));
      return;
    }
    if (current->Sign() == ref_id_) {
      GetRectIntersection(ref_rect_, intersection_rect, intersection_rect);
      return;
    } else {
      if (!current->Overflow().overflow_x || !current->Overflow().overflow_y) {
        float current_rect[4] = {0.f};
        current->GetBoundingClientRect(current_rect, true);
        if (!GetRectIntersection(current_rect, intersection_rect,
                                 intersection_rect)) {
          return;
        }
      }
    }
    current = current->Parent();
  }

  if (ref_id_ == -1 && !ui_owner_->Destroyed()) {
    ui_owner_->Root()->GetBoundingClientRect(intersection_rect, true);
    GetRectIntersection(intersection_rect, ref_rect_, intersection_rect);
  }
}

bool UIIntersectionObserver::HasCrossIntersectionRatioThreshold(
    UIIntersectionObserverTarget::UIIntersectionObserverEntry* old_entry,
    UIIntersectionObserverTarget::UIIntersectionObserverEntry* new_entry) {
  if (old_entry == nullptr || new_entry == nullptr) {
    return false;
  }
  float rect[4] = {0.f};
  old_entry->GetIntersectionRect(rect);
  float old_ratio =
      !IsRectEqualZero(rect) ? old_entry->IntersectionRatio() : -1.f;
  new_entry->GetIntersectionRect(rect);
  float new_ratio =
      !IsRectEqualZero(rect) ? new_entry->IntersectionRatio() : -1.f;
  if (base::FloatsEqualPrecise(old_ratio, new_ratio)) {
    return false;
  }
  for (auto threshold : intersection_ratio_thresholds_) {
    if (base::FloatsEqualPrecise(threshold, old_ratio) ||
        base::FloatsEqualPrecise(threshold, new_ratio) ||
        (base::FloatsLargerPrecise(new_ratio, threshold) &&
         base::FloatsLargerPrecise(threshold, old_ratio)) ||
        (base::FloatsLargerPrecise(old_ratio, threshold) &&
         base::FloatsLargerPrecise(threshold, new_ratio))) {
      return true;
    }
  }
  return false;
}

UIIntersectionObserverManager::UIIntersectionObserverManager(
    UIObserver* ui_observer, UIOwner* ui_owner)
    : ui_owner_(ui_owner), ui_observer_(ui_observer) {}

void UIIntersectionObserverManager::PostTask() {
  if (intersection_check_flag_) {
    return;
  }
  intersection_check_flag_ = true;
  ScheduleUIIntersectionCheck();
}

void UIIntersectionObserverManager::AddUIIntersectionObserver(
    UIIntersectionObserverManager* intersection_observer_manager,
    int intersection_observer_id, const std::string& js_component_id,
    const lepus::Value& options) {
  if (intersection_observers_.size() == 0) {
    ui_observer_->AddUILayoutObserver(this);
    ui_observer_->AddUIScrollObserver(this);
    ui_observer_->AddUIPropsChangeObserver(this);
  }
  if (intersection_observers_.find(intersection_observer_id) ==
      intersection_observers_.end()) {
    intersection_observers_.emplace(
        intersection_observer_id,
        UIIntersectionObserver(
            this, ui_owner_, intersection_observer_id,
            !js_component_id.empty() ? std::stoi(js_component_id) : -1,
            options));
  }
}

void UIIntersectionObserverManager::RemoveUIIntersectionObserver(
    int intersection_observer_id) {
  if (intersection_observers_.find(intersection_observer_id) !=
      intersection_observers_.end()) {
    intersection_observers_.erase(intersection_observer_id);
  }
  if (intersection_observers_.size() == 0) {
    ui_observer_->RemoveUILayoutObserver(this);
    ui_observer_->RemoveUIScrollObserver(this);
    ui_observer_->AddUIPropsChangeObserver(this);
  }
}

UIIntersectionObserver*
UIIntersectionObserverManager::GetUIIntersectionObserver(
    int intersection_observer_id) {
  if (intersection_observers_.find(intersection_observer_id) !=
      intersection_observers_.end()) {
    return &intersection_observers_.at(intersection_observer_id);
  }
  return nullptr;
}

void UIIntersectionObserverManager::ExecIntersectionCheck() {
  for (auto& observer : intersection_observers_) {
    observer.second.CheckIntersection();
  }
  intersection_check_flag_ = false;
}

void OnVSync(const std::weak_ptr<UIIntersectionObserverManager>& weak_this) {
  auto ui_intersection_observer_manager = weak_this.lock();
  if (!ui_intersection_observer_manager) {
    return;
  }
  ui_intersection_observer_manager->CheckOnUIThread();
}

void UIIntersectionObserverManager::CheckOnUIThread() {
  if (!intersection_check_flag_) {
    return;
  }
  ui_owner_->VSyncMonitor()->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<uintptr_t>(this),
      [weak_this = weak_from_this()](int64_t, int64_t) { OnVSync(weak_this); });

  auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
  if (time_stamp - last_intersection_check_time_ < 50) {
    return;
  }

  last_intersection_check_time_ = time_stamp;
  if (intersection_check_flag_) {
    ExecIntersectionCheck();
  }
}

void UIIntersectionObserverManager::ScheduleUIIntersectionCheck() {
  ui_owner_->VSyncMonitor()->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<uintptr_t>(this),
      [weak_this = weak_from_this()](int64_t, int64_t) { OnVSync(weak_this); });
}

void UIIntersectionObserverManager::CallJSIntersectionObserver(
    int32_t observer_id, int32_t callback_id, lepus::Value params) const {
  ui_owner_->Context()->CallJSIntersectionObserver(observer_id, callback_id,
                                                   std::move(params));
}

void UIIntersectionObserverManager::ScreenSize(float size[2]) {
  ui_owner_->Context()->ScreenSize(size);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
