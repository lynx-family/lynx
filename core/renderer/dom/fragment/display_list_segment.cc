// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_segment.h"

namespace lynx {
namespace tasm {
namespace {

bool IsDrawableOperation(DisplayListOpType type) {
  switch (type) {
    case DisplayListOpType::kBegin:
    case DisplayListOpType::kEnd:
    case DisplayListOpType::kDrawView:
    case DisplayListOpType::kClipRect:
    case DisplayListOpType::kRecordBox:
      return false;
    default:
      return true;
  }
}

}  // namespace

DisplayListSegmentResult SegmentDisplayList(const DisplayList& display_list) {
  DisplayListSegmentResult result;
  const size_t item_count = display_list.GetContentItemsSize();
  if (item_count == 0) {
    return result;
  }

  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  if (display_list.SubLayers().empty()) {
    bool has_drawable_content = false;
    bool has_draw_view = false;
    for (size_t i = 0; i < item_count; ++i) {
      if (items[i].type == DisplayListOpType::kDrawView) {
        has_draw_view = true;
        break;
      }
      has_drawable_content |= IsDrawableOperation(items[i].type);
    }
    if (!has_draw_view) {
      result.segments.emplace_back(
          DisplayListSegment{0, item_count, -1, 0.f, 0.f,
                             kInvalidDisplayListIndex, has_drawable_content});
      return result;
    }
  }

  size_t segment_start = 0;
  size_t segment_initial_state = kInvalidDisplayListIndex;
  int32_t preceding_view_id = -1;
  float preceding_view_offset_x = 0.f;
  float preceding_view_offset_y = 0.f;
  size_t active_state_index = kInvalidDisplayListIndex;
  bool segment_has_drawable_operation = false;
  base::Vector<size_t> scope_restore_states;

  auto add_state = [&result, &active_state_index](size_t item_index) {
    result.states.emplace_back(
        DisplayListReplayState{item_index, active_state_index});
    active_state_index = result.states.size() - 1;
  };
  auto add_segment = [&](size_t segment_end) {
    result.segments.emplace_back(DisplayListSegment{
        segment_start, segment_end, preceding_view_id, preceding_view_offset_x,
        preceding_view_offset_y, segment_initial_state,
        segment_has_drawable_operation});
  };

  for (size_t i = 0; i < item_count; ++i) {
    const auto& item = items[i];
    switch (item.type) {
      case DisplayListOpType::kBegin:
        scope_restore_states.emplace_back(active_state_index);
        add_state(i);
        break;
      case DisplayListOpType::kClipRect:
        if (!scope_restore_states.empty()) {
          add_state(i);
        }
        break;
      case DisplayListOpType::kEnd:
        if (!scope_restore_states.empty()) {
          active_state_index = scope_restore_states.back();
          scope_restore_states.pop_back();
        }
        break;
      case DisplayListOpType::kDrawView:
        add_segment(i);
        preceding_view_id = item.payload.draw_view.view_id;
        preceding_view_offset_x = item.payload.draw_view.offset_x;
        preceding_view_offset_y = item.payload.draw_view.offset_y;
        segment_start = i + 1;
        segment_initial_state = active_state_index;
        segment_has_drawable_operation = false;
        break;
      default:
        segment_has_drawable_operation |= IsDrawableOperation(item.type);
        break;
    }
  }

  add_segment(item_count);
  return result;
}

}  // namespace tasm
}  // namespace lynx
