// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_TAP_GESTURE_RECOGNIZER_H_
#define CLAY_UI_GESTURE_TAP_GESTURE_RECOGNIZER_H_

#include <functional>
#include <memory>

#include "clay/ui/gesture/gesture_recognizer.h"

namespace clay {

class GestureManager;

class TapGestureRecognizer : public PrimaryPointerGestureRecognizer {
 public:
  explicit TapGestureRecognizer(GestureManager* gesture_manager)
      : super(gesture_manager, std::nullopt) {}

  void SetTapUpCallback(OnTapUpCallback&& callback) { on_tap_up_ = callback; }
  void SetTapCallback(OnTapCallback&& callback) { on_tap_ = callback; }
  void SetTapDownCallback(OnTapDownCallback&& callback) {
    on_tap_down_ = callback;
  }
  void SetTapCancelCallback(OnTapCancelCallback&& callback) {
    on_tap_cancel_ = callback;
  }
  void SetSecondTapUpCallback(OnTapUpCallback&& callback) {
    on_second_tap_up_ = callback;
  }
  void SetSecondTapDownCallback(OnTapDownCallback&& callback) {
    on_second_tap_down_ = callback;
  }
  void SetSecondTapCallback(OnTapCallback&& callback) {
    on_second_tap_ = callback;
  }
  void SetSecondTapCancelCallback(OnTapCancelCallback&& callback) {
    on_second_tap_cancel_ = callback;
  }
  const char* GetMemberTag() const override { return "[Tap]"; }

  GestureRecognizerType getType() const override {
    return GestureRecognizerType::kTap;
  }

 private:
  using super = PrimaryPointerGestureRecognizer;

  void AddAllowedPointer(const PointerEvent& pointer) override;

  void OnGestureAccepted(int pointer_id) override;
  void OnGestureRejected(int pointer_id) override;

  void HandlePrimaryPointerEvent(const PointerEvent& event) override;

  void Reset();

  void CheckIfTapUp();

  void CheckIfTapDown();

  void CheckIfTapCancel();

  void OnTimeout() override;

  // 'Accept' doesn't mean it is a real tap. Maybe there no others arena members
  // competes together. So record accept state and check state when pointer up.
  bool accepted_ = false;
  bool sent_tap_down_ = false;
  std::unique_ptr<PointerEvent> down_event_;
  std::unique_ptr<PointerEvent> up_event_;
  OnTapUpCallback on_tap_up_;
  OnTapDownCallback on_tap_down_;
  OnTapCallback on_tap_;
  OnTapCancelCallback on_tap_cancel_;
  OnTapUpCallback on_second_tap_up_;
  OnTapCallback on_second_tap_;
  OnTapDownCallback on_second_tap_down_;
  OnTapCancelCallback on_second_tap_cancel_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_TAP_GESTURE_RECOGNIZER_H_
