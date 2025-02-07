// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_GRID_ITEM_INFO_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_GRID_ITEM_INFO_H_

#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/starlight/types/layout_directions.h"

namespace lynx {
namespace starlight {

constexpr int32_t kGridLineStart = 1;
constexpr int32_t kGridLineUnDefine = 0;

class LayoutObject;
class GridItemInfo {
 public:
  GridItemInfo(LayoutObject* item);
  ~GridItemInfo();

  LayoutObject* Item() const { return item_; }

  // parm: explicit_end is the end of explicit grid.
  // parm: axis_offset is the offset of base line, it make all grid line to
  // positive integer.
  void InitSpanInfo(Dimension dimension, int32_t explicit_end,
                    int32_t axis_offset, bool if_absolutely_positioned = false);
  void SetSpanPosition(Dimension dimension, int32_t start, int32_t end);
  void SetSpanSize(Dimension dimension, int32_t span);

  bool IsRowAxisUnDefine() const {
    return start_row_ == kGridLineUnDefine || end_row_ == kGridLineUnDefine;
  }
  bool IsColumnAxisUnDefine() const {
    return start_column_ == kGridLineUnDefine ||
           end_column_ == kGridLineUnDefine;
  }
  bool IsNoneAxisAuto() const {
    return !IsRowAxisUnDefine() && !IsColumnAxisUnDefine();
  }
  bool IsBothAxesAuto() const {
    return IsRowAxisUnDefine() && IsColumnAxisUnDefine();
  }
  bool IsAxisAuto(Dimension dimension) const {
    return dimension == kHorizontal ? IsColumnAxisUnDefine()
                                    : IsRowAxisUnDefine();
  }
  bool IsCrossFlexibleTrack(Dimension dimension) const {
    return dimension == kHorizontal ? cross_flexible_column_
                                    : cross_flexible_row_;
  }
  void SetIsCrossFlexibleTrack(Dimension dimension) {
    if (dimension == kHorizontal) {
      cross_flexible_column_ = true;
    } else {
      cross_flexible_row_ = true;
    }
  }

  int32_t SpanSize(Dimension dimension) const {
    return dimension == kHorizontal ? column_span_size_ : row_span_size_;
  }
  int32_t StartLine(Dimension dimension) const {
    return dimension == kHorizontal ? start_column_ : start_row_;
  }
  int32_t EndLine(Dimension dimension) const {
    return dimension == kHorizontal ? end_column_ : end_row_;
  }

  const Constraints& ContainingBlock() const { return containing_block_; }
  void SetContainingBlock(Dimension dimension,
                          const OneSideConstraint& one_side) {
    containing_block_[dimension] = one_side;
  }

 private:
  Constraints containing_block_;
  bool cross_flexible_column_ = false;
  bool cross_flexible_row_ = false;

  // item position
  int32_t start_row_ = kGridLineUnDefine;
  int32_t start_column_ = kGridLineUnDefine;
  int32_t end_row_ = kGridLineUnDefine;
  int32_t end_column_ = kGridLineUnDefine;

  // item span
  int32_t row_span_size_ = 0;
  int32_t column_span_size_ = 0;

  LayoutObject* item_;
};

// For item span size sort
struct ItemInfoEntry {
  GridItemInfo* item_info;
  float inline_axis_max_content_border_size_ = 0.f;
  float inline_axis_min_content_border_size_ = 0.f;
  float block_axis_max_content_border_size_ = 0.f;
  float block_axis_min_content_border_size_ = 0.f;

  int32_t SpanSize(Dimension dimension) const {
    return item_info->SpanSize(dimension);
  }
  // Intrinsic size contributions are based on the outer size of the box.
  float MaxContentContribution(Dimension dimension) const {
    return dimension == kHorizontal
               ? item_info->Item()->GetOuterWidthFromBorderBoxWidth(
                     inline_axis_max_content_border_size_)
               : item_info->Item()->GetOuterHeightFromBorderBoxHeight(
                     block_axis_max_content_border_size_);
  }
  float MinContentContribution(Dimension dimension) const {
    return dimension == kHorizontal
               ? item_info->Item()->GetOuterWidthFromBorderBoxWidth(
                     inline_axis_min_content_border_size_)
               : item_info->Item()->GetOuterHeightFromBorderBoxHeight(
                     block_axis_min_content_border_size_);
  }
  void SetMaxContentBorderSize(Dimension dimension, float size) {
    if (dimension == kHorizontal) {
      inline_axis_max_content_border_size_ = size;
    } else {
      block_axis_max_content_border_size_ = size;
    }
  }
  void SetMinContentBorderSize(Dimension dimension, float size) {
    if (dimension == kHorizontal) {
      inline_axis_min_content_border_size_ = size;
    } else {
      block_axis_min_content_border_size_ = size;
    }
  }
};
}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_GRID_ITEM_INFO_H_
