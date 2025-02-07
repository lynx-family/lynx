// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/grid_item_info.h"

#include <utility>

#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace starlight {

GridItemInfo::GridItemInfo(LayoutObject* item) : item_(item) {}

GridItemInfo::~GridItemInfo() = default;

void GridItemInfo::InitSpanInfo(Dimension dimension, int32_t explicit_end,
                                int32_t axis_offset,
                                bool if_absolutely_positioned) {
  const auto* style = Item()->GetCSSStyle();

  int32_t start = dimension == kHorizontal ? style->GetGridColumnStart()
                                           : style->GetGridRowStart();
  int32_t end = dimension == kHorizontal ? style->GetGridColumnEnd()
                                         : style->GetGridRowEnd();
  int32_t span = dimension == kHorizontal ? style->GetGridColumnSpan()
                                          : style->GetGridRowSpan();

  const bool is_start_definite = (start == kGridLineUnDefine) ? false : true;
  // If the start line is equal to the end line, remove the end line.
  const bool is_end_definite =
      (end == kGridLineUnDefine || start == end) ? false : true;

  // If a negative integer is given, it instead counts in reverse, starting from
  // the end edge of the explicit grid.
  start = start < 0 ? start + explicit_end + 1 : start;
  end = end < 0 ? end + explicit_end + 1 : end;

  // Move base line, make every axis be positive integer.
  start = is_start_definite ? start + axis_offset : start;
  end = is_end_definite ? end + axis_offset : end;

  // For grid item: If the placement for a grid item contains two lines, and the
  // start line is further end-ward than the end line, swap the two lines.
  if (!if_absolutely_positioned && start > end && is_start_definite &&
      is_end_definite) {
    std::swap(start, end);
  }

  // For grid item: A definite value for any two of Start, End, and Span in a
  // given dimension implies a definite value for the third.
  if (!if_absolutely_positioned) {
    end = (is_start_definite && !is_end_definite) ? start + span : end;
    start = (is_end_definite && !is_start_definite) ? end - span : start;
    span = (is_start_definite && is_end_definite) ? end - start : span;
  }

  SetSpanPosition(dimension, start, end);
  SetSpanSize(dimension, span);
}

void GridItemInfo::SetSpanPosition(Dimension dimension, int32_t start,
                                   int32_t end) {
  if (dimension == kHorizontal) {
    start_column_ = start;
    end_column_ = end;
  } else {
    start_row_ = start;
    end_row_ = end;
  }
}

void GridItemInfo::SetSpanSize(Dimension dimension, int32_t span) {
  if (dimension == kHorizontal) {
    column_span_size_ = span;
  } else {
    row_span_size_ = span;
  }
}

}  // namespace starlight
}  // namespace lynx
