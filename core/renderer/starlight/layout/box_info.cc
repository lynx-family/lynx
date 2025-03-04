// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/box_info.h"

#include <cmath>

#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/layout_directions.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {

namespace {
void SetIfChanged(bool& dirty_flag, float& target, float new_value) {
  if (base::FloatsNotEqual(target, new_value)) {
    dirty_flag = true;
    target = new_value;
  }
}

bool MarkShouldModify(const NLength& length) {
  return length.IsPercent() ? true : false;
}
}  // namespace

BoxInfo::BoxInfo() { ResetBoxInfo(); }

void BoxInfo::ResetBoxInfo() {
  std::fill(min_size_.begin(), min_size_.end(), 0.0f);
  std::fill(max_size_.begin(), max_size_.end(), CSS_UNDEFINED);
  std::fill(padding_.begin(), padding_.end(), 0.0f);
  std::fill(margin_.begin(), margin_.end(), 0.0f);

  box_info_props_modified = 0;

  values_of_width_modify_ = false;
  values_of_height_modify_ = false;

  std::fill(min_should_modify_.begin(), min_should_modify_.end(), true);
  std::fill(max_should_modify_.begin(), max_should_modify_.end(), true);
  std::fill(padding_should_modify_.begin(), padding_should_modify_.end(), true);
  std::fill(margin_should_modify_.begin(), margin_should_modify_.end(), true);
}

void BoxInfo::SetBoxInfoPropsModified() {
  box_info_props_modified = 1;

  std::fill(min_should_modify_.begin(), min_should_modify_.end(), true);
  std::fill(max_should_modify_.begin(), max_should_modify_.end(), true);
  std::fill(padding_should_modify_.begin(), padding_should_modify_.end(), true);
  std::fill(margin_should_modify_.begin(), margin_should_modify_.end(), true);
}

void BoxInfo::InitializeMarginPadding(const NLength& length,
                                      const LayoutUnit& available_size,
                                      float& value) {
  value = CalculateLengthValue(length, available_size);
}

void BoxInfo::ResolveMinMax(const NLength& width, const NLength& height,
                            const LayoutUnit& available_width,
                            const LayoutUnit& available_height,
                            const LayoutConfigs& layout_config,
                            const LayoutComputedStyle& style,
                            float default_value, DimensionValue<float>& value) {
  DimensionValue<LayoutUnit> size;
  size[kHorizontal] = NLengthToLayoutUnit(width, available_width);
  size[kVertical] = NLengthToLayoutUnit(height, available_height);
  if (!layout_config.IsFullQuirksMode()) {
    property_utils::HandleBoxSizing(style, *this, size, layout_config);
  }
  value[kHorizontal] = size[kHorizontal].IsDefinite()
                           ? size[kHorizontal].ToFloat()
                           : default_value;
  value[kVertical] =
      size[kVertical].IsDefinite() ? size[kVertical].ToFloat() : default_value;
}

// After containing block in formed, used to resolve box info that contains
// percentage, e.g., padding:calc(10% + 20px), margin:10%.
void BoxInfo::ResolveBoxInfoForAbsoluteAndFixed(
    const Constraints& constraints, LayoutObject& obj,
    const LayoutConfigs& layout_config) {
  if (layout_config.IsAbsoluteAndFixedBoxInfoQuirksMode()) {
    return;
  }
  const auto& style = *(obj.GetCSSStyle());
  const auto& available_width = constraints[kHorizontal].ToPercentBase();
  const auto& available_height = constraints[kVertical].ToPercentBase();

  for (int32_t dir_index = 0; dir_index < kDirectionCount; ++dir_index) {
    Direction direction = static_cast<Direction>(dir_index);
    const NLength& margin = logic_direction_utils::GetMargin(&style, direction);
    const NLength& padding =
        logic_direction_utils::GetPadding(&style, direction);
    if (margin.ContainsPercentage()) {
      InitializeMarginPadding(margin, available_width, margin_[direction]);
    }
    if (padding.ContainsPercentage()) {
      InitializeMarginPadding(padding, available_width, padding_[direction]);
    }
  }
  if (style.GetMinWidth().ContainsPercentage() ||
      style.GetMinHeight().ContainsPercentage()) {
    ResolveMinMax(style.GetMinWidth(), style.GetMinHeight(), available_width,
                  available_height, layout_config, style,
                  DefaultLayoutStyle::kDefaultMinSize, min_size_);
  }
  if (style.GetMaxWidth().ContainsPercentage() ||
      style.GetMaxHeight().ContainsPercentage()) {
    ResolveMinMax(style.GetMaxWidth(), style.GetMaxHeight(), available_width,
                  available_height, layout_config, style,
                  DefaultLayoutStyle::kDefaultMaxSize, max_size_);
  }
}

