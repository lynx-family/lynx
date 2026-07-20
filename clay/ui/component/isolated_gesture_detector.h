// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_ISOLATED_GESTURE_DETECTOR_H_
#define CLAY_UI_COMPONENT_ISOLATED_GESTURE_DETECTOR_H_

#include <cmath>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/gesture_recognizer.h"

namespace clay {

// Each isolatedGestureDetector will create its own gesture / arena manager,
// so that recognizers won't conflict with outer ones.
// This is helpful when needs to report lynx about which gesture matches
// no matter the gesture is accepted by some view or not.
class IsolatedGestureDetector : public HitTestable, public HitTestTarget {
 public:
  explicit IsolatedGestureDetector(fml::RefPtr<fml::TaskRunner> task_runner)
      : gesture_manager_(std::move(task_runner)) {}

  void DispatchPointerEvent(std::vector<PointerEvent>& events,
                            const HitTestResponsiveResult& result) {
    hit_test_responsive_result_ = result;
    gesture_manager_.HandlePointerEvents(this, events);
  }

  void TrackScrollTapSuppressionForPointerDown(const PointerEvent& event,
                                               const HitTestResult& result) {
    scroll_tap_suppression_states_.erase(event.pointer_id);
    if (event.device != PointerEvent::DeviceType::kTouch) {
      return;
    }

    ScrollableDirection scrollable_direction = ScrollableDirection::kNone;
    auto segment_start = result.begin();
    while (segment_start != result.end()) {
      while (segment_start != result.end() && !(*segment_start)) {
        ++segment_start;
      }
      if (segment_start == result.end()) {
        break;
      }

      const bool should_pass_event_to_native =
          (*segment_start)->ShouldPassEventToNative();
      auto segment_end = segment_start;
      while (segment_end != result.end() && *segment_end) {
        if (!should_pass_event_to_native) {
          scrollable_direction |= (*segment_end)->GetScrollableDirection();
        }
        ++segment_end;
      }
      segment_start = segment_end;
    }
    if (scrollable_direction != ScrollableDirection::kNone) {
      scroll_tap_suppression_states_.emplace(
          event.pointer_id,
          ScrollTapSuppressionState{event.position, scrollable_direction});
    }
  }

  void ClearScrollTapSuppressionForEndedEvents(
      const std::vector<PointerEvent>& events) {
    for (const auto& event : events) {
      if (event.type == PointerEvent::EventType::kUpEvent ||
          event.type == PointerEvent::EventType::kCancel ||
          event.type == PointerEvent::EventType::kPanZoomEndEvent) {
        scroll_tap_suppression_states_.erase(event.pointer_id);
      }
    }
  }

  void ClearScrollTapSuppressionStates() {
    scroll_tap_suppression_states_.clear();
  }

  void AddRecognizer(std::unique_ptr<GestureRecognizer>&& recognizer) {
    FML_DCHECK(recognizer->gesture_manager() == &gesture_manager_);
    recognizers_.emplace_back(std::move(recognizer));
  }

  GestureManager* gesture_manager() { return &gesture_manager_; }

  bool ShouldSuppressTapForScrollDrag(int pointer_id) {
    const auto state = scroll_tap_suppression_states_.find(pointer_id);
    if (state == scroll_tap_suppression_states_.end() ||
        !state->second.suppress_tap) {
      return false;
    }

    FML_LOG(INFO) << "[ClayScrollTap] action=suppress"
                  << " pointer_id=" << pointer_id
                  << " delta_x=" << state->second.suppression_delta_x
                  << " delta_y=" << state->second.suppression_delta_y
                  << " touch_slop="
                  << gesture_manager_.ConvertFrom<kPixelTypeLogical>(kTouchSlop)
                  << " scrollable_direction="
                  << static_cast<int>(state->second.scrollable_direction);
    return true;
  }

 private:
  struct ScrollTapSuppressionState {
    FloatPoint down_position;
    ScrollableDirection scrollable_direction;
    bool suppress_tap = false;
    float suppression_delta_x = 0;
    float suppression_delta_y = 0;
  };

  // Override HitTestable
  bool HitTest(const PointerEvent& event, HitTestResult& result) override {
    result.emplace_back(GetHitTestTargetWeakPtr());
    return true;
  }

  // Override HitTestTarget
  void HandleEvent(const PointerEvent& event) override {
    if (event.type == PointerEvent::EventType::kDownEvent) {
      for (auto& recognizer : recognizers_) {
        if (recognizer->getType() == GestureRecognizerType::kLongPress &&
            !hit_test_responsive_result_.has_longpress_event) {
          continue;
        }
        recognizer->AddPointer(event);
      }
      return;
    }

    if (event.type != PointerEvent::EventType::kMoveEvent &&
        event.type != PointerEvent::EventType::kUpEvent) {
      return;
    }
    const auto state = scroll_tap_suppression_states_.find(event.pointer_id);
    if (state == scroll_tap_suppression_states_.end() ||
        state->second.suppress_tap) {
      return;
    }

    const float delta_x = event.position.x() - state->second.down_position.x();
    const float delta_y = event.position.y() - state->second.down_position.y();
    const float touch_slop =
        gesture_manager_.ConvertFrom<kPixelTypeLogical>(kTouchSlop);
    const bool exceeded_horizontal_slop =
        (state->second.scrollable_direction &
         ScrollableDirection::kHorizontal) != ScrollableDirection::kNone &&
        std::abs(delta_x) > touch_slop;
    const bool exceeded_vertical_slop =
        (state->second.scrollable_direction & ScrollableDirection::kVertical) !=
            ScrollableDirection::kNone &&
        std::abs(delta_y) > touch_slop;
    if (exceeded_horizontal_slop || exceeded_vertical_slop) {
      state->second.suppress_tap = true;
      state->second.suppression_delta_x = delta_x;
      state->second.suppression_delta_y = delta_y;
    }
  }

  bool HasDragGestureRecognizer(ScrollDirection direction) const override {
    return false;
  }

  bool HasTapGestureRecognizer() const override { return false; }
  bool HasLongPressGestureRecognizer() const override { return false; }

  bool HasTapEvent() const override { return false; }
  bool HasLongPressEvent() const override { return false; }

  bool ShouldBlockNativeEvent() const override { return false; }

 private:
  GestureManager gesture_manager_;
  std::list<std::unique_ptr<GestureRecognizer>> recognizers_;
  std::unordered_map<int, ScrollTapSuppressionState>
      scroll_tap_suppression_states_;
  HitTestResponsiveResult hit_test_responsive_result_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_ISOLATED_GESTURE_DETECTOR_H_
