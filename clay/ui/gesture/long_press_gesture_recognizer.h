// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_LONG_PRESS_GESTURE_RECOGNIZER_H_
#define CLAY_UI_GESTURE_LONG_PRESS_GESTURE_RECOGNIZER_H_

#include <functional>
#include <memory>

#include "clay/ui/gesture/gesture_recognizer.h"

namespace clay {

class GestureManager;

// Some kind like drag gesture, except the trigger condition.
using OnLongPressDownCallback = std::function<void(const PointerEvent&)>;
using OnLongPressStartCallback = std::function<void(const PointerEvent&)>;
using OnLongPressMoveCallback = std::function<void(const PointerEvent&)>;
using OnLongPressEndCallback = std::function<void(const PointerEvent&)>;
using OnLongPressCancelCallback = std::function<void()>;

class LongPressGestureRecognizer : public PrimaryPointerGestureRecognizer {
 public:
  explicit LongPressGestureRecognizer(
      GestureManager* gesture_manager,
      uint64_t press_timeout = kLongPressTimeout)
      : super(gesture_manager, press_timeout) {}

  const char* GetMemberTag() const override { return "[LongPress]"; }

  GestureRecognizerType getType() const override {
    return GestureRecognizerType::kLongPress;
  }

  void SetLongPressDownCallback(OnLongPressDownCallback&& callback) {
    on_long_press_down_ = callback;
  }
  void SetLongPressStartCallback(OnLongPressStartCallback&& callback) {
    on_long_press_start_ = callback;
  }
  void SetLongPressMoveCallback(OnLongPressMoveCallback&& callback) {
    on_long_press_move_ = callback;
  }
  void SetLongPressEndCallback(OnLongPressEndCallback&& callback) {
    on_long_press_end_ = callback;
  }
  void SetLongPressCancelCallback(OnLongPressCancelCallback&& callback) {
    on_long_press_cancel_ = callback;
  }

 private:
  using super = PrimaryPointerGestureRecognizer;

  enum class LongPressState {
    kUnknown,
    kDetected,  // is long press.
    kDefunct,   // can not be long press.
  };

  void OnGestureAccepted(int pointer_id) override;

  void OnResolveAll(GestureDisposition disposition) override;

  void HandlePrimaryPointerEvent(const PointerEvent& event) override;

  void OnTimeout() override;

  void Reset();

  static constexpr uint64_t kLongPressTimeout = 500ul;

  // 'Accept' doesn't mean it is a real tap. Maybe there no others arena members
  // competes together. So record accept state and check state when pointer up.
  LongPressState state_ = LongPressState::kUnknown;
  std::unique_ptr<PointerEvent> down_event_;
  std::unique_ptr<PointerEvent> up_event_;

  // Fired immediately after down event.
  OnLongPressDownCallback on_long_press_down_;
  // Fired when long press timeout (and of course not rejected)
  OnLongPressStartCallback on_long_press_start_;
  // Fired after on_long_press_start_ when event move.
  OnLongPressMoveCallback on_long_press_move_;
  // Fired after on_long_press_start_ when event up.
  OnLongPressEndCallback on_long_press_end_;
  // Fired before on_long_press_start_ when event up.
  OnLongPressCancelCallback on_long_press_cancel_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_LONG_PRESS_GESTURE_RECOGNIZER_H_
