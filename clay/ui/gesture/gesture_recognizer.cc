// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/gesture_recognizer.h"

#include <math.h>

#include <memory>
#include <utility>

#include "clay/ui/common/isolate.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/macros.h"

namespace clay {

void GestureRecognizer::AddPointer(const PointerEvent& pointer) {
  if (delegate_ && !delegate_->IsPointerAllowed(*this, pointer)) {
    return;
  }
  AddAllowedPointer(pointer);
}

void GestureRecognizer::OnGestureAccepted(int pointer_id) {
  gesture_manager_->OnGestureAccepted(pointer_id, getType());
}

void OneSequenceGestureRecognizer::StartTrackingPointer(int pointer_id) {
  tracking_pointers_.emplace(pointer_id);
  GESTURE_LOG << GetMemberTag() << this << " start tracking pointer "
              << pointer_id << " , add to router id = " << route_id_;
  gesture_manager_->pointer_router().AddRoute(
      pointer_id, route_id_,
      [this](auto&& PH1) { HandleEvent(std::forward<decltype(PH1)>(PH1)); });
  arena_entries_[pointer_id] = gesture_manager_->arena_manager()->Add(
      pointer_id, weak_factory_.GetWeakPtr());
}

void OneSequenceGestureRecognizer::StopTrackingPointer(int pointer_id) {
  if (tracking_pointers_.count(pointer_id)) {
    GESTURE_LOG << GetMemberTag() << this << " stop tracking pointer "
                << pointer_id;
    tracking_pointers_.erase(pointer_id);
    gesture_manager_->pointer_router().RemoveRoute(pointer_id, route_id_);
    if (tracking_pointers_.empty()) {
      DidStopTrackingLastPointer(pointer_id);
    }
  }
}

OneSequenceGestureRecognizer::~OneSequenceGestureRecognizer() {
  for (int pointer_id : tracking_pointers_) {
    gesture_manager_->pointer_router().RemoveRoute(pointer_id, route_id_);
  }
}

void OneSequenceGestureRecognizer::ResolveAll(GestureDisposition disposition) {
  OnResolveAll(disposition);
  decltype(arena_entries_) local_entries = std::move(arena_entries_);
  for (auto& pair : local_entries) {
    pair.second->Resolve(disposition);
  }
}

void OneSequenceGestureRecognizer::ResolveOne(int pointer_id,
                                              GestureDisposition disposition) {
  OnResolveOne(pointer_id, disposition);
  auto iter = arena_entries_.find(pointer_id);
  if (iter != arena_entries_.end()) {
    std::unique_ptr<ArenaEntry> entry = std::move(iter->second);
    arena_entries_.erase(iter);
    entry->Resolve(disposition);
  }
}

void PrimaryPointerGestureRecognizer::AddAllowedPointer(
    const PointerEvent& event) {
  super::AddAllowedPointer(event);
  if (recognizer_state_ == GestureRecognizerState::kReady) {
    recognizer_state_ = GestureRecognizerState::kPossible;
    primary_pointer_ = event.pointer_id;
    initial_position_ = event.position;

    if (timeout_ms_) {
      if (!timer_) {
        timer_ = std::make_unique<fml::OneshotTimer>(
            timer_runner() ? timer_runner()
                           : gesture_manager_->GetTaskRunner());
      }
      GESTURE_LOG << "start a timer with timeout: " << *timeout_ms_;
      timer_->Start(fml::TimeDelta::FromMilliseconds(*timeout_ms_),
                    [this] { OnTimeout(); });
    }
  }
}

void PrimaryPointerGestureRecognizer::OnTimeout() {
  FML_DCHECK(false) << "Now that derived recognizers have set timeout, "
                       "so override to do something!";
}

void PrimaryPointerGestureRecognizer::HandleEvent(const PointerEvent& event) {
  GESTURE_LOG << GetMemberTag() << this
              << " handle primary pointer gesture: " << event.ToString();
  if (event.pointer_id == primary_pointer_ &&
      recognizer_state_ == GestureRecognizerState::kPossible) {
    if ((event.type == PointerEvent::EventType::kMoveEvent ||
         event.type == PointerEvent::EventType::kPanZoomUpdateEvent) &&
        !IsWithinDriftTolerance(event.position)) {
      GESTURE_LOG << "drift too long to be recognized as primary gesture";
      ResolveAll(GestureDisposition::kReject);
      StopTrackingPointer(event.pointer_id);
    } else {
      HandlePrimaryPointerEvent(event);
    }
  }

  if (event.type == PointerEvent::EventType::kUpEvent ||
      event.type == PointerEvent::EventType::kCancel ||
      event.type == PointerEvent::EventType::kPanZoomEndEvent) {
    StopTrackingPointer(event.pointer_id);
  }
}

bool PrimaryPointerGestureRecognizer::IsWithinDriftTolerance(
    const FloatPoint& position) {
  auto delta_point = position - initial_position_;
  auto delta_size = FloatSize(delta_point.x(), delta_point.y());
  auto tolerance = drift_tolerance_.value_or(
      gesture_manager_->ConvertFrom<kPixelTypeLogical>(kTouchSlop));
  return std::abs(delta_size.distance()) <= tolerance;
}

void PrimaryPointerGestureRecognizer::DidStopTrackingLastPointer(
    int pointer_id) {
  Reset();
}

void PrimaryPointerGestureRecognizer::OnGestureAccepted(int pointer_id) {
  GestureRecognizer::OnGestureAccepted(pointer_id);
  if (pointer_id == primary_pointer_ && timer_) {
    timer_.reset();
  }
}

void PrimaryPointerGestureRecognizer::OnGestureRejected(int pointer_id) {
  if (pointer_id == primary_pointer_ && timer_) {
    timer_.reset();
  }
}

void PrimaryPointerGestureRecognizer::Reset() {
  initial_position_ = FloatPoint();
  recognizer_state_ = GestureRecognizerState::kReady;
}

}  // namespace clay
