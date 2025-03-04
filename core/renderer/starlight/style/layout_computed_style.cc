// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/starlight/style/layout_computed_style.h"

#include "base/include/debug/lynx_assert.h"

namespace lynx {
namespace starlight {

LayoutComputedStyle::LayoutComputedStyle(
    double physical_pixels_per_layout_unit) {
  box_data_.Init();
  flex_data_.Init();
  grid_data_.Init();
  linear_data_.Init();
  relative_data_.Init();
  physical_pixels_per_layout_unit_ = physical_pixels_per_layout_unit;
}

LayoutComputedStyle::LayoutComputedStyle(const LayoutComputedStyle& o) {
  box_data_ = o.box_data_;
  flex_data_ = o.flex_data_;
  grid_data_ = o.grid_data_;
  linear_data_ = o.linear_data_;
  relative_data_ = o.relative_data_;
  physical_pixels_per_layout_unit_ = o.physical_pixels_per_layout_unit_;
}

void LayoutComputedStyle::Reset() {
  box_data_.Access()->Reset();
  flex_data_.Access()->Reset();
  grid_data_.Access()->Reset();
  linear_data_.Access()->Reset();
  relative_data_.Access()->Reset();
  surround_data_.Reset();

  position_ = DefaultLayoutStyle::SL_DEFAULT_POSITION;
  display_ = DefaultLayoutStyle::SL_DEFAULT_DISPLAY;
  direction_ = DefaultLayoutStyle::SL_DEFAULT_DIRECTION;
  box_sizing_ = DefaultLayoutStyle::SL_DEFAULT_BOX_SIZING;
}

DisplayType LayoutComputedStyle::GetDisplay(
    const LayoutConfigs& configs, const AttributesMap& attributes) const {
  const auto scroll = attributes.getScroll();

  if (scroll.has_value() && *scroll && display_ != DisplayType::kNone) {
    return DisplayType::kLinear;
  }

  if (display_ == DisplayType::kAuto) {
    if (!configs.css_align_with_legacy_w3c_ &&
        !configs.default_display_linear_) {
      return DisplayType::kFlex;
    } else {
      *(const_cast<DisplayType*>(&display_)) = DisplayType::kLinear;
      return DisplayType::kLinear;
    }
  } else if (display_ == DisplayType::kBlock) {
    if (!configs.css_align_with_legacy_w3c_) {
      LOGW("Unexpected display type:" << (int)display_
                                      << "!! Fall back to default display.");
      return DisplayType::kFlex;
    } else {
      return DisplayType::kLinear;
    }
  }
  return display_;
}

bool LayoutComputedStyle::DirectionIsReverse(const LayoutConfigs& configs,
                                             AttributesMap& attributes) const {
  auto display = GetDisplay(configs, attributes);
  if (display == DisplayType::kFlex) {
    return flex_data_->flex_direction_ == FlexDirectionType::kColumnReverse ||
           flex_data_->flex_direction_ == FlexDirectionType::kRowReverse;
  } else if (display == DisplayType::kLinear) {
    return linear_data_->linear_orientation_ ==
               LinearOrientationType::kHorizontalReverse ||
           linear_data_->linear_orientation_ ==
               LinearOrientationType::kVerticalReverse ||
           linear_data_->linear_orientation_ ==
               LinearOrientationType::kRowReverse ||
           linear_data_->linear_orientation_ ==
               LinearOrientationType::kColumnReverse;
  }
  return false;
}

bool LayoutComputedStyle::IsFlexRow(const LayoutConfigs& configs,
                                    const AttributesMap& attributes) const {
  if (GetDisplay(configs, attributes) == DisplayType::kFlex) {
    return flex_data_->flex_direction_ == FlexDirectionType::kRow ||
           flex_data_->flex_direction_ == FlexDirectionType::kRowReverse;
  }
  return false;
}

bool LayoutComputedStyle::IsRow(const LayoutConfigs& configs,
                                const AttributesMap& attributes) const {
  auto display = GetDisplay(configs, attributes);
  if (display == DisplayType::kFlex) {
    return flex_data_->flex_direction_ == FlexDirectionType::kRow ||
           flex_data_->flex_direction_ == FlexDirectionType::kRowReverse;
  } else if (display == DisplayType::kLinear) {
    return (linear_data_->linear_orientation_ ==
            LinearOrientationType::kHorizontal) ||
           (linear_data_->linear_orientation_ ==
            LinearOrientationType::kHorizontalReverse) ||
           (linear_data_->linear_orientation_ == LinearOrientationType::kRow) ||
           (linear_data_->linear_orientation_ ==
            LinearOrientationType::kRowReverse);
  }
  return true;
}

bool LayoutComputedStyle::IsBorderBox(const LayoutConfigs& configs) const {
  switch (box_sizing_) {
    case BoxSizingType::kBorderBox:
      return true;
    case BoxSizingType::kContentBox:
      return false;
    default:
      return !configs.css_align_with_legacy_w3c_;
  }
}

}  // namespace starlight
}  // namespace lynx
