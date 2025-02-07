// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/relative_layout_algorithm.h"

#include <algorithm>
#include <queue>
#include <set>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

namespace {

inline bool IsHorizontalCenter(RelativeCenterType type) {
  return type == RelativeCenterType::kHorizontal ||
         type == RelativeCenterType::kBoth;
}

inline bool IsVerticalCenter(RelativeCenterType type) {
  return type == RelativeCenterType::kVertical ||
         type == RelativeCenterType::kBoth;
}

int GetAlignPhysicalStart(const LayoutComputedStyle& css, Dimension dimension) {
  if (dimension == kVertical) {
    return css.GetRelativeAlignTop();
  } else {
    return css.GetRelativeAlignLeft();
  }
}

int GetAlignPhysicalEnd(const LayoutComputedStyle& css, Dimension dimension) {
  if (dimension == kVertical) {
    return css.GetRelativeAlignBottom();
  } else {
    return css.GetRelativeAlignRight();
  }
}

inline bool IsCenterAlign(const LayoutComputedStyle& css, Dimension dimension) {
  if (dimension == kVertical) {
    return IsVerticalCenter(css.GetRelativeCenter());
  } else {
    return IsHorizontalCenter(css.GetRelativeCenter());
  }
}

using DependencyGetter = std::function<void(
    size_t idx, const LayoutComputedStyle* style,
    std::set<size_t>& item_dependencies,
    RelativeLayoutAlgorithm::InlineDependencies& reverse_dependencies)>;

void SortDependencies(const LayoutItems& items,
                      const DependencyGetter& dependency_getter,
                      RelativeLayoutAlgorithm::InlineOrders& order) {
  std::set<size_t> unsorted;
  RelativeLayoutAlgorithm::InlineDependencies dependencies(items.size());
  RelativeLayoutAlgorithm::InlineDependencies reverse_dependencies(
      items.size());

  for (size_t idx = 0; idx < items.size(); ++idx) {
    unsorted.insert(idx);
    const auto* style = items[idx]->GetCSSStyle();
    auto& item_dependencies = dependencies[idx];
    dependency_getter(idx, style, item_dependencies, reverse_dependencies);
  }
  std::queue<size_t> current_start;
  for (size_t idx = 0; idx < items.size(); ++idx) {
    if (dependencies[idx].empty()) {
      current_start.push(idx);
    }
  }

  for (size_t idx = 0; idx < items.size(); ++idx) {
    size_t current;
    if (current_start.empty()) {
      // Some circular dependency happen, pick the one with smallest index
      current = *(unsorted.begin());
    } else {
      current = current_start.front();
      current_start.pop();
    }
    order[idx] = current;
    unsorted.erase(current);
    for (const auto is_dependent_by : reverse_dependencies[current]) {
      auto& item_dependencies = dependencies[is_dependent_by];
      item_dependencies.erase(current);
      if (item_dependencies.empty() &&
          unsorted.find(is_dependent_by) != unsorted.end()) {
        current_start.push(is_dependent_by);
      }
    }
  }
}

}  // namespace

RelativeLayoutAlgorithm::RelativeLayoutAlgorithm(LayoutObject* container)
    : LayoutAlgorithm(container) {}

void RelativeLayoutAlgorithm::SizeDeterminationByAlgorithm() {
  ResetMinMaxPosition();

  // Algorithm-1
  UpdateChildrenSize();
}

void RelativeLayoutAlgorithm::AlignInFlowItems() {
  // Recompute proposed position

  ResetMinMaxPosition();
  RecomputeProposedPosition(horizontal_order_, kHorizontal, kLeft);
  RecomputeProposedPosition(vertical_order_, kVertical, kTop);

  for (size_t idx = 0; idx < inflow_items_.size(); ++idx) {
    DirectionValue<LayoutUnit> position;

    LayoutObject* item = inflow_items_[idx];

    logic_direction_utils::SetBoundOffsetFrom(item, kLeft, BoundType::kMargin,
                                              BoundType::kContent,
                                              proposed_position_[idx][kLeft]);
    logic_direction_utils::SetBoundOffsetFrom(item, kTop, BoundType::kMargin,
                                              BoundType::kContent,
                                              proposed_position_[idx][kTop]);
  }
}

