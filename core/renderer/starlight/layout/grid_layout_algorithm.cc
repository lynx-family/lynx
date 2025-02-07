// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/grid_layout_algorithm.h"

#include <utility>

#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/position_layout_utils.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"

namespace lynx {
namespace starlight {
using namespace logic_direction_utils;  // NOLINT

GridLayoutAlgorithm::GridLayoutAlgorithm(LayoutObject* container)
    : LayoutAlgorithm(container) {}

void GridLayoutAlgorithm::InitializeAlgorithmEnv() {
  inline_gap_size_ = CalculateFloatSizeFromLength(GapStyle(InlineAxis()),
                                                  PercentBase(InlineAxis()));
  block_gap_size_ = CalculateFloatSizeFromLength(GapStyle(BlockAxis()),
                                                 PercentBase(BlockAxis()));

  const auto& auto_flow = container_style_->GetGridAutoFlow();
  is_dense_ = auto_flow == GridAutoFlowType::kDense ||
              auto_flow == GridAutoFlowType::kRowDense ||
              auto_flow == GridAutoFlowType::kColumnDense;
  if (auto_flow == GridAutoFlowType::kRow ||
      auto_flow == GridAutoFlowType::kRowDense ||
      auto_flow == GridAutoFlowType::kDense) {
    auto_placement_main_axis_ = InlineAxis();
    auto_placement_cross_axis_ = BlockAxis();
  } else {
    auto_placement_main_axis_ = BlockAxis();
    auto_placement_cross_axis_ = InlineAxis();
  }
}

void GridLayoutAlgorithm::Reset() {
  inline_gap_size_ = CalculateFloatSizeFromLength(GapStyle(InlineAxis()),
                                                  PercentBase(InlineAxis()));
  block_gap_size_ = CalculateFloatSizeFromLength(GapStyle(BlockAxis()),
                                                 PercentBase(BlockAxis()));
  inline_axis_start_ = 0;
  block_axis_start_ = 0;
  inline_axis_interval_ = 0;
  block_axis_interval_ = 0;

  grid_row_min_track_sizing_function_.clear();
  grid_row_max_track_sizing_function_.clear();
  grid_column_min_track_sizing_function_.clear();
  grid_column_max_track_sizing_function_.clear();
  grid_row_line_offset_from_container_padding_bound_.clear();
  grid_column_line_offset_from_container_padding_bound_.clear();
}

void GridLayoutAlgorithm::AlignInFlowItems() {
  for (const auto& item_info : grid_item_infos_) {
    LayoutObject* item = item_info.Item();
    const float inline_line_offset_from_content_bound =
        GridLineOffsetFromContainerPaddingBound(
            kHorizontal)[item_info.StartLine(kHorizontal)] -
        (HorizontalFront() == kRight ? container_->GetLayoutPaddingRight()
                                     : container_->GetLayoutPaddingLeft());
    const float block_line_offset_from_content_bound =
        GridLineOffsetFromContainerPaddingBound(
            kVertical)[item_info.StartLine(kVertical)] -
        container_->GetLayoutPaddingTop();

    const float offset_inline =
        inline_line_offset_from_content_bound + InlineAxisAlignment(item_info);
    const float offset_block =
        block_line_offset_from_content_bound + BlockAxisAlignment(item_info);

    SetBoundOffsetFrom(item, InlineFront(), BoundType::kMargin,
                       BoundType::kContent, offset_inline);
    SetBoundOffsetFrom(item, BlockFront(), BoundType::kMargin,
                       BoundType::kContent, offset_block);
  }
}

float GridLayoutAlgorithm::InlineAxisAlignment(const GridItemInfo& item_info) {
  const LayoutComputedStyle* item_style = item_info.Item()->GetCSSStyle();
  JustifyType justify_type = item_style->GetJustifySelfType();
  if (justify_type == JustifyType::kAuto) {
    justify_type = container_style_->GetJustifyItemsType();
  }

  const float available_space =
      item_info.ContainingBlock()[InlineAxis()].Size() -
      GetMarginBoundDimensionSize(item_info.Item(), InlineAxis());
  float item_offset_inline = 0.f;
  switch (justify_type) {
    case JustifyType::kAuto:
    case JustifyType::kStretch:
    case JustifyType::kStart:
      break;
    case JustifyType::kCenter: {
      item_offset_inline = available_space / 2;
      break;
    }
    case JustifyType::kEnd: {
      item_offset_inline = available_space;
      break;
    }
  }

  return item_offset_inline;
}

float GridLayoutAlgorithm::BlockAxisAlignment(const GridItemInfo& item_info) {
  const LayoutComputedStyle* item_style = item_info.Item()->GetCSSStyle();
  FlexAlignType align_type = item_style->GetAlignSelf();
  if (align_type == FlexAlignType::kAuto) {
    align_type = container_style_->GetAlignItems();
  }

  const float available_space =
      item_info.ContainingBlock()[BlockAxis()].Size() -
      GetMarginBoundDimensionSize(item_info.Item(), BlockAxis());
  float item_offset_block = 0.f;
  switch (align_type) {
    case FlexAlignType::kFlexStart:
    case FlexAlignType::kStart:
    case FlexAlignType::kStretch:
    case FlexAlignType::kAuto:
    case FlexAlignType::kBaseline:
      // do nothing
      break;
    case FlexAlignType::kCenter: {
      item_offset_block = available_space / 2;
      break;
    }
    case FlexAlignType::kEnd:
    case FlexAlignType::kFlexEnd: {
      item_offset_block = available_space;
      break;
    }
  }

  return item_offset_block;
}

// Special Handling for Absolute and Fixed in Grid
void GridLayoutAlgorithm::MeasureAbsoluteAndFixed() {
  for (GridItemInfo& item_info : grid_absolutely_positioned_item_infos_) {
    LayoutObject* const item = item_info.Item();
    Constraints containing_block;
    containing_block[InlineAxis()] = OneSideConstraint::Definite(
        CalcContainingBlock(InlineAxis(), item_info.StartLine(InlineAxis()),
                            item_info.EndLine(InlineAxis())));
    containing_block[BlockAxis()] = OneSideConstraint::Definite(
        CalcContainingBlock(BlockAxis(), item_info.StartLine(BlockAxis()),
                            item_info.EndLine(BlockAxis())));
    item->GetBoxInfo()->ResolveBoxInfoForAbsoluteAndFixed(
        containing_block, *item, item->GetLayoutConfigs());
    item_info.SetContainingBlock(InlineAxis(), containing_block[InlineAxis()]);
    item_info.SetContainingBlock(BlockAxis(), containing_block[BlockAxis()]);
    const Constraints& item_size_mode =
        position_utils::GetAbsoluteOrFixedItemSizeAndMode(item, container_,
                                                          containing_block);
    item->UpdateMeasure(item_size_mode, true);
  }
}

// Special Handling for Absolute and Fixed in Grid
void GridLayoutAlgorithm::AlignAbsoluteAndFixedItems() {
  for (const GridItemInfo& item_info : grid_absolutely_positioned_item_infos_) {
    LayoutObject* const item = item_info.Item();
    // If a grid-placement property refers to a non-existent line either by
    // explicitly specifying such a line or by spanning outside of the existing
    // implicit grid, it is instead treated as specifying auto (instead of
    // creating new implicit grid lines).
    float offset_inline =
        (item_info.StartLine(InlineAxis()) >
         static_cast<int32_t>(
             GridLineOffsetFromContainerPaddingBound(InlineAxis()).size()) -
             2)
            ? 0.f
            : GridLineOffsetFromContainerPaddingBound(
                  InlineAxis())[item_info.StartLine(InlineAxis())];
    float offset_block =
        (item_info.StartLine(BlockAxis()) >
         static_cast<int32_t>(
             GridLineOffsetFromContainerPaddingBound(BlockAxis()).size()) -
             2)
            ? 0.f
            : GridLineOffsetFromContainerPaddingBound(
                  BlockAxis())[item_info.StartLine(BlockAxis())];

    const float inline_padding_size =
        logic_direction_utils::GetPaddingBoundDimensionSize(container_,
                                                            kHorizontal);
    const float block_padding_size =
        logic_direction_utils::GetPaddingBoundDimensionSize(container_,
                                                            kVertical);
    const LayoutComputedStyle* item_style = item->GetCSSStyle();
    const auto left_offset = NLengthToLayoutUnit(
        item_style->GetLeft(),
        item_info.ContainingBlock()[kHorizontal].ToPercentBase());
    const auto right_offset = NLengthToLayoutUnit(
        item_style->GetRight(),
        item_info.ContainingBlock()[kHorizontal].ToPercentBase());
    const auto top_offset = NLengthToLayoutUnit(
        item_style->GetTop(),
        item_info.ContainingBlock()[kVertical].ToPercentBase());
    const auto bottom_offset = NLengthToLayoutUnit(
        item_style->GetBottom(),
        item_info.ContainingBlock()[kVertical].ToPercentBase());

    // Handle the logic of grid absolute items concerning rtl in advance.
    if (HorizontalFront() == kRight) {
      offset_inline = inline_padding_size - offset_inline;
      if (left_offset.IsIndefinite() && right_offset.IsIndefinite()) {
        offset_inline -= item->GetMarginBoundWidth();
      } else {
        offset_inline -= item_info.ContainingBlock()[kHorizontal].Size();
      }
    }

    // Handle left/right additionally.
    if (left_offset.IsIndefinite()) {
      if (right_offset.IsIndefinite()) {
        // if not setting left/right, consider justify-items/self.
        offset_inline +=
            (HorizontalFront() == kRight ? -InlineAxisAlignment(item_info)
                                         : InlineAxisAlignment(item_info));
      } else {
        offset_inline = inline_padding_size - offset_inline -
                        item_info.ContainingBlock()[kHorizontal].Size();
      }
    }

    // Handle top/bottom additionally.
    if (top_offset.IsIndefinite()) {
      if (bottom_offset.IsIndefinite()) {
        // if not setting top/bottom, consider align-items/self.
        offset_block += BlockAxisAlignment(item_info);
      } else {
        offset_block = block_padding_size - offset_block -
                       item_info.ContainingBlock()[kVertical].Size();
      }
    }

    position_utils::CalcStartOffset(
        item, BoundType::kPadding,
        BoxPositions{Position::kStart, Position::kStart},
        item_info.ContainingBlock(), kHorizontal, kLeft, offset_inline);

    position_utils::CalcStartOffset(
        item, BoundType::kPadding,
        BoxPositions{Position::kStart, Position::kStart},
        item_info.ContainingBlock(), kVertical, kTop, offset_block);
  }
}

void GridLayoutAlgorithm::SizeDeterminationByAlgorithm() {
  if (!has_placement_) {
    // Layout implicit axis
    PlaceGridItems();
    has_placement_ = true;
  }

  // grid item sizing
  GridItemSizing();

  // layout item
  MeasureGridItems();
}

void GridLayoutAlgorithm::PlaceGridItems() {
  PlaceItemCache place_items_cache;
  place_items_cache.reserve(inflow_items_.size());
  // 0. Generate anonymous grid items.
  // 1. Position anything that's not auto-positioned.
  PrePlaceGridItems(place_items_cache);
  // 2. Process the items locked a given row when
  // grid-auto-flow:row/dense/row dense, or else process the items locked to a
  // given column.
  PlaceGridItemsLockedToAutoPlacementCrossAxis(place_items_cache);
  // 3. Determine columns in the implicit grid When grid-auto-flow:row/dense/row
  // dense (or else rows). Already Done in the previous steps!!
  // 4. Position the remaining grid items.
  PlacementCursor cursor;
  for (GridItemInfo& item_info : grid_item_infos_) {
    if (item_info.IsBothAxesAuto()) {
      PlaceGridItemsWithBothAxesAuto(item_info, cursor, place_items_cache);
    } else if (item_info.IsAxisAuto(auto_placement_cross_axis_)) {
      PlaceGridItemsLockedToAutoPlacementMainAxis(item_info, cursor,
                                                  place_items_cache);
    }
  }
  // Placement of grid items has finished. Determine rows in the implicit grid
  // when grid-auto-flow:row/dense/row dense (or else columns) has been done.
}

void GridLayoutAlgorithm::PrePlaceGridItems(PlaceItemCache& place_item) {
  grid_item_infos_.reserve(inflow_items_.size());
  grid_absolutely_positioned_item_infos_.reserve(
      absolute_or_fixed_items_.size());

  // track end line = track_size + 1.
  const int32_t explicit_column_end =
      static_cast<int32_t>(
          container_style_->GetGridTemplateColumnsMinTrackingFunction()
              .size()) +
      1;
  const int32_t explicit_row_end =
      static_cast<int32_t>(
          container_style_->GetGridTemplateRowsMinTrackingFunction().size()) +
      1;

  int32_t min_row_axis = kGridLineStart;
  int32_t min_column_axis = kGridLineStart;

  const auto& ResolveMinAxis = [](Dimension dimension,
                                  const LayoutComputedStyle* style,
                                  int32_t explicit_end, int32_t& min_axis) {
    int32_t start = dimension == kHorizontal ? style->GetGridColumnStart()
                                             : style->GetGridRowStart();
    int32_t end = dimension == kHorizontal ? style->GetGridColumnEnd()
                                           : style->GetGridRowEnd();
    const int32_t span = dimension == kHorizontal ? style->GetGridColumnSpan()
                                                  : style->GetGridRowSpan();
    // If the start line is equal to the end line, remove the end line.
    if (start == end) {
      end = kGridLineUnDefine;
    }

    // If a negative integer is given, it instead counts in reverse,
    // starting from the end edge of the explicit grid.
    if (start < 0) {
      start += explicit_end + 1;
      min_axis = std::min(min_axis, start);
    }
    if (end < 0) {
      end += explicit_end + 1;
      min_axis = std::min(min_axis, end - span);
    }
    if (start == kGridLineUnDefine && end > 0) {
      min_axis = std::min(min_axis, end - span);
    }
  };

  // base line.
  for (const auto& inflow_item : inflow_items_) {
    const auto* child_style = inflow_item->GetCSSStyle();
    ResolveMinAxis(kVertical, child_style, explicit_row_end, min_row_axis);
    ResolveMinAxis(kHorizontal, child_style, explicit_column_end,
                   min_column_axis);
  }

  // Move base line.Make the axis start by 1.
  row_offset_ = kGridLineStart - min_row_axis;
  column_offset_ = kGridLineStart - min_column_axis;

  for (const auto& inflow_item : inflow_items_) {
    GridItemInfo item_info(inflow_item);

    item_info.InitSpanInfo(kVertical, explicit_row_end, row_offset_);
    item_info.InitSpanInfo(kHorizontal, explicit_column_end, column_offset_);

    grid_item_infos_.emplace_back(item_info);
  }

  for (const auto& absolute_or_fixed_item : absolute_or_fixed_items_) {
    GridItemInfo item_info(absolute_or_fixed_item);

    item_info.InitSpanInfo(kVertical, explicit_row_end, row_offset_, true);
    item_info.InitSpanInfo(kHorizontal, explicit_column_end, column_offset_,
                           true);

    grid_absolutely_positioned_item_infos_.emplace_back(item_info);
  }

  inline_track_count_ = static_cast<int32_t>(
      ExplicitTrackMinTrackSizingFunction(InlineAxis()).size());
  block_track_count_ = static_cast<int32_t>(
      ExplicitTrackMinTrackSizingFunction(BlockAxis()).size());
  for (auto& item_info : grid_item_infos_) {
    inline_track_count_ =
        std::max(inline_track_count_, item_info.EndLine(InlineAxis()) - 1);
    inline_track_count_ =
        std::max(inline_track_count_, item_info.SpanSize(InlineAxis()));
    block_track_count_ =
        std::max(block_track_count_, item_info.EndLine(BlockAxis()) - 1);
    block_track_count_ =
        std::max(block_track_count_, item_info.SpanSize(BlockAxis()));

    if (item_info.IsNoneAxisAuto()) {
      place_item.emplace_back(&item_info);
    }
  }
}

int32_t GridLayoutAlgorithm::FindNextAvailablePosition(
    Dimension locked_dimension, int32_t locked_start, int32_t locked_span,
    int32_t not_locked_initial_start, int32_t not_locked_span,
    int32_t not_locked_max_size, const PlaceItemCache& place_item) {
  Dimension not_locked_dimension =
      locked_dimension == kHorizontal ? kVertical : kHorizontal;
  std::vector<int> line_mark(not_locked_max_size + 1, 0);
  // If item intersects the expected value matrix().
  // Record the start/end position of the array at the corresponding position.
  // By the array, we can know which positions are available.
  for (const auto item_cache : place_item) {
    const auto& item_info = *item_cache;
    if (!item_info.IsNoneAxisAuto()) {
      continue;
    }

    if (item_info.StartLine(locked_dimension) >= locked_start + locked_span ||
        item_info.EndLine(locked_dimension) <= locked_start) {
      continue;
    }

    if (item_info.EndLine(not_locked_dimension) <= not_locked_initial_start) {
      continue;
    }

    line_mark[item_info.StartLine(not_locked_dimension)] += 1;
    line_mark[item_info.EndLine(not_locked_dimension)] -= 1;
  }

  int current_item_count = 0;
  for (int i = 1; i <= not_locked_initial_start; ++i) {
    current_item_count += line_mark[i];
  }
  int current_available_size = 0;
  for (int i = not_locked_initial_start + 1; i <= not_locked_max_size; ++i) {
    // No other item.
    if (!current_item_count) {
      ++current_available_size;
      if (current_available_size == not_locked_span) {
        return i - current_available_size;
      }
    } else {
      current_available_size = 0;
    }

    current_item_count += line_mark[i];
  }

  return kGridLineUnDefine;
}

void GridLayoutAlgorithm::PlaceGridItemsLockedToAutoPlacementCrossAxis(
    PlaceItemCache& place_item) {
  // Using in sparse (not dense) mode, records the end line of the latest item
  // that placed in this step/function in each row (column instead when
  // grid-auto-flow:column), ensuring the start line is past any grid
  // items previously placed in this row by this step/function.
  std::vector<int32_t> place_cache(
      GridTrackCount(auto_placement_cross_axis_) + 1, kGridLineStart);

  for (GridItemInfo& item_info : grid_item_infos_) {
    // Only process grid items with a definite row (in the direction of
    // auto_placement_cross_axis_, and column instead when
    // grid-auto-flow:column) position (that is, the grid-row-start and
    // grid-row-end properties define a definite grid position).
    if (!item_info.IsAxisAuto(auto_placement_main_axis_) ||
        item_info.IsBothAxesAuto()) {
      continue;
    }

    int32_t start_line = kGridLineStart;
    if (!IsDense() &&
        place_cache[item_info.StartLine(auto_placement_cross_axis_)] !=
            kGridLineStart) {
      start_line = place_cache[item_info.StartLine(auto_placement_cross_axis_)];
    }

    const int32_t span = item_info.SpanSize(auto_placement_main_axis_);
    start_line = FindNextAvailablePosition(
        auto_placement_cross_axis_,
        item_info.StartLine(auto_placement_cross_axis_),
        item_info.SpanSize(auto_placement_cross_axis_), start_line, span,
        GridTrackCount(auto_placement_main_axis_) + 1 + span, place_item);
    if (!IsDense()) {
      place_cache[item_info.StartLine(auto_placement_cross_axis_)] =
          start_line + span;
    }
    item_info.SetSpanPosition(auto_placement_main_axis_, start_line,
                              start_line + span);
    // Need to check it for every item_info.
    UpdateGridTrackCountIfNeeded(auto_placement_main_axis_,
                                 start_line + span - 1);
    place_item.emplace_back(&item_info);
  }
}

void GridLayoutAlgorithm::PlaceGridItemsLockedToAutoPlacementMainAxis(
    GridItemInfo& item_info, PlacementCursor& cursor,
    PlaceItemCache& place_item) {
  const int32_t previous_cursor_main_line = cursor.main_line;
  // Set the auto placement main axis's position (column position when
  // grid-auto-flow:row) of the cursor to the grid item’s column-start
  // line.
  cursor.main_line = item_info.StartLine(auto_placement_main_axis_);
  if (IsDense()) {
    // In dense mode, set the auto placement cross axis line (row when
    // grid-auto-flow:column) position of the cursor to the start-most row line
    // in the implicit grid.
    cursor.cross_line = kGridLineStart;
  } else if (item_info.StartLine(auto_placement_main_axis_) <
             previous_cursor_main_line) {
    // If this is less than the previous auto placement main axis's
    // position (column position when grid-auto-flow:row) of the cursor,
    // increment the row (when grid-auto-flow:row) position by 1.
    ++cursor.cross_line;
  }

  const int32_t cross_axis_span =
      item_info.SpanSize(auto_placement_cross_axis_);
  cursor.cross_line = FindNextAvailablePosition(
      auto_placement_main_axis_, cursor.main_line,
      item_info.SpanSize(auto_placement_main_axis_), cursor.cross_line,
      cross_axis_span,
      GridTrackCount(auto_placement_cross_axis_) + cross_axis_span + 1,
      place_item);

  item_info.SetSpanPosition(auto_placement_cross_axis_, cursor.cross_line,
                            cursor.cross_line + cross_axis_span);
  // Creating new line in auto placement cross axis (row when
  // grid-auto-flow:row/row dense) in the implicit grid as necessary
  UpdateGridTrackCountIfNeeded(auto_placement_cross_axis_,
                               cursor.cross_line + cross_axis_span - 1);
  place_item.emplace_back(&item_info);
}

void GridLayoutAlgorithm::PlaceGridItemsWithBothAxesAuto(
    GridItemInfo& item_info, PlacementCursor& cursor,
    PlaceItemCache& place_item) {
  // In dense mode, set the cursor’s row and column positions to start-most row
  // and column lines in the implicit grid.
  if (IsDense()) {
    cursor.main_line = kGridLineStart;
    cursor.cross_line = kGridLineStart;
  }
  const int32_t main_axis_track_count =
      GridTrackCount(auto_placement_main_axis_);
  const int32_t main_axis_span = item_info.SpanSize(auto_placement_main_axis_);
  const int32_t cross_axis_span =
      item_info.SpanSize(auto_placement_cross_axis_);

  while ((cursor.main_line = FindNextAvailablePosition(
              auto_placement_cross_axis_, cursor.cross_line, cross_axis_span,
              cursor.main_line, main_axis_span, main_axis_track_count + 1,
              place_item)) == kGridLineUnDefine) {
    // If not find available position in this
    // auto placement cross axis line (i.e., row when grid-auto-flow:row),
    // increment the auto-placement cursor’s row position (creating new rows in
    // the implicit grid as necessary), reset its column position to the
    // start-most column line in the implicit grid, and return to the previous
    // step.
    ++cursor.cross_line;
    cursor.main_line = kGridLineStart;
  }

  // If a non-overlapping position was found in the previous step, set the
  // item’s row-start and column-start lines to the cursor’s position
  item_info.SetSpanPosition(auto_placement_main_axis_, cursor.main_line,
                            cursor.main_line + main_axis_span);
  item_info.SetSpanPosition(auto_placement_cross_axis_, cursor.cross_line,
                            cursor.cross_line + cross_axis_span);
  UpdateGridTrackCountIfNeeded(auto_placement_cross_axis_,
                               cursor.cross_line + cross_axis_span - 1);
  place_item.emplace_back(&item_info);
}

void GridLayoutAlgorithm::GridItemSizing() {
  std::vector<float> inline_axis_base_size;
  std::vector<float> block_axis_base_size;
  std::vector<LayoutUnit> inline_axis_grow_limit;
  std::vector<LayoutUnit> block_axis_grow_limit;
  InitTrackSize(InlineAxis(), inline_axis_base_size, inline_axis_grow_limit);
  InitTrackSize(BlockAxis(), block_axis_base_size, block_axis_grow_limit);
  MeasureItemCache size_infos;
  const auto& ResolveTrackGridSize = [this, &size_infos](
                                         Dimension dimension,
                                         std::vector<float>& base_size,
                                         std::vector<LayoutUnit>& grow_limit) {
    ResolveIntrinsicTrackSizes(dimension, size_infos, base_size, grow_limit);
    MaximizeTracks(dimension, base_size, grow_limit);
    // Additionally, determine the container size respectively and resolve the
    // properties for justify-content (inline axis) and align-content (block
    // axis).
    ExpandFlexibleTracksAndStretchAutoTracks(dimension, size_infos, base_size);

    for (auto& item_info : grid_item_infos_) {
      // A grid item's grid area forms the containing block into which it is
      // laid out.
      const float containing_block_size =
          CalcContainingBlock(dimension, item_info.StartLine(dimension),
                              item_info.EndLine(dimension));
      item_info.SetContainingBlock(
          dimension, OneSideConstraint::Definite(containing_block_size));
      // 1. Resolve percentage margin
      // 2. Resolve box data.
      LayoutObject* const child = item_info.Item();
      child->GetBoxInfo()->UpdateBoxData(item_info.ContainingBlock(), *child,
                                         child->GetLayoutConfigs());
    }
  };

  CalcInlineAxisSizeContributions(size_infos);
  ResolveTrackGridSize(InlineAxis(), inline_axis_base_size,
                       inline_axis_grow_limit);
  CalcBlockAxisSizeContributions(size_infos);
  ResolveTrackGridSize(BlockAxis(), block_axis_base_size,
                       block_axis_grow_limit);
}

// Using both explicit and implicit track sizing properties to form the track
// sizing function for grid tracks. Subsequently, initialize the track sizes.
void GridLayoutAlgorithm::InitTrackSize(Dimension dimension,
                                        std::vector<float>& base_size,
                                        std::vector<LayoutUnit>& grow_limit) {
  const auto& explicit_track_min_track_sizing_function =
      ExplicitTrackMinTrackSizingFunction(dimension);
  const auto& explicit_track_max_track_sizing_function =
      ExplicitTrackMaxTrackSizingFunction(dimension);
  const auto& implicit_track_min_track_sizing_function =
      ImplicitTrackMinTrackSizingFunction(dimension);
  const auto& implicit_track_max_track_sizing_function =
      ImplicitTrackMaxTrackSizingFunction(dimension);
  auto& min_track_sizing_function = MinTrackSizingFunction(dimension);
  auto& max_track_sizing_function = MaxTrackSizingFunction(dimension);

  const size_t implicit_track_sizing_properties_size =
      implicit_track_min_track_sizing_function.size();
  const int32_t axis_offset =
      (dimension == kHorizontal) ? column_offset_ : row_offset_;
  // make sure (axis_offset % implicit_track_sizing_properties_size ==
  // implicit_track_sizing_properties_size - 1)
  const int32_t fill_size =
      implicit_track_sizing_properties_size != 0
          ? static_cast<int32_t>(implicit_track_sizing_properties_size) - 1 -
                (axis_offset % implicit_track_sizing_properties_size)
          : 0;
  // apply implicit track sizing properties to grid tracks crossing
  // negative axis.
  for (int32_t idx = kGridLineStart; idx <= axis_offset; ++idx) {
    if (implicit_track_sizing_properties_size != 0) {
      const size_t implicit_track_idx =
          (idx + fill_size) % implicit_track_sizing_properties_size;
      min_track_sizing_function.emplace_back(
          implicit_track_min_track_sizing_function[implicit_track_idx]);
      max_track_sizing_function.emplace_back(
          implicit_track_max_track_sizing_function[implicit_track_idx]);
    } else {
      min_track_sizing_function.emplace_back(NLength::MakeAutoNLength());
      max_track_sizing_function.emplace_back(NLength::MakeAutoNLength());
    }
  }

  // apply explicit track sizing properties to grid tracks.
  for (const auto& track_sizing_function :
       explicit_track_min_track_sizing_function) {
    min_track_sizing_function.emplace_back(track_sizing_function);
  }
  for (const auto& track_sizing_function :
       explicit_track_max_track_sizing_function) {
    max_track_sizing_function.emplace_back(track_sizing_function);
  }

  // apply implicit track sizing properties to grid tracks crossing
  // positive axis.
  const size_t explicit_track_sizing_properties_size =
      explicit_track_min_track_sizing_function.size();
  const size_t last_track_count =
      explicit_track_sizing_properties_size + axis_offset;
  const size_t axis_count = GridTrackCount(dimension);
  for (size_t idx = last_track_count; idx < axis_count; ++idx) {
    if (implicit_track_sizing_properties_size != 0) {
      const size_t implicit_track_idx =
          (idx - last_track_count) % implicit_track_sizing_properties_size;
      min_track_sizing_function.emplace_back(
          implicit_track_min_track_sizing_function[implicit_track_idx]);
      max_track_sizing_function.emplace_back(
          implicit_track_max_track_sizing_function[implicit_track_idx]);
    } else {
      min_track_sizing_function.emplace_back(NLength::MakeAutoNLength());
      max_track_sizing_function.emplace_back(NLength::MakeAutoNLength());
    }
  }

  // Initialize each track's base size and growth limit.
  const size_t tracks_size = GridTrackCount(dimension);
  base_size.resize(tracks_size);
  grow_limit.resize(tracks_size);
  for (size_t idx = 0; idx < tracks_size; ++idx) {
    switch (min_track_sizing_function[idx].GetType()) {
      case NLengthType::kNLengthUnit:
      case NLengthType::kNLengthPercentage:
      case NLengthType::kNLengthCalc: {
        const auto resolved_unit = NLengthToLayoutUnit(
            min_track_sizing_function[idx], PercentBase(dimension));
        base_size[idx] =
            resolved_unit.IsDefinite() ? resolved_unit.ToFloat() : 0.f;
        break;
      }
      case NLengthType::kNLengthAuto:
      case NLengthType::kNLengthMaxContent:
      case NLengthType::kNLengthFitContent:
      case NLengthType::kNLengthFr:
        base_size[idx] = 0.f;
        break;
      default:
        break;
    }

    switch (max_track_sizing_function[idx].GetType()) {
      case NLengthType::kNLengthUnit:
      case NLengthType::kNLengthPercentage:
      case NLengthType::kNLengthCalc:
        grow_limit[idx] = NLengthToLayoutUnit(max_track_sizing_function[idx],
                                              PercentBase(dimension));
        // In all cases, if the growth limit is less than the base size,
        // increase the growth limit to match the base size.
        if (grow_limit[idx].IsDefinite() &&
            base::FloatsLarger(base_size[idx], grow_limit[idx].ToFloat())) {
          grow_limit[idx] = LayoutUnit(base_size[idx]);
        }
        break;
      case NLengthType::kNLengthAuto:
      case NLengthType::kNLengthMaxContent:
      case NLengthType::kNLengthFitContent:
      case NLengthType::kNLengthFr: {
        grow_limit[idx] = LayoutUnit::Indefinite();
        break;
      }
      default:
        break;
    }
  }
}

void GridLayoutAlgorithm::CalcInlineAxisSizeContributions(
    MeasureItemCache& item_size_infos) {
  const auto& PreCalcTrackSize = [this](GridItemInfo& item_info, Dimension dis,
                                        Constraints& constraints) {
    const size_t start = item_info.StartLine(dis);
    const size_t end = item_info.EndLine(dis);

    LayoutUnit size = LayoutUnit(0);
    bool only_cross_fixed_tracks = true;
    for (size_t idx = start; idx < end; ++idx) {
      // If calculating the layout of a grid item in this step depends on the
      // available space in the block axis, assume the available space that it
      // would have if any row with a definite max track sizing function had
      // that size and all other rows were infinite.
      if (only_cross_fixed_tracks &&
          MaxTrackSizingFunction(dis)[idx - 1].IsUnitOrResolvableValue()) {
        size = size +
               NLengthToLayoutUnit(MaxTrackSizingFunction(dis)[idx - 1],
                                   PercentBase(dis)) +
               (idx == start ? 0 : GridGapSize(dis));
      } else {
        only_cross_fixed_tracks = false;
        // traverse all tracks the item crossing to make sure whether crossing
        // flexible track.
        if (MaxTrackSizingFunction(dis)[idx - 1].IsFr()) {
          item_info.SetIsCrossFlexibleTrack(dis);
        }
      }
    }

    if (start != end && size.IsDefinite() && only_cross_fixed_tracks) {
      constraints[dis] = OneSideConstraint::Definite(size.ToFloat());
    }
  };

  // measure for size contributions
  for (auto& item_info : grid_item_infos_) {
    auto* child = item_info.Item();
    Constraints constraints;
    if (!container_->GetLayoutConfigs().IsGridPreLayoutQuirksMode()) {
      PreCalcTrackSize(item_info, InlineAxis(), constraints);
      PreCalcTrackSize(item_info, BlockAxis(), constraints);
    }

    // TODO(yuanzhiwen): If both the grid container and all tracks have definite
    // sizes, also apply align-content to find the final effective size of any
    // gaps spanned by such items; otherwise ignore the effects of track
    // alignment in this estimation.

    auto child_constraints =
        property_utils::GenerateDefaultConstraints(*child, constraints);
    FloatSize layout_size = child->UpdateMeasure(child_constraints, false);

    child->GetBoxInfo()->UpdateBoxData(constraints, *child,
                                       child->GetLayoutConfigs());
    auto& entry = item_size_infos.emplace_back();
    entry.item_info = &item_info;
    entry.SetMaxContentBorderSize(InlineAxis(), layout_size.width_);
    entry.SetMinContentBorderSize(InlineAxis(), 0.f);
    if (container_->GetLayoutConfigs().IsGridNewQuirksMode()) {
      // To maintain compatibility with previous logic, we still calculate the
      // size contribution on block direction here.
      entry.SetMaxContentBorderSize(BlockAxis(), layout_size.height_);
      entry.SetMinContentBorderSize(BlockAxis(), 0.f);
    }
  }
}

void GridLayoutAlgorithm::CalcBlockAxisSizeContributions(
    MeasureItemCache& item_size_infos) {
  if (!container_->GetLayoutConfigs().IsGridNewQuirksMode()) {
    for (auto& item_size : item_size_infos) {
      const GridItemInfo& item_info = *item_size.item_info;
      LayoutObject* const child = item_info.Item();
      Constraints constraints;
      // To find the inline-axis available space for any items whose block-axis
      // size contributions require it, use the grid column sizes calculated in
      // the previous step. If the grid container's inline size is definite,
      // also apply justify-content to account for the effective column gap
      // sizes.
      constraints[InlineAxis()] = OneSideConstraint::Definite(
          item_info.ContainingBlock()[InlineAxis()].Size());
      auto child_constraints =
          property_utils::GenerateDefaultConstraints(*child, constraints);
      const FloatSize layout_size =
          child->UpdateMeasure(child_constraints, false);
      item_size.SetMaxContentBorderSize(BlockAxis(), layout_size.height_);
      item_size.SetMinContentBorderSize(BlockAxis(), 0.f);
    }
  }
}

void GridLayoutAlgorithm::MaximizeTracks(
    Dimension dimension, std::vector<float>& base_size,
    const std::vector<LayoutUnit>& grow_limit) const {
  if (!container_->GetLayoutConfigs().IsGridNewQuirksMode()) {
    float total_base_size_sum = 0.f;
    float free_space = 0.f;
    const int32_t grid_track_count = GridTrackCount(dimension);

    const auto& MaximizeTracksInner = [&base_size,
                                       grow_limit](float used_free_space) {
      size_t unfrozen_tracks_num = base_size.size();
      while (base::FloatsLarger(used_free_space, 0.f) &&
             unfrozen_tracks_num > 0) {
        const float space_per_track = used_free_space / unfrozen_tracks_num;
        unfrozen_tracks_num = 0;
        for (size_t idx = 0; idx < base_size.size(); ++idx) {
          if (grow_limit[idx].IsDefinite() &&
              base::FloatsLarger(grow_limit[idx].ToFloat(), base_size[idx])) {
            const float max_increment_size =
                grow_limit[idx].ToFloat() - base_size[idx];
            if (base::FloatsLarger(max_increment_size, space_per_track)) {
              base_size[idx] += space_per_track;
              used_free_space -= space_per_track;
              ++unfrozen_tracks_num;
            } else {
              base_size[idx] += max_increment_size;
              used_free_space -= max_increment_size;
            }
          }
        }
      }
    };

    if (IsSLDefiniteMode(container_constraints_[dimension].Mode())) {
      free_space = container_constraints_[dimension].Size();
      for (int32_t idx = 0; idx < grid_track_count; ++idx) {
        total_base_size_sum += base_size[idx];
      }
      total_base_size_sum += GridGapSize(dimension) * (base_size.size() - 1);
      free_space -= total_base_size_sum;
      MaximizeTracksInner(free_space);
    } else {
      float original_total_base_size_sum = 0.f;
      std::vector<float> original_base_size(base_size);
      for (int32_t idx = 0; idx < grid_track_count; ++idx) {
        original_total_base_size_sum += base_size[idx];
        base_size[idx] = grow_limit[idx].IsDefinite() &&
                                 base::FloatsLarger(grow_limit[idx].ToFloat(),
                                                    base_size[idx])
                             ? grow_limit[idx].ToFloat()
                             : base_size[idx];
        total_base_size_sum += base_size[idx];
      }
      // If this would cause the grid to be larger than the grid container’s
      // inner size as limited by its max-width/height, then redo this step,
      // treating the available grid space as equal to the grid container’s
      // inner size when it’s sized to its max-width/height.
      const float border_and_padding_size =
          logic_direction_utils::GetPaddingAndBorderDimensionSize(container_,
                                                                  dimension);
      BoxInfo* box_info = container_->GetBoxInfo();
      const float max_size =
          box_info->max_size_[dimension] - border_and_padding_size;
      if (base::FloatsLarger(total_base_size_sum, max_size)) {
        free_space = max_size - original_total_base_size_sum;
        base_size = std::move(original_base_size);
        MaximizeTracksInner(free_space);
      }
      // TODO(yuanzhiwen): consider preferred-size: fit-content (e.g.,
      // width:fit-content).
    }
  }
}

void GridLayoutAlgorithm::ResolveIntrinsicTrackSizes(
    Dimension dimension, MeasureItemCache& item_size_infos,
    std::vector<float>& base_size, std::vector<LayoutUnit>& grow_limit) {
  const auto& min_track_sizing_function = MinTrackSizingFunction(dimension);
  const auto& max_track_sizing_function = MaxTrackSizingFunction(dimension);
  const int32_t grid_track_count = GridTrackCount(dimension);
  const bool use_old_processing_logic =
      container_->GetLayoutConfigs().IsGridNewQuirksMode();
  // sort by span first.
  std::sort(item_size_infos.begin(), item_size_infos.end(),
            [dis = dimension](const ItemInfoEntry& a, const ItemInfoEntry& b) {
              return a.SpanSize(dis) < b.SpanSize(dis);
            });

  if (use_old_processing_logic) {
    // resolve tracks auto size
    for (const auto& item_size : item_size_infos) {
      const GridItemInfo& item_info = *item_size.item_info;
      if (item_info.SpanSize(dimension) == 0 ||
          item_info.IsCrossFlexibleTrack(dimension)) {
        continue;
      }

      const size_t start_line = item_info.StartLine(dimension);
      const size_t end_line = item_info.EndLine(dimension);

      size_t updated_track_count = 0;
      size_t track_zero_count = 0;
      float container_size_sum = 0.f;
      for (size_t idx = start_line; idx < end_line; ++idx) {
        if (min_track_sizing_function[idx - 1].IsAuto() &&
            max_track_sizing_function[idx - 1].IsAuto()) {
          if (base_size[idx - 1]) {
            ++updated_track_count;
          } else {
            ++track_zero_count;
          }
        }
        container_size_sum += base_size[idx - 1];
      }

      const float max_content_contribution =
          item_size.MaxContentContribution(dimension);
      if (container_size_sum >= max_content_contribution) {
        continue;
      }

      const float request_size = max_content_contribution - container_size_sum;
      float average_size = 0.f;
      if (track_zero_count) {
        average_size = request_size / track_zero_count;
      } else if (updated_track_count) {
        average_size = request_size / updated_track_count;
      }

      for (size_t idx = start_line; idx < end_line; ++idx) {
        if (min_track_sizing_function[idx - 1].IsAuto() &&
            max_track_sizing_function[idx - 1].IsAuto() &&
            (!track_zero_count || !base_size[idx - 1])) {
          base_size[idx - 1] += average_size;
        }
      }
    }
  } else {
    // place the items crossing flexible track to the end, because it needs to
    // be processed last.
    std::stable_partition(item_size_infos.begin(), item_size_infos.end(),
                          [dis = dimension](const ItemInfoEntry& a) {
                            return !a.item_info->IsCrossFlexibleTrack(dis);
                          });
    // collect various size contributions track index and resolve fit-content.
    std::vector<size_t> intrinsic_minimums_tracks_index_vec;
    std::vector<size_t> content_based_minimums_tracks_index_vec;
    std::vector<size_t> max_content_minimums_tracks_index_vec;
    std::vector<size_t> max_content_or_auto_minimums_tracks_index_vec;
    std::vector<size_t> max_content_maximums_tracks_index_vec;
    std::vector<size_t> intrinsic_maximums_tracks_index_vec;
    std::vector<float> fit_content_argument_value(grid_track_count, -1.f);
    for (int32_t idx = 0; idx < grid_track_count; ++idx) {
      switch (min_track_sizing_function[idx].GetType()) {
          // If the track was sized with a <flex> value or fit-content()
          // function, auto.
        case NLengthType::kNLengthFr:
        case NLengthType::kNLengthAuto:
        case NLengthType::kNLengthFitContent:
          intrinsic_minimums_tracks_index_vec.emplace_back(idx);
          max_content_or_auto_minimums_tracks_index_vec.emplace_back(idx);
          break;
        case NLengthType::kNLengthMaxContent:
          content_based_minimums_tracks_index_vec.emplace_back(idx);
          intrinsic_minimums_tracks_index_vec.emplace_back(idx);
          max_content_or_auto_minimums_tracks_index_vec.emplace_back(idx);
          max_content_minimums_tracks_index_vec.emplace_back(idx);
          break;
        default:
          break;
      }

      switch (max_track_sizing_function[idx].GetType()) {
          // In all cases, treat auto and fit-content() as max-content, except
          // where specified otherwise for fit-content().
        case NLengthType::kNLengthAuto:
        case NLengthType::kNLengthMaxContent:
          max_content_maximums_tracks_index_vec.emplace_back(idx);
          intrinsic_maximums_tracks_index_vec.emplace_back(idx);
          break;
        case NLengthType::kNLengthFitContent: {
          LayoutUnit fit_value = LayoutUnit::Indefinite();
          if (max_track_sizing_function[idx].NumericLength().HasValue()) {
            fit_value = NLengthToLayoutUnit(
                max_track_sizing_function[idx],
                container_constraints_[dimension].ToPercentBase());
          }
          // When not setting argument or resolve failed in fit-content, set it
          // to negative value.
          fit_content_argument_value[idx] =
              fit_value.IsDefinite() ? fit_value.ToFloat() : -1.f;
          intrinsic_maximums_tracks_index_vec.emplace_back(idx);
          max_content_maximums_tracks_index_vec.emplace_back(idx);
          break;
        }
        default:
          break;
      }
    }

    // collect items' size contributions.
    const size_t items_count = item_size_infos.size();
    std::vector<float> minimum_contributions(items_count, 0.f);
    std::vector<float> min_content_contributions(items_count, 0.f);
    std::vector<float> limited_min_content_contributions(items_count, 0.f);
    // Increase the length of the max-content contributions vector by one to
    // distinguish it from the minimum or min-content contributions.
    std::vector<float> max_content_contributions(items_count + 1, 0.f);
    std::vector<float> limited_max_content_contributions(items_count + 1, 0.f);
    for (size_t item_index = 0; item_index < item_size_infos.size();
         ++item_index) {
      const ItemInfoEntry& item_size = item_size_infos[item_index];
      const GridItemInfo& item_info = *item_size.item_info;
      const LayoutObject* item = item_info.Item();

      max_content_contributions[item_index] =
          item_size.MaxContentContribution(dimension);
      min_content_contributions[item_index] =
          item_size.MinContentContribution(dimension);

      // The minimum contribution of an item is the smallest outer size (margin
      // box) it can have.
      // if the item's computed preferred size behaves as auto or depends on the
      // size of its containing block in the relevant axis, its minimum
      // contribution is the outer size that would result from assuming the
      // item's used minimum size as its preferred size; else the item’s minimum
      // contribution is its min-content contribution. In Lynx, the default
      // minimum size is set to 0px, and the 'auto' value is currently not
      // supported.
      const NLength& preferred_size = (dimension == kHorizontal)
                                          ? item->GetCSSStyle()->GetWidth()
                                          : item->GetCSSStyle()->GetHeight();
      if (preferred_size.IsAuto() || preferred_size.ContainsPercentage()) {
        minimum_contributions[item_index] =
            (dimension == kHorizontal)
                ? item->GetOuterWidthFromBorderBoxWidth(
                      item->GetBoxInfo()->min_size_[dimension])
                : item->GetOuterHeightFromBorderBoxHeight(
                      item->GetBoxInfo()->min_size_[dimension]);
      } else {
        minimum_contributions[item_index] =
            min_content_contributions[item_index];
      }
      if (IsSLIndefiniteMode(container_constraints_[dimension].Mode())) {
        minimum_contributions[item_index] =
            min_content_contributions[item_index];
      }

      limited_max_content_contributions[item_index] =
          max_content_contributions[item_index];
      limited_min_content_contributions[item_index] =
          min_content_contributions[item_index];
      // For an item spanning multiple tracks, the upper limit used to calculate
      // its limited min-/max-content contribution is the sum of the fixed max
      // track sizing functions of any tracks it spans, and is applied if it
      // only spans such tracks.
      const size_t start_line = item_info.StartLine(dimension);
      const size_t end_line = item_info.EndLine(dimension);
      LayoutUnit upper_limit = LayoutUnit(0.f);
      for (size_t idx = start_line; idx < end_line; ++idx) {
        if (upper_limit.IsIndefinite()) {
          break;
        }
        const size_t track_index = idx - 1;
        upper_limit = upper_limit +
                      (base::FloatsLargerOrEqual(
                           fit_content_argument_value[track_index], 0.f)
                           ? LayoutUnit(fit_content_argument_value[track_index])
                           : grow_limit[track_index]);
      }

      if (upper_limit.IsDefinite()) {
        limited_max_content_contributions[item_index] =
            base::FloatsLarger(limited_max_content_contributions[item_index],
                               upper_limit.ToFloat())
                ? upper_limit.ToFloat()
                : limited_max_content_contributions[item_index];
        limited_min_content_contributions[item_index] =
            base::FloatsLarger(limited_min_content_contributions[item_index],
                               upper_limit.ToFloat())
                ? upper_limit.ToFloat()
                : limited_min_content_contributions[item_index];
      }
      // ultimately floored by its minimum contribution.
      limited_max_content_contributions[item_index] =
          base::FloatsLarger(limited_max_content_contributions[item_index],
                             minimum_contributions[item_index])
              ? limited_max_content_contributions[item_index]
              : minimum_contributions[item_index];
      limited_min_content_contributions[item_index] =
          base::FloatsLarger(limited_min_content_contributions[item_index],
                             minimum_contributions[item_index])
              ? limited_min_content_contributions[item_index]
              : minimum_contributions[item_index];
    }

    // size tracks to fit non-spanning items: For each track with an intrinsic
    // track sizing function and not a flexible sizing function, consider the
    // items in it with a span of 1:
    for (size_t item_index = 0; item_index < item_size_infos.size();
         ++item_index) {
      const GridItemInfo& item_info = *item_size_infos[item_index].item_info;
      if (item_info.SpanSize(dimension) != 1 ||
          item_info.IsCrossFlexibleTrack(dimension)) {
        break;
      }
      const size_t track_index = item_info.StartLine(dimension) - 1;
      // For min-content minimums: Lynx does not support min-content yet.

      // For max-content minimums:
      if (min_track_sizing_function[track_index].IsMaxContent()) {
        base_size[track_index] =
            base::FloatsLarger(max_content_contributions[item_index],
                               base_size[track_index])
                ? max_content_contributions[item_index]
                : base_size[track_index];
      } else if (min_track_sizing_function[track_index].IsAuto() ||
                 min_track_sizing_function[track_index].IsFitContent() ||
                 min_track_sizing_function[track_index].IsFr()) {
        // For auto minimums:
        // if the grid container is being sized under a min-/max-content
        // constraint,
        if (IsSLIndefiniteMode(container_constraints_[dimension].Mode())) {
          base_size[track_index] =
              base::FloatsLarger(base_size[track_index],
                                 limited_max_content_contributions[item_index])
                  ? base_size[track_index]
                  : limited_max_content_contributions[item_index];
        }
        // Otherwise, set the track's base size to the maximum of its items'
        // minimum contributions, floored at zero.
        else {
          base_size[track_index] =
              base::FloatsLarger(base_size[track_index],
                                 minimum_contributions[item_index])
                  ? base_size[track_index]
                  : minimum_contributions[item_index];
        }
      }

      // For min-content maximums: Lynx does not support min-content yet.

      // For max-content maximums:
      // In all cases, treat auto and fit-content() as max-content
      if (max_track_sizing_function[track_index].IsAuto() ||
          max_track_sizing_function[track_index].IsMaxContent() ||
          max_track_sizing_function[track_index].IsFitContent()) {
        if (grow_limit[track_index].IsDefinite()) {
          grow_limit[track_index] =
              base::FloatsLarger(max_content_contributions[item_index],
                                 grow_limit[track_index].ToFloat())
                  ? LayoutUnit(max_content_contributions[item_index])
                  : grow_limit[track_index];
        } else {
          grow_limit[track_index] =
              LayoutUnit(max_content_contributions[item_index]);
        }
      }
      // For fit-content() maximums, furthermore clamp this growth limit by
      // the fit-content() argument.
      if (max_track_sizing_function[track_index].IsFitContent() &&
          base::FloatsLargerOrEqual(fit_content_argument_value[track_index],
                                    0.f)) {
        grow_limit[track_index] =
            base::FloatsLarger(grow_limit[track_index].ToFloat(),
                               fit_content_argument_value[track_index])
                ? LayoutUnit(fit_content_argument_value[track_index])
                : grow_limit[track_index];
      }

      // In all cases, if a track's growth limit is now less than its base
      // size, increase the growth limit to match the base size.
      if (grow_limit[track_index].IsDefinite() &&
          base::FloatsLarger(base_size[track_index],
                             grow_limit[track_index].ToFloat())) {
        grow_limit[track_index] = LayoutUnit(base_size[track_index]);
      }
    }

    // Increase sizes to accommodate spanning items crossing content-sized
    // tracks.
    // What's more, increase sizes to accommodate spanning items crossing
    // flexible tracks in this part. (item_size_infos is sorted, and the items
    // crossing flexible track are placed at the end.)
    std::vector<bool> infinitely_growable(grid_track_count, false);
    int32_t span_count = 2;
    std::vector<size_t> considered_items_index_vec;
    for (size_t idx = 0; idx < item_size_infos.size(); ++idx) {
      const ItemInfoEntry& item_size_info = item_size_infos[idx];
      if (item_size_info.item_info->SpanSize(dimension) <= 1 &&
          !item_size_info.item_info->IsCrossFlexibleTrack(dimension)) {
        continue;
      }
      considered_items_index_vec.emplace_back(idx);
      const bool is_the_last_item_of_the_current_group_divided_by_span =
          (idx + 1 < item_size_infos.size()) &&
          (item_size_infos[idx + 1].item_info->SpanSize(dimension) >
           span_count);
      const bool is_the_last_item_not_crossing_flexible_tracks =
          (idx + 1 < item_size_infos.size()) &&
          !item_size_infos[idx].item_info->IsCrossFlexibleTrack(dimension) &&
          item_size_infos[idx + 1].item_info->IsCrossFlexibleTrack(dimension);
      // Call distributeExtraSpace by group:
      // 1. First, call 'distributeExtraSpace' for items not crossing flexible
      // tracks, grouped by span.
      // 2. Secondly, call 'distributeExtraSpace' considering all the items
      // crossing flexible track (together, rather than grouped by span size).
      if ((idx == item_size_infos.size() - 1) ||
          is_the_last_item_of_the_current_group_divided_by_span ||
          is_the_last_item_not_crossing_flexible_tracks) {
        bool whether_affect_base_sizes = true;

        // 1. For intrinsic minimums:
        if (IsSLIndefiniteMode(container_constraints_[dimension].Mode())) {
          DistributeExtraSpace(item_size_infos, base_size, grow_limit,
                               fit_content_argument_value, infinitely_growable,
                               dimension, whether_affect_base_sizes,
                               considered_items_index_vec,
                               intrinsic_minimums_tracks_index_vec,
                               limited_min_content_contributions);
        } else {
          DistributeExtraSpace(
              item_size_infos, base_size, grow_limit,
              fit_content_argument_value, infinitely_growable, dimension,
              whether_affect_base_sizes, considered_items_index_vec,
              intrinsic_minimums_tracks_index_vec, minimum_contributions);
        }

        // 2. For content-based minimums:
        DistributeExtraSpace(
            item_size_infos, base_size, grow_limit, fit_content_argument_value,
            infinitely_growable, dimension, whether_affect_base_sizes,
            considered_items_index_vec, content_based_minimums_tracks_index_vec,
            min_content_contributions);

        // 3. For max-content minimums:
        if (IsSLIndefiniteMode(container_constraints_[dimension].Mode())) {
          DistributeExtraSpace(item_size_infos, base_size, grow_limit,
                               fit_content_argument_value, infinitely_growable,
                               dimension, whether_affect_base_sizes,
                               considered_items_index_vec,
                               max_content_or_auto_minimums_tracks_index_vec,
                               limited_max_content_contributions);
        }

        // In all cases, continue to increase the base size of tracks with a
        // min track sizing function of max-content by distributing extra
        // space as needed to account for these items' max-content
        // contributions.
        DistributeExtraSpace(
            item_size_infos, base_size, grow_limit, fit_content_argument_value,
            infinitely_growable, dimension, whether_affect_base_sizes,
            considered_items_index_vec, max_content_minimums_tracks_index_vec,
            max_content_contributions);

        // 4. If at this point any track's growth limit is now less than its
        // base size, increase its growth limit to match its base size.
        for (int32_t idx = 0; idx < grid_track_count; ++idx) {
          grow_limit[idx] =
              (grow_limit[idx].IsDefinite() &&
               base::FloatsLarger(base_size[idx], grow_limit[idx].ToFloat()))
                  ? LayoutUnit(base_size[idx])
                  : grow_limit[idx];
        }

        whether_affect_base_sizes = false;
        if (!item_size_info.item_info->IsCrossFlexibleTrack(dimension)) {
          // 5. For intrinsic maximums:
          // Mark any tracks whose growth limit changed from infinite to finite
          // in this step as infinitely growable for the next step.
          DistributeExtraSpace(
              item_size_infos, base_size, grow_limit,
              fit_content_argument_value, infinitely_growable, dimension,
              whether_affect_base_sizes, considered_items_index_vec,
              intrinsic_maximums_tracks_index_vec, min_content_contributions);

          // 6. For max-content maximums:
          DistributeExtraSpace(
              item_size_infos, base_size, grow_limit,
              fit_content_argument_value, infinitely_growable, dimension,
              whether_affect_base_sizes, considered_items_index_vec,
              max_content_maximums_tracks_index_vec, max_content_contributions);
        }

        ++span_count;
        considered_items_index_vec.clear();
      }
    }

    // If any track still has an infinite growth limit (because, for example,
    // it had no items placed in it or it is a flexible track), set its growth
    // limit to its base size.
    for (int32_t idx = 0; idx < grid_track_count; ++idx) {
      if (grow_limit[idx].IsIndefinite()) {
        grow_limit[idx] = LayoutUnit(base_size[idx]);
      }
    }
  }
}

// To distribute extra space by increasing the affected sizes of a set of
// tracks as required by a set of intrinsic size contributions.
void GridLayoutAlgorithm::DistributeExtraSpace(
    const MeasureItemCache& item_size_infos, std::vector<float>& base_size,
    std::vector<LayoutUnit>& grow_limit,
    const std::vector<float>& fit_content_argument_value,
    std::vector<bool>& infinitely_growable, Dimension dimension,
    bool whether_affect_base_sizes,
    const std::vector<size_t>& considered_items_index_vec,
    const std::vector<size_t>& affected_track_index_vec,
    const std::vector<float>& size_contribution) {
  if (considered_items_index_vec.size() == 0 ||
      affected_track_index_vec.size() == 0) {
    return;
  }

  const int32_t grid_track_count = GridTrackCount(dimension);
  // Maintain separately for each affected base size or growth limit a
  // planned increase, initially set to 0. (This prevents the size
  // increases from becoming order-dependent.)
  std::vector<float> planned_increase(grid_track_count, 0.f);
  const std::vector<NLength>& max_track_sizing_function =
      MaxTrackSizingFunction(dimension);
  // The length of the max-content contributions vector is the item count
  // plus one, which distinguishes it from the minimum or min-content
  // contributions.
  const bool whether_minimum_or_min_content_contributions =
      (size_contribution.size() == item_size_infos.size());
  const bool if_resolve_item_crossing_flexible_track =
      item_size_infos[considered_items_index_vec[0]]
          .item_info->IsCrossFlexibleTrack(dimension);

  // For each considered item:
  for (size_t idx = 0; idx < considered_items_index_vec.size(); ++idx) {
    std::vector<float> item_incurred_increase(grid_track_count, 0.f);
    const size_t item_index = considered_items_index_vec[idx];
    const GridItemInfo& item_info = *item_size_infos[item_index].item_info;
    const size_t start_line = item_info.StartLine(dimension);
    const size_t end_line = item_info.EndLine(dimension);
    std::vector<size_t> affected_track_index_vec_item_cross;

    // 1. Find the space to distribute:
    float extra_space =
        size_contribution[item_index] -
        GridGapSize(dimension) * (item_info.SpanSize(dimension) - 1);
    // Subtract the corresponding size (base size or growth limit) of
    // 'every' spanned track from the item's size contribution to find
    // the item's remaining size contribution.
    for (size_t idx = start_line; idx < end_line; ++idx) {
      const size_t track_index = idx - 1;
      if (whether_affect_base_sizes ||
          (!whether_affect_base_sizes &&
           grow_limit[track_index].IsIndefinite())) {
        // For infinite growth limits, substitute the track's base size.
        extra_space -= base_size[track_index];
      } else {
        extra_space -= grow_limit[track_index].ToFloat();
      }

      // collect the affected tracks index which the item actually crossed.
      if (std::find(affected_track_index_vec.begin(),
                    affected_track_index_vec.end(),
                    track_index) != affected_track_index_vec.end()) {
        // when resolve items crossing flexible track, distributing
        // space only to flexible tracks (i.e. treating all other tracks
        // as having a fixed sizing function), so only collect the
        // flexible track here.
        if ((if_resolve_item_crossing_flexible_track &&
             max_track_sizing_function[track_index].IsFr()) ||
            (!if_resolve_item_crossing_flexible_track &&
             !max_track_sizing_function[track_index].IsFr())) {
          affected_track_index_vec_item_cross.emplace_back(track_index);
        }
      }
    }

    if (affected_track_index_vec_item_cross.size() == 0) {
      continue;
    }

    extra_space = base::FloatsLarger(extra_space, 0.f) ? extra_space : 0.f;

    // 2. Distribute space up to limits:
    bool all_tracks_frozen = false;
    std::vector<bool> frozen(grid_track_count, false);
    std::vector<float> flex_factor(grid_track_count, 0.f);
    while (true) {
      int32_t unfrozen_count = 0;
      float flex_factor_sum = 0.f;
      for (size_t idx = 0; idx < affected_track_index_vec_item_cross.size();
           ++idx) {
        const size_t track_index = affected_track_index_vec_item_cross[idx];
        if (!frozen[track_index]) {
          ++unfrozen_count;
        }
        if (if_resolve_item_crossing_flexible_track) {
          flex_factor[track_index] =
              max_track_sizing_function[track_index].GetRawValue();
          flex_factor_sum += flex_factor[track_index];
        }
      }

      all_tracks_frozen = (unfrozen_count == 0);
      if (all_tracks_frozen || base::FloatsLargerOrEqual(0.f, extra_space)) {
        break;
      }

      float hypothetical_distribution = extra_space / unfrozen_count;
      // if the sum of the flexible sizing functions of all flexible tracks
      // spanned by the item is greater than zero, distributing space to such
      // tracks according to the ratios of their flexible sizing functions
      // rather than distributing space equally
      if (base::FloatsLarger(flex_factor_sum, 0.f)) {
        hypothetical_distribution = extra_space / flex_factor_sum;
      }
      float item_incurred_increase_current_loop = 0.f;
      for (size_t idx = 0; idx < affected_track_index_vec_item_cross.size();
           ++idx) {
        const size_t track_index = affected_track_index_vec_item_cross[idx];
        if (!frozen[track_index]) {
          if (whether_affect_base_sizes) {
            if (!if_resolve_item_crossing_flexible_track) {
              const float hypothetical_base_size =
                  base_size[track_index] + hypothetical_distribution +
                  item_incurred_increase[track_index];
              if (grow_limit[track_index].IsDefinite() &&
                  base::FloatsLarger(hypothetical_base_size,
                                     grow_limit[track_index].ToFloat())) {
                item_incurred_increase_current_loop =
                    grow_limit[track_index].ToFloat() - base_size[track_index] -
                    item_incurred_increase[track_index];
                frozen[track_index] = true;
              } else {
                item_incurred_increase_current_loop = hypothetical_distribution;
                frozen[track_index] = false;
              }
            } else {
              item_incurred_increase_current_loop =
                  base::FloatsLarger(flex_factor_sum, 0.f)
                      ? hypothetical_distribution * flex_factor[track_index]
                      : hypothetical_distribution;
              ;
            }
          } else {
            // Note: If the affected size was a growth limit and the
            // track is not marked infinitely growable, then each
            // item-incurred increase will be zero.
            if (!infinitely_growable[track_index]) {
              frozen[track_index] = true;
              item_incurred_increase[track_index] = 0.f;
              continue;
            }

            LayoutUnit the_limit = infinitely_growable[track_index]
                                       ? LayoutUnit::Indefinite()
                                       : grow_limit[track_index];

            // However, limit the growth of any fit-content() tracks
            // by their fit-content() argument.
            if (max_track_sizing_function[track_index].IsFitContent() &&
                base::FloatsLargerOrEqual(
                    fit_content_argument_value[track_index], 0.f) &&
                (the_limit.IsIndefinite() ||
                 base::FloatsLarger(the_limit.ToFloat(),
                                    fit_content_argument_value[track_index]))) {
              the_limit = LayoutUnit(fit_content_argument_value[track_index]);
            }

            if (the_limit.IsIndefinite()) {
              item_incurred_increase_current_loop = hypothetical_distribution;
              frozen[track_index] = false;
            } else {
              const float hypothetical_grow_limit_value =
                  hypothetical_distribution +
                  item_incurred_increase[track_index] +
                  (grow_limit[track_index].IsDefinite()
                       ? grow_limit[track_index].ToFloat()
                       : 0.f);
              if (base::FloatsLarger(hypothetical_grow_limit_value,
                                     the_limit.ToFloat())) {
                if (grow_limit[track_index].IsDefinite()) {
                  const float hypothetical_incurred_increase_current_loop =
                      the_limit.ToFloat() - grow_limit[track_index].ToFloat() -
                      item_incurred_increase[track_index];
                  item_incurred_increase_current_loop =
                      base::FloatsLarger(
                          hypothetical_incurred_increase_current_loop, 0.f)
                          ? hypothetical_incurred_increase_current_loop
                          : 0.f;
                } else {
                  item_incurred_increase_current_loop =
                      the_limit.ToFloat() - item_incurred_increase[track_index];
                }
                frozen[track_index] = true;
              } else {
                item_incurred_increase_current_loop = hypothetical_distribution;
                frozen[track_index] = false;
              }
            }
          }

          extra_space -= item_incurred_increase_current_loop;
          item_incurred_increase[track_index] +=
              item_incurred_increase_current_loop;
        }
      }
    }

    // 3. Distribute space beyond limits:
    if (all_tracks_frozen && base::FloatsLarger(extra_space, 0.f)) {
      std::vector<size_t> track_index_vec_to_distribute;
      for (size_t idx = 0; idx < affected_track_index_vec_item_cross.size();
           ++idx) {
        const size_t track_index = affected_track_index_vec_item_cross[idx];
        const NLength track_sizing_function =
            max_track_sizing_function[track_index];
        // when accommodating minimum contributions or accommodating
        // min-content contributions: any affected track that happens
        // to also have an intrinsic max track sizing function
        if (whether_minimum_or_min_content_contributions) {
          if (track_sizing_function.IsAuto() ||
              track_sizing_function.IsMaxContent() ||
              track_sizing_function.IsFitContent()) {
            track_index_vec_to_distribute.emplace_back(track_index);
          }
          // when accommodating max-content contributions: any
          // affected track that happens to also have a max-content
          // max track sizing function
        } else {
          if (track_sizing_function.IsAuto() ||
              track_sizing_function.IsMaxContent() ||
              track_sizing_function.IsFitContent()) {
            track_index_vec_to_distribute.emplace_back(track_index);
          }
        }
      }

      // when if there are no such tracks (mentioned above) or
      // handling any intrinsic growth limit: all affected tracks.
      if (track_index_vec_to_distribute.size() == 0 ||
          !whether_affect_base_sizes) {
        track_index_vec_to_distribute = affected_track_index_vec_item_cross;
      }

      if (track_index_vec_to_distribute.size() != 0) {
        int32_t unfrozen_count = 0;
        std::vector<bool> frozen(grid_track_count, false);
        for (size_t idx = 0; idx < track_index_vec_to_distribute.size();
             ++idx) {
          const size_t track_index = track_index_vec_to_distribute[idx];
          // For this purpose, the max track sizing function of a
          // fit-content() track is treated as max-content until it
          // reaches the limit specified as the fit-content()
          // argument, after which it is treated as having a fixed
          // sizing function of that argument.
          if (max_track_sizing_function[track_index].IsFitContent() &&
              base::FloatsLargerOrEqual(fit_content_argument_value[track_index],
                                        0.f)) {
            float affected_track_hypothetical_size =
                item_incurred_increase[track_index];
            if (whether_affect_base_sizes) {
              affected_track_hypothetical_size += base_size[track_index];
            } else {
              affected_track_hypothetical_size +=
                  (grow_limit[track_index].IsDefinite()
                       ? grow_limit[track_index].ToFloat()
                       : 0.f);
            }

            if (base::FloatsLarger(fit_content_argument_value[track_index],
                                   affected_track_hypothetical_size)) {
              ++unfrozen_count;
            } else {
              frozen[track_index] = true;
            }
          } else {
            ++unfrozen_count;
          }
        }

        if (unfrozen_count != 0) {
          const float hypothetical_distribution = extra_space / unfrozen_count;
          for (size_t idx = 0; idx < track_index_vec_to_distribute.size();
               ++idx) {
            const size_t track_index = track_index_vec_to_distribute[idx];
            if (!frozen[track_index]) {
              item_incurred_increase[track_index] += hypothetical_distribution;
            }
          }
        }
      }
    }

    // 4. For each affected track, if the track's item-incurred
    // increase is larger than the track's planned increase set the
    // track's planned increase to that value.
    for (size_t idx = 0; idx < affected_track_index_vec_item_cross.size();
         ++idx) {
      const size_t track_index = affected_track_index_vec_item_cross[idx];
      planned_increase[track_index] =
          base::FloatsLarger(item_incurred_increase[track_index],
                             planned_increase[track_index])
              ? item_incurred_increase[track_index]
              : planned_increase[track_index];
    }
  }

  // Update the tracks' affected sizes
  for (size_t idx = 0; idx < affected_track_index_vec.size(); ++idx) {
    const size_t track_index = affected_track_index_vec[idx];
    if (whether_affect_base_sizes) {
      base_size[track_index] += planned_increase[track_index];
    } else {
      if (grow_limit[track_index].IsDefinite()) {
        grow_limit[track_index] = LayoutUnit(planned_increase[track_index] +
                                             grow_limit[track_index].ToFloat());
      } else {
        grow_limit[track_index] =
            base::FloatsLarger(planned_increase[track_index], 0.f)
                ? LayoutUnit(planned_increase[track_index] +
                             base_size[track_index])
                : grow_limit[track_index];
        // Mark any tracks whose growth limit changed from infinite to
        // finite in this step as infinitely growable for the next step.
        // When suppport min-content, will review it.
        if (whether_minimum_or_min_content_contributions) {
          infinitely_growable[track_index] = true;
        }
      }
    }
  }
}

void GridLayoutAlgorithm::ExpandFlexibleTracksAndStretchAutoTracks(
    Dimension dimension, const MeasureItemCache& item_size_infos,
    std::vector<float>& base_size) {
  const int32_t grid_track_count = GridTrackCount(dimension);
  const size_t grid_line_count =
      grid_track_count != 0 ? grid_track_count + 3 : 2;
  auto& grid_line_offset = GridLineOffsetFromContainerPaddingBound(dimension);
  grid_line_offset.resize(grid_line_count);
  grid_line_offset[0] = 0.f;

  // When there are only absolute children, the grid_track_count may be zero,
  // so we need to update container size here.
  if (grid_track_count == 0) {
    UpdateContainerSize(dimension, 0.f);
    grid_line_offset[1] =
        container_constraints_[dimension].Size() +
        (dimension == kHorizontal ? container_->GetLayoutPaddingLeft() +
                                        container_->GetLayoutPaddingRight()
                                  : container_->GetLayoutPaddingTop() +
                                        container_->GetLayoutPaddingBottom());
    return;
  }

  const auto& max_track_sizing_function = MaxTrackSizingFunction(dimension);
  // including gutters
  float total_base_size_sum = 0;
  size_t auto_track_count = 0;
  bool has_flexible_track = false;
  // A positive number indicates the flex factor of a flexible track,
  // A '0' denotes an inflexible track,
  // A '-1' indicates that the track is excluded from finding the size of an
  // fr.
  std::vector<float> flex_factor(grid_track_count, 0.f);
  for (int32_t idx = 0; idx < grid_track_count; ++idx) {
    if (max_track_sizing_function[idx].IsAuto()) {
      ++auto_track_count;
    } else if (max_track_sizing_function[idx].IsFr()) {
      flex_factor[idx] = max_track_sizing_function[idx].GetRawValue();
      has_flexible_track = true;
    }
    total_base_size_sum += base_size[idx];
  }

  total_base_size_sum += GridGapSize(dimension) * (grid_track_count - 1);

  // Expand Flexible Tracks
  if (has_flexible_track) {
    float a_space_to_fill = 0.f;
    float flex_fraction = 0.f;
    if (IsSLDefiniteMode(container_constraints_[dimension].Mode())) {
      const float free_space =
          container_constraints_[dimension].Size() - total_base_size_sum;
      if (base::FloatsLarger(free_space, 0)) {
        a_space_to_fill = container_constraints_[dimension].Size() -
                          GridGapSize(dimension) * (grid_track_count - 1);
        flex_fraction =
            FindTheSizeOfAnFr(base_size, flex_factor, a_space_to_fill);
      } else {
        flex_fraction = 0.f;
      }
    } else {
      // For each flexible track:
      for (int32_t idx = 0; idx < grid_track_count; ++idx) {
        if (base::FloatsLarger(flex_factor[idx], 1.0f)) {
          flex_fraction =
              std::max(flex_fraction, base_size[idx] / flex_factor[idx]);
        } else if (base::FloatsLarger(flex_factor[idx], 0.f)) {
          flex_fraction = std::max(flex_fraction, base_size[idx]);
        }
      }

      // For each grid item that crosses a flexible track:
      for (const auto& item_size : item_size_infos) {
        std::vector<float> used_flex_factor_for_each_item(flex_factor);
        const GridItemInfo& item_info = *item_size.item_info;
        const int32_t start_line = item_info.StartLine(dimension);
        const int32_t end_line = item_info.EndLine(dimension);
        if (start_line == end_line) {
          continue;
        }
        bool cross_flexible_track = false;
        // Reinitialize the flex factor vector to ensure all the grid tracks
        // that the item crosses are considered in specific 'finding the size of
        // an fr'.
        for (int32_t idx = 0; idx < grid_track_count; ++idx) {
          if (idx >= start_line - 1 && idx <= end_line - 2) {
            if (base::FloatsLarger(used_flex_factor_for_each_item[idx], 0.f)) {
              cross_flexible_track = true;
            }
          } else {
            used_flex_factor_for_each_item[idx] = -1.f;
          }
        }
        if (cross_flexible_track) {
          a_space_to_fill =
              item_size.MaxContentContribution(dimension) -
              (item_info.SpanSize(dimension) - 1) * GridGapSize(dimension);
          flex_fraction = std::max(
              flex_fraction,
              FindTheSizeOfAnFr(base_size, used_flex_factor_for_each_item,
                                a_space_to_fill));
        }
      }

      // If using this flex fraction would cause the grid to be smaller than the
      // grid container’s min-width/height (or larger than the grid container’s
      // max-width/height), then redo this step, treating the free space as
      // definite and the available grid space as equal to the grid container’s
      // inner size when it’s sized to its min-width/height (max-width/height).
      float hypothetical_grid_size =
          GridGapSize(dimension) * (grid_track_count - 1);
      for (int32_t idx = 0; idx < grid_track_count; ++idx) {
        // flexible track uses product of the used flex fraction and the track’s
        // flex factor as base size.
        hypothetical_grid_size += (base::FloatsLarger(flex_factor[idx], 0.f))
                                      ? flex_factor[idx] * flex_fraction
                                      : base_size[idx];
      }
      const float applied_size = property_utils::ApplyMinMaxToSpecificSize(
          hypothetical_grid_size, container_, dimension);
      if (base::FloatsNotEqual(hypothetical_grid_size, applied_size)) {
        const float free_space = applied_size - total_base_size_sum;
        if (base::FloatsLarger(free_space, 0)) {
          a_space_to_fill =
              applied_size - GridGapSize(dimension) * (grid_track_count - 1);
          flex_fraction =
              FindTheSizeOfAnFr(base_size, flex_factor, a_space_to_fill);
        } else {
          flex_fraction = 0.f;
        }
      }
    }

    if (base::FloatsLarger(flex_fraction, 0)) {
      for (int32_t idx = 0; idx < grid_track_count; ++idx) {
        if (base::FloatsLarger(flex_factor[idx], 0)) {
          const float adjusted_size = flex_fraction * flex_factor[idx];
          // For each flexible track, if the product of the used flex fraction
          // and the track’s flex factor is greater than the track’s base
          // size, set its base size to that product.
          if (base::FloatsLarger(adjusted_size, base_size[idx])) {
            total_base_size_sum += adjusted_size - base_size[idx];
            base_size[idx] = adjusted_size;
          }
        }
      }
    }
  }

  // Not consider 'min-content contribution of any grid item has changed based
  // on the row/column sizes and alignment calculated' respectively, so update
  // container size here.
  UpdateContainerSize(dimension, total_base_size_sum);

  const float free_space =
      container_constraints_[dimension].Size() - total_base_size_sum;

  // TODO(yuanzhiwen): consider leftover free-space is negative.
  if (base::FloatsLarger(free_space, 0)) {
    bool is_stretch = false;
    if (dimension == BlockAxis()) {
      AlignContentType align_content = container_style_->GetAlignContent();
      is_stretch = align_content == AlignContentType::kStretch;
      if (!is_stretch) {
        ResolveAlignContent(container_style_, grid_track_count, free_space,
                            block_axis_interval_, block_axis_start_);
      }
    } else {
      JustifyContentType justify_content =
          container_style_->GetJustifyContent();
      is_stretch = justify_content == JustifyContentType::kStretch;
      if (!is_stretch) {
        ResolveJustifyContent(container_style_, grid_track_count, free_space,
                              inline_axis_interval_, inline_axis_start_);
      }
    }
    // Stretch 'auto' Tracks:
    // This step expands tracks that have an auto max track sizing function by
    // dividing any remaining positive, definite free space equally amongst
    // them. If the free space is indefinite, but the grid container has a
    // definite min-width/height, use that size to calculate the free space for
    // this step instead.
    if (is_stretch) {
      const float average_size =
          (auto_track_count != 0) ? free_space / auto_track_count : 0.f;
      for (size_t idx = 0; idx < base_size.size(); ++idx) {
        if (max_track_sizing_function[idx].IsAuto()) {
          base_size[idx] += average_size;
        }
      }
    }
  }
  const float padding_start =
      (dimension == kHorizontal)
          ? (HorizontalFront() == kRight ? container_->GetLayoutPaddingRight()
                                         : container_->GetLayoutPaddingLeft())
          : container_->GetLayoutPaddingTop();
  grid_line_offset[1] =
      padding_start +
      (dimension == kHorizontal ? inline_axis_start_ : block_axis_start_);
  for (size_t idx = 2; idx < grid_line_count - 1; ++idx) {
    grid_line_offset[idx] =
        base_size[idx - 2] + grid_line_offset[idx - 1] +
        (idx == grid_line_count - 2 ? 0.f : GridGapSize(dimension));
  }
  grid_line_offset[grid_line_count - 1] =
      container_constraints_[dimension].Size() +
      (dimension == kHorizontal ? container_->GetLayoutPaddingLeft() +
                                      container_->GetLayoutPaddingRight()
                                : container_->GetLayoutPaddingTop() +
                                      container_->GetLayoutPaddingBottom());
}

float GridLayoutAlgorithm::FindTheSizeOfAnFr(
    const std::vector<float>& base_size, const std::vector<float>& flex_factor,
    float space_to_fill) const {
  std::vector<float> used_flex_factor(flex_factor);
  while (true) {
    float leftover_space = space_to_fill;
    float flex_factor_sum = 0.f;
    for (size_t idx = 0; idx < base_size.size(); ++idx) {
      if (base::FloatsEqual(used_flex_factor[idx], 0)) {
        leftover_space -= base_size[idx];
      } else if (base::FloatsLarger(used_flex_factor[idx], 0)) {
        flex_factor_sum += used_flex_factor[idx];
      }
    }
    flex_factor_sum = std::max(flex_factor_sum, 1.0f);
    const float hypothetical_fr_size = leftover_space / flex_factor_sum;
    bool has_product_less_than_base_size = false;
    for (size_t idx = 0; idx < base_size.size(); ++idx) {
      // If the product of the hypothetical fr size and a flexible track’s
      // flex factor is less than the track’s base size, restart this
      // algorithm treating all such tracks as inflexible.
      if (base::FloatsLarger(used_flex_factor[idx], 0) &&
          base::FloatsLarger(base_size[idx],
                             hypothetical_fr_size * used_flex_factor[idx])) {
        has_product_less_than_base_size = true;
        used_flex_factor[idx] = 0.f;
      }
    }
    if (!has_product_less_than_base_size) {
      return hypothetical_fr_size;
    }
  }
}

void GridLayoutAlgorithm::UpdateContainerSize(Dimension dimension,
                                              float track_size_sum) {
  if (IsSLDefiniteMode(container_constraints_[dimension].Mode())) {
    return;
  }

  track_size_sum = property_utils::ApplyMinMaxToSpecificSize(
      track_size_sum, container_, dimension);

  if (container_->GetLayoutConfigs().IsGridNewQuirksMode() &&
      IsSLAtMostMode(container_constraints_[dimension].Mode())) {
    track_size_sum =
        std::min(track_size_sum, container_constraints_[dimension].Size());
  }

  container_constraints_[dimension] =
      OneSideConstraint::Definite(track_size_sum);

  // resolve against the box’s content box when laying out the box’s contents.
  if (dimension == InlineAxis()) {
    inline_gap_size_ = CalculateFloatSizeFromLength(GapStyle(InlineAxis()),
                                                    PercentBase(InlineAxis()));
  } else {
    block_gap_size_ = CalculateFloatSizeFromLength(GapStyle(BlockAxis()),
                                                   PercentBase(BlockAxis()));
  }
}

void GridLayoutAlgorithm::MeasureGridItems() {
  for (const GridItemInfo& item_info : grid_item_infos_) {
    auto* child = item_info.Item();
    auto* child_style = child->GetCSSStyle();

    const Constraints& container_constraints = item_info.ContainingBlock();
    auto child_constraints = property_utils::GenerateDefaultConstraints(
        *child, container_constraints);

    if (IsSLAtMostMode(child_constraints[BlockAxis()].Mode()) &&
        ((child_style->GetAlignSelf() == FlexAlignType::kAuto &&
          container_style_->GetAlignItems() == FlexAlignType::kStretch) ||
         (child_style->GetAlignSelf() == FlexAlignType::kStretch))) {
      if (!GetMargin(child_style, BlockFront()).IsAuto() &&
          !GetMargin(child_style, BlockBack()).IsAuto()) {
        child_constraints[BlockAxis()] =
            OneSideConstraint::Definite(child_constraints[BlockAxis()].Size());
      }
    }

    if (IsSLAtMostMode(child_constraints[InlineAxis()].Mode()) &&
        ((child_style->GetJustifySelfType() == JustifyType::kAuto &&
          container_style_->GetJustifyItemsType() == JustifyType::kStretch) ||
         (child_style->GetJustifySelfType() == JustifyType::kStretch))) {
      if (!GetMargin(child_style, InlineFront()).IsAuto() &&
          !GetMargin(child_style, InlineBack()).IsAuto()) {
        child_constraints[InlineAxis()] =
            OneSideConstraint::Definite(child_constraints[InlineAxis()].Size());
      }
    }

    child->UpdateMeasure(child_constraints, true);
    //  resolve margin auto
    ResolveAutoMargins(child, container_constraints[InlineAxis()].Size(),
                       InlineAxis());
    ResolveAutoMargins(child, container_constraints[BlockAxis()].Size(),
                       BlockAxis());
  }
}

float GridLayoutAlgorithm::GridTrackCount(Dimension dimension) const {
  return dimension == InlineAxis() ? inline_track_count_ : block_track_count_;
}

float GridLayoutAlgorithm::GridGapSize(Dimension dimension) const {
  return dimension == InlineAxis() ? inline_gap_size_ + inline_axis_interval_
                                   : block_gap_size_ + block_axis_interval_;
}

std::vector<NLength>& GridLayoutAlgorithm::MinTrackSizingFunction(
    Dimension dimension) {
  return dimension == kHorizontal ? grid_column_min_track_sizing_function_
                                  : grid_row_min_track_sizing_function_;
}

std::vector<NLength>& GridLayoutAlgorithm::MaxTrackSizingFunction(
    Dimension dimension) {
  return dimension == kHorizontal ? grid_column_max_track_sizing_function_
                                  : grid_row_max_track_sizing_function_;
}

const std::vector<NLength>&
GridLayoutAlgorithm::ExplicitTrackMinTrackSizingFunction(
    Dimension dimension) const {
  return dimension == kHorizontal
             ? container_style_->GetGridTemplateColumnsMinTrackingFunction()
             : container_style_->GetGridTemplateRowsMinTrackingFunction();
}

const std::vector<NLength>&
GridLayoutAlgorithm::ExplicitTrackMaxTrackSizingFunction(
    Dimension dimension) const {
  return dimension == kHorizontal
             ? container_style_->GetGridTemplateColumnsMaxTrackingFunction()
             : container_style_->GetGridTemplateRowsMaxTrackingFunction();
}

const std::vector<NLength>&
GridLayoutAlgorithm::ImplicitTrackMinTrackSizingFunction(
    Dimension dimension) const {
  return dimension == kHorizontal
             ? container_style_->GetGridAutoColumnsMinTrackingFunction()
             : container_style_->GetGridAutoRowsMinTrackingFunction();
}

const std::vector<NLength>&
GridLayoutAlgorithm::ImplicitTrackMaxTrackSizingFunction(
    Dimension dimension) const {
  return dimension == kHorizontal
             ? container_style_->GetGridAutoColumnsMaxTrackingFunction()
             : container_style_->GetGridAutoRowsMaxTrackingFunction();
}

std::vector<float>&
GridLayoutAlgorithm::GridLineOffsetFromContainerPaddingBound(
    Dimension dimension) {
  return dimension == kHorizontal
             ? grid_column_line_offset_from_container_padding_bound_
             : grid_row_line_offset_from_container_padding_bound_;
}

float GridLayoutAlgorithm::CalcContainingBlock(Dimension dimension,
                                               int32_t start, int32_t end) {
  const auto& grid_line_offset =
      GridLineOffsetFromContainerPaddingBound(dimension);
  const int32_t grid_line_count = static_cast<int32_t>(grid_line_offset.size());

  // For absolutely positioned: If a grid-placement property refers to a
  // non-existent line either by explicitly specifying such a line or by
  // spanning outside of the existing implicit grid, it is instead treated as
  // specifying auto (instead of creating new implicit grid lines).
  if (start > grid_line_count - 2) {
    start = kGridLineUnDefine;
  }

  // Instead of auto-placement, an auto value for a grid-placement property
  // contributes a special line to the placement whose position is that of the
  // corresponding padding edge of the grid container. These lines become the
  // first and last lines (0th and -0th) of the augmented grid used for
  // positioning absolutely-positioned items.
  if (end == kGridLineUnDefine || end > grid_line_count - 2) {
    end = grid_line_count - 1;
  }
  if (start >= end) {
    return 0;
  }

  // Gutters only appear between tracks of the implicit grid; there is no gutter
  // before the first track or after the last track. (In particular, there is no
  // gutter between the first/last track of the implicit grid and the 'auto'
  // lines in the augmented grid.)
  return grid_line_offset[end] - grid_line_offset[start] -
         ((end > 1 && (end < grid_line_count - 2)) ? GridGapSize(dimension)
                                                   : 0.f);
}

}  // namespace starlight
}  // namespace lynx
