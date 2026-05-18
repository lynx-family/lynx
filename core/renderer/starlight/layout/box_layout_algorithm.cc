// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/box_layout_algorithm.h"

#include <algorithm>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {

BoxLayoutAlgorithm::BoxLayoutAlgorithm(LayoutObject* container)
    : LayoutAlgorithm(container) {}

void BoxLayoutAlgorithm::Reset() {
  max_child_margin_bound_width_ = 0.f;
  max_child_margin_bound_height_ = 0.f;
}

void BoxLayoutAlgorithm::InitializeAlgorithmEnv() { Reset(); }

void BoxLayoutAlgorithm::SizeDeterminationByAlgorithm() {
  MeasureChildren();
  DetermineContainerSize();
}

void BoxLayoutAlgorithm::MeasureChildren() {
  for (LayoutObject* child : inflow_items_) {
    Constraints child_constraints = property_utils::GenerateDefaultConstraints(
        *child, container_constraints_);
    MeasureInFlowItem(child, child_constraints, container_->GetFinalMeasure());

    max_child_margin_bound_width_ = std::max(
        max_child_margin_bound_width_,
        logic_direction_utils::GetMarginBoundDimensionSize(child, kHorizontal));
    max_child_margin_bound_height_ = std::max(
        max_child_margin_bound_height_,
        logic_direction_utils::GetMarginBoundDimensionSize(child, kVertical));
  }
}

void BoxLayoutAlgorithm::DetermineContainerSize() {
  if (!IsSLDefiniteMode(container_constraints_[kHorizontal].Mode())) {
    container_constraints_[kHorizontal] =
        OneSideConstraint::Definite(ResolveAutoSize(kHorizontal));
  }

  if (!IsSLDefiniteMode(container_constraints_[kVertical].Mode())) {
    container_constraints_[kVertical] =
        OneSideConstraint::Definite(ResolveAutoSize(kVertical));
  }

  for (LayoutObject* item : inflow_items_) {
    item->GetBoxInfo()->UpdateBoxData(container_constraints_, *item,
                                      item->GetLayoutConfigs());
  }
}

float BoxLayoutAlgorithm::ResolveAutoSize(Dimension dimension) const {
  float size = dimension == kHorizontal ? max_child_margin_bound_width_
                                        : max_child_margin_bound_height_;
  if (IsSLAtMostMode(container_constraints_[dimension].Mode())) {
    size = std::min(size, container_constraints_[dimension].Size());
  }
  return property_utils::ApplyMinMaxToSpecificSize(size, container_, dimension);
}

void BoxLayoutAlgorithm::AlignInFlowItems() {
  for (LayoutObject* item : inflow_items_) {
    logic_direction_utils::SetBoundOffsetFrom(
        item, HorizontalFront(), BoundType::kMargin, BoundType::kContent,
        ResolveAlignmentOffset(item, kHorizontal));
    logic_direction_utils::SetBoundOffsetFrom(
        item, VerticalFront(), BoundType::kMargin, BoundType::kContent,
        ResolveAlignmentOffset(item, kVertical));
  }
}

float BoxLayoutAlgorithm::ResolveAlignmentOffset(LayoutObject* item,
                                                 Dimension dimension) const {
  float available_space =
      AvailableContentSize(dimension) - ItemMarginBoundSize(item, dimension);
  available_space = std::max(available_space, 0.f);

  if (dimension == kHorizontal) {
    JustifyType justify = item->GetCSSStyle()->GetJustifySelfType();
    if (justify == JustifyType::kAuto) {
      justify = container_style_->GetJustifyItemsType();
    }

    switch (justify) {
      case JustifyType::kCenter:
        return available_space / 2.f;
      case JustifyType::kEnd:
        return available_space;
      case JustifyType::kAuto:
      case JustifyType::kStretch:
      case JustifyType::kStart:
        return 0.f;
    }
  }

  FlexAlignType align = item->GetCSSStyle()->GetAlignSelf();
  if (align == FlexAlignType::kAuto) {
    align = container_style_->GetAlignItems();
  }

  switch (align) {
    case FlexAlignType::kCenter:
      return available_space / 2.f;
    case FlexAlignType::kEnd:
    case FlexAlignType::kFlexEnd:
      return available_space;
    case FlexAlignType::kAuto:
    case FlexAlignType::kStretch:
    case FlexAlignType::kStart:
    case FlexAlignType::kFlexStart:
    case FlexAlignType::kBaseline:
      return 0.f;
  }

  return 0.f;
}

float BoxLayoutAlgorithm::AvailableContentSize(Dimension dimension) const {
  return logic_direction_utils::GetContentBoundDimensionSize(container_,
                                                             dimension);
}

float BoxLayoutAlgorithm::ItemMarginBoundSize(LayoutObject* item,
                                              Dimension dimension) const {
  return logic_direction_utils::GetMarginBoundDimensionSize(item, dimension);
}

void BoxLayoutAlgorithm::SetContainerBaseline() {
  container_->SetBaseline(0.f);
}

}  // namespace starlight
}  // namespace lynx
