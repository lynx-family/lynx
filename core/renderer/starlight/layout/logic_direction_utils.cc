// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/logic_direction_utils.h"

#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace starlight {
namespace logic_direction_utils {

float GetMarginBoundDimensionSize(const LayoutObject* item, Dimension axis) {
  return axis == Dimension::kHorizontal ? item->GetMarginBoundWidth()
                                        : item->GetMarginBoundHeight();
}

float GetPaddingBoundDimensionSize(const LayoutObject* item, Dimension axis) {
  return axis == Dimension::kHorizontal ? item->GetPaddingBoundWidth()
                                        : item->GetPaddingBoundHeight();
}

float GetContentBoundDimensionSize(const LayoutObject* item, Dimension axis) {
  return axis == Dimension::kHorizontal ? item->GetContentBoundWidth()
                                        : item->GetContentBoundHeight();
}

float GetBorderBoundDimensionSize(const LayoutObject* item, Dimension axis) {
  return axis == Dimension::kHorizontal ? item->GetBorderBoundWidth()
                                        : item->GetBorderBoundHeight();
}

void ResolveAutoMargins(LayoutObject* item, float content_size,
                        Dimension axis) {
  Direction front = axis == kHorizontal ? kLeft : kTop;
  Direction back = axis == kHorizontal ? kRight : kBottom;

  const bool margin_front_auto = GetMargin(item->GetCSSStyle(), front).IsAuto();
  const bool margin_back_auto = GetMargin(item->GetCSSStyle(), back).IsAuto();

  const float item_size = GetMarginBoundDimensionSize(item, axis);
  // Overflowing boxes ignore their auto margins and overflow in the end
  // direction.
  if (base::FloatsLarger(item_size, content_size)) {
    return;
  }

  if (margin_front_auto && margin_back_auto) {
    item->GetBoxInfo()->margin_[front] = (content_size - item_size) / 2;
    item->GetBoxInfo()->margin_[back] = (content_size - item_size) / 2;
  } else {
    if (margin_front_auto) {
      item->GetBoxInfo()->margin_[front] = content_size - item_size;
    } else if (margin_back_auto) {
      item->GetBoxInfo()->margin_[back] = content_size - item_size;
    }
  }
}

void ResolveAlignContent(const LayoutComputedStyle* css_style,
                         int32_t sub_item_count, float available_space,
                         float& axis_interval, float& axis_start,
                         bool negative_space_with_gap) {
  AlignContentType align_content = css_style->GetAlignContent();
  switch (align_content) {
    case AlignContentType::kStretch:  // not reachable
    case AlignContentType::kFlexStart: {
      break;
    }
    case AlignContentType::kFlexEnd: {
      axis_start = available_space;
      break;
    }
    case AlignContentType::kCenter: {
      axis_start = available_space / 2;
      break;
    }
    // In Flex: if the leftover free-space is negative or there is only a single
    // flex line in the flex container, space-between is identical to
    // flex-start.
    // TODO(yuanzhiwen): fix the previous resolving align-content part(negative
    // free-space case)
    case AlignContentType::kSpaceBetween: {
      if (sub_item_count > 1) {
        axis_interval = available_space / (sub_item_count - 1);
      }
      if (negative_space_with_gap) {
        axis_interval = 0.f;
      }
      break;
    }
    // In Flex: if the leftover free-space is negative, space-around is
    // identical to center. The lines will overflow equally in both directions.
    case AlignContentType::kSpaceAround: {
      if (sub_item_count) {
        axis_interval = available_space / sub_item_count;
        axis_start = axis_interval / 2;
      }
      if (negative_space_with_gap) {
        axis_interval = 0.f;
        axis_start = available_space / 2;
      }
      break;
    }
  }
}

void ResolveJustifyContent(const LayoutComputedStyle* css_style,
                           int32_t sub_item_count, float available_space,
                           float& axis_interval, float& axis_start,
                           bool negative_space_with_gap) {
  JustifyContentType justify_content = css_style->GetJustifyContent();
  switch (justify_content) {
    case JustifyContentType::kStretch:  // not reachable
    case JustifyContentType::kFlexStart: {
      break;
    }
    case JustifyContentType::kFlexEnd: {
      axis_start = available_space;
      break;
    }
    case JustifyContentType::kCenter: {
      axis_start = available_space / 2;
      break;
    }
    // In Flex: if the leftover free-space is negative or there is only a
    // single flex item on the line, space-between is identical to flex-start.
    // TODO(yuanzhiwen): fix the previous resolving justify-content
    // part(negative free-space case)
    case JustifyContentType::kSpaceBetween: {
      if (sub_item_count > 1) {
        axis_interval = available_space / (sub_item_count - 1);
      }
      if (negative_space_with_gap) {
        axis_interval = 0.f;
      }
      break;
    }
    // TODO(yuanzhiwen): If the leftover free-space is negative or there is only
    // a single flex item on the line, space-around is identical to center.
    case JustifyContentType::kSpaceAround: {
      if (sub_item_count) {
        axis_interval = available_space / sub_item_count;
        axis_start = axis_interval / 2;
      }
      if (negative_space_with_gap) {
        axis_interval = 0.f;
        axis_start = available_space / 2;
      }
      break;
    }
    case JustifyContentType::kSpaceEvenly: {
      axis_interval = available_space / (sub_item_count + 1);
      axis_start = axis_interval;
      break;
    }
  }
}

// only use in or after align
void SetBoundOffsetFrom(LayoutObject* item, Direction direction,
                        BoundType bound_type, BoundType container_bound_type,
                        float offset) {
  const LayoutObject* container =
      item->IsNewFixed() ? item->GetRoot() : item->ParentLayoutObject();
  if (direction == Direction::kLeft) {
    item->SetBoundLeftFrom(container, offset, bound_type, container_bound_type);
  } else if (direction == Direction::kTop) {
    item->SetBoundTopFrom(container, offset, bound_type, container_bound_type);
  } else if (direction == Direction::kRight) {
    item->SetBoundRightFrom(container, offset, bound_type,
                            container_bound_type);
  } else if (direction == Direction::kBottom) {
    item->SetBoundBottomFrom(container, offset, bound_type,
                             container_bound_type);
  }
}

float GetBoundOffsetFrom(const LayoutObject* item, Dimension axis,
                         BoundType bound_type, BoundType container_bound_type) {
  const LayoutObject* container =
      item->IsNewFixed() ? item->GetRoot() : item->ParentLayoutObject();
  return axis == Dimension::kHorizontal
             ? item->GetBoundLeftFrom(container, bound_type,
                                      container_bound_type)
             : item->GetBoundTopFrom(container, bound_type,
                                     container_bound_type);
}

float GetPaddingAndBorderDimensionSize(const LayoutObject* item,
                                       Dimension axis) {
  return axis == Dimension::kHorizontal ? item->GetPaddingAndBorderHorizontal()
                                        : item->GetPaddingAndBorderVertical();
}

const NLength& GetCSSDimensionSize(const LayoutComputedStyle* cssStyle,
                                   Dimension axis) {
  return axis == Dimension::kHorizontal ? cssStyle->GetWidth()
                                        : cssStyle->GetHeight();
}

float ClampExactSize(const LayoutObject* item, float size, Dimension axis) {
  return axis == Dimension::kHorizontal ? item->ClampExactWidth(size)
                                        : item->ClampExactHeight(size);
}

LinearGravityType GetLogicGravityType(LinearGravityType physical_gravity_type,
                                      Direction main_front) {
  LinearGravityType logic_gravity = LinearGravityType::kStart;
  if (main_front == Direction::kLeft) {
    if (physical_gravity_type == LinearGravityType::kRight) {
      logic_gravity = LinearGravityType::kEnd;
    }
  } else if (main_front == Direction::kRight) {
    if (physical_gravity_type == LinearGravityType::kLeft) {
      logic_gravity = LinearGravityType::kEnd;
    }
  } else if (main_front == Direction::kTop) {
    if (physical_gravity_type == LinearGravityType::kBottom) {
      logic_gravity = LinearGravityType::kEnd;
    }
  } else if (main_front == Direction::kBottom) {
    if (physical_gravity_type == LinearGravityType::kTop) {
      logic_gravity = LinearGravityType::kEnd;
    }
  }

  return logic_gravity;
}

const NLength GetSurroundOffset(const LayoutComputedStyle* cssStyle,
                                Direction direction) {
  if (direction == Direction::kLeft) {
    return cssStyle->GetLeft();
  } else if (direction == Direction::kRight) {
    return cssStyle->GetRight();
  } else if (direction == Direction::kTop) {
    return cssStyle->GetTop();
  } else {
    return cssStyle->GetBottom();
  }
}

const NLength GetMargin(const LayoutComputedStyle* cssStyle,
                        Direction direction) {
  if (direction == Direction::kLeft) {
    return cssStyle->GetMarginLeft();
  } else if (direction == Direction::kRight) {
    return cssStyle->GetMarginRight();
  } else if (direction == Direction::kTop) {
    return cssStyle->GetMarginTop();
  } else {
    return cssStyle->GetMarginBottom();
  }
}

const NLength GetPadding(const LayoutComputedStyle* cssStyle,
                         Direction direction) {
  if (direction == Direction::kLeft) {
    return cssStyle->GetPaddingLeft();
  } else if (direction == Direction::kRight) {
    return cssStyle->GetPaddingRight();
  } else if (direction == Direction::kTop) {
    return cssStyle->GetPaddingTop();
  } else {
    return cssStyle->GetPaddingBottom();
  }
}

}  // namespace logic_direction_utils
}  // namespace starlight
}  // namespace lynx
