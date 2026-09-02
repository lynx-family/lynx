// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SLOT_H_
#define CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SLOT_H_

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"

namespace clay {

class NestedScrollable;

class ScrollCoordinatorSlot
    : public WithTypeInfo<ScrollCoordinatorSlot, BaseView> {
 public:
  ScrollCoordinatorSlot(int id, PageView* page_view);
  ~ScrollCoordinatorSlot() override;

  void SetBound(float left, float top, float width, float height) override;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SLOT_H_
