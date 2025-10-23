// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/drag_gesture_recognizer.h"

#include <memory>

#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/macros.h"

namespace clay {

namespace {
// In pixels per second.
constexpr float kMinFlingVelocity = 50.f;
constexpr float kMaxFlingVelocity = 8000.f;
}  // namespace

std::unique_ptr<DragGestureRecognizer> CreateDragRecognizerByDirection(
    ScrollDirection direction, GestureManager* gesture_manager) {
  switch (direction) {
    case ScrollDirection::kHorizontal:
      return std::make_unique<HorizontalDragGestureRecognizer>(gesture_manager);
    case ScrollDirection::kVertical:
      return std::make_unique<VerticalDragGestureRecognizer>(gesture_manager);
    default:
      return std::make_unique<DragGestureRecognizer>(gesture_manager);
  }
}

DragGestureRecognizer::DragGestureRecognizer(GestureManager* gesture_manager)
    : super(gesture_manager) {}

void DragGestureRecognizer::AddAllowedPointer(const PointerEvent& pointer) {
  super::AddAllowedPointer(pointer);
  if (state_ == DragState::kReady) {
    state_ = DragState::kPossible;
    initial_pointer_ = pointer;
    pending_distance_ = FloatSize();
    if (on_drag_down_) {
      on_drag_down_(pointer);
    }
  } else if (state_ == DragState::kAccepted) {
    // Notify arena associated with the newer come-in pointer to win.
    ResolveAll(GestureDisposition::kAccept);
  }
}

void DragGestureRecognizer::HandleEvent(const PointerEvent& pointer) {
  // No longer active.
  if (state_ == DragState::kReady) {
    return;
  }

  if (!timestamp_offset_) {
    const fml::TimePoint now = fml::TimePoint::Now();
    const fml::TimeDelta timestamp =
        fml::TimeDelta::FromMicroseconds(pointer.timestamp);
    timestamp_offset_ = now - timestamp;
  }

  auto& velocity_tracker = velocity_trackers_[pointer.pointer_id];
  if (pointer.type == PointerEvent::EventType::kDownEvent ||
      pointer.type == PointerEvent::EventType::kMoveEvent ||
      pointer.type == PointerEvent::EventType::kUpEvent ||
      pointer.type == PointerEvent::EventType::kPanZoomStartEvent ||
      pointer.type == PointerEvent::EventType::kPanZoomUpdateEvent) {
    FloatPoint position{};
    switch (pointer.type) {
      case PointerEvent::EventType::kPanZoomStartEvent:
        position = {};
        break;
      case PointerEvent::EventType::kPanZoomUpdateEvent:
        position = pointer.pan;
        break;
      default:
        position = pointer.position;
        break;
    }
    velocity_tracker.AddPosition(position, pointer.timestamp);
  }

  // When touch slop is less than zero, enter to accept state without move
  // event.
  if ((pointer.type == PointerEvent::EventType::kDownEvent ||
       pointer.type == PointerEvent::EventType::kPanZoomStartEvent) &&
      GetTouchSlop() < 0) {
    ResolveAll(GestureDisposition::kAccept);
  }

  if (pointer.type == PointerEvent::EventType::kMoveEvent ||
      pointer.type == PointerEvent::EventType::kPanZoomUpdateEvent) {
    FloatSize delta = pointer.type == PointerEvent::EventType::kMoveEvent
                          ? pointer.delta
                          : pointer.pan_delta;
    if (state_ == DragState::kAccepted) {
      if (on_drag_update_) {
        on_drag_update_(pointer.position, GetDeltaForDetails(delta));
      }
    } else {
      // See if moved sufficient distance to resolve accept.
      pending_distance_ += delta;
      if (HasSufficientDistanceToAccept(pending_distance_)) {
        ResolveAll(GestureDisposition::kAccept);
      }
    }
  }

  if (pointer.type == PointerEvent::EventType::kUpEvent ||
      pointer.type == PointerEvent::EventType::kCancel ||
      pointer.type == PointerEvent::EventType::kPanZoomEndEvent) {
    GiveupPointer(pointer.pointer_id);
  }
}

void DragGestureRecognizer::DidStopTrackingLastPointer(int pointer_id) {
  GESTURE_LOG << GetMemberTag() << this << " did stop tracking last pointer "
              << pointer_id << " while current drag state is "
              << static_cast<int>(state_);
  switch (state_) {
    case DragState::kReady:
      FML_UNREACHABLE();
      break;
    case DragState::kPossible:
      ResolveAll(GestureDisposition::kReject);
      if (on_drag_cancel_) {
        on_drag_cancel_();
      }
      break;
    case DragState::kAccepted:
      if (on_drag_end_) {
        auto velocity_estimate =
            velocity_trackers_[pointer_id].GetVelocityEstimate(
                getType() == GestureRecognizerType::kVerticalDrag);
        GESTURE_LOG << GetMemberTag() << this << " end, velocity is "
                    << velocity_estimate.pixels_per_second.ToString()
                    << ", calculated in distance: "
                    << velocity_estimate.movement.ToString();
        if (IsFlingGesture(velocity_estimate)) {
          GESTURE_LOG << "is fling";
          on_drag_end_(
              Velocity(velocity_estimate.pixels_per_second)
                  .Clamp(GetMinFlingVelocity(), GetMaxFlingVelocity()));
        } else {
          GESTURE_LOG << "not fling";
          on_drag_end_(Velocity());
        }
      }
      break;
  }
  ResetState();
}

FloatSize DragGestureRecognizer::GetDeltaForDetails(
    const FloatSize& delta) const {
  return delta;
}

bool DragGestureRecognizer::HasSufficientDistanceToAccept(
    const FloatSize& movement) {
  return movement.distance() > GetTouchSlop();
}

bool DragGestureRecognizer::IsFlingGesture(const VelocityEstimate& velocity) {
  return velocity.movement.distance() > GetTouchSlop() &&
         velocity.pixels_per_second.distance() > kMinFlingVelocity;
}

void DragGestureRecognizer::OnGestureAccepted(int pointer_id) {
  GestureRecognizer::OnGestureAccepted(pointer_id);
  accepted_active_pointers_.emplace(pointer_id);
  // Maybe called by multiple pointers.
  if (state_ != DragState::kAccepted) {
    state_ = DragState::kAccepted;
    if (on_drag_start_) {
      // Default use the pointer when trigger start as the start position.
      auto start_position = initial_pointer_.position;
      start_position.MoveBy(pending_distance_);
      on_drag_start_(start_position);
    }
    // Notify other pointers.
    ResolveAll(GestureDisposition::kAccept);
  }
}

void DragGestureRecognizer::OnGestureRejected(int pointer_id) {
  GiveupPointer(pointer_id);
}

void DragGestureRecognizer::GiveupPointer(int pointer_id) {
  GESTURE_LOG << GetMemberTag() << this << " gives up pointer " << pointer_id;
  StopTrackingPointer(pointer_id);
  auto iter = accepted_active_pointers_.find(pointer_id);
  if (iter == accepted_active_pointers_.end()) {
    // Never accepted. Resolve as rejected.
    ResolveOne(pointer_id, GestureDisposition::kReject);
  }
}

void DragGestureRecognizer::ResetState() {
  state_ = DragState::kReady;
  velocity_trackers_.clear();
}

float DragGestureRecognizer::GetTouchSlop() {
  return touch_slop_.value_or(
      gesture_manager_->ConvertFrom<kPixelTypeLogical>(kTouchSlop));
}

FloatSize HorizontalDragGestureRecognizer::GetDeltaForDetails(
    const FloatSize& delta) const {
  return {delta.width(), 0.0};
}

bool HorizontalDragGestureRecognizer::HasSufficientDistanceToAccept(
    const FloatSize& distance) {
  return std::abs(distance.width()) > GetTouchSlop();
}

bool HorizontalDragGestureRecognizer::IsFlingGesture(
    const VelocityEstimate& velocity) {
  return abs(velocity.movement.width()) > GetTouchSlop() &&
         abs(velocity.pixels_per_second.width()) > GetMinFlingVelocity();
}

FloatSize VerticalDragGestureRecognizer::GetDeltaForDetails(
    const FloatSize& delta) const {
  return {0.0, delta.height()};
}

bool VerticalDragGestureRecognizer::HasSufficientDistanceToAccept(
    const FloatSize& distance) {
  return std::abs(distance.height()) > GetTouchSlop();
}

bool VerticalDragGestureRecognizer::IsFlingGesture(
    const VelocityEstimate& velocity) {
  return abs(velocity.movement.height()) > GetTouchSlop() &&
         abs(velocity.pixels_per_second.height()) > GetMinFlingVelocity();
}

float DragGestureRecognizer::GetMaxFlingVelocity() const {
  return gesture_manager_->ConvertFrom<kPixelTypeLogical>(kMaxFlingVelocity);
}

float DragGestureRecognizer::GetMinFlingVelocity() const {
  return gesture_manager_->ConvertFrom<kPixelTypeLogical>(kMinFlingVelocity);
}

}  // namespace clay
