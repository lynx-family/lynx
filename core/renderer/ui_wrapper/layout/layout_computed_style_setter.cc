// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/layout_computed_style_setter.h"

#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/starlight/style/layout_computed_style.h"
namespace lynx::tasm {

LayoutComputedStyleSetter::LayoutComputedStyleSetter(
    const CssMeasureContext& measure_context,
    const CSSParserConfigs& parser_configs, bool css_align_with_legacy_w3c,
    starlight::LayoutComputedStyle& style)
    : measure_context(measure_context),
      parser_configs(parser_configs),
      css_align_with_legacy_w3c(css_align_with_legacy_w3c),
      computed_style(style) {}

#define SUPPORTED_LENGTH_PROPERTY(V)                                           \
  V(Width, NLength, computed_style.box_data_.Access()->width_, WIDTH)          \
  V(Height, NLength, computed_style.box_data_.Access()->height_, HEIGHT)       \
  V(MinWidth, NLength, computed_style.box_data_.Access()->min_width_,          \
    MIN_WIDTH)                                                                 \
  V(MinHeight, NLength, computed_style.box_data_.Access()->min_height_,        \
    MIN_HEIGHT)                                                                \
  V(MaxWidth, NLength, computed_style.box_data_.Access()->max_width_,          \
    MAX_WIDTH)                                                                 \
  V(MaxHeight, NLength, computed_style.box_data_.Access()->max_height_,        \
    MAX_HEIGHT)                                                                \
  V(FlexBasis, NLength, computed_style.flex_data_.Access()->flex_basis_,       \
    FLEX_BASIS)                                                                \
  V(Left, NLength, computed_style.surround_data_.left_, FOUR_POSITION)         \
  V(Right, NLength, computed_style.surround_data_.right_, FOUR_POSITION)       \
  V(Top, NLength, computed_style.surround_data_.top_, FOUR_POSITION)           \
  V(Bottom, NLength, computed_style.surround_data_.bottom_, FOUR_POSITION)     \
  V(PaddingLeft, NLength, computed_style.surround_data_.padding_left_,         \
    PADDING)                                                                   \
  V(PaddingRight, NLength, computed_style.surround_data_.padding_right_,       \
    PADDING)                                                                   \
  V(PaddingTop, NLength, computed_style.surround_data_.padding_top_, PADDING)  \
  V(PaddingBottom, NLength, computed_style.surround_data_.padding_bottom_,     \
    PADDING)                                                                   \
  V(MarginLeft, NLength, computed_style.surround_data_.margin_left_, MARGIN)   \
  V(MarginRight, NLength, computed_style.surround_data_.margin_right_, MARGIN) \
  V(MarginTop, NLength, computed_style.surround_data_.margin_top_, MARGIN)     \
  V(MarginBottom, NLength, computed_style.surround_data_.margin_bottom_,       \
    MARGIN)                                                                    \
  V(GridRowGap, NLength, computed_style.grid_data_.Access()->grid_row_gap_,    \
    GRID_GAP)                                                                  \
  V(GridColumnGap, NLength,                                                    \
    computed_style.grid_data_.Access()->grid_column_gap_, GRID_GAP)

#define LAYOUT_SUPPORTED_ENUM_PROPERTY(V)                                      \
  V(LinearOrientation,                                                         \
    computed_style.linear_data_.Access()->linear_orientation_,                 \
    LinearOrientationType, SL_DEFAULT_LINEAR_ORIENTATION)                      \
  V(LinearGravity, computed_style.linear_data_.Access()->linear_gravity_,      \
    LinearGravityType, SL_DEFAULT_LINEAR_GRAVITY)                              \
  V(LinearDirection,                                                           \
    computed_style.linear_data_.Access()->linear_orientation_,                 \
    LinearOrientationType, SL_DEFAULT_LINEAR_ORIENTATION)                      \
  V(LinearLayoutGravity,                                                       \
    computed_style.linear_data_.Access()->linear_layout_gravity_,              \
    LinearLayoutGravityType, SL_DEFAULT_LINEAR_LAYOUT_GRAVITY)                 \
  V(LinearCrossGravity,                                                        \
    computed_style.linear_data_.Access()->linear_cross_gravity_,               \
    LinearCrossGravityType, SL_DEFAULT_LINEAR_CROSS_GRAVITY)                   \
  V(RelativeCenter, computed_style.relative_data_.Access()->relative_center_,  \
    RelativeCenterType, SL_DEFAULT_RELATIVE_CENTER)                            \
  V(JustifyItems, computed_style.grid_data_.Access()->justify_items_,          \
    JustifyType, SL_DEFAULT_JUSTIFY_ITEMS)                                     \
  V(JustifySelf, computed_style.grid_data_.Access()->justify_self_,            \
    JustifyType, SL_DEFAULT_JUSTIFY_SELF)                                      \
  V(GridAutoFlow, computed_style.grid_data_.Access()->grid_auto_flow_,         \
    GridAutoFlowType, SL_DEFAULT_GRID_AUTO_FLOW)                               \
  V(FlexDirection, computed_style.flex_data_.Access()->flex_direction_,        \
    FlexDirectionType, SL_DEFAULT_FLEX_DIRECTION)                              \
  V(JustifyContent, computed_style.flex_data_.Access()->justify_content_,      \
    JustifyContentType, SL_DEFAULT_JUSTIFY_CONTENT)                            \
  V(FlexWrap, computed_style.flex_data_.Access()->flex_wrap_, FlexWrapType,    \
    SL_DEFAULT_FLEX_WRAP)                                                      \
  V(AlignItems, computed_style.flex_data_.Access()->align_items_,              \
    FlexAlignType, SL_DEFAULT_ALIGN_ITEMS)                                     \
  V(AlignSelf, computed_style.flex_data_.Access()->align_self_, FlexAlignType, \
    SL_DEFAULT_ALIGN_SELF)                                                     \
  V(AlignContent, computed_style.flex_data_.Access()->align_content_,          \
    AlignContentType, SL_DEFAULT_ALIGN_CONTENT)                                \
  V(Direction, computed_style.direction_, DirectionType, SL_DEFAULT_DIRECTION) \
  V(Display, computed_style.display_, DisplayType, SL_DEFAULT_DISPLAY)         \
  V(Position, computed_style.position_, PositionType, SL_DEFAULT_POSITION)

#define SET_ENUM_PROPERTY(NAME, FILED, TYPE, DEFAULT)                   \
  bool LayoutComputedStyleSetter::Set##NAME(const CSSValue& value,      \
                                            const bool reset) const {   \
    return starlight::CSSStyleUtils::ComputeEnumStyle<starlight::TYPE>( \
        value, reset, FILED, starlight::DefaultLayoutStyle::DEFAULT,    \
        "NAME must be a enum!", parser_configs);                        \
  }
