// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_segment.h"

namespace lynx {
namespace tasm {

base::Vector<DisplayListSegment> SegmentDisplayList(
    const DisplayList& display_list) {
  base::Vector<DisplayListSegment> segments;
  const size_t item_count = display_list.GetContentItemsSize();
  if (item_count == 0) {
    return segments;
  }

  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  size_t segment_start = 0;
  int32_t preceding_view_id = -1;
  for (size_t i = 0; i < item_count; ++i) {
    if (items[i].type != DisplayListOpType::kDrawView) {
      continue;
    }
    segments.emplace_back(
        DisplayListSegment{segment_start, i, preceding_view_id});
    segment_start = i + 1;
    preceding_view_id = items[i].payload.draw_view.view_id;
  }
  segments.emplace_back(
      DisplayListSegment{segment_start, item_count, preceding_view_id});
  return segments;
}

}  // namespace tasm
}  // namespace lynx
