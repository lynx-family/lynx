// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/box_layout_algorithm.h"

#include <algorithm>

#include "base/include/float_comparison.h"
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
      container_->attr_map().getPropagateMinConstraints().value_or(false);

  for (LayoutObject* child : inflow_items_) {
    if (MatchesParentSize(child)) {
      continue;
    }

    Constraints child_constraints = property_utils::GenerateDefaultConstraints(
        *child, container_constraints_);
    const bool final_measure = container_->GetFinalMeasure();
    // Resolve propagated minimums with tentative measurements so that the
    // first final measurement receives the stabilized constraints.
    FloatSize child_size = child->UpdateMeasure(
        child_constraints, propagate_min_constraints ? false : final_measure);

    if (propagate_min_constraints) {
      const DimensionValue<float> propagated_min_constraints =
          PropagatedMinConstraintsForChild(*child);
      bool width_clamped = false;
      bool height_clamped = false;
      for (int pass = 0; pass < 2; ++pass) {
        bool needs_remeasure = false;
        if (!width_clamped &&
            base::FloatsLarger(propagated_min_constraints[kHorizontal],
                               child_size.width_)) {
          const float target_width =
              child->ClampExactWidth(propagated_min_constraints[kHorizontal]);
          if (base::FloatsNotEqual(target_width, child_size.width_)) {
            child_constraints[kHorizontal] =
                OneSideConstraint::Definite(target_width);
            width_clamped = true;
            needs_remeasure = true;
          }
        }
        if (!height_clamped &&
            base::FloatsLarger(propagated_min_constraints[kVertical],
                               child_size.height_)) {
          const float target_height =
              child->ClampExactHeight(propagated_min_constraints[kVertical]);
          if (base::FloatsNotEqual(target_height, child_size.height_)) {
            child_constraints[kVertical] =
                OneSideConstraint::Definite(target_height);
            height_clamped = true;
            needs_remeasure = true;
          }
        }
        if (!needs_remeasure) {
          break;
        }
        const bool has_unresolved_propagated_min =
            (!width_clamped &&
             base::FloatsLarger(propagated_min_constraints[kHorizontal],
                                0.f)) ||
            (!height_clamped &&
             base::FloatsLarger(propagated_min_constraints[kVertical], 0.f));
        if (final_measure && !has_unresolved_propagated_min) {
          break;
        }
        child_size = child->UpdateMeasure(child_constraints, false);
      }

      if (final_measure) {
        // A tentative measurement may have cached the same constraints.
        child->ClearCache();
        child_size = child->UpdateMeasure(child_constraints, true);
      }
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
  // Explicitly stretching both axes opts an item out of the box's automatic
  // size and defers its measurement until the box size has been resolved.
  const LayoutComputedStyle* style = item->GetCSSStyle();
  return style->GetJustifySelfType() == JustifyType::kStretch &&
         style->GetAlignSelf() == FlexAlignType::kStretch;
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
      AvailableContentSize(dimension) - ItemMarginBoundSize(item, dimension);

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
