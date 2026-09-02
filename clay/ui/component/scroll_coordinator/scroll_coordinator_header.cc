// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_coordinator/scroll_coordinator_header.h"

#include <memory>

#include "clay/ui/component/base_view.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

ScrollCoordinatorHeader::ScrollCoordinatorHeader(int id, PageView* page_view)
    : WithTypeInfo(id, "ScrollCoordinatorHeader",
                   std::make_unique<RenderContainer>(), page_view) {}

ScrollCoordinatorHeader::~ScrollCoordinatorHeader() = default;

void ScrollCoordinatorHeader::OnContentSizeChanged(const FloatRect& old_rect,
                                                   const FloatRect& new_rect) {
  if (scroll_height_call_back_) {
    scroll_height_call_back_(render_object()->Height());
  }
}

}  // namespace clay
