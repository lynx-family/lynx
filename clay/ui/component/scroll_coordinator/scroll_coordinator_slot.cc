// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_coordinator/scroll_coordinator_slot.h"

#include <memory>

#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

ScrollCoordinatorSlot::ScrollCoordinatorSlot(int id, PageView* page_view)
    : WithTypeInfo(id, "ScrollCoordinatorSlot",
                   std::make_unique<RenderContainer>(), page_view) {}

ScrollCoordinatorSlot::~ScrollCoordinatorSlot() = default;

void ScrollCoordinatorSlot::SetBound(float left, float top, float width,
                                     float height) {
  BaseView::SetBound(left, top, width, height);
  // Note: Sometimes Lynx updates the bounds of `ScrollCoordinatorSlot`
  // independently, which bypasses `ScrollCoordinatorScrollView::OnLayout`.
  // It causes incorrect layout of scroll-coordinator as a whole, so trigger
  // the parent's layout manually here.
  // Maybe there is somewhere more appropriately to trigger.
  MarkNeedsLayout(Parent());
}

}  // namespace clay
