// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_dispatcher.h"

#include <memory>
#include <utility>

#include "base/include/float_comparison.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_emitter.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/arena/gesture_arena_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
GestureReceiver EventDispatcher::long_press_receiver_callback_ =
    [](ArkUI_GestureEvent* event, void* user_data) {
      if (!user_data) {
        return;
      }
      auto event_dispatcher = reinterpret_cast<EventDispatcher*>(user_data);
      event_dispatcher->OnLongPressEvent(
          OH_ArkUI_GestureEvent_GetRawInputEvent(event));
    };

GestureReceiver EventDispatcher::tap_receiver_callback_ =
    [](ArkUI_GestureEvent* event, void* user_data) {
      if (!user_data) {
        return;
      }
      auto event_dispatcher = reinterpret_cast<EventDispatcher*>(user_data);
      event_dispatcher->OnTapEvent(
          OH_ArkUI_GestureEvent_GetRawInputEvent(event));
    };
// for gesture handler, record the current touch speed.
GestureReceiver EventDispatcher::velocity_tracker_pan_receiver_callback_ =
    [](ArkUI_GestureEvent* event, void* user_data) {
      if (!user_data) {
        return;
      }
      auto event_dispatcher = reinterpret_cast<EventDispatcher*>(user_data);
      event_dispatcher->OnGetVelocity(event);
    };

GestureInterrupter EventDispatcher::event_gesture_interrupter_callback_ =
    [](ArkUI_GestureInterruptInfo* info) -> ArkUI_GestureInterruptResult {
  auto event_dispatcher = NodeManager::Instance().GetEventDispatcher();
  if (!event_dispatcher) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  if (event_dispatcher->EventThrough()) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }

  auto gesture = OH_ArkUI_GestureInterruptInfo_GetRecognizer(info);
  if (gesture == event_dispatcher->long_press_gesture_ ||
      gesture == event_dispatcher->tap_gesture_) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }

  if (gesture == event_dispatcher->block_outer_pan_gesture_ &&
      event_dispatcher->ShouldBlockNativeEvent()) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }

  if (gesture == event_dispatcher->velocity_tracker_pan_gesture_ &&
      event_dispatcher->ui_owner_->GetGestureArenaManager() != nullptr) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }

  if (gesture == event_dispatcher->native_gesture_pan_gesture_) {
    if (event_dispatcher->ShouldInterceptGesture()) {
      return GESTURE_INTERRUPT_RESULT_CONTINUE;
    } else {
      return GESTURE_INTERRUPT_RESULT_REJECT;
    }
  }

  switch (event_dispatcher->ShouldConsumeSlideEvent()) {
    case ConsumeSlideDirection::kHorizontal:
      return gesture == event_dispatcher->consume_horizontal_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kVertical:
      return gesture == event_dispatcher->consume_vertical_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kUp:
      return gesture == event_dispatcher->consume_up_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kRight:
      return gesture == event_dispatcher->consume_right_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kDown:
      return gesture == event_dispatcher->consume_down_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kLeft:
      return gesture == event_dispatcher->consume_left_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kAll:
      return gesture == event_dispatcher->consume_all_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    default:
      return GESTURE_INTERRUPT_RESULT_REJECT;
  }
};

EventDispatcher::EventTargetDetail::EventTargetDetail(
    std::weak_ptr<EventTarget> active_target, float down_point[2]) {
  active_target_ = std::move(active_target);
  down_point_[0] = down_point[0];
  down_point_[1] = down_point[1];
  pre_point_[0] = down_point[0];
  pre_point_[1] = down_point[1];
}

void EventDispatcher::EventTargetDetail::GetDownPoint(float down_point[2]) {
  down_point[0] = down_point_[0];
  down_point[1] = down_point_[1];
}

void EventDispatcher::EventTargetDetail::GetPrePoint(float pre_point[2]) {
  pre_point[0] = pre_point_[0];
  pre_point[1] = pre_point_[1];
}

void EventDispatcher::EventTargetDetail::SetPrePoint(float pre_point[2]) {
  pre_point_[0] = pre_point[0];
  pre_point_[1] = pre_point[1];
}

