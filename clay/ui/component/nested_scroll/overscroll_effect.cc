// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/nested_scroll/overscroll_effect.h"

namespace clay {

void OffsetOverscrollEffect::OnOverscroll(FloatPoint new_offset,
                                          FloatPoint prev_offset) {
  if (new_offset.x() != prev_offset.x()) {
    bool left_side =
        new_offset.x() < 0 || (new_offset.x() == 0 && prev_offset.x() < 0);
    if (left_side) {
      render_scroll_->SetScrollLeft(new_offset.x());
    } else {
      render_scroll_->SetScrollLeft(render_scroll_->MaxScrollWidth() +
                                    new_offset.x());
    }
  }

  if (new_offset.y() != prev_offset.y()) {
    bool top_side =
        new_offset.y() < 0 || (new_offset.y() == 0 && prev_offset.y() < 0);
    if (top_side) {
      render_scroll_->SetScrollTop(new_offset.y());
    } else {
      render_scroll_->SetScrollTop(render_scroll_->MaxScrollHeight() +
                                   new_offset.y());
    }
  }
}

}  // namespace clay
