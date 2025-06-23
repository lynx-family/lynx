// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_DISPATCHER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_DISPATCHER_H_

#include <arkui/native_gesture.h>
#include <arkui/native_node.h>

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {
class UIOwner;
class UIRoot;
class UIBase;
class EventTarget;

class EventDispatcher {
 public:
  class EventTargetDetail {
   public:
    EventTargetDetail(std::weak_ptr<EventTarget> active_target,
                      float down_point[2]);

    std::weak_ptr<EventTarget> ActiveTarget() { return active_target_; }

    void GetDownPoint(float down_point[2]);

    void GetPrePoint(float pre_point[2]);

    void SetPrePoint(float pre_point[2]);

   private:
    std::weak_ptr<EventTarget> active_target_;
    float down_point_[2]{std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max()};
    float pre_point_[2]{std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()};
  };

  explicit EventDispatcher(UIOwner* ui_owner);

  ~EventDispatcher();

  void SetTapSlop(const std::string& tap_slop);

  void SetHasTouchPseudo(bool has_touch_pseudo) {
    has_touch_pseudo_ = has_touch_pseudo_ || has_touch_pseudo;
  }

  void SetLongPressDuration(int32_t long_press_duration);

  void OnGestureRecognized(UIBase* ui);
  void OnGestureRecognizedWithSign(int sign);

  void SetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target) {
    focused_target_ = focused_target;
  }

  void UnsetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target) {
    if (focused_target_.lock() == focused_target.lock()) {
      focused_target_ = std::weak_ptr<EventTarget>();
    }
  }

  void OnTouchEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                    bool from_overlay = false);

  void OnLongPressEvent(const ArkUI_UIInputEvent* event);

  void OnTapEvent(const ArkUI_UIInputEvent* event);

  void OnGetVelocity(const ArkUI_GestureEvent* event);

  void DispatchSingleTouchEvent(const std::string& name,
                                const ArkUI_UIInputEvent* event);

  void DispatchMultiTouchEvent(const std::string& name,
                               const lepus::Value& target_touch_map,
                               const ArkUI_UIInputEvent* event);

  bool EventThrough();

  bool ShouldBlockNativeEvent();

  ConsumeSlideDirection ShouldConsumeSlideEvent();

  void AttachGesturesToRoot(UIBase* root);

  bool CanConsumeTouchEvent(float point[2]);

 private:
  void InitTouchEnv(const ArkUI_UIInputEvent* event);

  void ResetTouchEnv(const ArkUI_UIInputEvent* event);

  void OnTouchDown(const ArkUI_UIInputEvent* event);

  void OnTouchMove(const ArkUI_UIInputEvent* event);

  void OnTouchUp(const ArkUI_UIInputEvent* event);

  void OnTouchCancel(const ArkUI_UIInputEvent* event);

  bool ShouldInterceptGesture();

  EventTarget* FindTarget(float point[2]);

  bool CanRespondTap();

  void GetTargetPoint(EventTarget* active_target, float target_point[2],
                      float page_point[2]);

  void GetPagePoint(float page_point[2], float node_point[2]);

  void AddTargetTouchMap(lepus::Value& target_touch_map,
                         const ArkUI_UIInputEvent* event);

  void UpdateFocusedTarget();

  void HandleTouchDown(const ArkUI_UIInputEvent* event);

  void HandleTouchMove(const ArkUI_UIInputEvent* event);

  void HandleTouchUp(const ArkUI_UIInputEvent* event);

  void HandleTouchCancel(const ArkUI_UIInputEvent* event);

  void ActivePseudoStatus();

  void DeactivatePseudoStatus(PseudoStatus status);

  bool IsTouchMoveOutside(EventTarget* target);

  bool IsActiveFinger(const ArkUI_UIInputEvent* event, size_t index);

  UIOwner* ui_owner_{nullptr};
  std::weak_ptr<EventTarget> root_target_;
  std::weak_ptr<EventTarget> first_active_target_;
  std::weak_ptr<EventTarget> focused_target_;
  std::unordered_map<int, EventTargetDetail> active_target_finger_map_;
  std::vector<std::weak_ptr<EventTarget>> event_target_chain_;
  std::unordered_set<int> gesture_recognized_target_set_;
  bool has_touch_moved_{false};
  bool first_touch_moved_{false};
  bool first_touch_outside_{false};
  bool from_overlay_{false};
  long long time_stamp_{0};
  unsigned int tap_slop_{5};
  bool has_touch_pseudo_{false};
  int32_t long_press_duration_{500};
  ArkUI_GestureRecognizer* long_press_gesture_{nullptr};
  ArkUI_GestureRecognizer* tap_gesture_{nullptr};
  ArkUI_GestureRecognizer* block_outer_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_horizontal_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_vertical_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_up_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_right_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_down_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_left_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* consume_all_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* velocity_tracker_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* native_gesture_pan_gesture_{nullptr};

  static GestureReceiver long_press_receiver_callback_;
  static GestureReceiver tap_receiver_callback_;
  static GestureInterrupter event_gesture_interrupter_callback_;
  static GestureReceiver velocity_tracker_pan_receiver_callback_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_DISPATCHER_H_
