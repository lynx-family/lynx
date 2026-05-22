// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_DRAG_GESTURE_RECOGNIZER_H_
#define CLAY_UI_GESTURE_DRAG_GESTURE_RECOGNIZER_H_

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_recognizer.h"
#include "clay/ui/gesture/velocity_tracker.h"

namespace clay {

// Happens whenever pointer down.
using DragDownCallback = std::function<void(const PointerEvent&)>;
// Happens after drag gesture is accepted.
using DragStartCallback = std::function<void(const FloatPoint&)>;
using DragUpdateCallback =
    std::function<void(const FloatPoint& position, const FloatSize& delta)>;
// Happens when pointer up with state == accepted.
// If velocity is not zero means fling gesture is detected.
using DragEndCallback = std::function<void(const Velocity&)>;
// Happens when pointer up with state != accepted.
using DragCancelCallback = std::function<void()>;

class GestureManager;

class DragGestureRecognizer : public OneSequenceGestureRecognizer {
 public:
  explicit DragGestureRecognizer(GestureManager* gesture_manager);

  const char* GetMemberTag() const override { return "[Drag]"; }

  void HandleEvent(const PointerEvent& pointer) override;

  GestureRecognizerType getType() const override {
    return GestureRecognizerType::kDragGesture;
  }

  void SetDragDownCallback(DragDownCallback&& on_drag_down) {
    on_drag_down_ = std::move(on_drag_down);
  }
  void SetDragStartCallback(DragStartCallback&& on_drag_start) {
    on_drag_start_ = std::move(on_drag_start);
  }
  void SetDragUpdateCallback(DragUpdateCallback&& on_drag_update) {
    on_drag_update_ = std::move(on_drag_update);
  }
  void SetDragEndCallback(DragEndCallback&& on_drag_end) {
    on_drag_end_ = std::move(on_drag_end);
  }
  void SetDragCancelCallback(DragCancelCallback&& on_drag_cancel) {
    on_drag_cancel_ = std::move(on_drag_cancel);
  }

  float GetTouchSlop();

  void SetTouchSlop(float slop) { touch_slop_ = slop; }

 protected:
  enum class DragState {
    kReady,
    kPossible,
    kAccepted,
  };

  virtual FloatSize GetDeltaForDetails(const FloatSize& delta) const;
  virtual bool HasSufficientDistanceToAccept(const FloatSize& movement);
  virtual bool IsFlingGesture(const VelocityEstimate& velocity);

  float GetMaxFlingVelocity() const;
  float GetMinFlingVelocity() const;

 private:
  using super = OneSequenceGestureRecognizer;

  void AddAllowedPointer(const PointerEvent& pointer) override;

  void DidStopTrackingLastPointer(int pointer_id) override;

  void OnGestureAccepted(int pointer_id) override;
  void OnGestureRejected(int pointer_id) override;

  void GiveupPointer(int pointer_id);
  void ResetState();

  DragDownCallback on_drag_down_;
  DragStartCallback on_drag_start_;
  DragUpdateCallback on_drag_update_;
  DragEndCallback on_drag_end_;
  DragCancelCallback on_drag_cancel_;

  DragState state_ = DragState::kReady;

  // For temporally store distance before accept to judge drag gesture.
  FloatSize pending_distance_;

  PointerEvent initial_pointer_;

  std::set<int> accepted_active_pointers_;

  // Movement permitted after touch and before accept. In arbitrary direction.
  std::optional<float> touch_slop_;

  // May have multiple pointers touch down one by one. That means the second
  // pointer may touch down before the first pointer touch up.
  std::map<int, VelocityTracker> velocity_trackers_;

  std::optional<fml::TimePoint> timestamp_offset_;
};

class HorizontalDragGestureRecognizer : public DragGestureRecognizer {
 public:
  explicit HorizontalDragGestureRecognizer(GestureManager* gesture_manager)
      : DragGestureRecognizer(gesture_manager) {}
  GestureRecognizerType getType() const override {
    return GestureRecognizerType::kHorizontalDrag;
  }
  FloatSize GetDeltaForDetails(const FloatSize& delta) const override;
  bool HasSufficientDistanceToAccept(const FloatSize& distance) override;
  bool IsFlingGesture(const VelocityEstimate& velocity) override;
};

class VerticalDragGestureRecognizer : public DragGestureRecognizer {
 public:
  explicit VerticalDragGestureRecognizer(GestureManager* gesture_manager)
      : DragGestureRecognizer(gesture_manager) {}
  GestureRecognizerType getType() const override {
    return GestureRecognizerType::kVerticalDrag;
  }
  FloatSize GetDeltaForDetails(const FloatSize& delta) const override;
  bool HasSufficientDistanceToAccept(const FloatSize& distance) override;
  bool IsFlingGesture(const VelocityEstimate& velocity) override;
};

std::unique_ptr<DragGestureRecognizer> CreateDragRecognizerByDirection(
    ScrollDirection direction, GestureManager* gesture_manager);

}  // namespace clay

#endif  // CLAY_UI_GESTURE_DRAG_GESTURE_RECOGNIZER_H_
