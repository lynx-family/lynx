// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_segment.h"

#include <utility>

namespace lynx {
namespace tasm {
namespace {
RoundedRectangle RecordBox(const DisplayListItem& item) {
  RoundedRectangle box;
  box.SetX(item.payload.record_box.x);
  box.SetY(item.payload.record_box.y);
  box.SetWidth(item.payload.record_box.w);
  box.SetHeight(item.payload.record_box.h);
  if (item.payload.record_box.has_radii) {
    box.SetRadiusXTopLeft(item.payload.record_box.radii[0]);
    box.SetRadiusYTopLeft(item.payload.record_box.radii[1]);
    box.SetRadiusXTopRight(item.payload.record_box.radii[2]);
    box.SetRadiusYTopRight(item.payload.record_box.radii[3]);
    box.SetRadiusXBottomRight(item.payload.record_box.radii[4]);
    box.SetRadiusYBottomRight(item.payload.record_box.radii[5]);
    box.SetRadiusXBottomLeft(item.payload.record_box.radii[6]);
    box.SetRadiusYBottomLeft(item.payload.record_box.radii[7]);
  }
  return box;
}

}  // namespace

base::Vector<DisplayListSegment> SegmentDisplayList(
    const DisplayList& display_list, std::vector<RoundedRectangle>& boxes) {
  boxes.clear();
  base::Vector<DisplayListSegment> segments;
  const size_t item_count = display_list.GetContentItemsSize();
  if (item_count == 0) {
    return segments;
  }

  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  DisplayListSegment segment;
  base::Vector<size_t> state;
  base::Vector<size_t> scopes;
  for (size_t i = 0; i < item_count; ++i) {
    switch (items[i].type) {
      case DisplayListOpType::kBegin:
        scopes.push_back(state.size());
        state.push_back(i);
        break;
      case DisplayListOpType::kEnd:
        if (!scopes.empty()) {
          state.resize<false>(scopes.back());
          scopes.pop_back();
        }
        break;
      case DisplayListOpType::kClipRect:
        state.push_back(i);
        break;
      case DisplayListOpType::kRecordBox:
        boxes.emplace_back(RecordBox(items[i]));
        break;
      case DisplayListOpType::kDrawView:
        segment.end_item_index = i;
        segments.push_back(std::move(segment));
        segment = DisplayListSegment{};
        segment.start_item_index = i + 1;
        segment.preceding_view_id = items[i].payload.draw_view.view_id;
        segment.state_item_indices = state;
        break;
      default:
        segment.has_drawing = true;
        break;
    }
  }
  segment.end_item_index = item_count;
  segments.push_back(std::move(segment));
  return segments;
}

}  // namespace tasm
}  // namespace lynx
