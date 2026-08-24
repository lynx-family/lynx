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
  const bool propagate_min_constraints =
      container_->GetCSSStyle()->GetXBoxPropagateMinConstraints();

  for (LayoutObject* child : inflow_items_) {
    if (MatchesParentSize(child)) {
      continue;
    }

    Constraints child_constraints = property_utils::GenerateDefaultConstraints(
        *child, container_constraints_);
    const bool final_measure = container_->GetFinalMeasure();
    DimensionValue<float> original_min_size;
    if (propagate_min_constraints) {
      const DimensionValue<float> propagated_min_constraints =
          PropagatedMinConstraintsForChild(*child);
      BoxInfo* child_box_info = child->GetBoxInfo();
      original_min_size = child_box_info->min_size_;
      child_box_info->min_size_[kHorizontal] =
          child->ClampExactWidth(propagated_min_constraints[kHorizontal]);
      child_box_info->min_size_[kVertical] =
          child->ClampExactHeight(propagated_min_constraints[kVertical]);
      child->ClearCache();
    }

    const FloatSize child_size =
        child->UpdateMeasure(child_constraints, final_measure);

    if (propagate_min_constraints) {
      child->GetBoxInfo()->min_size_ = original_min_size;
      child->ClearCache();
    }

    max_child_margin_bound_width_ =
        std::max(max_child_margin_bound_width_,
                 child_size.width_ + child->GetLayoutMarginLeft() +
                     child->GetLayoutMarginRight());
    max_child_margin_bound_height_ =
        std::max(max_child_margin_bound_height_,
                 child_size.height_ + child->GetLayoutMarginTop() +
                     child->GetLayoutMarginBottom());
  }
}

DimensionValue<float> BoxLayoutAlgorithm::PropagatedMinConstraintsForChild(
    const LayoutObject& child) const {
  DimensionValue<float> result{};
  for (Dimension dimension : {kHorizontal, kVertical}) {
    float min_size = 0.f;
    if (IsSLDefiniteMode(container_constraints_[dimension].Mode())) {
      min_size = container_constraints_[dimension].Size();
    } else {
      min_size = container_->GetBoxInfo()->min_size_[dimension] -
                 logic_direction_utils::GetPaddingAndBorderDimensionSize(
                     container_, dimension);
    }
    result[dimension] =
        std::max(property_utils::StripMargins(min_size, child, dimension), 0.f);
  }
  return result;
}

// Measure match-parent-size children after the container size is determined.
void BoxLayoutAlgorithm::MeasureAbsoluteAndFixed() {
  LayoutAlgorithm::MeasureAbsoluteAndFixed();
  MeasureMatchParentSizeChildren();
}

void BoxLayoutAlgorithm::MeasureMatchParentSizeChildren() {
  for (LayoutObject* child : inflow_items_) {
    if (!MatchesParentSize(child)) {
      continue;
    }

    Constraints child_constraints;
    child_constraints[kHorizontal] = OneSideConstraint::Definite(std::max(
        container_constraints_[kHorizontal].Size() -
            child->GetLayoutMarginLeft() - child->GetLayoutMarginRight(),
        0.f));
    child_constraints[kVertical] = OneSideConstraint::Definite(std::max(
        container_constraints_[kVertical].Size() - child->GetLayoutMarginTop() -
            child->GetLayoutMarginBottom(),
        0.f));
    child->UpdateMeasure(child_constraints, container_->GetFinalMeasure());
  }
}

bool BoxLayoutAlgorithm::MatchesParentSize(const LayoutObject* item) const {
  return item->GetCSSStyle()->GetXBoxMatchParentSize();
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
  const float available_space =
      logic_direction_utils::GetContentBoundDimensionSize(container_,
                                                          dimension) -
      logic_direction_utils::GetMarginBoundDimensionSize(item, dimension);

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

void BoxLayoutAlgorithm::SetContainerBaseline() {
  container_->SetBaseline(0.f);
}

}  // namespace starlight
}  // namespace lynx