LAYOUT_SUPPORTED_ENUM_PROPERTY(SET_ENUM_PROPERTY)
#undef SET_ENUM_PROPERTY

#define SET_LENGTH_PROPERTY(type_name, length, css_type, default_type)     \
  bool LayoutComputedStyleSetter::Set##type_name(const CSSValue& value,    \
                                                 const bool reset) const { \
    return starlight::CSSStyleUtils::ComputeLengthStyle(                   \
        value, reset, measure_context, css_type,                           \
        starlight::DefaultLayoutStyle::SL_DEFAULT_##default_type(),        \
        parser_configs);                                                   \
  }  // namespace tasm
SUPPORTED_LENGTH_PROPERTY(SET_LENGTH_PROPERTY)
#undef SET_LENGTH_PROPERTY

#define SUPPORTED_NUMBER_PROPERTY(V)                                           \
  V(FlexGrow, computed_style.flex_data_.Access()->flex_grow_, Float,           \
    SL_DEFAULT_FLEX_GROW)                                                      \
  V(FlexShrink, computed_style.flex_data_.Access()->flex_shrink_, Float,       \
    SL_DEFAULT_FLEX_SHRINK)                                                    \
  V(Order, computed_style.flex_data_.Access()->order_, Float,                  \
    SL_DEFAULT_ORDER)                                                          \
  V(LinearWeightSum, computed_style.linear_data_.Access()->linear_weight_sum_, \
    Float, SL_DEFAULT_LINEAR_WEIGHT_SUM)                                       \
  V(LinearWeight, computed_style.linear_data_.Access()->linear_weight_, Float, \
    SL_DEFAULT_LINEAR_WEIGHT)                                                  \
  V(AspectRatio, computed_style.box_data_.Access()->aspect_ratio_, Float,      \
    SL_DEFAULT_ASPECT_RATIO)                                                   \
  V(RelativeId, computed_style.relative_data_.Access()->relative_id_, Int,     \
    SL_DEFAULT_RELATIVE_ID)                                                    \
  V(RelativeAlignTop,                                                          \
    computed_style.relative_data_.Access()->relative_align_top_, Int,          \
    SL_DEFAULT_RELATIVE_ALIGN_TOP)                                             \
  V(RelativeAlignRight,                                                        \
    computed_style.relative_data_.Access()->relative_align_right_, Int,        \
    SL_DEFAULT_RELATIVE_ALIGN_RIGHT)                                           \
  V(RelativeAlignBottom,                                                       \
    computed_style.relative_data_.Access()->relative_align_bottom_, Int,       \
    SL_DEFAULT_RELATIVE_ALIGN_BOTTOM)                                          \
  V(RelativeAlignLeft,                                                         \
    computed_style.relative_data_.Access()->relative_align_left_, Int,         \
    SL_DEFAULT_RELATIVE_ALIGN_LEFT)                                            \
  V(RelativeTopOf, computed_style.relative_data_.Access()->relative_top_of_,   \
    Int, SL_DEFAULT_RELATIVE_TOP_OF)                                           \
  V(RelativeRightOf,                                                           \
    computed_style.relative_data_.Access()->relative_right_of_, Int,           \
    SL_DEFAULT_RELATIVE_RIGHT_OF)                                              \
  V(RelativeBottomOf,                                                          \
    computed_style.relative_data_.Access()->relative_bottom_of_, Int,          \
    SL_DEFAULT_RELATIVE_BOTTOM_OF)                                             \
  V(RelativeLeftOf, computed_style.relative_data_.Access()->relative_left_of_, \
    Int, SL_DEFAULT_RELATIVE_LEFT_OF)                                          \
  V(GridColumnSpan, computed_style.grid_data_.Access()->grid_column_span_,     \
    Int, SL_DEFAULT_GRID_SPAN)                                                 \
  V(GridRowSpan, computed_style.grid_data_.Access()->grid_row_span_, Int,      \
    SL_DEFAULT_GRID_SPAN)                                                      \
  V(GridRowStart, computed_style.grid_data_.Access()->grid_row_start_, Int,    \
    SL_DEFAULT_GRID_ITEM_POSITION)                                             \
  V(GridRowEnd, computed_style.grid_data_.Access()->grid_row_end_, Int,        \
    SL_DEFAULT_GRID_ITEM_POSITION)                                             \
  V(GridColumnStart, computed_style.grid_data_.Access()->grid_column_start_,   \
    Int, SL_DEFAULT_GRID_ITEM_POSITION)                                        \
  V(GridColumnEnd, computed_style.grid_data_.Access()->grid_column_end_, Int,  \
    SL_DEFAULT_GRID_ITEM_POSITION)

