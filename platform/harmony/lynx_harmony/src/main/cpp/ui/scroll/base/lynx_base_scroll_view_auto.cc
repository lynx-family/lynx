// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

namespace lynx {
namespace tasm {
namespace harmony {
void LynxBaseScrollView::GetScrollOffset(float scroll_offset[2]) {
  NodeManager::Instance().GetAttributeValues(
      node_, NODE_SCROLL_OFFSET, &scroll_offset[0], &scroll_offset[1]);
}

void LynxBaseScrollView::SetScrollContentSize(float content_size[2]) {
  if (vertical_) {
    SetScrollContentSizeVertically(content_size[1]);

    float width = 0;
    NodeManager::Instance().GetAttributeValues(node_, NODE_WIDTH, &width);
    NodeManager::Instance().SetAttributeWithNumberValue(scroll_content_,
                                                        NODE_WIDTH, width);
    NodeManager::Instance().SetAttributeWithNumberValue(
        scroll_content_, NODE_HEIGHT, content_size_[1]);
  } else {
    SetScrollContentSizeHorizontally(content_size[0]);

    float height = 0;
    NodeManager::Instance().GetAttributeValues(node_, NODE_HEIGHT, &height);
    NodeManager::Instance().SetAttributeWithNumberValue(scroll_content_,
                                                        NODE_HEIGHT, height);
    NodeManager::Instance().SetAttributeWithNumberValue(
        scroll_content_, NODE_WIDTH, content_size_[0]);
  }
}

void LynxBaseScrollView::ScrollByUnlimited(float delta[2]) {
  if (vertical_) {
    ScrollByUnlimitedVertically(delta[1]);
  } else {
    ScrollByUnlimitedHorizontally(delta[0]);
  }
}

void LynxBaseScrollView::ScrollBy(float delta[2]) {
  if (vertical_) {
    ScrollByVertically(delta[1]);
  } else {
    ScrollByHorizontally(delta[0]);
  }
}

void LynxBaseScrollView::ScrollToUnlimited(float offset[2]) {
  if (vertical_) {
    ScrollToUnlimitedVertically(offset[1]);
  } else {
    ScrollToUnlimitedHorizontally(offset[0]);
  }
}

void LynxBaseScrollView::ScrollTo(float offset[2]) {
  if (vertical_) {
    ScrollToVertically(offset[1]);
  } else {
    ScrollToHorizontally(offset[0]);
  }
}

void LynxBaseScrollView::AnimatedScrollTo(float offset[2]) {
  if (vertical_) {
    AnimatedScrollToVertically(offset[1]);
  } else {
    AnimatedScrollToHorizontally(offset[0]);
  }
}

void LynxBaseScrollView::AnimatedScrollToUnlimited(float offset[2]) {
  if (vertical_) {
    AnimatedScrollToUnlimitedVertically(offset[1]);
  } else {
    AnimatedScrollToUnlimitedHorizontally(offset[0]);
  }
}

void LynxBaseScrollView::GetScrollRange(float range[4]) {
  float scroll_range_horizontal[2]{0.f};
  GetScrollRangeHorizontally(scroll_range_horizontal);
  float scroll_range_vertical[2]{0.f};
  GetScrollRangeVertically(scroll_range_horizontal);

  range[0] = scroll_range_horizontal[0];
  range[1] = scroll_range_horizontal[1];
  range[2] = scroll_range_vertical[0];
  range[3] = scroll_range_vertical[1];
}

bool LynxBaseScrollView::CanScrollForwards() {
  if (vertical_) {
    return CanScrollForwardsVertically();
  } else {
    return CanScrollForwardsHorizontally();
  }
}

bool LynxBaseScrollView::CanScrollBackwards() {
  if (vertical_) {
    return CanScrollBackwardsVertically();
  } else {
    return CanScrollBackwardsHorizontally();
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
