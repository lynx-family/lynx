// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_handler.h"

#include <utility>

#include "base/include/float_comparison.h"
#include "core/event/touch_event.h"
#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"
#include "core/renderer/dom/fragment/event/platform_input_event.h"
#include "core/renderer/dom/fragment/event/platform_pointer_event.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

PlatformEventHandler::PlatformEventTargetDetail::PlatformEventTargetDetail(
    fml::RefPtr<PlatformEventTarget> target, float down_point[2])
    : target_(target) {
  memcpy(down_point_, down_point, sizeof(float) * 2);
}

void PlatformEventHandler::PlatformEventTargetDetail::GetDownPoint(
    float down_point[2]) {
  memcpy(down_point, down_point_, sizeof(float) * 2);
}

void PlatformEventHandler::PlatformEventTargetDetail::GetPrePoint(
    float pre_point[2]) {
  memcpy(pre_point, pre_point_, sizeof(float) * 2);
}

void PlatformEventHandler::PlatformEventTargetDetail::SetPrePoint(
    float pre_point[2]) {
  memcpy(pre_point_, pre_point, sizeof(float) * 2);
}

fml::RefPtr<PlatformEventTarget>
PlatformEventHandler::PlatformEventTargetDetail::Target() {
  return target_;
}

bool PlatformEventHandler::OnInputEvent(
    fml::RefPtr<PlatformEventTarget> target_tree, int int_event_data[],
    float float_event_data[]) {
  target_tree_ = target_tree;
  // int_event_data: [event_type, action_type, event_source, pointer_count, ...]
  int event_type = int_event_data[0];
  switch (event_type) {
    // pointer event
    case 0: {
      // float_event_data: [pointer_id, pointer_x, pointer_y, ...]
      auto pointer_event =
          PlatformPointerEvent(int_event_data, float_event_data);
      switch (pointer_event.ActionType()) {
        case 0: {
          HandlePointerDown(pointer_event);
          break;
        }
        case 2: {
          HandlePointerMove(pointer_event);
          break;
        }
        case 1: {
          HandlePointerUp(pointer_event);
          break;
        }
        case 3: {
          HandlePointerCancel(pointer_event);
          break;
        }
      }
      break;
    }
    // TODO(hexionghui): support keyboard event
    case 1: {
      break;
    }
    default:
      break;
  }
  if (EventThrough()) {
    LOGI("PlatformEventHandler::OnInputEvent EventThrough")
    return false;
  }

  // TODO(hexionghui): forward event to gesture.

  return true;
}

void PlatformEventHandler::OnGestureEvent(const std::string& name,
                                          PlatformPointerEvent& event) {
  if (!first_target_) {
    LOGE(
        "PlatformEventHandler::DispatchPointerEvent first_target_ is null for "
        "event: " +
        name);
    return;
  }
  float page_point[2] = {event.PointerX()[0], event.PointerY()[0]};
  float target_point[2] = {page_point[0], page_point[1]};
  GetTargetPoint(first_target_, target_point, page_point);
  float client_point[2] = {page_point[0], page_point[1]};
  platform_ref_->GetEventTargetHelper()->ConvertPointFromTargetToScreen(
      client_point, target_tree_, client_point);
  auto gesture_event = fml::MakeRefCounted<event::TouchEvent>(
      name, target_point[0], target_point[1], page_point[0], page_point[1],
      client_point[0], client_point[1]);
  platform_ref_->GetEventEmitter()->SendEvent(first_target_->Sign(),
                                              gesture_event);
}

void PlatformEventHandler::DispatchPointerEvent(
    const std::string& name, const lepus::Value& target_pointer_map) {
  if (!first_target_) {
    LOGE(
        "PlatformEventHandler::DispatchPointerEvent first_target_ is null for "
        "event: " +
        name);
    return;
  }
  auto event = fml::MakeRefCounted<event::TouchEvent>(name, target_pointer_map);
  platform_ref_->GetEventEmitter()->SendEvent(first_target_->Sign(), event);
}

void PlatformEventHandler::OnGestureRecognized(int sign) {
  gesture_recognized_target_set_.insert(sign);
}

void PlatformEventHandler::SetFocusedTarget(
    fml::RefPtr<PlatformEventTarget> focused_target) {
  focused_target_ = focused_target;
}

void PlatformEventHandler::UnsetFocusedTarget(
    fml::RefPtr<PlatformEventTarget> focused_target) {
  if (focused_target_ == focused_target) {
    focused_target_ = nullptr;
  }
}

