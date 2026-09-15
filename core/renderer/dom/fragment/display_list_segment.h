// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_SEGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_SEGMENT_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

namespace lynx {
namespace tasm {

struct DisplayListSegment {
  size_t start_item_index{0};
  size_t end_item_index{0};
  int32_t preceding_view_id{-1};

  // Active saves/translations and clips at the beginning of this segment.
  base::Vector<size_t> state_item_indices;
  bool has_drawing{false};

  size_t ItemCount() const { return end_item_index - start_item_index; }
  bool IsEmpty() const { return start_item_index == end_item_index; }
};

// Splits drawing operations around kDrawView so each resulting segment can be
// rendered on the corresponding side of a native view. Replaces boxes with all
// recorded rectangles in their original index order during the same traversal.
base::Vector<DisplayListSegment> SegmentDisplayList(
    const DisplayList& display_list, std::vector<RoundedRectangle>& boxes);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_SEGMENT_H_