#define SET_NUMBER_PROPERTY(NAME, FIELD, TYPE, DEFAULT)               \
  bool LayoutComputedStyleSetter::Set##NAME(const CSSValue& value,    \
                                            const bool reset) const { \
    return starlight::CSSStyleUtils::Compute##TYPE##Style(            \
        value, reset, FIELD, starlight::DefaultLayoutStyle::DEFAULT,  \
        #NAME " must be a " #TYPE "!", parser_configs);               \
  }

SUPPORTED_NUMBER_PROPERTY(SET_NUMBER_PROPERTY)
#undef SET_NUMBER_PROPERTY

bool LayoutComputedStyleSetter::SetBoxSizing(const tasm::CSSValue& value,
                                             const bool reset) const {
  const auto old_value = computed_style.box_sizing_;
  if (reset) {
    computed_style.box_sizing_ = starlight::BoxSizingType::kAuto;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsEnum(), parser_configs.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDBoxSizing).c_str(),
        tasm::ENUM_TYPE)

    computed_style.box_sizing_ =
        static_cast<starlight::BoxSizingType>(value.GetValue().Number());
  }
  return old_value != computed_style.box_sizing_;
}

bool LayoutComputedStyleSetter::SetBorderWidth(const tasm::CSSValue& value,
                                               const bool reset) const {
  return UnitHandler::CSSMethodUnreachable(
      parser_configs.enable_css_strict_mode);
}