bool PlatformEventHandler::EventThrough() {
  if (!first_target_) {
    return false;
  }
  return first_target_->EventThrough(first_pointer_down_point_);
}

int PlatformEventHandler::EventHandlerState() { return event_handler_state_; }

void PlatformEventHandler::SetTapSlop(const std::string& tap_slop) {}

void PlatformEventHandler::SetLongPressDuration(int32_t long_press_duration) {}

void PlatformEventHandler::SetHasPointerPseudo(bool has_pointer_pseudo) {
  has_pointer_pseudo_ = has_pointer_pseudo_ || has_pointer_pseudo;
}

void PlatformEventHandler::InitPointerEnv(PlatformPointerEvent& event) {
  int num = event.PointerCount();
  for (int i = 0; i < num; ++i) {
    int pointer_id = event.PointerID()[i];
    float pointer_x = event.PointerX()[i];
    float pointer_y = event.PointerY()[i];
    auto hit_target = FindTarget(pointer_x, pointer_y);
    LOGI("PlatformEventHandler::InitPointerEnv pointer id:" +
         std::to_string(pointer_id) + " x:" + std::to_string(pointer_x) +
         " y:" + std::to_string(pointer_y) + " target:" +
         (hit_target ? std::to_string(hit_target->Sign()) : "null"))
    float down_point[2] = {pointer_x, pointer_y};
    if (pointer_id == 0) {
      first_target_ = hit_target;
      memcpy(first_pointer_down_point_, down_point, sizeof(float) * 2);
    }
    target_pointer_map_.insert_or_assign(
        pointer_id, PlatformEventTargetDetail(hit_target, down_point));
  }
}

void PlatformEventHandler::ResetPointerEnv(PlatformPointerEvent& event) {
  int num = event.PointerCount();
  for (int i = 0; i < num; ++i) {
    target_pointer_map_.erase(event.PointerID()[i]);
  }
  has_pointer_moved_ = false;
}

void PlatformEventHandler::InitClickEnv() {
  click_target_chain_.clear();
  if (!first_target_) {
    return;
  }
  auto target = first_target_;
  while (target && target->ParentTarget() != target) {
    click_target_chain_.push_back(target);
    target = target->ParentTarget();
  }

  while (!click_target_chain_.empty()) {
    auto& last_target = click_target_chain_.front();
    if (!last_target) {
      click_target_chain_.pop_front();
      continue;
    }
    bool has_click_event = false;
    for (const auto& event : last_target->EventSet()) {
      if (event == PlatformEventName::kClick) {
        // the click_target_chain is constructed using the first node in the
        // event response chain that registers the click event.
        has_click_event = true;
        break;
      }
    }
    if (has_click_event) {
      break;
    } else {
      click_target_chain_.pop_front();
    }
  }

  for (auto click_target : click_target_chain_) {
    if (!click_target) {
      continue;
    }
    click_target->OnResponseChain();
  }
}

void PlatformEventHandler::ResetClickEnv() {
  for (const auto& click_target : click_target_chain_) {
    if (!click_target) {
      continue;
    }
    click_target->OffResponseChain();
  }
}

void PlatformEventHandler::OnPointerDown(PlatformPointerEvent& event) {
  int num = event.PointerCount();
  for (int i = 0; i < num; ++i) {
    if (event.PointerID()[i] == 0) {
      gesture_recognized_target_set_.clear();
      event_target_chain_.clear();
      first_pointer_moved_ = false;
      first_pointer_outside_ = false;
      InitClickEnv();
      ActivePseudoStatus();
      break;
    }
  }
}

