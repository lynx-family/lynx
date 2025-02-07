// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_RELATIVE_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_RELATIVE_LAYOUT_ALGORITHM_H_

#include <map>
#include <set>

#include "core/renderer/starlight/layout/layout_algorithm.h"

namespace lynx {
namespace starlight {
class LayoutObject;
class LayoutComputedStyle;

class RelativeLayoutAlgorithm : public LayoutAlgorithm {
 public:
  using InlineOrders = base::InlineVector<size_t, kChildrenInlineVectorSize>;
  using InlineDependencies =
      base::InlineVector<std::set<size_t>, kChildrenInlineVectorSize>;

  RelativeLayoutAlgorithm(LayoutObject*);
  ~RelativeLayoutAlgorithm() = default;

  void SizeDeterminationByAlgorithm() override;
  void AlignInFlowItems() override;

  void InitializeAlgorithmEnv() override;
  void SetContainerBaseline() override{};

 private:
  void UpdateChildrenSize();

  void DetermineContainerSizeHorizontal();
  void DetermineContainerSizeVertical();

  void UpdateContainerSize();

  void GenerateIDMap();
  void Sort();

  Constraints ComputeConstraints(
      size_t idx, DirectionValue<LayoutUnit>& position_constraint,
      bool horizontal_only) const;

  LayoutUnit GetPositionConstraints(const LayoutObject& obj,
                                    Direction direction) const;
  void GetPositionConstraints(const LayoutObject& obj,
                              DirectionValue<LayoutUnit>& position_constraint,
                              bool horizontal_only) const;

  void ComputeProposedPositions(
      size_t idx, const DirectionValue<LayoutUnit>& position_constraint,
      const FloatSize& layout_result, Dimension dimension);

  void ComputePosition(const LayoutComputedStyle& css, Dimension dimension,
                       const float size_with_margin,
                       const DirectionValue<LayoutUnit>& position_constraint,
                       DirectionValue<float>& position) const;
  void ResetMinMaxPosition();

  void AddDependencyForID(size_t idx, int id,
                          std::set<size_t>& item_dependencies,
                          InlineDependencies& reverse_dependencies,
                          Dimension dimension) const;

  void AddDependencyForIDVertical(
      size_t idx, const LayoutComputedStyle* style,
      std::set<size_t>& item_dependencies,
      InlineDependencies& reverse_dependencies) const;

  void AddDependencyForIDHorizontal(
      size_t idx, const LayoutComputedStyle* style,
      std::set<size_t>& item_dependencies,
      InlineDependencies& reverse_dependencies) const;

  void RecomputeProposedPosition(const InlineOrders& orders,
                                 Dimension dimension, Direction start);

  base::InlineVector<DirectionValue<float>, kChildrenInlineVectorSize>
      proposed_position_;
  DimensionValue<float> min_position_;
  DimensionValue<float> max_position_;
  std::map<int, size_t> id_map_;
  InlineOrders horizontal_order_;
  InlineOrders vertical_order_;
  base::InlineVector<FloatSize, kChildrenInlineVectorSize> layout_results_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_RELATIVE_LAYOUT_ALGORITHM_H_