void RelativeLayoutAlgorithm::UpdateChildrenSize() {
  const auto& MeasureWithOrder = [this](const InlineOrders& orders,
                                        Dimension dimension, bool do_once) {
    const size_t item_size = inflow_items_.size();
    for (size_t order = 0; order < item_size; ++order) {
      const size_t current_to_measure = orders[order];
      LayoutObject* child = inflow_items_[current_to_measure];
      DirectionValue<LayoutUnit> position_constraints;
      const auto child_constraints =
          ComputeConstraints(current_to_measure, position_constraints,
                             !do_once && (dimension != kVertical));
      FloatSize result;
      result = child->UpdateMeasure(child_constraints,
                                    container_->GetFinalMeasure());

      layout_results_[current_to_measure] = result;
      if (do_once) {
        ComputeProposedPositions(current_to_measure, position_constraints,
                                 result, kHorizontal);
        ComputeProposedPositions(current_to_measure, position_constraints,
                                 result, kVertical);
      } else {
        ComputeProposedPositions(current_to_measure, position_constraints,
                                 result, dimension);
      }
    }
  };

  if (container_style_->GetRelativeLayoutOnce()) {
    MeasureWithOrder(vertical_order_, kVertical, true);
    DetermineContainerSizeHorizontal();
    DetermineContainerSizeVertical();
  } else {
    // Measure with horizontal position constraints first
    MeasureWithOrder(horizontal_order_, kHorizontal, false);
    // Determine horizontal size
    DetermineContainerSizeHorizontal();
    ResetMinMaxPosition();
    max_position_[kHorizontal] = container_constraints_[kHorizontal].Size();
    RecomputeProposedPosition(horizontal_order_, kHorizontal, kLeft);
    // Measure with vertical position constraints
    MeasureWithOrder(vertical_order_, kVertical, false);
    RecomputeProposedPosition(vertical_order_, kVertical, kTop);
    DetermineContainerSizeVertical();
  }
}

void RelativeLayoutAlgorithm::DetermineContainerSizeHorizontal() {
  bool flag_change = false;
  if (!IsSLDefiniteMode(container_constraints_[kHorizontal].Mode())) {
    container_constraints_[kHorizontal] = OneSideConstraint::Definite(
        max_position_[kHorizontal] - min_position_[kHorizontal]);
    flag_change = true;
  }
  if (flag_change) UpdateContainerSize();
}

void RelativeLayoutAlgorithm::DetermineContainerSizeVertical() {
  bool flag_change = false;
  if (!IsSLDefiniteMode(container_constraints_[kVertical].Mode())) {
    container_constraints_[kVertical] = OneSideConstraint::Definite(
        max_position_[kVertical] - min_position_[kVertical]);
    flag_change = true;
  }
  if (flag_change) UpdateContainerSize();
}

void RelativeLayoutAlgorithm::UpdateContainerSize() {
  for (LayoutObject* item : inflow_items_) {
    item->GetBoxInfo()->UpdateBoxData(container_constraints_, *item,
                                      item->GetLayoutConfigs());
  }
}

void RelativeLayoutAlgorithm::InitializeAlgorithmEnv() {
  layout_results_.resize<true>(inflow_items_.size());
  proposed_position_.resize<true>(inflow_items_.size(),
                                  DirectionValue<float>({0.f, 0.f, 0.f, 0.f}));
  vertical_order_.resize<true>(inflow_items_.size());
  if (!container_style_->GetRelativeLayoutOnce()) {
    horizontal_order_.resize<true>(inflow_items_.size());
  }

  GenerateIDMap();
  Sort();
}