EventDispatcher::EventDispatcher(UIOwner* ui_owner) : ui_owner_(ui_owner) {
  NodeManager::Instance().SetEventDispatcher(this);
  velocity_tracker_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 0);
  NodeManager::Instance().SetGestureEventTarget(
      velocity_tracker_pan_gesture_, GESTURE_EVENT_ACTION_UPDATE, this,
      EventDispatcher::velocity_tracker_pan_receiver_callback_);
  block_outer_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
  consume_horizontal_pan_gesture_ = NodeManager::Instance().CreatePanGesture(
      1, GESTURE_DIRECTION_HORIZONTAL, 5);
  native_gesture_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
  consume_vertical_pan_gesture_ = NodeManager::Instance().CreatePanGesture(
      1, GESTURE_DIRECTION_VERTICAL, 5);
  consume_up_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_UP, 5);
  consume_right_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_RIGHT, 5);
  consume_down_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_DOWN, 5);
  consume_left_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_LEFT, 5);
  consume_all_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
}

EventDispatcher::~EventDispatcher() {
  NodeManager::Instance().SetEventDispatcher(nullptr);
  if (long_press_gesture_) {
    NodeManager::Instance().DisposeGesture(long_press_gesture_);
  }
  if (tap_gesture_) {
    NodeManager::Instance().DisposeGesture(tap_gesture_);
  }
  NodeManager::Instance().DisposeGesture(block_outer_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_horizontal_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_vertical_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_up_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_right_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_down_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_left_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_all_pan_gesture_);

  NodeManager::Instance().DisposeGesture(native_gesture_pan_gesture_);
  NodeManager::Instance().DisposeGesture(velocity_tracker_pan_gesture_);
}

void EventDispatcher::AttachGesturesToRoot(UIBase* root) {
  if (ui_owner_->Destroyed() || !root->RootNode()) {
    return;
  }
  root_target_ = root->WeakTarget();
  if (long_press_gesture_) {
    NodeManager::Instance().AddGestureToNode(
        root->RootNode(), long_press_gesture_, PARALLEL, NORMAL_GESTURE_MASK);
  }
  if (tap_gesture_) {
    NodeManager::Instance().AddGestureToNode(root->RootNode(), tap_gesture_,
                                             PARALLEL, NORMAL_GESTURE_MASK);
  }
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           block_outer_pan_gesture_, PARALLEL,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_horizontal_pan_gesture_,
                                           PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_vertical_pan_gesture_,
                                           PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(
      root->RootNode(), consume_up_pan_gesture_, PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_right_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_down_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_left_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_all_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           native_gesture_pan_gesture_,
                                           PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           velocity_tracker_pan_gesture_,
                                           PARALLEL, NORMAL_GESTURE_MASK);

  NodeManager::Instance().SetGestureInterrupterToNode(
      root->RootNode(), EventDispatcher::event_gesture_interrupter_callback_);
}

void EventDispatcher::InitTouchEnv(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    float page_point[2] = {OH_ArkUI_PointerEvent_GetXByIndex(event, i),
                           OH_ArkUI_PointerEvent_GetYByIndex(event, i)};
    GetPagePoint(page_point, page_point);
    EventTarget* best_hittest_target = FindTarget(page_point);
    if (best_hittest_target == nullptr) {
      continue;
    }
    LOGI("EventDispatcher InitTouchEnv hit target: "
         << best_hittest_target->Sign())
    if (OH_ArkUI_PointerEvent_GetPointerId(event, i) == 0 ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_MOUSE ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN) {
      first_active_target_ = best_hittest_target->WeakTarget();
    }
    active_target_finger_map_.insert_or_assign(
        OH_ArkUI_PointerEvent_GetPointerId(event, i),
        EventTargetDetail(best_hittest_target->WeakTarget(), page_point));
  }
}

void EventDispatcher::ResetTouchEnv(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    active_target_finger_map_.erase(
        OH_ArkUI_PointerEvent_GetPointerId(event, i));
  }
  has_touch_moved_ = false;
}

