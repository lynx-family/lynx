// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_MULTI_TAP_GESTURE_RECOGNIZER_H_
#define CLAY_UI_GESTURE_MULTI_TAP_GESTURE_RECOGNIZER_H_

#include <map>
#include <memory>
#include <utility>

#include "clay/ui/gesture/gesture_recognizer.h"

namespace clay {

class GestureManager;

// Track tap gesture for one pointer for multi taps gestures.
class TapTracker {
 public:
  TapTracker(GestureManager* gesture_manager, const PointerEvent& event,
             std::unique_ptr<ArenaEntry> entry);
  ~TapTracker();
  void StartTracking(RouteId route_id, PointerRoute&& route);
  void StopTracking();
  bool IsWithinTolerance(const PointerEvent& other, float tolerance);
  void Resolve(GestureDisposition disposition);

  const PointerEvent& pointer() { return event_; }

 private:
  bool has_start_tracking_ = false;
  RouteId route_id_ = 0;
  GestureManager* gesture_manager_;
  PointerEvent event_;
  std::unique_ptr<ArenaEntry> arena_entry_;
};

class MultiTapGestureRecognizer : public GestureRecognizer {
 public:
  explicit MultiTapGestureRecognizer(GestureManager* gesture_manager)
      : GestureRecognizer(gesture_manager) {}

  // Set Multi Tap Callbacks, index start from 1
  void SetMultiTapCallback(OnMultiTapCallback&& callback) {
    on_multi_tap_ = std::move(callback);
  }
  const char* GetMemberTag() const override { return "[Multi-Tap]"; }

 protected:
  void AddAllowedPointer(const PointerEvent& pointer) override;

  void OnGestureAccepted(int pointer_id) override;

  void OnGestureRejected(int pointer_id) override;

  std::unique_ptr<fml::OneshotTimer> timer_;
  std::unique_ptr<TapTracker> last_tap_;

  std::map<int, std::unique_ptr<TapTracker>> tracking_taps_;
  // TODO(wangchen): need to distinguish Tap、TapDown、TapUp、TapCancel
  OnMultiTapCallback on_multi_tap_;
  int current_taps_ = 0;
  FloatPoint first_tap_position_;

  GestureRecognizerType getType() const override {
    return GestureRecognizerType::kDoubleTap;
  }

 private:
  void HandleEvent(const PointerEvent& event);
  void HandleMultiTap(std::unique_ptr<TapTracker>& tracker);
  void TrackTap(const PointerEvent& event);
  void Reset();
  void Reject(TapTracker* tracker);
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_MULTI_TAP_GESTURE_RECOGNIZER_H_