Constraints RelativeLayoutAlgorithm::ComputeConstraints(
    size_t idx, DirectionValue<LayoutUnit>& position_constraint,
    bool horizontal_only) const {
  // TODO: Aspect-ratio
  const auto& obj = *inflow_items_[idx];
  auto child_constraints = GenerateDefaultConstraint(obj);
  GetPositionConstraints(obj, position_constraint, horizontal_only);

  const auto ComputeOneSide = [&obj, &position_constraint, &child_constraints](
                                  Direction start, Direction end,
                                  Dimension dimension) {
    if (position_constraint[start].IsDefinite() &&
        position_constraint[end].IsDefinite()) {
      float constraint = position_constraint[end].ToFloat() -
                         position_constraint[start].ToFloat();
      constraint = property_utils::StripMargins(constraint, obj, dimension);
      child_constraints[dimension] =
          OneSideConstraint(constraint, SLMeasureModeDefinite);
    } else if ((position_constraint[start].IsDefinite() ||
                position_constraint[end].IsDefinite()) &&
               child_constraints[dimension].Mode() == SLMeasureModeAtMost) {
      if (position_constraint[start].IsDefinite()) {
        child_constraints[dimension] =
            OneSideConstraint::AtMost(child_constraints[dimension].Size() -
                                      position_constraint[start].ToFloat());
      } else {
        child_constraints[dimension] =
            OneSideConstraint::AtMost(position_constraint[end].ToFloat());
      }
    }
  };

  if (container_style_->GetRelativeLayoutOnce()) {
    ComputeOneSide(kLeft, kRight, kHorizontal);
    ComputeOneSide(kTop, kBottom, kVertical);
    property_utils::ApplyAspectRatio(&obj, child_constraints);

  } else {
    if (horizontal_only) {
      ComputeOneSide(kLeft, kRight, kHorizontal);
    } else {
      position_constraint[kLeft] = proposed_position_[idx][kLeft];
      position_constraint[kRight] = proposed_position_[idx][kRight];
      child_constraints[kHorizontal] =
          OneSideConstraint::Definite(property_utils::StripMargins(
              proposed_position_[idx][kRight] - proposed_position_[idx][kLeft],
              obj, kHorizontal));
      ComputeOneSide(kTop, kBottom, kVertical);
      property_utils::ApplyAspectRatio(&obj, child_constraints);
    }
  }

  return child_constraints;
}

void RelativeLayoutAlgorithm::GetPositionConstraints(
    const LayoutObject& obj, DirectionValue<LayoutUnit>& position_constraint,
    bool horizontal_only) const {
  for (int direction = 0; direction < kDirectionCount; ++direction) {
    if (!horizontal_only || (direction != kBottom && direction != kTop)) {
      position_constraint[direction] =
          GetPositionConstraints(obj, static_cast<Direction>(direction));
    }
  }
}

LayoutUnit RelativeLayoutAlgorithm::GetPositionConstraints(
    const LayoutObject& obj, Direction direction) const {
  const LayoutComputedStyle& css = *(obj.GetCSSStyle());
  const auto& ResolveOneDependency = [this](
                                         int align_id, Direction align_side,
                                         Dimension parent_side) -> LayoutUnit {
    if (align_id != RelativeAlignType::kNone) {
      if (align_id != RelativeAlignType::kParent) {
        auto iter = id_map_.find(align_id);
        if (iter != id_map_.end()) {
          return LayoutUnit(proposed_position_[iter->second][align_side]);
        }
      } else {
        if (container_constraints_[parent_side].Mode() ==
            SLMeasureModeDefinite) {
          if (align_side == kLeft || align_side == kTop) {
            return LayoutUnit(0.f);
          } else {
            return LayoutUnit(container_constraints_[parent_side].Size());
          }
        }
      }
    }
    return LayoutUnit();
  };

  LayoutUnit result;

  switch (direction) {
    case kLeft:
      result =
          ResolveOneDependency(css.GetRelativeAlignLeft(), kLeft, kHorizontal);
      if (result.IsIndefinite()) {
        result =
            ResolveOneDependency(css.GetRelativeRightOf(), kRight, kHorizontal);
      }
      return result;
    case kRight:
      result = ResolveOneDependency(css.GetRelativeAlignRight(), kRight,
                                    kHorizontal);
      if (result.IsIndefinite()) {
        result =
            ResolveOneDependency(css.GetRelativeLeftOf(), kLeft, kHorizontal);
      }
      return result;
    case kTop:
      result = ResolveOneDependency(css.GetRelativeAlignTop(), kTop, kVertical);
      if (result.IsIndefinite()) {
        result =
            ResolveOneDependency(css.GetRelativeBottomOf(), kBottom, kVertical);
      }
      return result;
    case kBottom:
      result = ResolveOneDependency(css.GetRelativeAlignBottom(), kBottom,
                                    kVertical);
      if (result.IsIndefinite()) {
        result = ResolveOneDependency(css.GetRelativeTopOf(), kTop, kVertical);
      }
      return result;
    default:
      NOTREACHED();
  }

  return result;
}

