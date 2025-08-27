// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_LAYOUT_COMPUTED_STYLE_H_
#define CORE_RENDERER_STARLIGHT_STYLE_LAYOUT_COMPUTED_STYLE_H_

#include <vector>

#include "core/renderer/starlight/style/box_data.h"
#include "core/renderer/starlight/style/data_ref.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/style/flex_data.h"
#include "core/renderer/starlight/style/grid_data.h"
#include "core/renderer/starlight/style/layout_style_utils.h"
#include "core/renderer/starlight/style/linear_data.h"
#include "core/renderer/starlight/style/relative_data.h"
#include "core/renderer/starlight/style/surround_data.h"
#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {

namespace starlight {

class LayoutComputedStyle {
 public:
  LYNX_EXPORT LayoutComputedStyle(double physical_pixels_per_layout_unit);
  LYNX_EXPORT LayoutComputedStyle(const LayoutComputedStyle& o);
  ~LayoutComputedStyle() = default;

  void Reset();

  bool DirectionIsReverse(const LayoutConfigs& configs,
                          AttributesMap& attributes) const;
  bool IsRow(const LayoutConfigs& configs,
             const AttributesMap& attributes) const;
  bool IsFlexRow(const LayoutConfigs& configs,
                 const AttributesMap& attributes) const;
  bool IsBorderBox(const LayoutConfigs& configs) const;
  LYNX_EXPORT bool IsRtl() const { return direction_ == DirectionType::kRtl; }
  bool IsLynxRtl() const { return direction_ == DirectionType::kLynxRtl; }
  bool IsAnyRtl() const { return IsRtl() || IsLynxRtl(); }

  DirectionType GetDirection() const { return direction_; }
  PositionType GetPosition() const { return position_; }
  DisplayType GetDisplay(const LayoutConfigs& configs,
                         const AttributesMap& attributes) const;

  DataRef<BoxData> box_data_;
  DataRef<FlexData> flex_data_;
  DataRef<GridData> grid_data_;
  DataRef<LinearData> linear_data_;
  DataRef<RelativeData> relative_data_;
  SurroundData surround_data_;

  BoxSizingType box_sizing_{DefaultLayoutStyle::SL_DEFAULT_BOX_SIZING};
  DisplayType display_{DefaultLayoutStyle::SL_DEFAULT_DISPLAY};
  PositionType position_{DefaultLayoutStyle::SL_DEFAULT_POSITION};
  DirectionType direction_{DefaultLayoutStyle::SL_DEFAULT_DIRECTION};

  // BoxData
  const NLength& GetWidth() const { return box_data_->width_; }
  const NLength& GetHeight() const { return box_data_->height_; }
  const NLength& GetMinWidth() const { return box_data_->min_width_; }
  const NLength& GetMaxWidth() const { return box_data_->max_width_; }
  const NLength& GetMinHeight() const { return box_data_->min_height_; }
  const NLength& GetMaxHeight() const { return box_data_->max_height_; }
  float GetAspectRatio() const { return box_data_->aspect_ratio_; }

  // LinearData
  LinearOrientationType GetLinearOrientation() const {
    return linear_data_->linear_orientation_;
  }
  LinearLayoutGravityType GetLinearLayoutGravity() const {
    return linear_data_->linear_layout_gravity_;
  }
  LinearGravityType GetLinearGravity() const {
    return linear_data_->linear_gravity_;
  }

  LinearCrossGravityType GetLinearCrossGravity() const {
    return linear_data_->linear_cross_gravity_;
  }

  float GetLinearWeightSum() const { return linear_data_->linear_weight_sum_; }
  float GetLinearWeight() const { return linear_data_->linear_weight_; }

