// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_coordinator/scroll_coordinator_slot_drag.h"

#include <memory>
#include <string>
#include <utility>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {
ScrollCoordinatorSlotDrag::ScrollCoordinatorSlotDrag(int id,
                                                     PageView* page_view)
    : WithTypeInfo(id, "ScrollCoordinatorSlotDrag",
                   std::make_unique<RenderContainer>(), page_view) {}

ScrollCoordinatorSlotDrag::~ScrollCoordinatorSlotDrag() = default;

void ScrollCoordinatorSlotDrag::SetAttribute(const char* attr_c,
                                             const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kHeaderOverSlot) {
    head_over_slot_ = attribute_utils::GetBool(value, false);
  } else if (kw == KeywordID::kEnableDrag) {
    auto enable_drag = attribute_utils::GetBool(value, true);
    if (enable_drag_ != enable_drag) {
      enable_drag_ = enable_drag;
      if (enable_drag_) {
        if (drag_gesture_recognizer_) {
          RemoveGestureRecognizer(drag_gesture_recognizer_);
          drag_gesture_recognizer_ = nullptr;
        }
      } else {
        std::unique_ptr<DragGestureRecognizer> drag_gesture_recognizer =
            std::make_unique<DragGestureRecognizer>(
                page_view_->gesture_manager());
        drag_gesture_recognizer_ = drag_gesture_recognizer.get();
        AddGestureRecognizer(std::move(drag_gesture_recognizer));
      }
    }
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

}  // namespace clay