void RelativeLayoutAlgorithm::ComputeProposedPositions(
    size_t idx, const DirectionValue<LayoutUnit>& position_constraint,
    const FloatSize& layout_result, Dimension dimension) {
  float size_with_margin =
      dimension == kHorizontal
          ? inflow_items_[idx]->GetOuterWidthFromBorderBoxWidth(
                layout_result.width_)
          : inflow_items_[idx]->GetOuterHeightFromBorderBoxHeight(
                layout_result.height_);

  // Process align constraints
  auto& position = proposed_position_[idx];
  const Direction start =
      logic_direction_utils::DimensionPhysicalStart(dimension);
  const Direction end = logic_direction_utils::DimensionPhysicalEnd(dimension);

  const auto& css = *(inflow_items_[idx]->GetCSSStyle());

  ComputePosition(css, dimension, size_with_margin, position_constraint,
                  position);

  // Update min/max side
  if (!IsSLDefiniteMode(container_constraints_[dimension].Mode())) {
    min_position_[dimension] =
        std::min(min_position_[dimension], position[start]);
    max_position_[dimension] =
        std::max(max_position_[dimension], position[end]);
  }
}

void RelativeLayoutAlgorithm::GenerateIDMap() {
  for (size_t idx = 0; idx < inflow_items_.size(); ++idx) {
    int id = inflow_items_[idx]->GetCSSStyle()->GetRelativeId();
    if (id != DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ID) {
      id_map_[id] = idx;
    }
  }
}

void RelativeLayoutAlgorithm::AddDependencyForID(
    size_t idx, int id, std::set<size_t>& item_dependencies,
    InlineDependencies& reverse_dependencies, Dimension dimension) const {
  const auto entry = id_map_.find(id);
  if (entry != id_map_.end()) {
    item_dependencies.insert(entry->second);
    reverse_dependencies[entry->second].insert(idx);
  }
}

void RelativeLayoutAlgorithm::AddDependencyForIDVertical(
    size_t idx, const LayoutComputedStyle* style,
    std::set<size_t>& item_dependencies,
    InlineDependencies& reverse_dependencies) const {
  AddDependencyForID(idx, style->GetRelativeTopOf(), item_dependencies,
                     reverse_dependencies, kVertical);
  AddDependencyForID(idx, style->GetRelativeBottomOf(), item_dependencies,
                     reverse_dependencies, kVertical);
  AddDependencyForID(idx, style->GetRelativeAlignTop(), item_dependencies,
                     reverse_dependencies, kVertical);
  AddDependencyForID(idx, style->GetRelativeAlignBottom(), item_dependencies,
                     reverse_dependencies, kVertical);
}

void RelativeLayoutAlgorithm::AddDependencyForIDHorizontal(
    size_t idx, const LayoutComputedStyle* style,
    std::set<size_t>& item_dependencies,
    InlineDependencies& reverse_dependencies) const {
  AddDependencyForID(idx, style->GetRelativeRightOf(), item_dependencies,
                     reverse_dependencies, kHorizontal);
  AddDependencyForID(idx, style->GetRelativeLeftOf(), item_dependencies,
                     reverse_dependencies, kHorizontal);
  AddDependencyForID(idx, style->GetRelativeAlignLeft(), item_dependencies,
                     reverse_dependencies, kHorizontal);
  AddDependencyForID(idx, style->GetRelativeAlignRight(), item_dependencies,
                     reverse_dependencies, kHorizontal);
}

