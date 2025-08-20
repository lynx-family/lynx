// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/layout_algorithm.h"

#include <algorithm>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/position_layout_utils.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/layout/relative_layout_algorithm.h"

namespace lynx {
namespace starlight {

LayoutAlgorithm::LayoutAlgorithm(LayoutObject* container)
    : DirectionSelector(
          container->GetCSSStyle()->IsRow(container->GetLayoutConfigs(),
                                          container->attr_map()),
          container->GetCSSStyle()->DirectionIsReverse(
              container->GetLayoutConfigs(), container->attr_map()),
          container->GetCSSStyle()->IsAnyRtl()),
      container_(container),
      container_style_(container->GetCSSStyle()) {}

LayoutAlgorithm::~LayoutAlgorithm() {
  container_ = nullptr;
  container_style_ = nullptr;
}

void LayoutAlgorithm::Update(const Constraints& constraints) {
  UpdateAvailableSizeAndMode(constraints);
  Reset();
}

void LayoutAlgorithm::Initialize(const Constraints& constraints,
                                 const SLNodeSet* fixed_node_set) {
  UpdateAvailableSizeAndMode(constraints);
  InitializeChildren(fixed_node_set);
  InitializeAlgorithmEnv();
}

void LayoutAlgorithm::InitializeChildren(const SLNodeSet* fixed_node_set) {
  bool need_order = false;
  if (container_->GetEnableFixedNew()) {
    InitializeFixedNode(fixed_node_set);
  }
  const int child_count = container_->GetChildCount();
  if (child_count > 0) {
    inflow_items_.reserve(child_count);
    absolute_or_fixed_items_.reserve(child_count);
    sticky_items.reserve(child_count);
  }
  for (Node* node = container_->FirstChild(); node != nullptr;
       node = node->Next()) {
    LayoutObject* const child = static_cast<LayoutObject*>(node);
    CollectLayoutableChildren(child, need_order);
  }

  if (need_order) {
    std::stable_sort(inflow_items_.begin(), inflow_items_.end(),
                     [](LayoutObject* obj1, LayoutObject* obj2) -> bool {
                       return obj1->GetCSSStyle()->GetOrder() <
                              obj2->GetCSSStyle()->GetOrder();
                     });
  }
}

void LayoutAlgorithm::InitializeFixedNode(const SLNodeSet* fixed_node_set) {
  // Only called by root's LayoutAlgorithm.
  if (fixed_node_set != nullptr) {
    const auto container_display = container_->GetCSSStyle()->GetDisplay(
        container_->GetLayoutConfigs(), container_->attr_map());
    if (!container_->GetLayoutConfigs().IsFullQuirksMode() ||
        container_display == DisplayType::kFlex) {
      for (auto& item : *fixed_node_set) {
        if (item->GetCSSStyle()->GetDisplay(container_->GetLayoutConfigs(),
                                            container_->attr_map()) ==
            DisplayType::kNone) {
          item->LayoutDisplayNone();
          continue;
        }

        Constraints containing_block = container_constraints_;
        if (!container_->GetLayoutConfigs()
                 .IsAbsoluteAndFixedBoxInfoQuirksMode()) {
          containing_block =
              position_utils::GetContainingBlockForAbsoluteAndFixed(
                  container_, container_constraints_);
        }
        item->GetBoxInfo()->InitializeBoxInfo(containing_block, *item,
                                              item->GetLayoutConfigs());
        absolute_or_fixed_items_.push_back(item);
        item->SetContainingBlockEstablisher(container_);
      }
    }
  }
}

void LayoutAlgorithm::HandleDisplayContents(LayoutObject* const item,
                                            bool& need_order) {
  item->HideLayoutObject();
  item->SetContainingBlockEstablisher(nullptr);
  for (int i = 0; i < item->GetChildCount(); ++i) {
    LayoutObject* child = static_cast<LayoutObject*>(item->Find(i));
    CollectLayoutableChildren(child, need_order);
  }
}

void LayoutAlgorithm::CollectLayoutableChildren(LayoutObject* const item,
                                                bool& need_order) {
  if (item->IsNewFixed()) {
    return;
  }
  const DisplayType display_type = item->GetCSSStyle()->GetDisplay(
      item->GetLayoutConfigs(), item->attr_map());
  if (display_type == DisplayType::kNone) {
    item->LayoutDisplayNone();
    return;
  } else if (display_type == DisplayType::kContents) {
    HandleDisplayContents(item, need_order);
    return;
  }

  Constraints containing_block = container_constraints_;
  if (item->IsFixedOrAbsolute() &&
      !container_->GetLayoutConfigs().IsAbsoluteAndFixedBoxInfoQuirksMode()) {
    containing_block = position_utils::GetContainingBlockForAbsoluteAndFixed(
        container_, container_constraints_);
  }

  item->GetBoxInfo()->InitializeBoxInfo(containing_block, *item,
                                        item->GetLayoutConfigs());

  if (!(container_->GetLayoutConfigs().IsFullQuirksMode() &&
        container_->GetCSSStyle()->GetDisplay(container_->GetLayoutConfigs(),
                                              container_->attr_map()) !=
            DisplayType::kFlex) &&
      item->IsFixedOrAbsolute()) {
    absolute_or_fixed_items_.push_back(item);
    item->SetContainingBlockEstablisher(container_);
    return;
  } else if (item->GetCSSStyle()->GetPosition() == PositionType::kSticky) {
    sticky_items.push_back(item);
  }

  inflow_items_.push_back(item);
  item->SetContainingBlockEstablisher(container_);

  if (item->GetCSSStyle()->GetOrder()) {
    need_order = true;
  }
}

bool LayoutAlgorithm::IsInflowSubTreeInSync() const {
  for (const auto* item : inflow_items_) {
    if (!item->IsInflowSubTreeInSyncWithLastMeasurement()) {
      return false;
    }
  }
  return true;
}

void LayoutAlgorithm::UpdateAvailableSizeAndMode(
    const Constraints& constraints) {
  container_constraints_ = Constraints();
  if (constraints[kHorizontal].Mode() != SLMeasureModeIndefinite) {
    container_constraints_[kHorizontal] =
        OneSideConstraint(constraints[kHorizontal].Size() -
                              container_->GetPaddingAndBorderHorizontal(),
                          constraints[kHorizontal].Mode());
  }
  if (constraints[kVertical].Mode() != SLMeasureModeIndefinite) {
    container_constraints_[kVertical] =
        OneSideConstraint(constraints[kVertical].Size() -
                              container_->GetPaddingAndBorderVertical(),
                          constraints[kVertical].Mode());
  }
}

FloatSize LayoutAlgorithm::PostLayoutProcessingAndResultBorderBoxSize() {
  FloatSize result;
  DCHECK(container_constraints_[kHorizontal].Mode() == SLMeasureModeDefinite);
  DCHECK(container_constraints_[kVertical].Mode() == SLMeasureModeDefinite);
  result.width_ = container_->GetBorderBoxWidthFromInnerWidth(
      container_constraints_[kHorizontal].Size());
  result.height_ = container_->GetBorderBoxHeightFromInnerHeight(
      container_constraints_[kVertical].Size());
  result.width_ = container_->ClampExactWidth(result.width_);
  result.height_ = container_->ClampExactHeight(result.height_);
  container_constraints_[kHorizontal] = OneSideConstraint::Definite(
      container_->GetInnerWidthFromBorderBoxWidth(result.width_));
  container_constraints_[kVertical] = OneSideConstraint::Definite(
      container_->GetInnerHeightFromBorderBoxHeight(result.height_));

  AfterResultBorderBoxSize();
  return result;
}

void LayoutAlgorithm::AfterResultBorderBoxSize() {}

void LayoutAlgorithm::AlignAbsoluteAndFixedItems() {
  for (LayoutObject* item : absolute_or_fixed_items_) {
    auto item_initial_position = GetAbsoluteOrFixedItemInitialPosition(item);

    std::array<Direction, 2> directions;
    directions[kHorizontal] = HorizontalFront();
    directions[kVertical] = VerticalFront();
    position_utils::CalcAbsoluteOrFixedPosition(
        item, container_, container_constraints_, item_initial_position,
        directions);
  }
}

// Absolute | Fixed
void LayoutAlgorithm::MeasureAbsoluteAndFixed() {
  for (LayoutObject* item : absolute_or_fixed_items_) {
    // Prevent some unexpected behaviors, such as prevent fixed node's
    // UpdateMeasure from being called by the root node's algorithm during the
    // Alignment stage.
    if (item->GetShouldDisplayNone()) {
      continue;
    }
    auto* positioning_container =
        position_utils::GetAbsolutePositionContainingBlock(item, container_);
    const Constraints containing_block =
        positioning_container == container_
            ? position_utils::GetContainingBlockForAbsoluteAndFixed(
                  container_, container_constraints_)
            : position_utils::GetAbsolutePositionContainingBlockConstraints(
                  positioning_container);
    // when containing block is formed, resolve box info that contains
    // percentage
    if (positioning_container != container_) {
      // Reinitialize against the final ancestor size, including invalidation
      // when percentage padding or minimum sizes change.
      item->GetBoxInfo()->InitializeBoxInfo(containing_block, *item,
                                            item->GetLayoutConfigs());
    } else {
      item->GetBoxInfo()->ResolveBoxInfoForAbsoluteAndFixed(
          containing_block, *item, item->GetLayoutConfigs());
    }
    Constraints item_size_mode =
        position_utils::GetAbsoluteOrFixedItemSizeAndMode(
            item, positioning_container, containing_block);
    item->UpdateMeasure(item_size_mode, true);
  }
}

void LayoutAlgorithm::HandleRelativePosition() {
  if (container_->GetLayoutConfigs().IsFullQuirksMode() &&
      container_->GetCSSStyle()->GetDisplay(container_->GetLayoutConfigs(),
                                            container_->attr_map()) !=
          DisplayType::kFlex) {
    return;
  }

  for (LayoutObject* item : inflow_items_) {
    if (item->GetCSSStyle()->GetPosition() == PositionType::kRelative) {
      position_utils::CalcRelativePosition(item, container_constraints_);
    }
  }
}

void LayoutAlgorithm::ItemsUpdateAlignment() {
  for (LayoutObject* item : inflow_items_) {
    item->UpdateAlignment();
  }
  for (LayoutObject* item : absolute_or_fixed_items_) {
    item->UpdateAlignment();
  }

  for (LayoutObject* item : sticky_items) {
    position_utils::UpdateStickyItemPosition(item, container_constraints_);
  }
}

void LayoutAlgorithm::Alignment() {
  // The measure of absolute and fixed object is on align stage to avoid
  // unnecessary measurement
  MeasureAbsoluteAndFixed();

  AlignInFlowItems();

  AlignAbsoluteAndFixedItems();

  HandleRelativePosition();

  ItemsUpdateAlignment();
}

FloatSize LayoutAlgorithm::SizeDetermination() {
  SizeDeterminationByAlgorithm();

  return PostLayoutProcessingAndResultBorderBoxSize();
}

Constraints LayoutAlgorithm::GenerateDefaultConstraint(
    const LayoutObject& child) const {
  return property_utils::GenerateDefaultConstraints(child,
                                                    container_constraints_);
}

float LayoutAlgorithm::CalculateFloatSizeFromLength(
    const NLength& length, const LayoutUnit& percent_base) {
  return NLengthToLayoutUnit(length, percent_base)
      .ClampIndefiniteToZero()
      .ToFloat();
}

const NLength& LayoutAlgorithm::GapStyle(Dimension dimension) const {
  return dimension == kHorizontal ? container_style_->GetGridColumnGap()
                                  : container_style_->GetGridRowGap();
}

void LayoutAlgorithm::SetContainerBaseline() {}

}  // namespace starlight
}  // namespace lynx
