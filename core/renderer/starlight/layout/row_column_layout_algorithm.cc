// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/row_column_layout_algorithm.h"

#include <algorithm>
#include <cmath>

#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"

namespace lynx {
namespace starlight {

using logic_direction_utils::SizeDimension;

void RowColumnLayoutAlgorithm::Reset() {
  InitializeChildren(nullptr);
  InitializeAlgorithmEnv();
}

void RowColumnLayoutAlgorithm::InitializeChildren(const SLNodeSet*) {
  children_.clear();
  inflow_items_.clear();
  children_.reserve(container_->GetChildCount());
  inflow_items_.reserve(container_->GetChildCount());
  // Compose consumes children in source order; visibility follows tree
  // membership.
  for (auto* node = container_->FirstChild(); node; node = node->Next()) {
    inflow_items_.push_back(static_cast<LayoutObject*>(node));
    children_.push_back(Child{});
  }
}

void RowColumnLayoutAlgorithm::InitializeAlgorithmEnv() {
  DCHECK(compose_constraints_.has_value());
  container_style_ = container_->GetCSSStyle();
  static_cast<DirectionSelector&>(*this) =
      DirectionSelector(container_style_->IsRow(container_->GetLayoutConfigs(),
                                                container_->attr_map()),
                        false, container_style_->IsAnyRtl());
  before_alignment_line_ = 0;
  const auto arrangement = container_style_->GetJustifyContent();
  const bool distributed = arrangement == JustifyContentType::kSpaceBetween ||
                           arrangement == JustifyContentType::kSpaceAround ||
                           arrangement == JustifyContentType::kSpaceEvenly;
  const float main_max = IsHorizontal() ? compose_constraints_->max_width
                                        : compose_constraints_->max_height;
  // Arrangement's distributed variants have spacing == 0. A fixed gap maps
  // to spacedBy(space, alignment), whose alignment is Start, Center or End.
  spacing_ = distributed
                 ? 0
                 : NLengthToLayoutUnit(
                       IsHorizontal() ? container_style_->GetGridColumnGap()
                                      : container_style_->GetGridRowGap(),
                       std::isfinite(main_max) ? LayoutUnit(main_max)
                                               : LayoutUnit::Indefinite())
                       .ClampIndefiniteToZero()
                       .ToFloat();
}

void RowColumnLayoutAlgorithm::SizeDeterminationByAlgorithm() {
  const auto size = MeasureChildren();
  container_constraints_[kHorizontal] = OneSideConstraint::Definite(
      compose_constraints_->ConstrainWidth(size.width_));
  container_constraints_[kVertical] = OneSideConstraint::Definite(
      compose_constraints_->ConstrainHeight(size.height_));
  ArrangeChildren();
}

void RowColumnLayoutAlgorithm::SetContainerBaseline() {
  container_->SetBaseline(0.f);
}

void RowColumnLayoutAlgorithm::MeasureChild(size_t index, float main_min,
                                            float main_max, float cross_max) {
  const ComposeConstraints constraints =
      IsHorizontal() ? ComposeConstraints{main_min, main_max, 0, cross_max}
                     : ComposeConstraints{0, cross_max, main_min, main_max};
  children_[index].size =
      inflow_items_[index]->UpdateMeasureWithComposeConstraints(
          constraints, container_->GetFinalMeasure());
}

FloatSize RowColumnLayoutAlgorithm::MeasureChildren() {
  const auto& constraints = *compose_constraints_;
  const float main_min =
      IsHorizontal() ? constraints.min_width : constraints.min_height;
  // Native scrolling is hosted on this node rather than a separate Compose
  // Scroll modifier. Match ScrollNode: unbound the content's scrolling axis;
  // shared postprocessing still constrains the viewport to the original range.
  const float main_max =
      container_->attr_map().getScroll().value_or(false)
          ? ComposeConstraints::kInfinity
          : (IsHorizontal() ? constraints.max_width : constraints.max_height);
  const float cross_min =
      IsHorizontal() ? constraints.min_height : constraints.min_width;
  const float cross_max =
      IsHorizontal() ? constraints.max_height : constraints.max_width;
  float total_weight = 0;
  float fixed_space = 0;
  float last_spacing = 0;
  size_t weighted_count = 0;

  // RowColumnMeasurePolicy.measure: nonweighted children consume the remaining
  // maximum in source order. A weighted child is not measured in this pass.
  for (size_t i = 0; i < children_.size(); ++i) {
    const float weight = inflow_items_[i]->GetCSSStyle()->GetLinearWeight();
    if (weight > 0) {
      total_weight += weight;
      ++weighted_count;
    } else {
      const float remaining = main_max - fixed_space;
      MeasureChild(i, 0, std::max(0.f, remaining), cross_max);
      const float size = SizeDimension(children_[i].size, MainAxis());
      last_spacing = std::min(spacing_, std::max(0.f, remaining - size));
      fixed_space += size + last_spacing;
    }
  }

  float weighted_space = 0;
  if (weighted_count == 0) {
    fixed_space -= last_spacing;
  } else {
    const float target = std::isfinite(main_max) ? main_max : main_min;
    const float weighted_spacing = spacing_ * (weighted_count - 1);
    const float remaining =
        std::max(0.f, target - fixed_space - weighted_spacing);
    const float weight_unit = remaining / total_weight;
    for (size_t i = 0; i < children_.size(); ++i) {
      const auto* style = inflow_items_[i]->GetCSSStyle();
      const float weight = style->GetLinearWeight();
      if (weight <= 0) continue;
      const float allocation = weight_unit * weight;
      MeasureChild(i, allocation, allocation, cross_max);
      weighted_space += SizeDimension(children_[i].size, MainAxis());
    }
    weighted_space = std::max(0.f, std::min(weighted_space + weighted_spacing,
                                            main_max - fixed_space));
  }

  float cross_space = cross_min;
  float after_alignment_line = 0;
  for (size_t i = 0; i < children_.size(); ++i) {
    const auto& size = children_[i].size;
    cross_space = std::max(cross_space, SizeDimension(size, CrossAxis()));
    if (IsHorizontal() &&
        CrossAlignment(*inflow_items_[i]) == FlexAlignType::kBaseline) {
      before_alignment_line_ = std::max(before_alignment_line_, size.baseline_);
      after_alignment_line =
          std::max(after_alignment_line, size.height_ - size.baseline_);
    }
  }
  main_layout_size_ =
      std::max(main_min, std::max(0.f, fixed_space + weighted_space));
  FloatSize size;
  SizeDimension(size, MainAxis()) = main_layout_size_;
  SizeDimension(size, CrossAxis()) =
      std::max(cross_space, before_alignment_line_ + after_alignment_line);
  return size;
}

void RowColumnLayoutAlgorithm::ArrangeChildren() {
  if (children_.empty()) return;
  const float total = main_layout_size_;
  const bool rtl = IsHorizontal() && container_style_->IsAnyRtl();
  const auto arrangement = container_style_->GetJustifyContent();
  if (spacing_ != 0) {
    // Arrangement.SpacedAligned uses source order in both directions. In RTL,
    // clipping each gap at the right edge differs from reversing the input.
    float cursor = rtl ? total : 0;
    float last_spacing = 0;
    for (auto& child : children_) {
      const float size = SizeDimension(child.size, MainAxis());
      if (rtl) {
        child.main_position = std::max(0.f, cursor - size);
        last_spacing = std::min(spacing_, child.main_position);
        cursor = child.main_position - last_spacing;
      } else {
        child.main_position = std::min(cursor, total - size);
        last_spacing = std::min(spacing_, total - child.main_position - size);
        cursor = child.main_position + size + last_spacing;
      }
    }
    const float free =
        rtl ? cursor + last_spacing : total - cursor + last_spacing;
    if (free > 0) {
      float offset = 0;
      if (arrangement == JustifyContentType::kCenter) {
        offset = free / 2 - (rtl ? free : 0);
      } else if (arrangement == JustifyContentType::kFlexEnd) {
        offset = rtl ? -free : free;
      }
      for (auto& child : children_) child.main_position += offset;
    }
    return;
  }

  float consumed = 0;
  for (const auto& child : children_) {
    consumed += SizeDimension(child.size, MainAxis());
  }
  const float free = total - consumed;
  float current = rtl ? free : 0;
  float gap = 0;
  switch (arrangement) {
    case JustifyContentType::kCenter:
      current = free / 2;
      break;
    case JustifyContentType::kFlexEnd:
      current = rtl ? 0 : free;
      break;
    case JustifyContentType::kSpaceBetween:
      gap = free / std::max<size_t>(children_.size() - 1, 1);
      current = rtl && children_.size() == 1 ? gap : 0;
      break;
    case JustifyContentType::kSpaceAround:
      gap = free / children_.size();
      current = gap / 2;
      break;
    case JustifyContentType::kSpaceEvenly:
      gap = free / (children_.size() + 1);
      current = gap;
      break;
    default:
      break;
  }
  for (size_t i = 0; i < children_.size(); ++i) {
    auto& child = children_[rtl ? children_.size() - i - 1 : i];
    child.main_position = current;
    current += SizeDimension(child.size, MainAxis()) + gap;
  }
}

FlexAlignType RowColumnLayoutAlgorithm::CrossAlignment(
    const LayoutObject& child) const {
  const auto align = child.GetCSSStyle()->GetAlignSelf();
  return align == FlexAlignType::kAuto ? container_style_->GetAlignItems()
                                       : align;
}

float RowColumnLayoutAlgorithm::CrossPosition(size_t index) const {
  const auto& size = children_[index].size;
  const float available = container_constraints_[CrossAxis()].Size() -
                          SizeDimension(size, CrossAxis());
  const bool rtl = !IsHorizontal() && container_style_->IsAnyRtl();
  switch (CrossAlignment(*inflow_items_[index])) {
    case FlexAlignType::kCenter:
      return available / 2;
    case FlexAlignType::kEnd:
    case FlexAlignType::kFlexEnd:
      return rtl ? 0 : available;
    case FlexAlignType::kBaseline:
      if (IsHorizontal()) return before_alignment_line_ - size.baseline_;
      break;
    default:
      break;
  }
  // Compose defaults to Top/Start; CSS stretch does not stretch these children.
  return rtl ? available : 0;
}

void RowColumnLayoutAlgorithm::AlignInFlowItems() {
  for (size_t i = 0; i < children_.size(); ++i) {
    const float main = children_[i].main_position;
    const float cross = CrossPosition(i);
    inflow_items_[i]->SetBoundLeftFrom(container_,
                                       IsHorizontal() ? main : cross,
                                       BoundType::kMargin, BoundType::kContent);
    inflow_items_[i]->SetBoundTopFrom(container_, IsHorizontal() ? cross : main,
                                      BoundType::kMargin, BoundType::kContent);
  }
}

void RowColumnLayoutAlgorithm::Alignment() {
  // Compose placement owns every child; CSS offsets and sticky/fixed
  // positioning must not modify the positions produced by the Compose
  // arrangement.
  AlignInFlowItems();
  for (auto* child : inflow_items_) child->UpdateAlignment();
}

}  // namespace starlight
}  // namespace lynx