void EventDispatcher::OnTouchDown(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (OH_ArkUI_PointerEvent_GetPointerId(event, i) == 0 ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_MOUSE ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN) {
      first_touch_moved_ = false;
      first_touch_outside_ = false;
      gesture_recognized_target_set_.clear();
      event_target_chain_.clear();
      ActivePseudoStatus();
      break;
    }
  }
}

void EventDispatcher::OnTouchMove(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  bool first_touch_changed = false;
  float pre_page_point[2] = {0.f};
  for (size_t i = 0; i < num; ++i) {
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, i);
    float page_point[2] = {OH_ArkUI_PointerEvent_GetXByIndex(event, i),
                           OH_ArkUI_PointerEvent_GetYByIndex(event, i)};
    GetPagePoint(page_point, page_point);
    if (auto touch_target = active_target_finger_map_.find(pointer_id);
        touch_target != active_target_finger_map_.end()) {
      touch_target->second.GetPrePoint(pre_page_point);
      if (base::FloatsNotEqual(page_point[0], pre_page_point[0]) ||
          base::FloatsNotEqual(page_point[1], pre_page_point[1])) {
        has_touch_moved_ = true;
        touch_target->second.SetPrePoint(page_point);
        if (!first_touch_moved_ &&
            (pointer_id == 0 ||
             OH_ArkUI_UIInputEvent_GetSourceType(event) ==
                 UI_INPUT_EVENT_SOURCE_TYPE_MOUSE ||
             OH_ArkUI_UIInputEvent_GetSourceType(event) ==
                 UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN)) {
          first_touch_changed = true;
          float down_page_point[2] = {0.f};
          touch_target->second.GetDownPoint(down_page_point);
          if (base::FloatsLarger(
                  pow(abs(page_point[0] - down_page_point[0]), 2) +
                      pow(abs(page_point[1] - down_page_point[1]), 2),
                  pow(tap_slop_, 2))) {
            first_touch_moved_ = true;
          }
        }
      }
    }
  }

  if (first_touch_changed) {
    if (auto first_touch_target = active_target_finger_map_.find(0);
        first_touch_target != active_target_finger_map_.end()) {
      first_touch_target->second.GetPrePoint(pre_page_point);
      first_touch_outside_ = first_touch_outside_ ||
                             IsTouchMoveOutside(FindTarget(pre_page_point));
      if (first_touch_moved_ || first_touch_outside_ || CanRespondTap()) {
        DeactivatePseudoStatus(PseudoStatus::kActive);
      }
    }
  }
}

void EventDispatcher::OnTouchUp(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    if (OH_ArkUI_PointerEvent_GetPointerId(event, i) == 0 ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_MOUSE ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN) {
      UpdateFocusedTarget();
      DeactivatePseudoStatus(PseudoStatus::kAll);
      break;
    }
  }
}

void EventDispatcher::OnTouchCancel(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (OH_ArkUI_PointerEvent_GetPointerId(event, i) == 0 ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_MOUSE ||
        OH_ArkUI_UIInputEvent_GetSourceType(event) ==
            UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN) {
      UpdateFocusedTarget();
      DeactivatePseudoStatus(PseudoStatus::kAll);
      break;
    }
  }
}

EventTarget* EventDispatcher::FindTarget(float point[2]) {
  if (root_target_.expired()) {
    return nullptr;
  }
  return root_target_.lock()->HitTest(point);
}

bool EventDispatcher::CanRespondTap() {
  if (first_active_target_.expired()) {
    return false;
  }
  if (gesture_recognized_target_set_.empty()) {
    return true;
  }

  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (gesture_recognized_target_set_.find(target->Sign()) !=
        gesture_recognized_target_set_.end()) {
      return false;
    }
    target = target->ParentTarget();
  }
  return true;
}

