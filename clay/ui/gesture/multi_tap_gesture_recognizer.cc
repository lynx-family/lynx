// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/multi_tap_gesture_recognizer.h"

#include <memory>

#include "clay/ui/common/isolate.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/macros.h"

namespace clay {

#if (defined(OS_MAC) || defined(OS_WIN))
constexpr float kMultiTapSlop = 2.5f;
#else
constexpr float kMultiTapSlop = 18.f;
#endif
constexpr uint64_t kMultiTapTimeout = 300;

TapTracker::TapTracker(GestureManager* gesture_manager,
                       const PointerEvent& event,
                       std::unique_ptr<ArenaEntry> entry)
    : gesture_manager_(gesture_manager),
      event_(event),
      arena_entry_(std::move(entry)) {}

TapTracker::~TapTracker() { StopTracking(); }

void TapTracker::StartTracking(RouteId route_id, PointerRoute&& route) {
  has_start_tracking_ = true;
  route_id_ = route_id;
  gesture_manager_->pointer_router().AddRoute(event_.pointer_id, route_id_,
                                              std::move(route));
}

void TapTracker::StopTracking() {
  if (has_start_tracking_) {
    has_start_tracking_ = false;
    gesture_manager_->pointer_router().RemoveRoute(event_.pointer_id,
                                                   route_id_);
  }
}

bool TapTracker::IsWithinTolerance(const PointerEvent& other, float tolerance) {
  return (other.position - event_.position).distance() < tolerance;
}

void TapTracker::Resolve(GestureDisposition disposition) {
  arena_entry_->Resolve(disposition);
}

void MultiTapGestureRecognizer::AddAllowedPointer(const PointerEvent& pointer) {
  if (last_tap_) {
    if ((first_tap_position_ - pointer.position).distance() >=
        gesture_manager_->ConvertFrom<kPixelTypeLogical>(kMultiTapSlop)) {
      // Don't reject. Just abandon this tap as latter taps can trigger gesture.
      GESTURE_LOG << "New pointer's position is too far from the first tap. "
                     "Discard this pointer.";
      return;
    }
  }
  TrackTap(pointer);
}

void MultiTapGestureRecognizer::TrackTap(const PointerEvent& pointer) {
  timer_.reset();
  auto tracker = std::make_unique<TapTracker>(
      gesture_manager_, pointer,
      gesture_manager_->arena_manager()->Add(pointer.pointer_id,
                                             weak_factory_.GetWeakPtr()));
  tracker->StartTracking(route_id(), [this](auto&& PH1) {
    HandleEvent(std::forward<decltype(PH1)>(PH1));
  });
  tracking_taps_.emplace(pointer.pointer_id, std::move(tracker));
}

void MultiTapGestureRecognizer::HandleEvent(const PointerEvent& event) {
  auto iter = tracking_taps_.find(event.pointer_id);
  FML_DCHECK(iter != tracking_taps_.end());

  auto& tracker = iter->second;
  if (event.type == PointerEvent::EventType::kUpEvent) {
    HandleMultiTap(tracker);
  } else if (event.type == PointerEvent::EventType::kMoveEvent) {
    if (!tracker->IsWithinTolerance(
            event,
            gesture_manager_->ConvertFrom<kPixelTypeLogical>(kMultiTapSlop))) {
      Reject(tracker.get());
    }
  }
}

void MultiTapGestureRecognizer::HandleMultiTap(
    std::unique_ptr<TapTracker>& tracker) {
  GESTURE_LOG << "Handle Multi tap " << tracker->pointer().pointer_id
              << " position=(" << tracker->pointer().position.x() << ","
              << tracker->pointer().position.y() << ")";
  timer_ = std::make_unique<fml::OneshotTimer>(
      timer_runner() ? timer_runner() : gesture_manager_->GetTaskRunner());
  timer_->Start(fml::TimeDelta::FromMilliseconds(kMultiTapTimeout), [this]() {
    GESTURE_LOG << "MultiTap timeout!";
    Reset();
  });
  current_taps_++;
  if (last_tap_) {
    last_tap_->Resolve(GestureDisposition::kAccept);
  } else {
    first_tap_position_ = tracker->pointer().position;
  }
  if (on_multi_tap_) {
    on_multi_tap_(tracker->pointer(), current_taps_);
  }
  gesture_manager_->arena_manager()->Hold(tracker->pointer().pointer_id);
  tracker->StopTracking();
  last_tap_ = std::move(tracker);
  // ownership transfered to last_tap_
  tracking_taps_.clear();
}

void MultiTapGestureRecognizer::Reset() {
  timer_.reset();
  current_taps_ = 0;
  if (last_tap_) {
    auto holder = std::move(last_tap_);
    holder->Resolve(GestureDisposition::kReject);
    GESTURE_LOG << "Release arena when reset.";
    gesture_manager_->arena_manager()->Release(holder->pointer().pointer_id);
  }

  auto temp = std::move(tracking_taps_);
  for (auto& pair : temp) {
    Reject(pair.second.get());
  }
}

void MultiTapGestureRecognizer::Reject(TapTracker* tracker) {
  auto iter = tracking_taps_.find(tracker->pointer().pointer_id);
  if (iter == tracking_taps_.end()) {
    // Already rejected
    return;
  }
  GESTURE_LOG << "Reject tracker " << tracker->pointer().pointer_id;
  auto holder = std::move(iter->second);
  tracking_taps_.erase(iter);
  tracker->StopTracking();
  tracker->Resolve(GestureDisposition::kReject);
  Reset();
}

void MultiTapGestureRecognizer::OnGestureAccepted(int pointer_id) {}

void MultiTapGestureRecognizer::OnGestureRejected(int pointer_id) {
  auto iter = tracking_taps_.find(pointer_id);
  if (iter == tracking_taps_.end()) {
    if (last_tap_ && last_tap_->pointer().pointer_id == pointer_id) {
      Reset();
    }
  } else {
    Reject(iter->second.get());
  }
}

}  // namespace clay
