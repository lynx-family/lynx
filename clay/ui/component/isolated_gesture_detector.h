// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_ISOLATED_GESTURE_DETECTOR_H_
#define CLAY_UI_COMPONENT_ISOLATED_GESTURE_DETECTOR_H_

#include <list>
#include <memory>
#include <utility>
#include <vector>

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

  void AddRecognizer(std::unique_ptr<GestureRecognizer>&& recognizer) {
    FML_DCHECK(recognizer->gesture_manager() == &gesture_manager_);
    recognizers_.emplace_back(std::move(recognizer));
  }

  GestureManager* gesture_manager() { return &gesture_manager_; }

 private:
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
  HitTestResponsiveResult hit_test_responsive_result_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_ISOLATED_GESTURE_DETECTOR_H_