#ifdef DEFAULT_CSS_VALUE
#undef DEFAULT_CSS_VALUE
#endif

#define DEFAULT_CSS_VALUE(type, name)                       \
  (type ? starlight::DefaultLayoutStyle::W3C_DEFAULT_##name \
        : starlight::DefaultLayoutStyle::SL_DEFAULT_##name)

static bool CalculateFromBorderWidthStringToFloat(
    const tasm::CSSValue& value, float& result,
    const tasm::CssMeasureContext& context, const bool reset,
    const bool css_align_with_legacy_w3c,
    const tasm::CSSParserConfigs& configs) {
  if (reset) {
    result = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER);
    return true;
  }

  auto [length, success] =
      starlight::CSSStyleUtils::ToLength(value, context, configs);
  if (!success || (!length.IsUnit() && !length.IsCalc()) ||
      (length.IsCalc() && length.NumericLength().ContainsPercentage())) {
    return false;
  }
  result = starlight::CSSStyleUtils::GetBorderWidthFromLengthToFloat(length,
                                                                     context);
  return true;
}

static bool SetBorderWidthHelper(const bool cssAlignWithLegacyW3C,
                                 const tasm::CssMeasureContext& context,
                                 float& width, const tasm::CSSValue& value,
                                 const tasm::CSSParserConfigs& configs,
                                 const bool reset) {
  const float old_value = width;
  if (UNLIKELY(!CalculateFromBorderWidthStringToFloat(
          value, width, context, reset, cssAlignWithLegacyW3C, configs))) {
    return false;
  }
  return width != old_value;
}

bool LayoutComputedStyleSetter::SetBorderLeftWidth(const tasm::CSSValue& value,
                                                   const bool reset) const {
  starlight::CSSStyleUtils::PrepareOptional(
      computed_style.surround_data_.border_data_, css_align_with_legacy_w3c);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c, measure_context,
      computed_style.surround_data_.border_data_->width_left, value,
      parser_configs, reset);
}
bool LayoutComputedStyleSetter::SetBorderRightWidth(const tasm::CSSValue& value,
                                                    const bool reset) const {
  starlight::CSSStyleUtils::PrepareOptional(
      computed_style.surround_data_.border_data_, css_align_with_legacy_w3c);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c, measure_context,
      computed_style.surround_data_.border_data_->width_right, value,
      parser_configs, reset);
}

bool LayoutComputedStyleSetter::SetBorderTopWidth(const tasm::CSSValue& value,
                                                  const bool reset) const {
  starlight::CSSStyleUtils::PrepareOptional(
      computed_style.surround_data_.border_data_, css_align_with_legacy_w3c);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c, measure_context,
      computed_style.surround_data_.border_data_->width_top, value,
      parser_configs, reset);
}

bool LayoutComputedStyleSetter::SetBorderBottomWidth(
    const tasm::CSSValue& value, const bool reset) const {
  starlight::CSSStyleUtils::PrepareOptional(
      computed_style.surround_data_.border_data_, css_align_with_legacy_w3c);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c, measure_context,
      computed_style.surround_data_.border_data_->width_bottom, value,
      parser_configs, reset);
}

bool LayoutComputedStyleSetter::SetPadding(const tasm::CSSValue& value,
                                           const bool reset) const {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs.enable_css_strict_mode);
}
bool LayoutComputedStyleSetter::SetMargin(const tasm::CSSValue& value,
                                          const bool reset) const {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs.enable_css_strict_mode);
}

bool LayoutComputedStyleSetter::SetFlex(const tasm::CSSValue& value,
                                        const bool reset) const {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs.enable_css_strict_mode);
}

bool LayoutComputedStyleSetter::SetBorder(const tasm::CSSValue& value,
                                          const bool reset) const {
  return false;
}

bool LayoutComputedStyleSetter::SetBorderRight(const tasm::CSSValue& value,
                                               const bool reset) const {
  return false;
}
bool LayoutComputedStyleSetter::SetBorderLeft(const tasm::CSSValue& value,
                                              const bool reset) const {
  return false;
}
bool LayoutComputedStyleSetter::SetBorderTop(const tasm::CSSValue& value,
                                             const bool reset) const {
  return false;
}
bool LayoutComputedStyleSetter::SetBorderBottom(const tasm::CSSValue& value,
                                                const bool reset) const {
  return false;
}