  // RelativeData
  int GetRelativeId() const { return relative_data_->relative_id_; }
  int GetRelativeAlignTop() const {
    return relative_data_->relative_align_top_;
  }
  int GetRelativeAlignRight() const {
    return relative_data_->relative_align_right_;
  }
  int GetRelativeAlignBottom() const {
    return relative_data_->relative_align_bottom_;
  }
  int GetRelativeAlignLeft() const {
    return relative_data_->relative_align_left_;
  }
  int GetRelativeTopOf() const { return relative_data_->relative_top_of_; }
  int GetRelativeRightOf() const { return relative_data_->relative_right_of_; }
  int GetRelativeBottomOf() const {
    return relative_data_->relative_bottom_of_;
  }
  int GetRelativeLeftOf() const { return relative_data_->relative_left_of_; }
  bool GetRelativeLayoutOnce() const {
    return relative_data_->relative_layout_once_;
  }
  RelativeCenterType GetRelativeCenter() const {
    return relative_data_->relative_center_;
  }

  int32_t GetGridColumnStart() const { return grid_data_->grid_column_start_; }
  int32_t GetGridColumnEnd() const { return grid_data_->grid_column_end_; }
  int32_t GetGridRowStart() const { return grid_data_->grid_row_start_; }
  int32_t GetGridRowEnd() const { return grid_data_->grid_row_end_; }
  int32_t GetGridColumnSpan() const { return grid_data_->grid_column_span_; }
  int32_t GetGridRowSpan() const { return grid_data_->grid_row_span_; }

