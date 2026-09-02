// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SLOT_DRAG_H_
#define CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SLOT_DRAG_H_

#include "clay/ui/component/base_view.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
namespace clay {

// using ScrollHeightCallback = std::function<void(const int scroll_height)>;

class ScrollCoordinatorSlotDrag
    : public WithTypeInfo<ScrollCoordinatorSlotDrag, BaseView> {
 public:
  ScrollCoordinatorSlotDrag(int id, PageView* page_view);
  ~ScrollCoordinatorSlotDrag() override;

  void SetAttribute(const char* attr, const clay::Value& value) override;

 private:
  bool head_over_slot_ = false;
  bool enable_drag_ = true;
  DragGestureRecognizer* drag_gesture_recognizer_ = nullptr;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SLOT_DRAG_H_