void EventDispatcher::UpdateFocusedTarget() {
  auto active_target = first_active_target_.lock();
  auto focused_target = focused_target_.lock();
  if (active_target && !active_target->IgnoreFocus()) {
    if (focused_target) {
      if (focused_target != active_target) {
        focused_target->OnFocusChange(
            false, active_target != nullptr && active_target->Focusable());
      }
    }
    active_target->OnFocusChange(
        true, focused_target != nullptr && focused_target->Focusable());
    focused_target_ = first_active_target_;
  }
}

void EventDispatcher::GetTargetPoint(EventTarget* active_target,
                                     float target_point[2],
                                     float page_point[2]) {
  active_target = active_target->FirstUITarget();
  if (!active_target) {
    return;
  }
  UIBase* active_ui = reinterpret_cast<UIBase*>(active_target);
  if (active_ui && !ui_owner_->Destroyed()) {
    LynxUIHelper::ConvertPointFromAncestorToDescendant(
        target_point, ui_owner_->Root(), active_ui, page_point);
  }
}

void EventDispatcher::GetPagePoint(float page_point[2], float node_point[2]) {
  if (!from_overlay_ || ui_owner_->Destroyed()) {
    return;
  }
  auto root = ui_owner_->Root();

  ArkUI_IntOffset page_offset;
  OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root->GetProxyNode(),
                                                      &page_offset);
  float node_point_x = node_point[0], node_point_y = node_point[1];
  float scaled_density = root->GetContext()->ScaledDensity();
  page_point[0] = node_point_x - page_offset.x / scaled_density;
  page_point[1] = node_point_y - page_offset.y / scaled_density;
}

void EventDispatcher::AddTargetTouchMap(lepus::Value& target_touch_map,
                                        const ArkUI_UIInputEvent* event) {
  auto dict = target_touch_map.Table();
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, i);
    if (auto touch_target = active_target_finger_map_.find(pointer_id);
        touch_target != active_target_finger_map_.end()) {
      auto active_target = touch_target->second.ActiveTarget().lock().get();
      if (!active_target) {
        continue;
      }

      std::string target_sign = std::to_string(active_target->Sign());
      float page_point[2] = {OH_ArkUI_PointerEvent_GetXByIndex(event, i),
                             OH_ArkUI_PointerEvent_GetYByIndex(event, i)};
      GetPagePoint(page_point, page_point);
      float target_point[2] = {page_point[0], page_point[1]};
      GetTargetPoint(active_target, target_point, page_point);
      float client_point[2] = {
          OH_ArkUI_PointerEvent_GetWindowXByIndex(event, i),
          OH_ArkUI_PointerEvent_GetWindowYByIndex(event, i)};

      auto touch = lepus::CArray::Create();
      touch->emplace_back(pointer_id);
      touch->emplace_back(client_point[0]);
      touch->emplace_back(client_point[1]);
      touch->emplace_back(page_point[0]);
      touch->emplace_back(page_point[1]);
      touch->emplace_back(target_point[0]);
      touch->emplace_back(target_point[1]);

      if (auto it = dict->find(target_sign); it != dict->end()) {
        it->second.Array()->emplace_back(std::move(touch));
      } else {
        auto array = lepus::CArray::Create();
        array->emplace_back(std::move(touch));
        dict->SetValue(target_sign, std::move(array));
      }
    }
  }
}

void EventDispatcher::SetTapSlop(const std::string& tap_slop) {
  if (tap_gesture_) {
    return;
  }
  float screen_size[2] = {0};
  ui_owner_->Context()->ScreenSize(screen_size);
  tap_slop_ = LynxUnitUtils::ToVPFromUnitValue(
      tap_slop, screen_size[0], ui_owner_->Context()->DevicePixelRatio(), 5.f);
  tap_slop_ = tap_slop_ > 0 ? tap_slop_ : 5.f;
  tap_gesture_ = NodeManager::Instance().createTapGestureWithDistanceThreshold(
      1, 1, tap_slop_);
  if (tap_gesture_ == nullptr) {
    return;
  }
  NodeManager::Instance().SetGestureEventTarget(
      tap_gesture_, GESTURE_EVENT_ACTION_ACCEPT, this,
      EventDispatcher::tap_receiver_callback_);
  if (!root_target_.expired()) {
    NodeManager::Instance().AddGestureToNode(root_target_.lock()->RootNode(),
                                             tap_gesture_, PARALLEL,
                                             NORMAL_GESTURE_MASK);
  }
}