bool LayoutComputedStyleSetter::SetContent(const tasm::CSSValue& value,
                                           const bool reset) const {
  // deprecated.
  return false;
}

bool LayoutComputedStyleSetter::SetRelativeLayoutOnce(
    const tasm::CSSValue& value, const bool reset) const {
  return starlight::CSSStyleUtils::ComputeBoolStyle(
      value, reset,
      computed_style.relative_data_.Access()->relative_layout_once_,
      starlight::DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LAYOUT_ONCE,
      "relative-layout-once must be a bool!", parser_configs);
}

bool LayoutComputedStyleSetter::SetGridTemplateColumns(
    const tasm::CSSValue& value, const bool reset) const {
  return starlight::CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, measure_context,
      computed_style.grid_data_.Access()
          ->grid_template_columns_min_track_sizing_function_,
      computed_style.grid_data_.Access()
          ->grid_template_columns_max_track_sizing_function_,
      starlight::DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-template-columns must be an array!", parser_configs);
}
bool LayoutComputedStyleSetter::SetGridTemplateRows(const tasm::CSSValue& value,
                                                    const bool reset) const {
  return starlight::CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, measure_context,
      computed_style.grid_data_.Access()
          ->grid_template_rows_min_track_sizing_function_,
      computed_style.grid_data_.Access()
          ->grid_template_rows_max_track_sizing_function_,
      starlight::DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-template-rows must be an array!", parser_configs);
}
bool LayoutComputedStyleSetter::SetGridAutoColumns(const tasm::CSSValue& value,
                                                   const bool reset) const {
  return starlight::CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, measure_context,
      computed_style.grid_data_.Access()
          ->grid_auto_columns_min_track_sizing_function_,
      computed_style.grid_data_.Access()
          ->grid_auto_columns_max_track_sizing_function_,
      starlight::DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-auto-columns must be an array!", parser_configs);
}
bool LayoutComputedStyleSetter::SetGridAutoRows(const tasm::CSSValue& value,
                                                const bool reset) const {
  return starlight::CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, measure_context,
      computed_style.grid_data_.Access()
          ->grid_auto_rows_min_track_sizing_function_,
      computed_style.grid_data_.Access()
          ->grid_auto_rows_max_track_sizing_function_,
      starlight::DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-auto-rows must be an array!", parser_configs);
}

bool LayoutComputedStyleSetter::SetListCrossAxisGap(const CSSValue& value,
                                                    const bool reset) const {
  return starlight::CSSStyleUtils::ComputeLengthStyle(
      value, reset, measure_context, computed_style.list_cross_axis_gap_,
      starlight::DefaultLayoutStyle::SL_DEFAULT_ZEROLENGTH(), parser_configs);
}

bool LayoutComputedStyleSetter::SetGap(const CSSValue& value,
                                       const bool reset) const {
  return UnitHandler::CSSMethodUnreachable(
      parser_configs.enable_css_strict_mode);
}

bool LayoutComputedStyleSetter::SetColumnGap(const CSSValue& value,
                                             const bool reset) const {
  return SetGridColumnGap(value, reset);
}

bool LayoutComputedStyleSetter::SetRowGap(const tasm::CSSValue& value,
                                          const bool reset) const {
  return SetGridRowGap(value, reset);
}

using SetterFunc = bool (LayoutComputedStyleSetter::*)(const CSSValue& value,
                                                       const bool reset) const;

static constexpr std::array<SetterFunc, kPropertyEnd> kWriter = [] {
  std::array<SetterFunc, kPropertyEnd> writer = {nullptr};
#define BIND_SETTER_FUN(NAME, STATUS)              \
  writer[tasm::CSSPropertyID::kPropertyID##NAME] = \
      &LayoutComputedStyleSetter::Set##NAME;
  FOREACH_LAYOUT_PROPERTY(BIND_SETTER_FUN)
  return writer;
}();

bool LayoutComputedStyleSetter::SetValue(const CSSPropertyID id,
                                         const CSSValue& value,
                                         const bool reset) const {
  if (id > kPropertyStart && id < kPropertyEnd) {
    if (const SetterFunc setter = kWriter[id]) {
      return (this->*setter)(value, reset);
    }
  }
  return false;
}

}  // namespace lynx::tasm
