// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_SEGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_SEGMENT_H_

#include <cstddef>
#include <cstdint>
#include <limits>

#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {

inline constexpr size_t kInvalidDisplayListIndex =
    std::numeric_limits<size_t>::max();

// A persistent state node shared by all segments that start in the same
// kBegin/kClipRect scope.
struct DisplayListReplayState {
  size_t item_index{kInvalidDisplayListIndex};
  size_t parent_state_index{kInvalidDisplayListIndex};
};

struct DisplayListSegment {
  size_t start_item_index{0};
  size_t end_item_index{0};
  int32_t preceding_view_id{-1};
  float preceding_view_offset_x{0.f};
  float preceding_view_offset_y{0.f};
  size_t initial_state_index{kInvalidDisplayListIndex};
  bool has_drawable_content{false};

  size_t ItemCount() const { return end_item_index - start_item_index; }
};

struct DisplayListSegmentResult {
  base::Vector<DisplayListSegment> segments;
  base::Vector<DisplayListReplayState> states;
};

// Splits drawing operations around kDrawView. Replay states are shared by all
// segments that start in the same kBegin/kClipRect scope.
DisplayListSegmentResult SegmentDisplayList(const DisplayList& display_list);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_SEGMENT_H_