void PlatformEventHandler::OnPointerMove(PlatformPointerEvent& event) {
  int num = event.PointerCount();
  bool first_pointer_changed = false;
  float pre_page_point[2] = {0.f};
  for (int i = 0; i < num; ++i) {
    int pointer_id = event.PointerID()[i];
    float page_point[2] = {event.PointerX()[i], event.PointerY()[i]};
    if (auto target = target_pointer_map_.find(pointer_id);
        target != target_pointer_map_.end()) {
      target->second.GetPrePoint(pre_page_point);
      // check for pointer movement.
      if (base::FloatsNotEqual(page_point[0], pre_page_point[0]) ||
          base::FloatsNotEqual(page_point[1], pre_page_point[1])) {
        has_pointer_moved_ = true;
        target->second.SetPrePoint(page_point);
        if (pointer_id == 0 && !first_pointer_moved_) {
          first_pointer_changed = true;
          float down_page_point[2] = {0.f};
          target->second.GetDownPoint(down_page_point);
          // check if the first pointer movement exceeds the threshold.
          if (base::FloatsLarger(
                  pow(abs(page_point[0] - down_page_point[0]), 2) +
                      pow(abs(page_point[1] - down_page_point[1]), 2),
                  pow(tap_slop_, 2))) {
            first_pointer_moved_ = true;
          }
        }
      }
    }
  }

  if (first_pointer_changed) {
    if (auto first_pointer_target = target_pointer_map_.find(0);
        first_pointer_target != target_pointer_map_.end()) {
      first_pointer_target->second.GetPrePoint(pre_page_point);
      // check whether it exceeds the bounds of the node registered by the click
      // event.
      if (!click_target_chain_.empty()) {
        auto target = FindTarget(pre_page_point[0], pre_page_point[1]);
        auto click_target = click_target_chain_.front();
        first_pointer_outside_ =
            first_pointer_outside_ || IsPointerMoveOutside(target);
      }
      // check if the movement threshold is exceeded or there is node scrolling.
      if (first_pointer_moved_ || !CanRespondTap(first_target_)) {
        DeactivatePseudoStatus(LynxPseudoStatus::kActive);
      }
    }
  }
}

void PlatformEventHandler::OnPointerUp(PlatformPointerEvent& event) {
  int num = event.PointerCount();
  for (int i = 0; i < num; ++i) {
    if (event.PointerID()[i] == 0) {
      OnGestureEvent("tap", event);
      OnGestureEvent("click", event);
      ResetClickEnv();
      UpdateFocusedTarget();
      DeactivatePseudoStatus(LynxPseudoStatus::kAll);
      break;
    }
  }
}

void PlatformEventHandler::OnPointerCancel(PlatformPointerEvent& event) {
  int num = event.PointerCount();
  for (int i = 0; i < num; ++i) {
    if (event.PointerID()[i] == 0) {
      ResetClickEnv();
      UpdateFocusedTarget();
      DeactivatePseudoStatus(LynxPseudoStatus::kAll);
      break;
    }
  }
}

void PlatformEventHandler::HandlePointerDown(PlatformPointerEvent& event) {
  InitPointerEnv(event);
  if (EventThrough()) {
    ResetPointerEnv(event);
    return;
  }
  auto target_pointer_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetPointerMap(target_pointer_map, event);
  DispatchPointerEvent("touchstart", target_pointer_map);
  OnPointerDown(event);
}

void PlatformEventHandler::HandlePointerMove(PlatformPointerEvent& event) {
  OnPointerMove(event);
  if (!has_pointer_moved_) {
    return;
  }
  auto target_pointer_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetPointerMap(target_pointer_map, event);
  DispatchPointerEvent("touchmove", target_pointer_map);
}

void PlatformEventHandler::HandlePointerUp(PlatformPointerEvent& event) {
  auto target_pointer_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetPointerMap(target_pointer_map, event);
  DispatchPointerEvent("touchend", target_pointer_map);
  OnPointerUp(event);
  ResetPointerEnv(event);
}

void PlatformEventHandler::HandlePointerCancel(PlatformPointerEvent& event) {
  auto target_pointer_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetPointerMap(target_pointer_map, event);
  DispatchPointerEvent("touchcancel", target_pointer_map);
  OnPointerCancel(event);
  ResetPointerEnv(event);
}

fml::RefPtr<PlatformEventTarget> PlatformEventHandler::FindTarget(
    float pointer_x, float pointer_y) {
  if (!target_tree_) {
    return nullptr;
  }
  float point[] = {pointer_x, pointer_y};
  return target_tree_->HitTest(point);
}

void PlatformEventHandler::UpdateFocusedTarget() {
  if (first_target_ && !first_target_->IgnoreFocus()) {
    if (focused_target_) {
      if (focused_target_ != first_target_) {
        focused_target_->OnFocusChange(false, first_target_->Focusable());
      }
    }
    first_target_->OnFocusChange(
        true, focused_target_ && focused_target_->Focusable());
    focused_target_ = first_target_;
  }
}

bool PlatformEventHandler::CanRespondTap(
    fml::RefPtr<PlatformEventTarget> target) {
  if (!target) {
    return false;
  }
  if (gesture_recognized_target_set_.empty()) {
    return true;
  }

  while (target && target->ParentTarget() != target) {
    if (gesture_recognized_target_set_.find(target->Sign()) !=
        gesture_recognized_target_set_.end()) {
      // it means that a node in the event response chain is scrolling.
      return false;
    }
    target = target->ParentTarget();
  }
  return true;
}