void RelativeLayoutAlgorithm::Sort() {
  if (container_style_->GetRelativeLayoutOnce()) {
    const auto& dependency_getter =
        [this](size_t idx, const LayoutComputedStyle* style,
               std::set<size_t>& item_dependencies,
               InlineDependencies& reverse_dependencies) {
          AddDependencyForIDHorizontal(idx, style, item_dependencies,
                                       reverse_dependencies);
          AddDependencyForIDVertical(idx, style, item_dependencies,
                                     reverse_dependencies);
        };
    SortDependencies(inflow_items_, dependency_getter, vertical_order_);
    horizontal_order_ = vertical_order_;

  } else {
    const auto& dependency_getter_horizontal =
        [this](size_t idx, const LayoutComputedStyle* style,
               std::set<size_t>& item_dependencies,
               InlineDependencies& reverse_dependencies) {
          AddDependencyForIDHorizontal(idx, style, item_dependencies,
                                       reverse_dependencies);
        };

    const auto& dependency_getter_vertical =
        [this](size_t idx, const LayoutComputedStyle* style,
               std::set<size_t>& item_dependencies,
               InlineDependencies& reverse_dependencies) {
          AddDependencyForIDVertical(idx, style, item_dependencies,
                                     reverse_dependencies);
        };

    SortDependencies(inflow_items_, dependency_getter_horizontal,
                     horizontal_order_);
    SortDependencies(inflow_items_, dependency_getter_vertical,
                     vertical_order_);
  }
}

void RelativeLayoutAlgorithm::ComputePosition(
    const LayoutComputedStyle& css, Dimension dimension,
    const float size_with_margin,
    const DirectionValue<LayoutUnit>& position_constraint,
    DirectionValue<float>& position) const {
  const Direction start =
      logic_direction_utils::DimensionPhysicalStart(dimension);
  const Direction end = logic_direction_utils::DimensionPhysicalEnd(dimension);

  if (position_constraint[start].IsDefinite()) {
    position[start] = position_constraint[start].ToFloat();
  }
  if (position_constraint[end].IsDefinite()) {
    position[end] = position_constraint[end].ToFloat();
  }

  if (position_constraint[start].IsIndefinite() &&
      position_constraint[end].IsDefinite()) {
    // End is definite but start is not definite
    position[start] = position[end] - size_with_margin;

  } else if (position_constraint[end].IsIndefinite() &&
             position_constraint[start].IsDefinite()) {
    // Start is definite but end is not definite
    position[end] = position[start] + size_with_margin;

  } else if (position_constraint[start].IsIndefinite() &&
             position_constraint[end].IsIndefinite()) {
    // Neither size is defined
    if (GetAlignPhysicalEnd(css, dimension) == RelativeAlignType::kParent) {
      // Align to parent end, while parent size is wrap content
      position[end] = max_position_[dimension];
      position[start] = position[end] - size_with_margin;

    } else if (GetAlignPhysicalStart(css, dimension) ==
                   RelativeAlignType::kParent ||
               !IsCenterAlign(css, dimension)) {
      // Default or align to parent start, while parent size is wrap content
      position[start] = min_position_[dimension];
      position[end] = position[start] + size_with_margin;

    } else {
      // Center case
      position[start] = min_position_[dimension] +
                        (max_position_[dimension] - min_position_[dimension] -
                         size_with_margin) /
                            2.f;
      position[end] = position[start] + size_with_margin;
    }
  }
}

void RelativeLayoutAlgorithm::ResetMinMaxPosition() {
  min_position_[kVertical] = min_position_[kHorizontal] = 0.f;
  if (container_constraints_[kVertical].Mode() == SLMeasureModeDefinite) {
    max_position_[kVertical] = container_constraints_[kVertical].Size();
  } else {
    max_position_[kVertical] = 0.f;
  }
  if (container_constraints_[kHorizontal].Mode() == SLMeasureModeDefinite) {
    max_position_[kHorizontal] = container_constraints_[kHorizontal].Size();
  } else {
    max_position_[kHorizontal] = 0.f;
  }
}

void RelativeLayoutAlgorithm::RecomputeProposedPosition(
    const InlineOrders& orders, Dimension dimension, Direction start) {
  for (size_t order = 0; order < inflow_items_.size(); ++order) {
    DirectionValue<LayoutUnit> position;

    const auto idx = orders[order];
    LayoutObject* item = inflow_items_[idx];
    // recompute final position
    GetPositionConstraints(*item, position, dimension != kVertical);
    ComputeProposedPositions(idx, position, layout_results_[idx], dimension);
  }
}

}  // namespace starlight
}  // namespace lynx