  const std::vector<NLength>& GetGridTemplateColumns() const {
    return grid_data_->grid_template_columns_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridTemplateRows() const {
    return grid_data_->grid_template_rows_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridAutoColumns() const {
    return grid_data_->grid_auto_columns_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridAutoRows() const {
    return grid_data_->grid_auto_rows_min_track_sizing_function_;
  }

  const std::vector<NLength>& GetGridTemplateColumnsMinTrackingFunction()
      const {
    return grid_data_->grid_template_columns_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridTemplateColumnsMaxTrackingFunction()
      const {
    return grid_data_->grid_template_columns_max_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridTemplateRowsMinTrackingFunction() const {
    return grid_data_->grid_template_rows_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridTemplateRowsMaxTrackingFunction() const {
    return grid_data_->grid_template_rows_max_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridAutoColumnsMinTrackingFunction() const {
    return grid_data_->grid_auto_columns_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridAutoColumnsMaxTrackingFunction() const {
    return grid_data_->grid_auto_columns_max_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridAutoRowsMinTrackingFunction() const {
    return grid_data_->grid_auto_rows_min_track_sizing_function_;
  }
  const std::vector<NLength>& GetGridAutoRowsMaxTrackingFunction() const {
    return grid_data_->grid_auto_rows_max_track_sizing_function_;
  }
  const NLength& GetGridColumnGap() const {
    return grid_data_->grid_column_gap_;
  }
  const NLength& GetGridRowGap() const { return grid_data_->grid_row_gap_; }
  GridAutoFlowType GetGridAutoFlow() const {
    return grid_data_->grid_auto_flow_;
  }
  JustifyType GetJustifySelfType() const { return grid_data_->justify_self_; }
  JustifyType GetJustifyItemsType() const { return grid_data_->justify_items_; }

  // FlexData
#define STYLE_GET_FLEX_PROPERTY(return_value, func_name, property_name) \
  return_value Get##func_name() const { return flex_data_->property_name; }
  STYLE_GET_FLEX_PROPERTY(float, FlexGrow, flex_grow_)
  STYLE_GET_FLEX_PROPERTY(float, FlexShrink, flex_shrink_)
  STYLE_GET_FLEX_PROPERTY(const NLength&, FlexBasis, flex_basis_)
  STYLE_GET_FLEX_PROPERTY(FlexDirectionType, FlexDirection, flex_direction_)
  STYLE_GET_FLEX_PROPERTY(FlexWrapType, FlexWrap, flex_wrap_)
  STYLE_GET_FLEX_PROPERTY(JustifyContentType, JustifyContent, justify_content_)
  STYLE_GET_FLEX_PROPERTY(FlexAlignType, AlignItems, align_items_)
  STYLE_GET_FLEX_PROPERTY(FlexAlignType, AlignSelf, align_self_)
  STYLE_GET_FLEX_PROPERTY(AlignContentType, AlignContent, align_content_);
  STYLE_GET_FLEX_PROPERTY(float, Order, order_)
#undef STYLE_GET_FLEX_PROPERTY

  // SurroundData
  const NLength& GetLeft() const { return surround_data_.left_; }
  const NLength& GetRight() const { return surround_data_.right_; }
  const NLength& GetTop() const { return surround_data_.top_; }
  const NLength& GetBottom() const { return surround_data_.bottom_; }
  const NLength& GetPaddingLeft() const { return surround_data_.padding_left_; }
  const NLength& GetPaddingRight() const {
    return surround_data_.padding_right_;
  }
  const NLength& GetPaddingTop() const { return surround_data_.padding_top_; }
  const NLength& GetPaddingBottom() const {
    return surround_data_.padding_bottom_;
  }
  const NLength& GetMarginLeft() const { return surround_data_.margin_left_; }
  const NLength& GetMarginRight() const { return surround_data_.margin_right_; }
  const NLength& GetMarginTop() const { return surround_data_.margin_top_; }
  const NLength& GetMarginBottom() const {
    return surround_data_.margin_bottom_;
  }

  BorderStyleType GetBorderLeftStyle() const {
    return surround_data_.border_data_
               ? surround_data_.border_data_->style_left
               : DefaultLayoutStyle::SL_DEFAULT_BORDER_STYLE;
  }
  BorderStyleType GetBorderRightStyle() const {
    return surround_data_.border_data_
               ? surround_data_.border_data_->style_right
               : DefaultLayoutStyle::SL_DEFAULT_BORDER_STYLE;
  }
  BorderStyleType GetBorderTopStyle() const {
    return surround_data_.border_data_
               ? surround_data_.border_data_->style_top
               : DefaultLayoutStyle::SL_DEFAULT_BORDER_STYLE;
  }
  BorderStyleType GetBorderBottomStyle() const {
    return surround_data_.border_data_
               ? surround_data_.border_data_->style_bottom
               : DefaultLayoutStyle::SL_DEFAULT_BORDER_STYLE;
  }

  // BorderWidth
  float GetBorderLeftWidth() const {
    return surround_data_.border_data_ ? surround_data_.border_data_->width_left
                                       : DefaultLayoutStyle::SL_DEFAULT_BORDER;
  }
  float GetBorderTopWidth() const {
    return surround_data_.border_data_ ? surround_data_.border_data_->width_top
                                       : DefaultLayoutStyle::SL_DEFAULT_BORDER;
  }
  float GetBorderRightWidth() const {
    return surround_data_.border_data_
               ? surround_data_.border_data_->width_right
               : DefaultLayoutStyle::SL_DEFAULT_BORDER;
  }
  float GetBorderBottomWidth() const {
    return surround_data_.border_data_
               ? surround_data_.border_data_->width_bottom
               : DefaultLayoutStyle::SL_DEFAULT_BORDER;
  }

  float GetBorderWidthHorizontal() const {
    return GetBorderLeftWidth() + GetBorderRightWidth();
  }
  float GetBorderWidthVertical() const {
    return GetBorderTopWidth() + GetBorderBottomWidth();
  }

  float GetBorderFinalLeftWidth() const {
    return GetBorderFinalWidth(GetBorderLeftWidth(), GetBorderLeftStyle());
  }
  float GetBorderFinalTopWidth() const {
    return GetBorderFinalWidth(GetBorderTopWidth(), GetBorderTopStyle());
  }
  float GetBorderFinalRightWidth() const {
    return GetBorderFinalWidth(GetBorderRightWidth(), GetBorderRightStyle());
  }
  float GetBorderFinalBottomWidth() const {
    return GetBorderFinalWidth(GetBorderBottomWidth(), GetBorderBottomStyle());
  }

  float GetBorderFinalWidthHorizontal() const {
    return GetBorderFinalLeftWidth() + GetBorderFinalRightWidth();
  }
  float GetBorderFinalWidthVertical() const {
    return GetBorderFinalTopWidth() + GetBorderFinalBottomWidth();
  }

  float GetListMainAxisGap() const {
    return LayoutStyleUtils::RoundValueToPixelGrid(
        linear_data_->list_main_axis_gap_.GetRawValue(),
        physical_pixels_per_layout_unit_);
  }

  float GetListCrossAxisGap() const {
    return LayoutStyleUtils::RoundValueToPixelGrid(
        linear_data_->list_cross_axis_gap_.GetRawValue(),
        physical_pixels_per_layout_unit_);
  }

#define SUPPORTED_LAYOUT_PROPERTY(V)                                     \
  V(Width, NLength, box_data_.Access()->width_, WIDTH)                   \
  V(Height, NLength, box_data_.Access()->height_, HEIGHT)                \
  V(MinWidth, NLength, box_data_.Access()->min_width_, MIN_WIDTH)        \
  V(MinHeight, NLength, box_data_.Access()->min_height_, MIN_HEIGHT)     \
  V(MaxWidth, NLength, box_data_.Access()->max_width_, MAX_WIDTH)        \
  V(MaxHeight, NLength, box_data_.Access()->max_height_, MAX_HEIGHT)     \
  V(FlexBasis, NLength, flex_data_.Access()->flex_basis_, FLEX_BASIS)    \
  V(Left, NLength, surround_data_.left_, FOUR_POSITION)                  \
  V(Right, NLength, surround_data_.right_, FOUR_POSITION)                \
  V(Top, NLength, surround_data_.top_, FOUR_POSITION)                    \
  V(Bottom, NLength, surround_data_.bottom_, FOUR_POSITION)              \
  V(PaddingLeft, NLength, surround_data_.padding_left_, PADDING)         \
  V(PaddingRight, NLength, surround_data_.padding_right_, PADDING)       \
  V(PaddingTop, NLength, surround_data_.padding_top_, PADDING)           \
  V(PaddingBottom, NLength, surround_data_.padding_bottom_, PADDING)     \
  V(MarginLeft, NLength, surround_data_.margin_left_, MARGIN)            \
  V(MarginRight, NLength, surround_data_.margin_right_, MARGIN)          \
  V(MarginTop, NLength, surround_data_.margin_top_, MARGIN)              \
  V(MarginBottom, NLength, surround_data_.margin_bottom_, MARGIN)        \
  V(ColumnGap, NLength, grid_data_.Access()->grid_column_gap_, GRID_GAP) \
  V(RowGap, NLength, grid_data_.Access()->grid_row_gap_, GRID_GAP)

#define SUPPORTED_ENUM_LAYOUT_PROPERTY(V)                                      \
  V(FlexDirection, FlexDirectionType, flex_data_.Access()->flex_direction_,    \
    FLEX_DIRECTION)                                                            \
  V(JustifyContent, JustifyContentType, flex_data_.Access()->justify_content_, \
    JUSTIFY_CONTENT)                                                           \
  V(FlexWrap, FlexWrapType, flex_data_.Access()->flex_wrap_, FLEX_WRAP)        \
  V(FlexGrow, float, flex_data_.Access()->flex_grow_, FLEX_GROW)               \
  V(FlexShrink, float, flex_data_.Access()->flex_shrink_, FLEX_SHRINK)         \
  V(AspectRatio, float, box_data_.Access()->aspect_ratio_, ASPECT_RATIO)       \
  V(AlignItems, FlexAlignType, flex_data_.Access()->align_items_, ALIGN_ITEMS) \
  V(AlignSelf, FlexAlignType, flex_data_.Access()->align_self_, ALIGN_SELF)    \
  V(AlignContent, AlignContentType, flex_data_.Access()->align_content_,       \
    ALIGN_CONTENT)                                                             \
  V(Position, PositionType, position_, POSITION)                               \
  V(Direction, DirectionType, direction_, DIRECTION)                           \
  V(Display, DisplayType, display_, DISPLAY)                                   \
  V(BoxSizing, BoxSizingType, box_sizing_, BOX_SIZING)                         \
  V(Order, int32_t, flex_data_.Access()->order_, ORDER)                        \
  V(LinearOrientation, LinearOrientationType,                                  \
    linear_data_.Access()->linear_orientation_, LINEAR_ORIENTATION)            \
  V(LinearLayoutGravity, LinearLayoutGravityType,                              \
    linear_data_.Access()->linear_layout_gravity_, LINEAR_LAYOUT_GRAVITY)      \
  V(LinearGravity, LinearGravityType, linear_data_.Access()->linear_gravity_,  \
    LINEAR_GRAVITY)                                                            \
  V(LinearCrossGravity, LinearCrossGravityType,                                \
    linear_data_.Access()->linear_cross_gravity_, LINEAR_CROSS_GRAVITY)

#define SET_ENUM_LAYOUT_PROPERTY(type_name, enum_type, css_type, default_type) \
  LYNX_EXPORT bool Set##type_name(const enum_type value,                       \
                                  const bool reset = false) {                  \
    enum_type old_value = css_type;                                            \
    css_type = reset ? DefaultLayoutStyle::SL_DEFAULT_##default_type : value;  \
    return old_value != css_type;                                              \
  }
  SUPPORTED_ENUM_LAYOUT_PROPERTY(SET_ENUM_LAYOUT_PROPERTY)
#undef SET_ENUM_LAYOUT_PROPERTY

#define SET_LAYOUT_PROPERTY(type_name, enum_type, css_type, default_type) \
  LYNX_EXPORT bool Set##type_name(const enum_type& value,                 \
                                  const bool reset = false) {             \
    enum_type old_value = css_type;                                       \
    css_type =                                                            \
        reset ? DefaultLayoutStyle::SL_DEFAULT_##default_type() : value;  \
    return old_value != css_type;                                         \
  }
  SUPPORTED_LAYOUT_PROPERTY(SET_LAYOUT_PROPERTY)
#undef SET_LAYOUT_PROPERTY

  float PhysicalPixelsPerLayoutUnit() const {
    return physical_pixels_per_layout_unit_;
  }

  void SetPhysicalPixelsPerLayoutUnit(float physical_pixels_per_layout_unit) {
    physical_pixels_per_layout_unit_ = physical_pixels_per_layout_unit;
  }

  // Only for Starlight standalone, and supports setting floating-point values
  // only.
#define SET_BORDER_WIDTH_PROPERTY(direction, field)                   \
  bool SetBorder##direction##Width(float value, bool reset = false) { \
    if (!surround_data_.border_data_) {                               \
      surround_data_.border_data_ = BordersData();                    \
    }                                                                 \
    const float old_value = surround_data_.border_data_->field;       \
    surround_data_.border_data_->field =                              \
        reset ? DefaultLayoutStyle::SL_DEFAULT_BORDER : value;        \
    return old_value != surround_data_.border_data_->field;           \
  }
#define SUPPORTED_BORDER_WIDTH_PROPERTY(V) \
  V(Left, width_left)                      \
  V(Top, width_top)                        \
  V(Right, width_right)                    \
  V(Bottom, width_bottom)

  SUPPORTED_BORDER_WIDTH_PROPERTY(SET_BORDER_WIDTH_PROPERTY)
#undef SUPPORTED_BORDER_WIDTH_PROPERTY
#undef SET_BORDER_WIDTH_PROPERTY

 private:
  float physical_pixels_per_layout_unit_;
  float GetBorderFinalWidth(float width, BorderStyleType style) const {
    return (style != BorderStyleType::kNone && style != BorderStyleType::kHide)
               ? width
               : 0.f;
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_LAYOUT_COMPUTED_STYLE_H_
