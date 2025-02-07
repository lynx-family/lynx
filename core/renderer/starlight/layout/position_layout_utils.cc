// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/position_layout_utils.h"

#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"

namespace lynx {
namespace starlight {
namespace {

float CalcInitialOffset(float container_size, float absolute_item_size,
                        Position position) {
  float offset = 0;
  if (Position::kStart == position) {
    offset = 0;
  } else if (Position::kCenter == position) {
    offset = (container_size - absolute_item_size) / 2;
  } else {
    offset = container_size - absolute_item_size;
  }

  return offset;
}

float CalcLengthValue(const NLength& length, float screen_width,
                      const Constraints& constraints, Dimension dimension) {
  const auto offset =
      NLengthToLayoutUnit(length, constraints[dimension].ToPercentBase());
  return offset.IsIndefinite() ? -1e+10 : offset.ToFloat();
}
}  // namespace
namespace position_utils {

void CalcRelativePosition(LayoutObject* item,
                          const Constraints& content_constraints) {
  const LayoutComputedStyle* item_style = item->GetCSSStyle();

  const auto left = NLengthToLayoutUnit(
      item_style->GetLeft(), content_constraints[kHorizontal].ToPercentBase());
  if (left.IsDefinite()) {
    item->SetBorderBoundLeftFromParentPaddingBound(
        item->GetBorderBoundLeftFromParentPaddingBound() + left.ToFloat());
  } else {
    const auto right =
        NLengthToLayoutUnit(item_style->GetRight(),
                            content_constraints[kHorizontal].ToPercentBase());
    if (right.IsDefinite()) {
      item->SetBorderBoundLeftFromParentPaddingBound(
          item->GetBorderBoundLeftFromParentPaddingBound() - right.ToFloat());
    }
  }

  const auto top = NLengthToLayoutUnit(
      item_style->GetTop(), content_constraints[kVertical].ToPercentBase());
  if (top.IsDefinite()) {
    item->SetBorderBoundTopFromParentPaddingBound(
        item->GetBorderBoundTopFromParentPaddingBound() + top.ToFloat());
  } else {
    const auto bottom =
        NLengthToLayoutUnit(item_style->GetBottom(),
                            content_constraints[kVertical].ToPercentBase());
    if (bottom.IsDefinite()) {
      item->SetBorderBoundTopFromParentPaddingBound(
          item->GetBorderBoundTopFromParentPaddingBound() - bottom.ToFloat());
    }
  }
}

Constraints GetContainingBlockForAbsoluteAndFixed(
    LayoutObject* container, const Constraints& base_containing_block) {
  Constraints constraints = base_containing_block;
  if (base_containing_block[kHorizontal].Mode() == SLMeasureModeIndefinite ||
      base_containing_block[kVertical].Mode() == SLMeasureModeIndefinite) {
    return constraints;
  }
  if (!container->IsAbsoluteInContentBound()) {
    constraints[kHorizontal] =
        OneSideConstraint(base_containing_block[kHorizontal].Size() +
                              container->GetLayoutPaddingLeft() +
                              container->GetLayoutPaddingRight(),
                          base_containing_block[kHorizontal].Mode());

    constraints[kVertical] =
        OneSideConstraint(base_containing_block[kVertical].Size() +
                              container->GetLayoutPaddingTop() +
                              container->GetLayoutPaddingBottom(),
                          base_containing_block[kVertical].Mode());
  }

  return constraints;
}

Constraints GetAbsoluteOrFixedItemSizeAndMode(
    LayoutObject* absolute_or_fixed_item, LayoutObject* container,
    const Constraints& absolute_box) {
  Constraints ret = property_utils::GenerateDefaultConstraints(
      *absolute_or_fixed_item, absolute_box);

  if (container->GetLayoutConfigs().IsFullQuirksMode()) {
    if (ret[kHorizontal].Mode() != SLMeasureModeDefinite) {
      ret[kHorizontal] = OneSideConstraint::Indefinite();
    }
    if (ret[kVertical].Mode() != SLMeasureModeDefinite) {
      ret[kVertical] = OneSideConstraint::Indefinite();
    }
  }

  const LayoutComputedStyle* item_style = absolute_or_fixed_item->GetCSSStyle();

  LayoutUnit left = NLengthToLayoutUnit(
      item_style->GetLeft(), absolute_box[kHorizontal].ToPercentBase());
  LayoutUnit right = NLengthToLayoutUnit(
      item_style->GetRight(), absolute_box[kHorizontal].ToPercentBase());
  LayoutUnit top = NLengthToLayoutUnit(item_style->GetTop(),
                                       absolute_box[kVertical].ToPercentBase());
  LayoutUnit bottom = NLengthToLayoutUnit(
      item_style->GetBottom(), absolute_box[kVertical].ToPercentBase());

  const FourValue& margin = absolute_or_fixed_item->GetBoxInfo()->margin_;

  // absolutely positioned elements can be made to fill the available vertical
  // space by specifying both top and bottom and leaving height unspecified
  // (that is, auto). They can likewise be made to fill the available horizontal
  // space by specifying both left and right and leaving width as auto.
  if (item_style->GetWidth().IsAuto()) {
    if (left.IsDefinite() && right.IsDefinite()) {
      ret[kHorizontal] = OneSideConstraint::Definite(
          absolute_box[kHorizontal].Size() -
          (left.ToFloat() + right.ToFloat() + margin[kLeft] + margin[kRight]));
    }
  }

  if (item_style->GetHeight().IsAuto()) {
    if (top.IsDefinite() && bottom.IsDefinite()) {
      ret[kVertical] = OneSideConstraint::Definite(
          absolute_box[kVertical].Size() -
          (top.ToFloat() + bottom.ToFloat() + margin[kTop] + margin[kBottom]));
    }
  }

  if (!container->GetLayoutConfigs().IsFullQuirksMode()) {
    // Width not defined here
    const auto StripOffsetFromConstraints = [&ret](const LayoutUnit& side,
                                                   Dimension axis) {
      if (ret[axis].Mode() == SLMeasureModeAtMost && side.IsDefinite()) {
        ret[axis] =
            OneSideConstraint::AtMost(ret[axis].Size() - side.ToFloat());
      }
    };
    StripOffsetFromConstraints(left, kHorizontal);
    StripOffsetFromConstraints(right, kHorizontal);
    StripOffsetFromConstraints(top, kVertical);
    StripOffsetFromConstraints(bottom, kVertical);
  }

  property_utils::ApplyAspectRatio(absolute_or_fixed_item, ret);

  return ret;
}

void CalcAbsoluteOrFixedPosition(
    LayoutObject* absolute_or_fixed_item, LayoutObject* container,
    const Constraints& container_constraints,
    BoxPositions absolute_or_fixed_item_initial_position,
    std::array<Direction, 2> directions) {
  const auto containing_block =
      GetContainingBlockForAbsoluteAndFixed(container, container_constraints);
  const auto container_bound_type = container->IsAbsoluteInContentBound()
                                        ? BoundType::kContent
                                        : BoundType::kPadding;

  CalcStartOffset(absolute_or_fixed_item, container_bound_type,
                  absolute_or_fixed_item_initial_position, containing_block,
                  kHorizontal, directions[kHorizontal]);

  CalcStartOffset(absolute_or_fixed_item, container_bound_type,
                  absolute_or_fixed_item_initial_position, containing_block,
                  kVertical, directions[kVertical]);
}

// Start always means left or top for now.
void CalcStartOffset(LayoutObject* absolute_or_fixed_item,
                     BoundType container_bound_type,
                     DimensionValue<Position> positions,
                     const Constraints& containing_block, Dimension dimension,
                     Direction direction, float offset) {
  const float margin_bound_size =
      logic_direction_utils::GetMarginBoundDimensionSize(absolute_or_fixed_item,
                                                         dimension);

  const float init_start =
      CalcInitialOffset(containing_block[dimension].Size(), margin_bound_size,
                        positions[dimension]);

  const LayoutComputedStyle* item_style = absolute_or_fixed_item->GetCSSStyle();
  const auto start_offset = NLengthToLayoutUnit(
      dimension == Dimension::kHorizontal ? item_style->GetLeft()
                                          : item_style->GetTop(),
      containing_block[dimension].ToPercentBase());
  const auto end_offset = NLengthToLayoutUnit(
      dimension == Dimension::kHorizontal ? item_style->GetRight()
                                          : item_style->GetBottom(),
      containing_block[dimension].ToPercentBase());
  if (start_offset.IsDefinite()) {
    logic_direction_utils::SetBoundOffsetFrom(
        absolute_or_fixed_item,
        dimension == Dimension::kHorizontal ? kLeft : kTop, BoundType::kMargin,
        container_bound_type, start_offset.ToFloat() + offset);
  } else if (end_offset.IsDefinite()) {
    logic_direction_utils::SetBoundOffsetFrom(
        absolute_or_fixed_item,
        dimension == Dimension::kHorizontal ? kRight : kBottom,
        BoundType::kMargin, container_bound_type,
        end_offset.ToFloat() + offset);
  } else {
    logic_direction_utils::SetBoundOffsetFrom(
        absolute_or_fixed_item, direction, BoundType::kMargin,
        container_bound_type, init_start + offset);
  }
}

void UpdateStickyItemPosition(LayoutObject* sticky_item, float screen_width,
                              const Constraints& constraints) {
  auto item_style = sticky_item->GetCSSStyle();

  const float left = CalcLengthValue(item_style->GetLeft(), screen_width,
                                     constraints, kHorizontal);
  const float right = CalcLengthValue(item_style->GetRight(), screen_width,
                                      constraints, kHorizontal);
  const float top = CalcLengthValue(item_style->GetTop(), screen_width,
                                    constraints, kVertical);
  const float bottom = CalcLengthValue(item_style->GetBottom(), screen_width,
                                       constraints, kVertical);

  sticky_item->UpdatePositions(left, top, right, bottom);
}

Position ReversePosition(Position pos) {
  if (Position::kStart == pos) {
    return Position::kEnd;
  } else if (Position::kEnd == pos) {
    return Position::kStart;
  }

  return Position::kCenter;
}

}  // namespace position_utils
}  // namespace starlight
}  // namespace lynx