void PlatformEventHandler::ActivePseudoStatus() {
  if (!first_target_) {
    return;
  }
  auto current = first_target_;
  while (current && current->ParentTarget() != current) {
    event_target_chain_.push_back(current);
    current->OnPseudoStatusChanged(LynxPseudoStatus::kNone,
                                   LynxPseudoStatus::kActive);
    if (has_pointer_pseudo_) {
      // update :active for target.
      platform_ref_->UpdatePseudoStatusStatus(
          current->Sign(), static_cast<uint32_t>(LynxPseudoStatus::kNone),
          static_cast<uint32_t>(LynxPseudoStatus::kActive));
    }
    if (!current->TouchPseudoPropagation()) {
      break;
    }
    current = current->ParentTarget();
  }
}

void PlatformEventHandler::DeactivatePseudoStatus(LynxPseudoStatus status) {
  int int_status = static_cast<int>(status);
  for (auto target : event_target_chain_) {
    if (!target) {
      continue;
    }
    int current_status = static_cast<int>(target->GetPseudoStatus());
    target->OnPseudoStatusChanged(
        static_cast<LynxPseudoStatus>(current_status),
        static_cast<LynxPseudoStatus>(current_status & ~int_status));
    if (has_pointer_pseudo_) {
      // update :active for target.
      platform_ref_->UpdatePseudoStatusStatus(
          target->Sign(), static_cast<uint32_t>(current_status),
          static_cast<uint32_t>(current_status & ~int_status));
    }
  }
}

bool PlatformEventHandler::IsPointerMoveOutside(
    fml::RefPtr<PlatformEventTarget> target) {
  if (!target) {
    return true;
  }

  std::vector<fml::RefPtr<PlatformEventTarget>> target_chain;
  while (target && target->ParentTarget() != target) {
    target_chain.push_back(target);
    target = target->ParentTarget();
  }

  // if the length of the new event response chain is less than
  // click_target_chain_ or if there are different nodes, it is determined that
  // the element is removed from the range.
  if (target_chain.size() < click_target_chain_.size()) {
    return true;
  }
  int num = static_cast<int>(click_target_chain_.size());
  for (int i = 0; i < num; ++i) {
    // judge with sign not target.
    if (!click_target_chain_[i] || !target_chain[i] ||
        click_target_chain_[i]->Sign() != target_chain[i]->Sign()) {
      return true;
    }
  }
  return false;
}

void PlatformEventHandler::GetTargetPoint(
    fml::RefPtr<PlatformEventTarget> target, float target_point[2],
    float page_point[2]) {
  auto root_target =
      platform_ref_->GetEventTargetHelper()->GetRootEventTarget();
  if (!root_target) {
    return;
  }
  platform_ref_->GetEventTargetHelper()->ConvertPointFromAncestorToDescendant(
      target_point, root_target, target, page_point);
}

void PlatformEventHandler::AddTargetPointerMap(lepus::Value& target_pointer_map,
                                               PlatformPointerEvent& event) {
  auto dict = target_pointer_map.Table();
  int num = event.PointerCount();
  for (int i = 0; i < num; ++i) {
    int pointer_id = event.PointerID()[i];
    if (auto pointer_target = target_pointer_map_.find(pointer_id);
        pointer_target != target_pointer_map_.end()) {
      auto target = pointer_target->second.Target();
      if (!target) {
        continue;
      }

      std::string target_sign = std::to_string(target->Sign());
      float page_point[2] = {event.PointerX()[i], event.PointerY()[i]};
      float target_point[2] = {page_point[0], page_point[1]};
      GetTargetPoint(target, target_point, page_point);
      float client_point[2] = {page_point[0], page_point[1]};
      platform_ref_->GetEventTargetHelper()->ConvertPointFromTargetToScreen(
          client_point, target_tree_, client_point);

      auto pointer = lepus::CArray::Create();
      pointer->emplace_back(pointer_id);
      pointer->emplace_back(client_point[0]);
      pointer->emplace_back(client_point[1]);
      pointer->emplace_back(page_point[0]);
      pointer->emplace_back(page_point[1]);
      pointer->emplace_back(target_point[0]);
      pointer->emplace_back(target_point[1]);

      if (auto it = dict->find(target_sign); it != dict->end()) {
        it->second.Array()->emplace_back(std::move(pointer));
      } else {
        auto array = lepus::CArray::Create();
        array->emplace_back(std::move(pointer));
        dict->SetValue(target_sign, std::move(array));
      }
    }
  }
}

}  // namespace tasm
}  // namespace lynx
