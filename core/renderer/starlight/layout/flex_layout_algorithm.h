// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_FLEX_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_FLEX_LAYOUT_ALGORITHM_H_

#include <memory>

#include "core/renderer/starlight/layout/flex_info.h"
#include "core/renderer/starlight/layout/layout_algorithm.h"

namespace lynx {
namespace starlight {
class LayoutObject;

class LayoutComputedStyle;

class FlexLayoutAlgorithm : public LayoutAlgorithm {
 public:
  FlexLayoutAlgorithm(LayoutObject*);

  // TODO(zzz):unified handle process
  void SizeDeterminationByAlgorithm() override;
  void AlignInFlowItems() override;

  BoxPositions GetAbsoluteOrFixedItemInitialPosition(
      LayoutObject* absolute_or_fixed_item) override;

  void InitializeAlgorithmEnv() override;
  void Reset() override;
  void SetContainerBaseline() override;

 private:
  /*Algorithm-3
   * Determine the flex base size and hypothetical main size of each item:*/
  float DetermineFlexBaseSizeAndHypotheticalMainSize();
  float ChildCalculateFlexBasis(int32_t idx);

  // Algorithm-4 Calculate the main size of the flex container and collect flex
  // items into flex lines
  float CalculateFlexContainerMainSize(float total_hypothetical_main_size);

  // Algorithm-5 Determine the main size of the flex container
  void DetermineFlexContainerMainSize(float flex_container_main_size);

  // Algorithm-6 Resolve the flexible lengths of all the flex items to find
  // their used main size.
  void ResolveFlexibleLengths(LineInfo& line_info);

  // Algorithm-7 Determine the hypothetical cross size of each item
  void DetermineHypotheticalCrossSize();

  // Algorithm-8 Calculate the cross size of each flex line.
  void CalculateCrossSizeOfEachFlexLine();

  // Algorithm-11 Determine the used cross size of each flex item
  bool ShouldApplyStretchAndLayoutLater(int32_t idx);
  void DetermineUsedCrossSizeOfEachFlexItem();
  bool IsCrossSizeAutoAndMarginNonAuto(int32_t idx);

  // Algorithm-12 Distribute any remaining free space
  void DistributeRemainingFreeSpace(const LineInfo& line_info);
  bool CalculateAndSetAutoMargins(LayoutItems& line_items,
                                  float remaining_free_space);
  void CalculateJustifyContent(const LineInfo& line_info,
                               float& main_axis_start,
                               float& main_axis_interval);
  void MainAxisAlignment(LayoutItems& line_items, float main_axis_start,
                         float main_axis_interval);
  void CalculateAlignContent(float& cross_axis_start,
                             float& cross_axis_interval);
  void CrossAxisAlignment(const LineInfo& line_info, float& line_cross_offset);

  // Algorithm-14 Align all flex items along the cross-axis per align-self
  void AlignItems(int32_t idx, float line_cross_size, float line_cross_offset,
                  float line_baseline);

  void CalculateWrapReverse();

  // Algorithm-15 Determine the flex container’s used cross size:
  void DetermineContainerCrossSize();

  // SOME UPDATE FUNCTIONS

  void UpdateContainerMainSize(float container_main_size);
  void UpdateCrossSize(float container_cross_size);

  float GetOuterHypotheticalMainSize(int32_t idx);
  float GetOuterFlexBaseMainSIze(int32_t idx);
  float GetOuterHypotheticalCrossSize(int32_t idx);

  float CalculateOffsetFromTopMarginEdgeToBaseline(int32_t idx);
  float CalculateFlexLineCrossSizeConsiderBaseline(
      float largest_outer_hypothetical_cross_size, float max_possible_baseline,
      int32_t start, int32_t end);

  Position GetAbsoluteOrFixedItemCrossAxisPosition(
      LayoutObject* absolute_or_fixed_item);
  Position GetAbsoluteOrFixedItemMainAxisPosition(
      LayoutObject* absolute_or_fixed_item);

  std::unique_ptr<FlexInfo> flex_info_;
};
}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_FLEX_LAYOUT_ALGORITHM_H_
