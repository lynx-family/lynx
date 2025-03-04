// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/linear_layout_algorithm.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/float_comparison.h"
#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/elastic_layout_utils.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"

namespace lynx {
namespace starlight {

LinearLayoutAlgorithm::LinearLayoutAlgorithm(LayoutObject* container)
    : LayoutAlgorithm(container),
      total_main_size_(0),
      total_cross_size_(0),
      remaining_size_(0),
      baseline_(0) {}

void LinearLayoutAlgorithm::Reset() {
  std::fill(main_size_.begin(), main_size_.end(), -1);
  std::fill(cross_size_.begin(), cross_size_.end(), -1);
  total_main_size_ = 0;
  total_cross_size_ = 0;
  remaining_size_ = 0;
  baseline_ = 0;
}

void LinearLayoutAlgorithm::SizeDeterminationByAlgorithm() {
  // Algorithm-1
  DetermineItemSize();
  // Algorithm-2
  DetermineContainerSize();
}

/* Algorithm-1
 * Traverse each item once and measure the size.
 */
void LinearLayoutAlgorithm::DetermineItemSize() {
  const float weight_sum = container_style_->GetLinearWeightSum();
  bool weight_enabled = true;
  if (container_->GetLayoutConfigs().IsFullQuirksMode()) {
    weight_enabled = base::FloatsLarger(weight_sum, 0.f);
  }
  const size_t item_size = inflow_items_.size();
  InlineFloatArray bases(item_size, 0.f);

  for (size_t idx = 0; idx < item_size; ++idx) {
    if (base::FloatsLargerOrEqual(
            0.0f, inflow_items_[idx]->GetCSSStyle()->GetLinearWeight()) ||
        !weight_enabled ||
        !IsSLDefiniteMode(container_constraints_[kMainAxis].Mode())) {
      UpdateChildSize(idx);
      bases[idx] = main_size_[idx];
    }
  }
  if (IsSLDefiniteMode(container_constraints_[kMainAxis].Mode()) &&
      weight_enabled) {
    LayoutWeightedChildren(bases);
  }

  for (size_t idx = 0; idx < item_size; ++idx) {
    const FourValue& margin = inflow_items_[idx]->GetBoxInfo()->margin_;
    total_main_size_ +=
        main_size_[idx] + margin[MainFront()] + margin[MainBack()];
    total_cross_size_ =
        std::max(total_cross_size_,
                 cross_size_[idx] + margin[CrossFront()] + margin[CrossBack()]);
  }
}

void LinearLayoutAlgorithm::LayoutWeightedChildren(
    const InlineFloatArray& base_sizes) {
  const float weight_sum = container_style_->GetLinearWeightSum();
  const size_t item_size = inflow_items_.size();
  InlineFloatArray hypothetical_size(item_size, 0.f);
  ElasticLayoutUtils::ComputeHypotheticalSizes(inflow_items_, base_sizes, *this,
                                               hypothetical_size);
  ElasticLayoutUtils::ElasticInfos infos(inflow_items_, base_sizes,
                                         hypothetical_size, true, *this, 0,
                                         item_size, 0.f);

  if (base::FloatsLarger(weight_sum, 0.f)) {
    infos.total_elastic_factor_override_ = weight_sum;
  }

  const auto weight_getter = [](const LayoutObject& item) {
    return item.GetCSSStyle()->GetLinearWeight();
  };
  ElasticLayoutUtils::ComputeElasticItemSizes(
      infos, container_constraints_[MainAxis()].Size(), weight_getter,
      main_size_);
  for (size_t idx = 0; idx < item_size; ++idx) {
    if (base::FloatsLarger(inflow_items_[idx]->GetCSSStyle()->GetLinearWeight(),
                           0.0f)) {
      UpdateChildSize(idx);
    }
  }
}

/* Algorithm-2
 * Calculate the container size.
 */
void LinearLayoutAlgorithm::DetermineContainerSize() {
  bool flag_change = false;
  if (!IsSLDefiniteMode(container_constraints_[kMainAxis].Mode())) {
    container_constraints_[kMainAxis] =
        OneSideConstraint::Definite(total_main_size_);
    flag_change = true;
  }

  if (!IsSLDefiniteMode(container_constraints_[kCrossAxis].Mode())) {
    container_constraints_[kCrossAxis] =
        OneSideConstraint::Definite(total_cross_size_);
    flag_change = true;
  }

  if (flag_change) UpdateContainerSize();
}

void LinearLayoutAlgorithm::UpdateChildSize(const size_t idx) {
  UpdateChildSizeInternal(idx, container_constraints_);
}

/* Algorithm-3
 * Update child size.
 */
void LinearLayoutAlgorithm::UpdateChildSizeInternal(
    const size_t idx, const Constraints& used_container_constraints) {
  const bool is_row = IsHorizontal();
  LayoutObject* child = inflow_items_[idx];
  const LayoutComputedStyle* child_style = child->GetCSSStyle();
  const auto& margin = child->GetBoxInfo()->margin_;
  Constraints child_constraints;

  if (container_->GetLayoutConfigs().IsFullQuirksMode()) {
    const auto preferred_size = property_utils::ComputePreferredSize(
        *child, used_container_constraints);
    child_constraints[kHorizontal].ApplySize(preferred_size[kHorizontal]);
    child_constraints[kVertical].ApplySize(preferred_size[kVertical]);
  } else {
    child_constraints = property_utils::GenerateDefaultConstraints(
        *child, used_container_constraints);
    // The main axis container of linear layout is always indefinite if child
    // main size is not specified as a definite value.
    if (child_constraints[MainAxis()].Mode() == SLMeasureModeAtMost) {
      child_constraints[MainAxis()] = OneSideConstraint::Indefinite();
    }
  }
  if (main_size_[idx] != -1) {
    child_constraints[MainAxis()] =
        OneSideConstraint::Definite(main_size_[idx]);
    const LayoutUnit& length_on_cross_axis =
        NLengthToLayoutUnit(logic_direction_utils::GetCSSDimensionSize(
                                child->GetCSSStyle(), CrossAxis()),
                            container_constraints_[kCrossAxis].ToPercentBase());

    if (length_on_cross_axis.IsIndefinite() &&
        !base::FloatsEqual(child_style->GetAspectRatio(), -1.0f)) {
      child_constraints[CrossAxis()] = OneSideConstraint::Indefinite();
      property_utils::ApplyAspectRatio(child, child_constraints);
    }
  }

  if (used_container_constraints[CrossAxis()].Mode() == SLMeasureModeDefinite) {
    if (((!IsSLDefiniteMode(child_constraints[CrossAxis()].Mode()) &&
          GetComputedLinearLayoutGravity(*child_style) ==
              LinearLayoutGravityType::kNone) &&
         !logic_direction_utils::GetCSSDimensionSize(child_style, CrossAxis())
              .IsIntrinsic()) ||
        IsLayoutGravityFill(GetComputedLinearLayoutGravity(*child_style))) {
      float stretched_size = used_container_constraints[CrossAxis()].Size() -
                             margin[CrossFront()] - margin[CrossBack()];
      child_constraints[CrossAxis()] =
          OneSideConstraint::Definite(stretched_size);
    }
  }

  FloatSize result;
  // Make sure every child will only be measured once.
  result =
      child->UpdateMeasure(child_constraints, container_->GetFinalMeasure());

  main_size_[idx] = is_row ? result.width_ : result.height_;
  cross_size_[idx] = is_row ? result.height_ : result.width_;
}

/*
 * Align items.
 */
void LinearLayoutAlgorithm::AlignInFlowItems() {
  if (inflow_items_.empty()) {
    return;
  }

  float main_offset = 0.f;
  float avg_offset = 0.f;
  size_t item_count = inflow_items_.size();

  // transfer to logic direction
  LinearGravityType gravity = GetLogicLinearGravityType();

  if (gravity == LinearGravityType::kSpaceBetween) {
    if (item_count == 1)
      avg_offset = 0;
    else
      avg_offset = remaining_size_ / (item_count - 1);
  } else if (IsGravityAfter(gravity)) {
    main_offset = logic_direction_utils::GetContentBoundDimensionSize(
                      container_, MainAxis()) -
                  total_main_size_;
  } else if (IsGravityCenter(gravity)) {
    // main_offset should subtract the left margin of the first item. We do
    // not do the subtraction right now as we do it when calculating the
    // main offset of the first item.
    main_offset = (logic_direction_utils::GetContentBoundDimensionSize(
                       container_, MainAxis()) -
                   total_main_size_) /
                  2.f;
  }

  for (size_t i = 0; i < item_count; ++i) {
    LayoutObject* item = inflow_items_[i];
    logic_direction_utils::SetBoundOffsetFrom(item, MainFront(),
                                              BoundType::kMargin,
                                              BoundType::kContent, main_offset);
    main_offset +=
        logic_direction_utils::GetMarginBoundDimensionSize(item, MainAxis());

    if (gravity == LinearGravityType::kSpaceBetween) {
      main_offset +=
          (i != item_count - 2) ? avg_offset : remaining_size_ - avg_offset * i;
    }

    CrossAxisAlignment(inflow_items_[i]);
  }

  HandleScrollView();
}

void LinearLayoutAlgorithm::CrossAxisAlignment(LayoutObject* item) {
  const LayoutComputedStyle* item_style = item->GetCSSStyle();
  const auto layout_gravity = GetComputedLinearLayoutGravity(*item_style);
  float cross_offset = 0.f;

  if (IsLayoutGravityDefault(layout_gravity)) {
    cross_offset = 0.f;
  } else if (IsLayoutGravityAfter(layout_gravity)) {
    cross_offset =
        logic_direction_utils::GetContentBoundDimensionSize(container_,
                                                            CrossAxis()) -
        logic_direction_utils::GetMarginBoundDimensionSize(item, CrossAxis());
  } else if (IsLayoutGravityCenter(layout_gravity)) {
    cross_offset = (logic_direction_utils::GetContentBoundDimensionSize(
                        container_, CrossAxis()) -
                    logic_direction_utils::GetMarginBoundDimensionSize(
                        item, CrossAxis())) /
                   2.f;
  }
  if (!container_->GetLayoutConfigs().IsFullQuirksMode()) {
    float content_size = logic_direction_utils::GetBorderBoundDimensionSize(
        container_, CrossAxis());
    logic_direction_utils::ResolveAutoMargins(item, content_size, CrossAxis());
  }

  logic_direction_utils::SetBoundOffsetFrom(item, CrossFront(),
                                            BoundType::kMargin,
                                            BoundType::kContent, cross_offset);
}

void LinearLayoutAlgorithm::InitializeAlgorithmEnv() {
  main_size_.resize<true>(inflow_items_.size(), -1);
  cross_size_.resize<true>(inflow_items_.size(), -1);
}

void LinearLayoutAlgorithm::UpdateContainerSize() {
  for (LayoutObject* item : inflow_items_) {
    item->GetBoxInfo()->UpdateBoxData(container_constraints_, *item,
                                      item->GetLayoutConfigs());
  }
}

void LinearLayoutAlgorithm::AfterResultBorderBoxSize() {
  remaining_size_ = container_constraints_[kMainAxis].Size() - total_main_size_;
  if (remaining_size_ < 0) remaining_size_ = 0;
}

bool LinearLayoutAlgorithm::IsLayoutGravityDefault(
    LinearLayoutGravityType layout_gravity) const {
  return layout_gravity == LinearLayoutGravityType::kLeft ||
         layout_gravity == LinearLayoutGravityType::kTop ||
         layout_gravity == LinearLayoutGravityType::kNone ||
         layout_gravity == LinearLayoutGravityType::kFillHorizontal ||
         layout_gravity == LinearLayoutGravityType::kFillVertical ||
         layout_gravity == LinearLayoutGravityType::kStretch ||
         layout_gravity == LinearLayoutGravityType::kStart;
}

bool LinearLayoutAlgorithm::IsLayoutGravityAfter(
    LinearLayoutGravityType layout_gravity) const {
  return layout_gravity == LinearLayoutGravityType::kRight ||
         layout_gravity == LinearLayoutGravityType::kBottom ||
         layout_gravity == LinearLayoutGravityType::kEnd;
}

bool LinearLayoutAlgorithm::IsLayoutGravityCenter(
    LinearLayoutGravityType layout_gravity) const {
  return layout_gravity == LinearLayoutGravityType::kCenterHorizontal ||
         layout_gravity == LinearLayoutGravityType::kCenterVertical ||
         layout_gravity == LinearLayoutGravityType::kCenter;
}

bool LinearLayoutAlgorithm::IsLayoutGravityFill(
    LinearLayoutGravityType layout_gravity) const {
  return layout_gravity == LinearLayoutGravityType::kFillHorizontal ||
         layout_gravity == LinearLayoutGravityType::kFillVertical ||
         layout_gravity == LinearLayoutGravityType::kStretch;
}

bool LinearLayoutAlgorithm::IsGravityPhysical(LinearGravityType gravity) const {
  return gravity == LinearGravityType::kLeft ||
         gravity == LinearGravityType::kRight ||
         gravity == LinearGravityType::kTop ||
         gravity == LinearGravityType::kBottom;
}

bool LinearLayoutAlgorithm::IsGravityAfter(LinearGravityType gravity) const {
  return gravity == LinearGravityType::kEnd;
}

bool LinearLayoutAlgorithm::IsGravityCenter(LinearGravityType gravity) const {
  return gravity == LinearGravityType::kCenterHorizontal ||
         gravity == LinearGravityType::kCenterVertical ||
         gravity == LinearGravityType::kCenter;
}

BoxPositions LinearLayoutAlgorithm::GetAbsoluteOrFixedItemInitialPosition(
    LayoutObject* absolute_or_fixed_item) {
  BoxPositions item_position;

  item_position[MainAxis()] =
      GetAbsoluteOrFixedItemMainAxisPosition(absolute_or_fixed_item);
  item_position[CrossAxis()] =
      GetAbsoluteOrFixedItemCrossAxisPosition(absolute_or_fixed_item);

  return item_position;
}

Position LinearLayoutAlgorithm::GetAbsoluteOrFixedItemCrossAxisPosition(
    LayoutObject* absolute_or_fixed_item) {
  Position position = Position::kStart;
  const auto style = *absolute_or_fixed_item->GetCSSStyle();

  const LinearLayoutGravityType align_type =
      GetComputedLinearLayoutGravity(style);
  if (IsLayoutGravityCenter(align_type)) {
    position = Position::kCenter;
  } else if (IsLayoutGravityAfter(align_type)) {
    position = Position::kEnd;
  }
  // else position will be start
  return position;
}

Position LinearLayoutAlgorithm::GetAbsoluteOrFixedItemMainAxisPosition(
    LayoutObject* absolute_or_fixed_item) {
  Position position = Position::kStart;
  const LinearGravityType gravity_type = GetLogicLinearGravityType();

  if (IsGravityCenter(gravity_type)) {
    position = Position::kCenter;
  } else if (IsGravityAfter(gravity_type)) {
    position = Position::kEnd;
  }
  // else position will be start

  return position;
}

LinearLayoutGravityType LinearLayoutAlgorithm::GetComputedLinearLayoutGravity(
    const LayoutComputedStyle& style) const {
  static const base::NoDestructor<
      std::unordered_map<FlexAlignType, LinearLayoutGravityType>>
      flex_align_to_linear_layout_gravity{
          {{FlexAlignType::kFlexStart, LinearLayoutGravityType::kStart},
           {FlexAlignType::kFlexEnd, LinearLayoutGravityType::kEnd},
           {FlexAlignType::kCenter, LinearLayoutGravityType::kCenter},
           {FlexAlignType::kStretch, LinearLayoutGravityType::kStretch},
           {FlexAlignType::kAuto, LinearLayoutGravityType::kNone},
           {FlexAlignType::kStart, LinearLayoutGravityType::kStart},
           {FlexAlignType::kEnd, LinearLayoutGravityType::kEnd}}};
  auto item_layout_gravity = style.GetLinearLayoutGravity();
  auto align_self = style.GetAlignSelf();
  auto align_items = container_style_->GetAlignItems();
  if (!container_->GetLayoutConfigs().GetIsTargetSdkVerionHigherThan213() &&
      (align_self == FlexAlignType::kBaseline ||
       align_items == FlexAlignType::kBaseline)) {
    container_->SendLayoutEvent(
        LayoutEventType::LayoutStyleError,
        LayoutErrorData(
            "Linear layout does not support align-items:baseline and child "
            "with "
            "align-self:baseline. It will crash when engineVersion<=2.13.",
            "Not use these styles in Linear layout. Or you should set "
            "display:flex "
            "on the element with align-items:baseline and on the element "
            "containing a child element with align-self:baseline."));
  }
  if (container_->GetLayoutConfigs().LinearSupportFlexStyleMode() &&
      item_layout_gravity == LinearLayoutGravityType::kNone &&
      flex_align_to_linear_layout_gravity.get()->count(align_self) > 0) {
    item_layout_gravity =
        flex_align_to_linear_layout_gravity.get()->at(align_self);
  }
  if (item_layout_gravity == LinearLayoutGravityType::kNone) {
    switch (container_style_->GetLinearCrossGravity()) {
      case LinearCrossGravityType::kStart:
        item_layout_gravity = LinearLayoutGravityType::kStart;
        break;
      case LinearCrossGravityType::kEnd:
        item_layout_gravity = LinearLayoutGravityType::kEnd;
        break;
      case LinearCrossGravityType::kCenter:
        item_layout_gravity = LinearLayoutGravityType::kCenter;
        break;
      case LinearCrossGravityType::kStretch:
        item_layout_gravity = LinearLayoutGravityType::kStretch;
        break;
      default:
        break;
    }
  }
  if (container_->GetLayoutConfigs().LinearSupportFlexStyleMode() &&
      item_layout_gravity == LinearLayoutGravityType::kNone &&
      align_items != FlexAlignType::kStretch &&
      flex_align_to_linear_layout_gravity.get()->count(align_items) > 0) {
    // align-items: stretch will not be supported in Linear Layout
    item_layout_gravity =
        flex_align_to_linear_layout_gravity.get()->at(align_items);
  }

  if (!IsHorizontal() && container_style_->IsRtl()) {
    if (item_layout_gravity == LinearLayoutGravityType::kLeft) {
      item_layout_gravity = LinearLayoutGravityType::kRight;
    } else if (item_layout_gravity == LinearLayoutGravityType::kRight) {
      item_layout_gravity = LinearLayoutGravityType::kLeft;
    }
  }

  return item_layout_gravity;
}

LinearGravityType LinearLayoutAlgorithm::GetLogicLinearGravityType() const {
  static const base::NoDestructor<
      std::unordered_map<JustifyContentType, LinearGravityType>>
      justify_content_to_linear_gravity{{
          {JustifyContentType::kFlexStart, LinearGravityType::kStart},
          {JustifyContentType::kFlexEnd, LinearGravityType::kEnd},
          {JustifyContentType::kCenter, LinearGravityType::kCenter},
          {JustifyContentType::kSpaceBetween, LinearGravityType::kSpaceBetween},
          {JustifyContentType::kSpaceAround, LinearGravityType::kStart},
          {JustifyContentType::kSpaceEvenly, LinearGravityType::kStart},
          {JustifyContentType::kStretch, LinearGravityType::kStart},
      }};
  auto gravity = container_style_->GetLinearGravity();
  auto justify_content = container_style_->GetJustifyContent();

  // None is the same as start
  if (container_->GetLayoutConfigs().LinearSupportFlexStyleMode() &&
      gravity == LinearGravityType::kNone &&
      justify_content_to_linear_gravity.get()->count(justify_content) > 0) {
    gravity = justify_content_to_linear_gravity.get()->at(justify_content);
  }
  if (IsHorizontal() && container_style_->IsLynxRtl()) {
    if (gravity == LinearGravityType::kLeft) {
      gravity = LinearGravityType::kRight;
    } else if (gravity == LinearGravityType::kRight) {
      gravity = LinearGravityType::kLeft;
    }
  }

  if (IsGravityPhysical(gravity)) {
    gravity = logic_direction_utils::GetLogicGravityType(gravity, MainFront());
  }

  return gravity;
}

void LinearLayoutAlgorithm::HandleScrollView() {
  // transfer child x-coordinate to positive
  // if parent is horizontal and horizontal direction is from right to left
  if ((container_->GetTag() == "scroll-view" ||
       container_->GetTag() == "x-scroll-view") &&
      MainFront() == kRight) {
    float child_width_sum = 0.0f;

    for (LayoutObject* item : inflow_items_) {
      child_width_sum += item->GetMarginBoundWidth();
    }

    float shift_length = child_width_sum - container_->GetContentBoundWidth();

    if (shift_length > 0) {
      for (LayoutObject* item : inflow_items_) {
        item->SetBoundLeftFrom(
            container_,
            item->GetBoundLeftFrom(container_, BoundType::kMargin,
                                   BoundType::kContent) +
                shift_length,
            BoundType::kMargin, BoundType::kContent);
      }
    }
  }
}

void LinearLayoutAlgorithm::SetContainerBaseline() {
  if (IsHorizontal()) {
    // In the Linear display, the baseline of container is decided by the item
    // with largest distance between its cross-start margin edge and its
    // baseline
    for (LayoutObject* item : inflow_items_) {
      float cross_offset = 0.f;
      cross_offset = item->GetOffsetFromTopMarginEdgeToBaseline();
      const auto layout_gravity =
          GetComputedLinearLayoutGravity(*item->GetCSSStyle());

      if (IsLayoutGravityDefault(layout_gravity)) {
        baseline_ = std::max(baseline_, cross_offset);
        continue;
      }

      if (IsLayoutGravityAfter(layout_gravity)) {
        cross_offset += logic_direction_utils::GetContentBoundDimensionSize(
                            container_, CrossAxis()) -
                        logic_direction_utils::GetMarginBoundDimensionSize(
                            item, CrossAxis());
      } else if (IsLayoutGravityCenter(layout_gravity)) {
        cross_offset += (logic_direction_utils::GetContentBoundDimensionSize(
                             container_, CrossAxis()) -
                         logic_direction_utils::GetMarginBoundDimensionSize(
                             item, CrossAxis())) /
                        2.f;
      }

      baseline_ = std::max(baseline_, cross_offset);
    }
    container_->SetBaseline(baseline_);
  } else if (!container_->GetLayoutConfigs()
                  .IsBaselineSupportVerticalQuirksMode()) {
    if (inflow_items_.empty()) {
      return;
    }
    float main_offset = 0.f;
    // transfer to logic direction
    LinearGravityType gravity = GetLogicLinearGravityType();

    if (IsGravityAfter(gravity)) {
      main_offset = logic_direction_utils::GetContentBoundDimensionSize(
                        container_, MainAxis()) -
                    total_main_size_;
    } else if (IsGravityCenter(gravity)) {
      main_offset = (logic_direction_utils::GetContentBoundDimensionSize(
                         container_, MainAxis()) -
                     total_main_size_) /
                    2.f;
    }
    container_->SetBaseline(
        main_offset + inflow_items_[0]->GetOffsetFromTopMarginEdgeToBaseline());
  }
}

}  // namespace starlight
}  // namespace lynx
