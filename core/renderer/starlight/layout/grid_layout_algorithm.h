// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_GRID_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_GRID_LAYOUT_ALGORITHM_H_

#include <algorithm>
#include <vector>

#include "core/renderer/starlight/layout/grid_item_info.h"
#include "core/renderer/starlight/layout/layout_algorithm.h"

namespace lynx {
namespace starlight {

class LayoutObject;
class GridLayoutAlgorithm : public LayoutAlgorithm {
 public:
  explicit GridLayoutAlgorithm(LayoutObject*);

  void InitializeAlgorithmEnv() override;
  void Reset() override;
  void AlignInFlowItems() override;
  void MeasureAbsoluteAndFixed() override;
  void AlignAbsoluteAndFixedItems() override;
  void SizeDeterminationByAlgorithm() override;
  void SetContainerBaseline() override{};

 private:
  // The auto-placement cursor defines the current “insertion point” in the
  // grid, specified as a pair of row and column grid lines.
  struct PlacementCursor {
    int32_t main_line = kGridLineStart;
    int32_t cross_line = kGridLineStart;
  };

  using PlaceItemCache = std::vector<GridItemInfo*>;
  using MeasureItemCache = std::vector<ItemInfoEntry>;

  // The following grid item placement algorithm resolves automatic positions of
  // grid items into definite positions, ensuring that every grid item has a
  // well-defined grid area to lay out into.
  void PlaceGridItems();
  void PrePlaceGridItems(PlaceItemCache& place_item);
  void PlaceGridItemsLockedToAutoPlacementCrossAxis(PlaceItemCache& place_item);
  void PlaceGridItemsLockedToAutoPlacementMainAxis(GridItemInfo& grid_info,
                                                   PlacementCursor& cursor,
                                                   PlaceItemCache& place_item);
  // Position the items have an automatic grid position in both axes.
  void PlaceGridItemsWithBothAxesAuto(GridItemInfo& grid_info,
                                      PlacementCursor& cursor,
                                      PlaceItemCache& place_item);
  int32_t FindNextAvailablePosition(Dimension locked_dimension,
                                    int32_t locked_start, int32_t locked_span,
                                    int32_t not_locked_initial_start,
                                    int32_t not_locked_span,
                                    int32_t not_locked_max_size,
                                    const PlaceItemCache& place_item);

  // measure track size.
  void GridItemSizing();
  void InitTrackSize(Dimension dimension, std::vector<float>& base_size,
                     std::vector<LayoutUnit>& grow_limit);
  void CalcInlineAxisSizeContributions(MeasureItemCache& item_size_infos);
  void CalcBlockAxisSizeContributions(MeasureItemCache& item_size_infos);
  void ResolveIntrinsicTrackSizes(Dimension dimension,
                                  MeasureItemCache& item_size_infos,
                                  std::vector<float>& base_size,
                                  std::vector<LayoutUnit>& grow_limit);
  void DistributeExtraSpace(
      const MeasureItemCache& item_size_infos, std::vector<float>& base_size,
      std::vector<LayoutUnit>& grow_limit,
      const std::vector<float>& fit_content_argument_value,
      std::vector<bool>& infinitely_growable, Dimension dimension,
      bool whether_affect_base_sizes,
      const std::vector<size_t>& considered_items_index_vec,
      const std::vector<size_t>& affected_track_index_vec,
      const std::vector<float>& size_contribution);
  void MaximizeTracks(Dimension dimension, std::vector<float>& base_size,
                      const std::vector<LayoutUnit>& grow_limit) const;
  void ExpandFlexibleTracksAndStretchAutoTracks(
      Dimension dimension, const MeasureItemCache& item_size_infos,
      std::vector<float>& base_size);
  float FindTheSizeOfAnFr(const std::vector<float>& base_size,
                          const std::vector<float>& flex_factor,
                          float space_to_fill) const;

  // update container size
  void UpdateContainerSize(Dimension dimension, float track_size_sum);
  inline void UpdateGridTrackCountIfNeeded(Dimension dimension,
                                           int32_t track_count) {
    if (dimension == InlineAxis()) {
      inline_track_count_ = std::max(inline_track_count_, track_count);
    } else {
      block_track_count_ = std::max(block_track_count_, track_count);
    }
  }
  // measure grid items with containing block.
  void MeasureGridItems();

