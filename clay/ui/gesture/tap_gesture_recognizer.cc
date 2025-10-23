// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/tap_gesture_recognizer.h"

#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/arena_manager.h"
#include "clay/ui/gesture/macros.h"

namespace clay {

void TapGestureRecognizer::AddAllowedPointer(const PointerEvent& pointer) {
  if (recognizer_state_ == GestureRecognizerState::kReady) {
    if (down_event_ || up_event_) {
      Reset();
    }
    down_event_ = std::make_unique<PointerEvent>(pointer);
  }
  super::AddAllowedPointer(pointer);
}

void TapGestureRecognizer::Reset() {
  super::Reset();
  down_event_.reset();
  up_event_.reset();
  accepted_ = false;
  sent_tap_down_ = false;
}

void TapGestureRecognizer::HandlePrimaryPointerEvent(
    const PointerEvent& event) {
  if (event.type == PointerEvent::EventType::kUpEvent) {
    up_event_ = std::make_unique<PointerEvent>(event);
    CheckIfTapUp();
  } else if (event.type == PointerEvent::EventType::kDownEvent) {
    down_event_ = std::make_unique<PointerEvent>(event);
    CheckIfTapDown();
  } else if (event.type == PointerEvent::EventType::kCancel) {
    ResolveAll(GestureDisposition::kReject);
    if (sent_tap_down_) {
      CheckIfTapCancel();
    }
    Reset();
  }
}

void TapGestureRecognizer::CheckIfTapUp() {
  if (!accepted_ || !up_event_) {
    return;
  }

  GESTURE_LOG << GetMemberTag() << this
              << " CheckIfTapUp. pointer_id: " << up_event_->pointer_id;

  if (down_event_->device == PointerEvent::kTouch) {
    if (on_tap_up_) {
      on_tap_up_(*up_event_);
    }
  } else {
    if (down_event_->buttons == PointerEvent::kPrimary) {
      if (on_tap_up_) {
        on_tap_up_(*up_event_);
      }
      if (on_tap_) {
        on_tap_();
      }
    }
    if (down_event_->buttons == PointerEvent::kSecondary) {
      if (on_second_tap_) {
        on_second_tap_();
      }
      if (on_second_tap_up_) {
        on_second_tap_up_(*up_event_);
      }
    }
  }
  Reset();
}

void TapGestureRecognizer::CheckIfTapDown() {
  if (sent_tap_down_ || !down_event_) {
    return;
  }

  GESTURE_LOG << GetMemberTag() << this
              << " CheckIfTapDown. pointer_id: " << down_event_->pointer_id;

  if (down_event_->buttons == PointerEvent::kPrimary) {
    if (on_tap_down_) {
      on_tap_down_(*down_event_);
    }
  } else if (down_event_->buttons == PointerEvent::kSecondary) {
    if (on_second_tap_down_) {
      on_second_tap_down_(*down_event_);
    }
  }

  sent_tap_down_ = true;
}

void TapGestureRecognizer::CheckIfTapCancel() {
  if (down_event_->buttons == PointerEvent::kPrimary) {
    if (on_tap_cancel_) {
      on_tap_cancel_();
    }
  } else if (down_event_->buttons == PointerEvent::kSecondary) {
    if (on_second_tap_cancel_) {
      on_second_tap_cancel_();
    }
  }
}

void TapGestureRecognizer::OnGestureAccepted(int pointer_id) {
  super::OnGestureAccepted(pointer_id);

  if (primary_pointer() == pointer_id) {
    accepted_ = true;
    CheckIfTapUp();
  }
}

void TapGestureRecognizer::OnGestureRejected(int pointer_id) {
  super::OnGestureRejected(pointer_id);

  if (primary_pointer() == pointer_id) {
    Reset();
  }
}

void TapGestureRecognizer::OnTimeout() {}

}  // namespace clay