void EventDispatcher::SetLongPressDuration(int32_t long_press_duration) {
  if (long_press_gesture_) {
    return;
  }
  long_press_duration_ = long_press_duration > 0 ? long_press_duration : 500;
  long_press_gesture_ = NodeManager::Instance().CreateLongPressGesture(
      1, false, long_press_duration_);
  if (long_press_gesture_ == nullptr) {
    return;
  }
  NodeManager::Instance().SetGestureEventTarget(
      long_press_gesture_, GESTURE_EVENT_ACTION_ACCEPT, this,
      EventDispatcher::long_press_receiver_callback_);
  if (!root_target_.expired()) {
    NodeManager::Instance().AddGestureToNode(root_target_.lock()->RootNode(),
                                             long_press_gesture_, PARALLEL,
                                             NORMAL_GESTURE_MASK);
  }
}

void EventDispatcher::OnGestureRecognized(UIBase* ui) {
  gesture_recognized_target_set_.insert(ui->Sign());
}

void EventDispatcher::OnGestureRecognizedWithSign(int sign) {
  gesture_recognized_target_set_.insert(sign);
}

void EventDispatcher::HandleTouchDown(const ArkUI_UIInputEvent* event) {
  InitTouchEnv(event);
  if (EventThrough()) {
    ResetTouchEnv(event);
    return;
  }
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  DispatchMultiTouchEvent(TouchEvent::START, target_touch_map, event);
  OnTouchDown(event);
}

void EventDispatcher::HandleTouchMove(const ArkUI_UIInputEvent* event) {
  OnTouchMove(event);
  if (!has_touch_moved_) {
    return;
  }
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  DispatchMultiTouchEvent(TouchEvent::MOVE, target_touch_map, event);
}

void EventDispatcher::HandleTouchUp(const ArkUI_UIInputEvent* event) {
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  DispatchMultiTouchEvent(TouchEvent::UP, target_touch_map, event);
  OnTouchUp(event);
  ResetTouchEnv(event);
}

void EventDispatcher::HandleTouchCancel(const ArkUI_UIInputEvent* event) {
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  DispatchMultiTouchEvent(TouchEvent::CANCEL, target_touch_map, event);
  OnTouchCancel(event);
  ResetTouchEnv(event);
}

void EventDispatcher::ActivePseudoStatus() {
  if (first_active_target_.expired()) {
    return;
  }
  EventTarget* current = first_active_target_.lock().get();
  while (current != nullptr && current->ParentTarget() != current) {
    event_target_chain_.push_back(current->WeakTarget());
    current->OnPseudoStatusChanged(PseudoStatus::kNone, PseudoStatus::kActive);
    if (has_touch_pseudo_) {
      ui_owner_->SendPseudoStatusEvent(current->Sign(), PseudoStatus::kNone,
                                       PseudoStatus::kActive);
    }
    if (!current->TouchPseudoPropagation()) {
      break;
    }
    current = current->ParentTarget();
  }
}

void EventDispatcher::DeactivatePseudoStatus(PseudoStatus status) {
  int32_t int_status = static_cast<int32_t>(status);
  for (auto target : event_target_chain_) {
    if (target.expired()) {
      continue;
    }
    auto current = target.lock();
    int32_t current_status = static_cast<int32_t>(current->GetPseudoStatus());
    current->OnPseudoStatusChanged(
        static_cast<PseudoStatus>(current_status),
        static_cast<PseudoStatus>(current_status & ~int_status));
    if (has_touch_pseudo_) {
      ui_owner_->SendPseudoStatusEvent(
          current->Sign(), static_cast<PseudoStatus>(current_status),
          static_cast<PseudoStatus>(current_status & ~int_status));
    }
  }
}