  // alignment
  float InlineAxisAlignment(const GridItemInfo& item_info);
  float BlockAxisAlignment(const GridItemInfo& item_info);
  bool IsDense() const { return is_dense_; }
  std::vector<NLength>& MinTrackSizingFunction(Dimension dimension);
  std::vector<NLength>& MaxTrackSizingFunction(Dimension dimension);
  float GridTrackCount(Dimension dimension) const;
  float GridGapSize(Dimension dimension) const;
  const std::vector<NLength>& ExplicitTrackMinTrackSizingFunction(
      Dimension dimension) const;
  const std::vector<NLength>& ExplicitTrackMaxTrackSizingFunction(
      Dimension dimension) const;
  const std::vector<NLength>& ImplicitTrackMinTrackSizingFunction(
      Dimension dimension) const;
  const std::vector<NLength>& ImplicitTrackMaxTrackSizingFunction(
      Dimension dimension) const;

  // Valid after 'ExpandFlexibleTracksAndStretchAutoTracks' has finished, taking
  // into consideration 'align-content', 'justify-content', and 'gutters'.
  // Including the special line (auto) - first and last lines (0th and -0th) of
  // the augmented grid used for positioning absolutely-positioned items. When
  // the line acquired thickness from the gutter, the following value is the
  // offset from the line's end side to the container's padding bound.
  // For example, for 'width: 400px (padding-bound-width)',
  // 'grid-template-columns: 50px 100px', 'padding: 10px', 'gap: 10px'. We get
  // [0 10 70 170 400].
  std::vector<float>& GridLineOffsetFromContainerPaddingBound(
      Dimension dimension);
  // This calculation depends on "GridLineOffsetFromContainerPaddingBound".
  float CalcContainingBlock(Dimension dimension, int32_t start, int32_t end);

  // Dimension for grid.
  // Writing-mode is not yet supported, inline axis is always equals horizontal
  // axis;
  inline Dimension InlineAxis() const { return kHorizontal; }
  inline Dimension BlockAxis() const { return kVertical; }
  inline Direction InlineFront() const { return HorizontalFront(); }
  inline Direction InlineBack() const { return HorizontalBack(); }
  inline Direction BlockFront() const { return CrossFront(); }
  inline Direction BlockBack() const { return CrossBack(); }

  // auto flow
  bool is_dense_ = false;
  bool has_placement_ = false;
  // If auto_placement_main_axis_ is kHorizontal (when grid-auto-flow:row/row
  // dense/dense), the auto-placement algorithm places items by filling each row
  // (writing-mode is not yet supported, row is used here) in turn, adding new
  // rows as necessary.
  Dimension auto_placement_main_axis_ = kHorizontal;
  // The auto_placement_cross_axis_ runs across the auto_placement_main_axis_.
  Dimension auto_placement_cross_axis_ = kVertical;

  // grid item position offset.
  int32_t row_offset_ = 0;
  int32_t column_offset_ = 0;

  // implicit axis count
  int32_t inline_track_count_ = 0;
  int32_t block_track_count_ = 0;

  // justify-content/align-content gap size.
  float inline_axis_interval_ = 0;
  float block_axis_interval_ = 0;
  // start gap for justify-content/align-content.
  float inline_axis_start_ = 0;
  float block_axis_start_ = 0;
  // implicit axis gap size
  float inline_gap_size_ = 0;
  float block_gap_size_ = 0;

  std::vector<GridItemInfo> grid_item_infos_;
  std::vector<GridItemInfo> grid_absolutely_positioned_item_infos_;

  std::vector<NLength> grid_row_min_track_sizing_function_;
  std::vector<NLength> grid_row_max_track_sizing_function_;
  std::vector<NLength> grid_column_min_track_sizing_function_;
  std::vector<NLength> grid_column_max_track_sizing_function_;

  std::vector<float> grid_row_line_offset_from_container_padding_bound_;
  std::vector<float> grid_column_line_offset_from_container_padding_bound_;
};

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_GRID_LAYOUT_ALGORITHM_H_