void BoxInfo::InitializeBoxInfo(const Constraints& constraints,
                                LayoutObject& obj,
                                const LayoutConfigs& layout_config) {
  // The changes of Minimum size and padding in box info will be treated as
  // changing the CSS. Cache will be cleaned if any of this changed
  const auto& style = *(obj.GetCSSStyle());

  values_of_width_modify_ = false;
  values_of_height_modify_ = false;

  bool dirty = false;
  const auto& available_width = constraints[kHorizontal].ToPercentBase();
  const auto& available_height = constraints[kVertical].ToPercentBase();

  for (int32_t dir_index = 0; dir_index < kDirectionCount; ++dir_index) {
    Direction direction = static_cast<Direction>(dir_index);
    InitializeMarginPadding(logic_direction_utils::GetMargin(&style, direction),
                            available_width, margin_[direction]);
    margin_should_modify_[direction] =
        MarkShouldModify(logic_direction_utils::GetMargin(&style, direction));
    float padding_new_value = 0;
    InitializeMarginPadding(
        logic_direction_utils::GetPadding(&style, direction), available_width,
        padding_new_value);

    padding_should_modify_[direction] =
        MarkShouldModify(logic_direction_utils::GetPadding(&style, direction));
    SetIfChanged(dirty, padding_[direction], padding_new_value);

    if (!values_of_width_modify_) {
      values_of_width_modify_ =
          padding_should_modify_[direction] || margin_should_modify_[direction];
    }
  }

  {
    min_should_modify_[kHorizontal] = MarkShouldModify(style.GetMinWidth());
    min_should_modify_[kVertical] = MarkShouldModify(style.GetMinHeight());
    if (!values_of_width_modify_) {
      values_of_width_modify_ = min_should_modify_[kHorizontal];
    }
    if (!values_of_height_modify_) {
      values_of_height_modify_ = min_should_modify_[kVertical];
    }

    DimensionValue<float> result;
    ResolveMinMax(style.GetMinWidth(), style.GetMinHeight(), available_width,
                  available_height, layout_config, style,
                  DefaultLayoutStyle::kDefaultMinSize, result);

    SetIfChanged(dirty, min_size_[kHorizontal], result[kHorizontal]);
    SetIfChanged(dirty, min_size_[kVertical], result[kVertical]);
  }

  {
    max_should_modify_[kHorizontal] = MarkShouldModify(style.GetMaxWidth());
    max_should_modify_[kVertical] = MarkShouldModify(style.GetMaxHeight());

    if (!values_of_width_modify_) {
      values_of_width_modify_ = max_should_modify_[kHorizontal];
    }
    if (!values_of_height_modify_) {
      values_of_height_modify_ = max_should_modify_[kVertical];
    }

    ResolveMinMax(style.GetMaxWidth(), style.GetMaxHeight(), available_width,
                  available_height, layout_config, style,
                  DefaultLayoutStyle::kDefaultMaxSize, max_size_);
  }
  if (dirty) {
    obj.ClearCache();
  }
  box_info_props_modified = 0;
}

float BoxInfo::CalculateLengthValue(const NLength& length,
                                    const LayoutUnit& available_width) {
  return NLengthToLayoutUnit(length, available_width)
      .ClampIndefiniteToZero()
      .ToFloat();
}

void BoxInfo::UpdateHorizontalBoxData(const LayoutUnit& available_width,
                                      const LayoutComputedStyle& style,
                                      bool& dirty) {
  if (!values_of_width_modify_) {
    return;
  }

  for (int32_t dir_index = 0; dir_index < kDirectionCount; ++dir_index) {
    Direction direction = static_cast<Direction>(dir_index);
    if (padding_should_modify_[direction]) {
      const float new_value = CalculateLengthValue(
          logic_direction_utils::GetPadding(&style, direction),
          available_width);
      SetIfChanged(dirty, padding_[direction], new_value);
    }

    if (margin_should_modify_[direction]) {
      margin_[direction] = CalculateLengthValue(
          logic_direction_utils::GetMargin(&style, direction), available_width);
    }
  }
}

void BoxInfo::UpdateBoxData(const Constraints& constraints, LayoutObject& obj,
                            const LayoutConfigs& layout_config) {
  const LayoutUnit& available_width = constraints[kHorizontal].ToPercentBase();
  const LayoutUnit& available_height = constraints[kVertical].ToPercentBase();
  const auto& style = *(obj.GetCSSStyle());
  bool dirty = false;
  UpdateHorizontalBoxData(available_width, style, dirty);
  // rely on padding size.
  if (min_should_modify_[kHorizontal] || min_should_modify_[kVertical]) {
    DimensionValue<float> result;
    ResolveMinMax(style.GetMinWidth(), style.GetMinHeight(), available_width,
                  available_height, layout_config, style,
                  DefaultLayoutStyle::kDefaultMinSize, result);

    SetIfChanged(dirty, min_size_[kHorizontal], result[kHorizontal]);
    SetIfChanged(dirty, min_size_[kVertical], result[kVertical]);
  }

  if (max_should_modify_[kHorizontal] || max_should_modify_[kVertical]) {
    ResolveMinMax(style.GetMaxWidth(), style.GetMaxHeight(), available_width,
                  available_height, layout_config, style,
                  DefaultLayoutStyle::kDefaultMaxSize, max_size_);
  }

  if (dirty) {
    obj.ClearCache();
  }
}
}  // namespace starlight
}  // namespace lynx