void EventDispatcher::OnGetVelocity(const ArkUI_GestureEvent* event) {
  float velocity_x = OH_ArkUI_PanGesture_GetVelocityX(event);
  float velocity_y = OH_ArkUI_PanGesture_GetVelocityY(event);
  if (ui_owner_) {
    ui_owner_->SetVelocityToGestureArena(velocity_x, velocity_y);
  }
}

bool EventDispatcher::IsTouchMoveOutside(EventTarget* target) {
  if (target == nullptr) {
    return true;
  }

  std::vector<EventTarget*> target_chain;
  while (target != nullptr && target->ParentTarget() != target) {
    target_chain.push_back(target);
    target = target->ParentTarget();
  }
  if (target_chain.size() < event_target_chain_.size()) {
    return true;
  }

  for (size_t i = 0; i < event_target_chain_.size(); ++i) {
    if (event_target_chain_[i].expired() ||
        event_target_chain_[i].lock().get() != target_chain[i]) {
      return true;
    }
  }
  return false;
}

bool EventDispatcher::IsActiveFinger(const ArkUI_UIInputEvent* event,
                                     size_t index) {
  if (auto action = OH_ArkUI_UIInputEvent_GetAction(event);
      action == UI_TOUCH_EVENT_ACTION_MOVE ||
      action == UI_TOUCH_EVENT_ACTION_CANCEL) {
    return true;
  }
  float active_x = OH_ArkUI_PointerEvent_GetX(event);
  float active_y = OH_ArkUI_PointerEvent_GetY(event);
  float finger_x = OH_ArkUI_PointerEvent_GetXByIndex(event, index);
  float finger_y = OH_ArkUI_PointerEvent_GetYByIndex(event, index);
  return base::FloatsEqual(active_x, finger_x) &&
         base::FloatsEqual(active_y, finger_y);
}

void EventDispatcher::EventDispatcher::OnTouchEvent(
    const ArkUI_UIInputEvent* event, UIBase* root, bool from_overlay) {
  if (ui_owner_->Destroyed()) {
    return;
  }
  time_stamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  NodeManager::Instance().SetEventDispatcher(this);
  if (OH_ArkUI_UIInputEvent_GetAction(event) == UI_TOUCH_EVENT_ACTION_DOWN) {
    float active_x = OH_ArkUI_PointerEvent_GetX(event);
    float active_y = OH_ArkUI_PointerEvent_GetY(event);
    LOGI("EventDispatcher OnTouchEvent down x:" << active_x
                                                << ", y:" << active_y)
    from_overlay_ = from_overlay;
    root_target_ = root->WeakTarget();
    HandleTouchDown(event);
  } else if (!first_active_target_.expired() &&
             !active_target_finger_map_.empty()) {
    if (EventThrough()) {
      return;
    }
    switch (OH_ArkUI_UIInputEvent_GetAction(event)) {
      case UI_TOUCH_EVENT_ACTION_MOVE: {
        HandleTouchMove(event);
        break;
      }
      case UI_TOUCH_EVENT_ACTION_UP: {
        HandleTouchUp(event);
        break;
      }
      case UI_TOUCH_EVENT_ACTION_CANCEL: {
        HandleTouchCancel(event);
        break;
      }
    }
  }

  if (EventThrough() ||
      (OH_ArkUI_UIInputEvent_GetAction(event) == UI_TOUCH_EVENT_ACTION_MOVE &&
       !has_touch_moved_)) {
    return;
  }

  if (!first_active_target_.expired()) {
    first_active_target_.lock()->DispatchTouch(event);
  }
}

void EventDispatcher::DispatchSingleTouchEvent(
    const std::string& name, const ArkUI_UIInputEvent* event) {
  if (first_active_target_.expired()) {
    return;
  }

  auto active_target = first_active_target_.lock().get();
  TouchEvent touch_event(active_target->Sign(), name);
  auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
  touch_event.SetTimeStamp(time_stamp);
  float scaled_density = ui_owner_->Context()->ScaledDensity();
  float page_point[2] = {
      OH_ArkUI_PointerEvent_GetXByIndex(event, 0) / scaled_density,
      OH_ArkUI_PointerEvent_GetYByIndex(event, 0) / scaled_density};
  GetPagePoint(page_point, page_point);
  float target_point[2] = {page_point[0], page_point[1]};
  GetTargetPoint(active_target, target_point, page_point);
  float client_point[2] = {
      OH_ArkUI_PointerEvent_GetWindowXByIndex(event, 0) / scaled_density,
      OH_ArkUI_PointerEvent_GetWindowYByIndex(event, 0) / scaled_density};
  touch_event.SetTargetPoint(target_point);
  touch_event.SetPagePoint(page_point);
  touch_event.SetClientPoint(client_point);
  touch_event.SetTimeStamp(OH_ArkUI_UIInputEvent_GetEventTime(event));
  ui_owner_->SendEvent(touch_event);
}

void EventDispatcher::DispatchMultiTouchEvent(
    const std::string& name, const lepus::Value& target_touch_map,
    const ArkUI_UIInputEvent* event) {
  TouchEvent touch_event(name, target_touch_map);
  touch_event.SetTimeStamp(time_stamp_);
  ui_owner_->SendEvent(touch_event);
  if (ui_owner_->GetGestureArenaManager()) {
    if (name == TouchEvent::START) {
      ui_owner_->SetActiveUIToGestureArenaAtDownEvent(first_active_target_);
    }
    ui_owner_->DispatchTouchEventToGestureArena(
        name, std::make_shared<TouchEvent>(touch_event), event);
  }
}

void EventDispatcher::OnLongPressEvent(const ArkUI_UIInputEvent* event) {
  if (first_active_target_.expired()) {
    return;
  }
  DispatchSingleTouchEvent(TouchEvent::LONGPRESS, event);
}

void EventDispatcher::OnTapEvent(const ArkUI_UIInputEvent* event) {
  bool can_respond_tap = CanRespondTap();
  if (first_active_target_.expired() || first_touch_moved_ ||
      !can_respond_tap) {
    LOGI("EventDispatcher OnTapEvent tap failed: "
         << first_active_target_.expired() << ", " << first_touch_moved_ << ", "
         << can_respond_tap)
    return;
  }
  DispatchSingleTouchEvent(TouchEvent::TAP, event);
}

bool EventDispatcher::EventThrough() {
  if (first_active_target_.expired()) {
    return false;
  }
  return first_active_target_.lock()->EventThrough();
}

bool EventDispatcher::ShouldInterceptGesture() {
  if (first_active_target_.expired()) {
    return false;
  }
  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (target->IsInterceptGesture()) {
      return true;
    }
    target = target->ParentTarget();
  }
  return false;
}

bool EventDispatcher::ShouldBlockNativeEvent() {
  if (first_active_target_.expired()) {
    return false;
  }

  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (target->BlockNativeEvent()) {
      return true;
    }
    target = target->ParentTarget();
  }
  return false;
}

ConsumeSlideDirection EventDispatcher::ShouldConsumeSlideEvent() {
  if (first_active_target_.expired()) {
    return ConsumeSlideDirection::kNone;
  }

  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (target->ConsumeSlideEvent() != ConsumeSlideDirection::kNone) {
      // TODO(hexionghui): Should collect all consume-slide-event.
      return target->ConsumeSlideEvent();
    }
    target = target->ParentTarget();
  }
  return ConsumeSlideDirection::kNone;
}

bool EventDispatcher::CanConsumeTouchEvent(float point[2]) {
  if (ui_owner_->Destroyed()) {
    return false;
  }
  auto root = ui_owner_->Root();

  ArkUI_IntOffset page_offset;
  OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root->GetProxyNode(),
                                                      &page_offset);
  float node_point_x = point[0], node_point_y = point[1];
  float scaled_density = root->GetContext()->ScaledDensity();
  point[0] = node_point_x - page_offset.x / scaled_density;
  point[1] = node_point_y - page_offset.y / scaled_density;

  EventTarget* active_target = FindTarget(point);
  if (active_target == nullptr) {
    return false;
  }
  return !active_target->EventThrough();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
