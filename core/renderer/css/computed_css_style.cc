// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/computed_css_style.h"

#include <cmath>
#include <utility>

#include "base/include/algorithm.h"
#include "base/include/compiler_specific.h"
#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_debug_msg.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/style/color.h"

namespace lynx {

using base::FloatsEqual;

namespace starlight {

using CSSValuePattern = tasm::CSSValuePattern;

const ComputedCSSStyle::StyleFunc* ComputedCSSStyle::FuncMap() {
  static const StyleFunc* func_map_ = []() {
    static StyleFunc style_funcs[tasm::CSSPropertyID::kPropertyEnd] = {nullptr};
#define DECLARE_PROPERTY_SETTER(name, c, value) \
  style_funcs[tasm::kPropertyID##name] = &ComputedCSSStyle::Set##name;
    FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_SETTER)
#undef DECLARE_PROPERTY_SETTER
    return style_funcs;
  }();
  return func_map_;
}

const ComputedCSSStyle::StyleGetterFunc* ComputedCSSStyle::GetterFuncMap() {
  static const StyleGetterFunc* getter_func_map_ = []() {
    static StyleGetterFunc map[tasm::CSSPropertyID::kPropertyEnd] = {nullptr};
#define DECLARE_PLATFORM_PROPERTY_GETTER(name) \
  map[tasm::kPropertyID##name] = &ComputedCSSStyle::name##ToLepus;
    FOREACH_PLATFORM_PROPERTY(DECLARE_PLATFORM_PROPERTY_GETTER)
#undef DECLARE_PLATFORM_PROPERTY_GETTER
#undef FOREACH_PLATFORM_PROPERTY
    return map;
  }();
  return getter_func_map_;
}

const ComputedCSSStyle::StyleInheritFuncMap&
ComputedCSSStyle::InheritFuncMap() {
  static base::NoDestructor<ComputedCSSStyle::StyleInheritFuncMap>
      inherit_func_map_{{
#define DECLARE_PLATFORM_PROPERTY_INHERIT_FUNC(name) \
  {tasm::kPropertyID##name, &ComputedCSSStyle::Inherit##name},
          FOREACH_PLATFORM_COMPLEX_INHERITABLE_PROPERTY(
              DECLARE_PLATFORM_PROPERTY_INHERIT_FUNC)
#undef DECLARE_PLATFORM_PROPERTY_INHERIT_FUNC
#undef FOREACH_PLATFORM_COMPLEX_INHERITABLE_PROPERTY
      }};
  return *inherit_func_map_;
}

namespace {
bool CalculateFromBorderWidthStringToFloat(
    const tasm::CSSValue& value, float& result,
    const tasm::CssMeasureContext& context, const bool reset,
    bool css_align_with_legacy_w3c, const tasm::CSSParserConfigs& configs) {
  if (reset) {
    result = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER);
    return true;
  }

  auto parse_result = CSSStyleUtils::ToLength(value, context, configs);
  if (!parse_result.second ||
      (!parse_result.first.IsUnit() && !parse_result.first.IsCalc()) ||
      (parse_result.first.IsCalc() &&
       parse_result.first.NumericLength().ContainsPercentage())) {
    return false;
  }
  result = CSSStyleUtils::GetBorderWidthFromLengthToFloat(parse_result.first,
                                                          context);
  return true;
}

bool CalculateCSSValueToFloat(const tasm::CSSValue& value, float& result,
                              const tasm::CssMeasureContext& context,
                              const tasm::CSSParserConfigs& configs,
                              bool is_font_relevant = false) {
  auto parse_result =
      CSSStyleUtils::ToLength(value, context, configs, is_font_relevant);
  if (!parse_result.second) {
    return false;
  }

  if (parse_result.first.IsCalc()) {
    result = CSSStyleUtils::RoundValueToPixelGrid(
        parse_result.first.NumericLength().GetFixedPart(),
        context.physical_pixels_per_layout_unit_);
  } else {
    // FIXME(zhixuan): The function has the legact bug to return percentage
    // value as fixed value for non calc length. Will fix later.
    result = CSSStyleUtils::RoundValueToPixelGrid(
        parse_result.first.GetRawValue(),
        context.physical_pixels_per_layout_unit_);
  }
  return true;
}

lepus::Value ShadowDataToLepus(std::vector<ShadowData> shadows) {
  auto group = lepus::CArray::Create();
  for (const auto& shadow_data : shadows) {
    auto item = lepus::CArray::Create();
    item->emplace_back(shadow_data.h_offset);
    item->emplace_back(shadow_data.v_offset);
    item->emplace_back(shadow_data.blur);
    item->emplace_back(shadow_data.spread);
    item->emplace_back(static_cast<int>(shadow_data.option));
    item->emplace_back(shadow_data.color);
    group->emplace_back(std::move(item));
  }
  return lepus::Value(std::move(group));
}

bool SetLayoutAnimationTimingFunctionInternal(
    const tasm::CSSValue& value, const bool reset,
    TimingFunctionData& timing_function,
    const tasm::CSSParserConfigs& configs) {
  const lepus::Value& lepus_val = value.GetValue();
  // TimingFunction's input value must be a non-empty array , if not we will
  // reset it.
  bool reset_internal =
      (reset || !lepus_val.IsArray() || lepus_val.Array()->size() <= 0);
  const lepus::Value& param =
      reset_internal ? lepus_val : lepus_val.Array()->get(0);
  return CSSStyleUtils::ComputeTimingFunction(param, reset_internal,
                                              timing_function, configs);
}

bool SetBorderWidthHelper(bool cssAlignWithLegacyW3C,
                          const tasm::CssMeasureContext& context, float& width,
                          const tasm::CSSValue& value,
                          const tasm::CSSParserConfigs& configs,
                          const bool reset) {
  float old_value = width;
  if (UNLIKELY(!CalculateFromBorderWidthStringToFloat(
          value, width, context, reset, cssAlignWithLegacyW3C, configs))) {
    return false;
  }
  return width != old_value;
}

bool SetBorderRadiusHelper(NLength& radiusX, NLength& radiusY,
                           const lynx::tasm::CssMeasureContext& context,
                           tasm::CSSPropertyID cssID,
                           const tasm::CSSValue& value, const bool reset,
                           const tasm::CSSParserConfigs& configs) {
  if (reset) {
    radiusX = radiusY = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray(), configs.enable_css_strict_mode, tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(cssID).c_str(), tasm::ARRAY_TYPE)

    auto arr = value.GetValue().Array();
    auto parse_result = CSSStyleUtils::ToLength(
        tasm::CSSValue(arr->get(0),
                       static_cast<CSSValuePattern>(arr->get(1).Number())),
        context, configs);

    CSS_HANDLER_FAIL_IF_NOT(parse_result.second, configs.enable_css_strict_mode,
                            tasm::SET_PROPERTY_ERROR,
                            tasm::CSSProperty::GetPropertyName(cssID).c_str())

    radiusX = std::move(parse_result.first);

    parse_result = CSSStyleUtils::ToLength(
        tasm::CSSValue(arr->get(2),
                       static_cast<CSSValuePattern>(arr->get(3).Number())),
        context, configs);

    CSS_HANDLER_FAIL_IF_NOT(parse_result.second, configs.enable_css_strict_mode,
                            tasm::SET_PROPERTY_ERROR,
                            tasm::CSSProperty::GetPropertyName(cssID).c_str())

    radiusY = std::move(parse_result.first);
  }
  return true;
}

lepus_value LayoutAnimationTimingFunctionToLepusHelper(
    const TimingFunctionData& timingFunction) {
  auto array = lepus::CArray::Create();
  array->emplace_back(static_cast<int>(timingFunction.timing_func));
  array->emplace_back(static_cast<int>(timingFunction.steps_type));
  array->emplace_back(timingFunction.x1);
  array->emplace_back(timingFunction.y1);
  array->emplace_back(timingFunction.x2);
  array->emplace_back(timingFunction.y2);
  return lepus_value(std::move(array));
}

bool SetBackgroundOrMaskImage(std::optional<BackgroundData>& data,
                              const tasm::CSSValue& value, bool reset) {
  CSSStyleUtils::PrepareOptional(data);
  auto old_value = data->image;
  data->image_count = DefaultComputedStyle::DEFAULT_LONG;
  data->image = lepus::Value();
  if (!reset) {
    if (!value.IsArray()) {
      return false;
    }
    auto array = value.GetValue().Array();
    for (size_t i = 0; i < array->size(); i++) {
      const auto& img = array->get(i);
      if (img.IsNumber()) {
        ++data->image_count;
      }
    }
    data->image = value.GetValue();
  }
  return old_value != data->image;
}

bool SetBackgroundOrMaskPosition(std::optional<BackgroundData>& data,
                                 const tasm::CssMeasureContext& context,
                                 const tasm::CSSParserConfigs& configs,
                                 const tasm::CSSValue& value, bool reset) {
  CSSStyleUtils::PrepareOptional(data);
  auto old_value = data->position;
  data->position.clear();
  if (!reset) {
    if (!value.IsArray()) {
      return false;
    }
    auto pos_arr = value.GetValue().Array();
    for (size_t i = 0; i != pos_arr->size(); ++i) {
      auto array = pos_arr->get(i).Array();
      uint32_t pos_x_type = static_cast<uint32_t>(array->get(0).Number());
      uint32_t pos_y_type = static_cast<uint32_t>(array->get(2).Number());
      // position x
      if (pos_x_type ==
          static_cast<uint32_t>(BackgroundPositionType::kCenter)) {
        data->position.emplace_back(NLength::MakePercentageNLength(50.f));
      } else if (pos_x_type ==
                 static_cast<uint32_t>(BackgroundPositionType::kLeft)) {
        data->position.emplace_back(NLength::MakePercentageNLength(0.f));
      } else if (pos_x_type ==
                 static_cast<uint32_t>(BackgroundPositionType::kRight)) {
        data->position.emplace_back(NLength::MakePercentageNLength(100.f));
      } else {
        auto pattern = static_cast<uint32_t>(array->get(0).Number());
        data->position.emplace_back(
            CSSStyleUtils::ToLength(
                tasm::CSSValue{array->get(1),
                               static_cast<CSSValuePattern>(pattern)},
                context, configs)
                .first);
      }

      // position y
      if (pos_y_type ==
          static_cast<uint32_t>(BackgroundPositionType::kCenter)) {
        data->position.emplace_back(NLength::MakePercentageNLength(50.f));
      } else if (pos_y_type ==
                 static_cast<uint32_t>(BackgroundPositionType::kTop)) {
        data->position.emplace_back(NLength::MakePercentageNLength(0.f));
      } else if (pos_y_type ==
                 static_cast<uint32_t>(BackgroundPositionType::kBottom)) {
        data->position.emplace_back(NLength::MakePercentageNLength(100.f));
      } else {
        auto pattern = static_cast<uint32_t>(array->get(2).Number());
        data->position.emplace_back(
            CSSStyleUtils::ToLength(
                tasm::CSSValue{array->get(3),
                               static_cast<CSSValuePattern>(pattern)},
                context, configs)
                .first);
      }
    }
  }
  return old_value != data->position;
}

bool SetBackgroundOrMaskSize(std::optional<BackgroundData>& data,
                             const tasm::CssMeasureContext& context,
                             const tasm::CSSParserConfigs& configs,
                             const tasm::CSSValue& value, bool reset) {
  CSSStyleUtils::PrepareOptional(data);
  auto old_value = data->size;
  data->size.clear();
  if (!reset) {
    if (!value.IsArray()) {
      return false;
    }
    auto size_arr = value.GetValue().Array();
    for (size_t i = 0; i != size_arr->size(); ++i) {
      auto array = size_arr->get(i).Array();
      auto pattern = static_cast<uint32_t>(array->get(0).Number());
      data->size.emplace_back(
          CSSStyleUtils::ToLength(
              tasm::CSSValue(array->get(1),
                             static_cast<CSSValuePattern>(pattern)),
              context, configs)
              .first);
      pattern = static_cast<uint32_t>(array->get(2).Number());
      data->size.emplace_back(
          CSSStyleUtils::ToLength(
              tasm::CSSValue(array->get(3),
                             static_cast<CSSValuePattern>(pattern)),
              context, configs)
              .first);
    }
  }
  return old_value != data->size;
}

bool SetBackgroundOrMaskClip(std::optional<BackgroundData>& data,
                             const tasm::CSSValue& value, bool reset) {
  CSSStyleUtils::PrepareOptional(data);
  auto old_value = data->clip;
  data->clip.clear();
  if (!reset) {
    if (!value.IsArray()) {
      return false;
    }
    auto clip_arr = value.GetValue().Array();
    for (size_t i = 0; i < clip_arr->size(); i++) {
      auto clip_type = static_cast<uint32_t>(clip_arr->get(i).Number());
      data->clip.emplace_back(static_cast<BackgroundClipType>(clip_type));
    }
  }
  return old_value != data->clip;
}

bool SetBackgroundOrMaskOrigin(std::optional<BackgroundData>& data,
                               const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(data);
  auto old_value = data->origin;
  data->origin.clear();
  if (!reset) {
    if (!value.IsArray()) {
      return false;
    }
    auto origin_arr = value.GetValue().Array();
    for (size_t i = 0; i < origin_arr->size(); i++) {
      auto origin_type = static_cast<uint32_t>(origin_arr->get(i).Number());
      data->origin.emplace_back(static_cast<BackgroundOriginType>(origin_type));
    }
  }
  return old_value != data->origin;
}

bool SetBackgroundOrMaskRepeat(std::optional<BackgroundData>& data,
                               const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(data);
  auto old_value = data->repeat;
  data->repeat.clear();
  if (!reset) {
    if (!value.IsArray()) {
      return false;
    }
    auto repeat_arr = value.GetValue().Array();
    for (size_t i = 0; i < repeat_arr->size(); i++) {
      auto repeat_type =
          static_cast<uint32_t>(repeat_arr->get(i).Array()->get(0).Number());
      data->repeat.emplace_back(static_cast<BackgroundRepeatType>(repeat_type));
      repeat_type =
          static_cast<uint32_t>(repeat_arr->get(i).Array()->get(1).Number());
      data->repeat.emplace_back(static_cast<BackgroundRepeatType>(repeat_type));
    }
  }
  return old_value != data->repeat;
}

lepus_value BackgroundOrMaskImageToLepus(
    const std::optional<BackgroundData>& data,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (data && data->image.IsArray()) {
    auto array = data->image.Array();
    for (size_t i = 0; i < array->size(); i++) {
      const auto& img = array->get(i);
      if (!img.IsNumber()) {
        continue;
      }
      if (img.Number() ==
          static_cast<uint32_t>(
              starlight::BackgroundImageType::kRadialGradient)) {
        // Radial gradient data: [shape_arr, color_array, position_array]
        const auto& gradient_data = array->get(i + 1);
        CSSStyleUtils::ComputeRadialGradient(gradient_data, context, configs);
      }
    }
    return data->image;
  } else {
    return lepus::Value{lepus::CArray::Create()};
  }
}

lepus_value BackgroundOrMaskSizeToLepus(
    const std::optional<BackgroundData>& data) {
  if (data && !data->size.empty()) {
    auto array = lepus::CArray::Create();
    for (const auto& size : data->size) {
      CSSStyleUtils::AddLengthToArray(array, size);
    }
    return lepus::Value{std::move(array)};
  } else {
    return lepus::Value{lepus::CArray::Create()};
  }
}

lepus_value BackgroundOrMaskClipToLepus(
    const std::optional<BackgroundData>& data) {
  if (data && !data->clip.empty()) {
    auto array = lepus::CArray::Create();
    for (const auto& clip : data->clip) {
      array->emplace_back(static_cast<int32_t>(clip));
    }
    return lepus::Value{std::move(array)};
  } else {
    return lepus::Value{lepus::CArray::Create()};
  }
}

lepus_value BackgroundOrMaskOriginToLepus(
    const std::optional<BackgroundData>& data) {
  if (data && !data->origin.empty()) {
    auto array = lepus::CArray::Create();
    for (const auto& origin : data->origin) {
      array->emplace_back(static_cast<int32_t>(origin));
    }
    return lepus::Value{std::move(array)};
  } else {
    return lepus::Value{lepus::CArray::Create()};
  }
}

lepus_value BackgroundOrMaskPositionToLepus(
    const std::optional<BackgroundData>& data) {
  if (data && !data->position.empty()) {
    auto array = lepus::CArray::Create();
    for (const auto& pos : data->position) {
      CSSStyleUtils::AddLengthToArray(array, pos);
    }
    return lepus::Value{std::move(array)};
  }
  return lepus::Value{lepus::CArray::Create()};
}

lepus_value BackgroundOrMaskRepeatToLepus(
    const std::optional<BackgroundData>& data) {
  if (data && !data->repeat.empty()) {
    auto array = lepus::CArray::Create();
    for (const auto& repeat : data->repeat) {
      array->emplace_back(static_cast<int32_t>(repeat));
    }
    return lepus_value{std::move(array)};
  } else {
    return lepus::Value{lepus::CArray::Create()};
  }
}

}  // namespace

float ComputedCSSStyle::SAFE_AREA_INSET_TOP_ = 0;
float ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_ = 0;
float ComputedCSSStyle::SAFE_AREA_INSET_LEFT_ = 0;
float ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_ = 0;

ComputedCSSStyle::ComputedCSSStyle(float layouts_unit_per_px,
                                   double physical_pixels_per_layout_unit)
    : length_context_(0.f, layouts_unit_per_px, physical_pixels_per_layout_unit,
                      layouts_unit_per_px * DEFAULT_FONT_SIZE_DP,
                      layouts_unit_per_px * DEFAULT_FONT_SIZE_DP, LayoutUnit(),
                      LayoutUnit()),
      layout_computed_style_(physical_pixels_per_layout_unit) {}

ComputedCSSStyle::ComputedCSSStyle(const ComputedCSSStyle& o)
    : length_context_(o.length_context_),
      layout_computed_style_(o.layout_computed_style_) {}

void ComputedCSSStyle::Reset() {
  layout_computed_style_.Reset();

  opacity_ = DefaultComputedStyle::DEFAULT_OPACITY;
  z_index_ = DefaultComputedStyle::DEFAULT_LONG;

  ResetOverflow();

  text_attributes_.reset();
  transform_raw_.reset();
  transform_origin_.reset();
  animation_data_.reset();
  transition_data_.reset();
  layout_animation_data_.reset();
  enter_transition_data_.reset();
  exit_transition_data_.reset();
  pause_transition_data_.reset();
  resume_transition_data_.reset();
  filter_.reset();
  visibility_ = DefaultComputedStyle::DEFAULT_VISIBILITY;
  caret_color_ = base::String();
  const float default_font_size =
      length_context_.layouts_unit_per_px_ * DEFAULT_FONT_SIZE_DP;
  SetFontSize(default_font_size, default_font_size);
}

bool ComputedCSSStyle::SetValue(tasm::CSSPropertyID id,
                                const tasm::CSSValue& value, bool reset) {
  const auto* funcMap = FuncMap();
  if (id > tasm::CSSPropertyID::kPropertyStart &&
      id < tasm::CSSPropertyID::kPropertyEnd) {
    if (StyleFunc func = funcMap[id]) {
      return (this->*func)(value, reset);
    }
  }
  LynxWarning(false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
              "SetValue can't find style func id:%d", id);
  return false;
}

void ComputedCSSStyle::ResetValue(tasm::CSSPropertyID id) {
  const auto* funcMap = FuncMap();
  if (id > tasm::CSSPropertyID::kPropertyStart &&
      id < tasm::CSSPropertyID::kPropertyEnd) {
    if (StyleFunc func = funcMap[id]) {
      tasm::CSSValue value;
      (this->*func)(value, true);
      return;
    }
  }
  LynxWarning(false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
              "ResetValue can't find style func id:%d", id);
}

void ComputedCSSStyle::SetOverflowDefaultVisible(
    bool default_overflow_visible) {
  default_overflow_visible_ = default_overflow_visible;
  ResetOverflow();
}

void ComputedCSSStyle::ResetOverflow() {
  const auto& overflow = default_overflow_visible_ ? OverflowType::kVisible
                                                   : OverflowType::kHidden;
  overflow_ = overflow;
  overflow_x_ = overflow;
  overflow_y_ = overflow;
}

lepus_value ComputedCSSStyle::GetValue(tasm::CSSPropertyID id) {
  const auto* getterFuncMap = GetterFuncMap();
  if (id > tasm::CSSPropertyID::kPropertyStart &&
      id < tasm::CSSPropertyID::kPropertyEnd) {
    if (StyleGetterFunc func = getterFuncMap[id]) {
      return (this->*func)();
    }
  }
  LynxWarning(false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_GETTER,
              "GetValue can't find style getter id:%d", id);
  return lepus::Value();
}

bool ComputedCSSStyle::InheritValue(tasm::CSSPropertyID id,
                                    const ComputedCSSStyle& from) {
  const auto& inheritFuncMap = InheritFuncMap();
  auto iter = inheritFuncMap.find(id);
  if (iter == inheritFuncMap.end()) {
    LynxWarning(false, error::E_CSS_COMPUTED_CSS_VALUE_UNSUPPORTED_INHERITANCE,
                "Inherit is not supported for style: \"%s\"",
                tasm::CSSProperty::GetPropertyName(id).c_str());
    return false;
  }
  StyleInheritFunc func = iter->second;
  return (this->*func)(from);
}

#define SUPPORTED_LENGTH_PROPERTY(V)                                           \
  V(Width, NLength, layout_computed_style_.box_data_.Access()->width_, WIDTH)  \
  V(Height, NLength, layout_computed_style_.box_data_.Access()->height_,       \
    HEIGHT)                                                                    \
  V(MinWidth, NLength, layout_computed_style_.box_data_.Access()->min_width_,  \
    MIN_WIDTH)                                                                 \
  V(MinHeight, NLength,                                                        \
    layout_computed_style_.box_data_.Access()->min_height_, MIN_HEIGHT)        \
  V(MaxWidth, NLength, layout_computed_style_.box_data_.Access()->max_width_,  \
    MAX_WIDTH)                                                                 \
  V(MaxHeight, NLength,                                                        \
    layout_computed_style_.box_data_.Access()->max_height_, MAX_HEIGHT)        \
  V(FlexBasis, NLength,                                                        \
    layout_computed_style_.flex_data_.Access()->flex_basis_, FLEX_BASIS)       \
  V(Left, NLength, layout_computed_style_.surround_data_.left_, FOUR_POSITION) \
  V(Right, NLength, layout_computed_style_.surround_data_.right_,              \
    FOUR_POSITION)                                                             \
  V(Top, NLength, layout_computed_style_.surround_data_.top_, FOUR_POSITION)   \
  V(Bottom, NLength, layout_computed_style_.surround_data_.bottom_,            \
    FOUR_POSITION)                                                             \
  V(PaddingLeft, NLength, layout_computed_style_.surround_data_.padding_left_, \
    PADDING)                                                                   \
  V(PaddingRight, NLength,                                                     \
    layout_computed_style_.surround_data_.padding_right_, PADDING)             \
  V(PaddingTop, NLength, layout_computed_style_.surround_data_.padding_top_,   \
    PADDING)                                                                   \
  V(PaddingBottom, NLength,                                                    \
    layout_computed_style_.surround_data_.padding_bottom_, PADDING)            \
  V(MarginLeft, NLength, layout_computed_style_.surround_data_.margin_left_,   \
    MARGIN)                                                                    \
  V(MarginRight, NLength, layout_computed_style_.surround_data_.margin_right_, \
    MARGIN)                                                                    \
  V(MarginTop, NLength, layout_computed_style_.surround_data_.margin_top_,     \
    MARGIN)                                                                    \
  V(MarginBottom, NLength,                                                     \
    layout_computed_style_.surround_data_.margin_bottom_, MARGIN)

bool ComputedCSSStyle::SetFlexGrow(const tasm::CSSValue& value,
                                   const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset, layout_computed_style_.flex_data_.Access()->flex_grow_,
      DefaultLayoutStyle::SL_DEFAULT_FLEX_GROW, "flex-grow must be a number!",
      parser_configs_);
}

bool ComputedCSSStyle::SetFlexShrink(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset, layout_computed_style_.flex_data_.Access()->flex_shrink_,
      DefaultLayoutStyle::SL_DEFAULT_FLEX_SHRINK,
      "flex-shrink must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetOrder(const tasm::CSSValue& value, const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset, layout_computed_style_.flex_data_.Access()->order_,
      DefaultLayoutStyle::SL_DEFAULT_ORDER, "order must be a number!",
      parser_configs_);
}

bool ComputedCSSStyle::SetLinearWeightSum(const tasm::CSSValue& value,
                                          const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_weight_sum_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_WEIGHT_SUM,
      "linear-weight-sum must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLinearWeight(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_weight_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_WEIGHT,
      "linear-weight must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetAspectRatio(const tasm::CSSValue& value,
                                      const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset, layout_computed_style_.box_data_.Access()->aspect_ratio_,
      DefaultLayoutStyle::SL_DEFAULT_ASPECT_RATIO,
      "aspect-ratio must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetBorderLeftWidth(const tasm::CSSValue& value,
                                          const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c_, length_context_,
      layout_computed_style_.surround_data_.border_data_->width_left, value,
      parser_configs_, reset);
}

bool ComputedCSSStyle::SetBorderTopWidth(const tasm::CSSValue& value,
                                         const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c_, length_context_,
      layout_computed_style_.surround_data_.border_data_->width_top, value,
      parser_configs_, reset);
}

bool ComputedCSSStyle::SetBorderRightWidth(const tasm::CSSValue& value,
                                           const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c_, length_context_,
      layout_computed_style_.surround_data_.border_data_->width_right, value,
      parser_configs_, reset);
}

bool ComputedCSSStyle::SetBorderBottomWidth(const tasm::CSSValue& value,
                                            const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return SetBorderWidthHelper(
      css_align_with_legacy_w3c_, length_context_,
      layout_computed_style_.surround_data_.border_data_->width_bottom, value,
      parser_configs_, reset);
}

bool ComputedCSSStyle::SetBorder(const tasm::CSSValue& value,
                                 const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetTextStroke(const tasm::CSSValue& value,
                                     const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetTextStrokeColor(const tasm::CSSValue& value,
                                          const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value_color = text_attributes_->text_stroke_color;
  if (reset) {
    text_attributes_->text_stroke_color = DefaultColor::DEFAULT_COLOR;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsNumber(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDTextStrokeColor)
            .c_str(),
        tasm::NUMBER_TYPE)

    if (value.IsNumber()) {
      text_attributes_->text_stroke_color =
          static_cast<unsigned int>(value.GetValue().Number());
    }
  }
  return old_value_color != text_attributes_->text_stroke_color;
}

bool ComputedCSSStyle::SetTextStrokeWidth(const tasm::CSSValue& value,
                                          const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value = text_attributes_->text_stroke_width;
  if (reset) {
    text_attributes_->text_stroke_width = DefaultComputedStyle::DEFAULT_FLOAT;
  } else {
    auto parse_result =
        CSSStyleUtils::ToLength(value, length_context_, parser_configs_, false);
    if (!parse_result.second) {
      return false;
    }
    if (parse_result.first.IsUnit()) {
      text_attributes_->text_stroke_width = parse_result.first.GetRawValue();
    } else {
      return false;
    }
  }
  return base::FloatsNotEqual(text_attributes_->text_stroke_width, old_value);
}

bool ComputedCSSStyle::SetBorderTop(const tasm::CSSValue& value,
                                    const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderRight(const tasm::CSSValue& value,
                                      const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderBottom(const tasm::CSSValue& value,
                                       const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetFontScale(float font_scale) {
  if (font_scale == length_context_.font_scale_) {
    return false;
  }
  length_context_.font_scale_ = font_scale;
  return true;
}
bool ComputedCSSStyle::SetBorderLeft(const tasm::CSSValue& value,
                                     const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetMarginInlineStart(const tasm::CSSValue& value,
                                            const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetMarginInlineEnd(const tasm::CSSValue& value,
                                          const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetPaddingInlineStart(const tasm::CSSValue& value,
                                             const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetPaddingInlineEnd(const tasm::CSSValue& value,
                                           const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderInlineStartWidth(const tasm::CSSValue& value,
                                                 const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderInlineEndWidth(const tasm::CSSValue& value,
                                               const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderInlineStartColor(const tasm::CSSValue& value,
                                                 const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderInlineEndColor(const tasm::CSSValue& value,
                                               const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderInlineStartStyle(const tasm::CSSValue& value,
                                                 const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderInlineEndStyle(const tasm::CSSValue& value,
                                               const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderStartStartRadius(const tasm::CSSValue& value,
                                                 const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderEndStartRadius(const tasm::CSSValue& value,
                                               const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderStartEndRadius(const tasm::CSSValue& value,
                                               const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderEndEndRadius(const tasm::CSSValue& value,
                                             const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetRelativeAlignInlineStart(const tasm::CSSValue& value,
                                                   const bool reset) {
  NOTREACHED();
  return false;
}

bool ComputedCSSStyle::SetRelativeAlignInlineEnd(const tasm::CSSValue& value,
                                                 const bool reset) {
  NOTREACHED();
  return false;
}

bool ComputedCSSStyle::SetRelativeInlineStartOf(const tasm::CSSValue& value,
                                                const bool reset) {
  NOTREACHED();
  return false;
}

bool ComputedCSSStyle::SetRelativeInlineEndOf(const tasm::CSSValue& value,
                                              const bool reset) {
  NOTREACHED();
  return false;
}

#define SET_LENGTH_PROPERTY(type_name, length, css_type, default_type)     \
  bool ComputedCSSStyle::Set##type_name(const tasm::CSSValue& value,       \
                                        const bool reset) {                \
    return CSSStyleUtils::ComputeLengthStyle(                              \
        value, reset, length_context_, css_type,                           \
        DefaultLayoutStyle::SL_DEFAULT_##default_type(), parser_configs_); \
  }
SUPPORTED_LENGTH_PROPERTY(SET_LENGTH_PROPERTY)
#undef SET_LENGTH_PROPERTY

bool ComputedCSSStyle::SetFlexDirection(const tasm::CSSValue& value,
                                        const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<FlexDirectionType>(
      value, reset, layout_computed_style_.flex_data_.Access()->flex_direction_,
      DefaultLayoutStyle::SL_DEFAULT_FLEX_DIRECTION,
      "flex-direction must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetJustifyContent(const tasm::CSSValue& value,
                                         const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<JustifyContentType>(
      value, reset,
      layout_computed_style_.flex_data_.Access()->justify_content_,
      DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_CONTENT,
      "justify-content must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetFlexWrap(const tasm::CSSValue& value,
                                   const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<FlexWrapType>(
      value, reset, layout_computed_style_.flex_data_.Access()->flex_wrap_,
      DefaultLayoutStyle::SL_DEFAULT_FLEX_WRAP, "flex-warp must be a enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetAlignItems(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<FlexAlignType>(
      value, reset, layout_computed_style_.flex_data_.Access()->align_items_,
      DefaultLayoutStyle::SL_DEFAULT_ALIGN_ITEMS, "align-items must be a enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetAlignSelf(const tasm::CSSValue& value,
                                    const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<FlexAlignType>(
      value, reset, layout_computed_style_.flex_data_.Access()->align_self_,
      DefaultLayoutStyle::SL_DEFAULT_ALIGN_SELF, "align-self must be a enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetAlignContent(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<AlignContentType>(
      value, reset, layout_computed_style_.flex_data_.Access()->align_content_,
      DefaultLayoutStyle::SL_DEFAULT_ALIGN_CONTENT,
      "align-content must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetPosition(const tasm::CSSValue& value,
                                   const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<PositionType>(
      value, reset, layout_computed_style_.position_,
      DefaultLayoutStyle::SL_DEFAULT_POSITION, "position must be a enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetDirection(const tasm::CSSValue& value,
                                    const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<DirectionType>(
      value, reset, layout_computed_style_.direction_,
      DefaultLayoutStyle::SL_DEFAULT_DIRECTION, "direction must be a enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetOverflow(const tasm::CSSValue& value,
                                   const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<OverflowType>(
      value, reset, overflow_, GetDefaultOverflowType(),
      "overflow must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetDisplay(const tasm::CSSValue& value,
                                  const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<DisplayType>(
      value, reset, layout_computed_style_.display_,
      DefaultLayoutStyle::SL_DEFAULT_DISPLAY, "display must be a enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetLinearOrientation(const tasm::CSSValue& value,
                                            const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<LinearOrientationType>(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_orientation_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_ORIENTATION,
      "linear-orientation must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetLinearDirection(const tasm::CSSValue& value,
                                          const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<LinearOrientationType>(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_orientation_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_ORIENTATION,
      "linear-direction must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetLinearLayoutGravity(const tasm::CSSValue& value,
                                              const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<LinearLayoutGravityType>(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_layout_gravity_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_LAYOUT_GRAVITY,
      "linear-layout-gravity must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetLinearGravity(const tasm::CSSValue& value,
                                        const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<LinearGravityType>(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_gravity_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_GRAVITY,
      "linear-gravity must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetLinearCrossGravity(const tasm::CSSValue& value,
                                             const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<LinearCrossGravityType>(
      value, reset,
      layout_computed_style_.linear_data_.Access()->linear_cross_gravity_,
      DefaultLayoutStyle::SL_DEFAULT_LINEAR_CROSS_GRAVITY,
      "linear-cross-gravity must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetBoxSizing(const tasm::CSSValue& value,
                                    const bool reset) {
  auto old_value = layout_computed_style_.box_sizing_;
  if (reset) {
    layout_computed_style_.box_sizing_ = BoxSizingType::kAuto;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsEnum(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDBoxSizing).c_str(),
        tasm::ENUM_TYPE)

    layout_computed_style_.box_sizing_ =
        static_cast<BoxSizingType>(value.GetValue().Number());
  }
  return old_value != layout_computed_style_.box_sizing_;
}

bool ComputedCSSStyle::SetRelativeId(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_id_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ID, "relative-id must be a int!",
      parser_configs_);
}

bool ComputedCSSStyle::SetRelativeAlignTop(const tasm::CSSValue& value,
                                           const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_align_top_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_TOP,
      "relative-align-top must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeAlignRight(const tasm::CSSValue& value,
                                             const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_align_right_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_RIGHT,
      "relative-align-right must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeAlignBottom(const tasm::CSSValue& value,
                                              const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_align_bottom_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_BOTTOM,
      "relative-align-bottom must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeAlignLeft(const tasm::CSSValue& value,
                                            const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_align_left_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_LEFT,
      "relative-align-left must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeTopOf(const tasm::CSSValue& value,
                                        const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_top_of_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_TOP_OF,
      "relative-top-of must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeRightOf(const tasm::CSSValue& value,
                                          const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_right_of_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_RIGHT_OF,
      "relative-right-of must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeBottomOf(const tasm::CSSValue& value,
                                           const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_bottom_of_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_BOTTOM_OF,
      "relative-bottom-of must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeLeftOf(const tasm::CSSValue& value,
                                         const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_left_of_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LEFT_OF,
      "relative-left-of must be a int!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeLayoutOnce(const tasm::CSSValue& value,
                                             const bool reset) {
  return CSSStyleUtils::ComputeBoolStyle(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_layout_once_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LAYOUT_ONCE,
      "relative-layout-once must be a bool!", parser_configs_);
}

bool ComputedCSSStyle::SetRelativeCenter(const tasm::CSSValue& value,
                                         const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<RelativeCenterType>(
      value, reset,
      layout_computed_style_.relative_data_.Access()->relative_center_,
      DefaultLayoutStyle::SL_DEFAULT_RELATIVE_CENTER,
      "relative-center must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetGridTemplateColumns(const tasm::CSSValue& value,
                                              const bool reset) {
  return CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, length_context_,
      layout_computed_style_.grid_data_.Access()
          ->grid_template_columns_min_track_sizing_function_,
      layout_computed_style_.grid_data_.Access()
          ->grid_template_columns_max_track_sizing_function_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-template-columns must be an array!", parser_configs_);
}

bool ComputedCSSStyle::SetGridTemplateRows(const tasm::CSSValue& value,
                                           const bool reset) {
  return CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, length_context_,
      layout_computed_style_.grid_data_.Access()
          ->grid_template_rows_min_track_sizing_function_,
      layout_computed_style_.grid_data_.Access()
          ->grid_template_rows_max_track_sizing_function_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-template-rows must be an array!", parser_configs_);
}

bool ComputedCSSStyle::SetGridAutoColumns(const tasm::CSSValue& value,
                                          const bool reset) {
  return CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, length_context_,
      layout_computed_style_.grid_data_.Access()
          ->grid_auto_columns_min_track_sizing_function_,
      layout_computed_style_.grid_data_.Access()
          ->grid_auto_columns_max_track_sizing_function_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-auto-columns must be an array!", parser_configs_);
}

bool ComputedCSSStyle::SetGridAutoRows(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeGridTrackSizing(
      value, reset, length_context_,
      layout_computed_style_.grid_data_.Access()
          ->grid_auto_rows_min_track_sizing_function_,
      layout_computed_style_.grid_data_.Access()
          ->grid_auto_rows_max_track_sizing_function_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK(),
      "grid-auto-rows must be an array!", parser_configs_);
}

bool ComputedCSSStyle::SetGridColumnSpan(const tasm::CSSValue& value,
                                         const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.grid_data_.Access()->grid_column_span_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_SPAN,
      "grid-column-span must be an int!", parser_configs_);
}

bool ComputedCSSStyle::SetGridRowSpan(const tasm::CSSValue& value,
                                      const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset, layout_computed_style_.grid_data_.Access()->grid_row_span_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_SPAN, "grid-row-span must be an int!",
      parser_configs_);
}

bool ComputedCSSStyle::SetGridRowStart(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset, layout_computed_style_.grid_data_.Access()->grid_row_start_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION,
      "grid-row-start must be an int!", parser_configs_);
}

bool ComputedCSSStyle::SetGridRowEnd(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset, layout_computed_style_.grid_data_.Access()->grid_row_end_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION,
      "grid-row-end must be an int!", parser_configs_);
}

bool ComputedCSSStyle::SetGridColumnStart(const tasm::CSSValue& value,
                                          const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.grid_data_.Access()->grid_column_start_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION,
      "grid-column-start must be an int!", parser_configs_);
}

bool ComputedCSSStyle::SetGridColumnEnd(const tasm::CSSValue& value,
                                        const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset,
      layout_computed_style_.grid_data_.Access()->grid_column_end_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION,
      "grid-column-end must be an int!", parser_configs_);
}

bool ComputedCSSStyle::SetGap(const tasm::CSSValue& value, const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

// Reuse the grid gap setting, because flex also supports using
// grid-column-gap/grid-row-gap
bool ComputedCSSStyle::SetColumnGap(const tasm::CSSValue& value,
                                    const bool reset) {
  return SetGridColumnGap(value, reset);
}

bool ComputedCSSStyle::SetRowGap(const tasm::CSSValue& value,
                                 const bool reset) {
  return SetGridRowGap(value, reset);
}

bool ComputedCSSStyle::SetGridColumnGap(const tasm::CSSValue& value,
                                        const bool reset) {
  return CSSStyleUtils::ComputeLengthStyle(
      value, reset, length_context_,
      layout_computed_style_.grid_data_.Access()->grid_column_gap_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_GAP(), parser_configs_);
}

bool ComputedCSSStyle::SetGridRowGap(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeLengthStyle(
      value, reset, length_context_,
      layout_computed_style_.grid_data_.Access()->grid_row_gap_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_GAP(), parser_configs_);
}

bool ComputedCSSStyle::SetJustifyItems(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<JustifyType>(
      value, reset, layout_computed_style_.grid_data_.Access()->justify_items_,
      DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_ITEMS,
      "justify-items must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetJustifySelf(const tasm::CSSValue& value,
                                      const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<JustifyType>(
      value, reset, layout_computed_style_.grid_data_.Access()->justify_self_,
      DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_SELF,
      "justify-self must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetGridAutoFlow(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<GridAutoFlowType>(
      value, reset, layout_computed_style_.grid_data_.Access()->grid_auto_flow_,
      DefaultLayoutStyle::SL_DEFAULT_GRID_AUTO_FLOW,
      "grid-auto-flow must be a enum!", parser_configs_);
}

bool ComputedCSSStyle::SetFlex(const tasm::CSSValue& value, const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetFlexFlow(const tasm::CSSValue& value,
                                   const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetPadding(const tasm::CSSValue& value,
                                  const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetMargin(const tasm::CSSValue& value,
                                 const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}
bool ComputedCSSStyle::SetInsetInlineStart(const tasm::CSSValue& value,
                                           const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetInsetInlineEnd(const tasm::CSSValue& value,
                                         const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBorderWidth(const tasm::CSSValue& value,
                                      const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

// setter

// deprecated. Here is only a Placeholder.
bool ComputedCSSStyle::SetImplicitAnimation(const tasm::CSSValue& value,
                                            const bool reset) {
  return false;
}

bool ComputedCSSStyle::SetOpacity(const tasm::CSSValue& value,
                                  const bool reset) {
  return CSSStyleUtils::ComputeFloatStyle(
      value, reset, opacity_, DefaultComputedStyle::DEFAULT_OPACITY,
      "opacity must be a float!", parser_configs_);
}

bool ComputedCSSStyle::SetOverflowX(const tasm::CSSValue& value,
                                    const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<OverflowType>(
      value, reset, overflow_x_, GetDefaultOverflowType(),
      "overflow-x must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetOverflowY(const tasm::CSSValue& value,
                                    const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<OverflowType>(
      value, reset, overflow_y_, GetDefaultOverflowType(),
      "overflow-y must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetFontSize(const tasm::CSSValue& value,
                                   const bool reset) {
  PrepareOptionalForTextAttributes();
  const float default_font_size =
      DEFAULT_FONT_SIZE_DP * length_context_.layouts_unit_per_px_;
  auto old_value = text_attributes_->font_size;
  if (reset) {
    text_attributes_->font_size = default_font_size;
  } else {
    text_attributes_->font_size = length_context_.cur_node_font_size_;
  }
  return base::FloatsNotEqual(text_attributes_->font_size, old_value);
}

bool ComputedCSSStyle::SetLineHeight(const tasm::CSSValue& value,
                                     const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value = text_attributes_->computed_line_height;
  if (reset) {
    text_attributes_->computed_line_height =
        DefaultComputedStyle::DEFAULT_LINE_HEIGHT;
    text_attributes_->line_height_factor =
        DefaultComputedStyle::DEFAULT_LINE_HEIGHT_FACTOR;
  } else {
    if (value.IsNumber() || value.IsPercent()) {
      auto old_factor = text_attributes_->line_height_factor;
      text_attributes_->line_height_factor =
          value.GetValue().Number() / (value.IsPercent() ? 100 : 1);
      text_attributes_->computed_line_height =
          text_attributes_->line_height_factor *
          length_context_.cur_node_font_size_;
      // Either the computed line height or the line height factor changes may
      // affect how the line height behaves.
      return base::FloatsNotEqual(text_attributes_->computed_line_height,
                                  old_value) ||
             base::FloatsNotEqual(old_factor,
                                  text_attributes_->computed_line_height);
    } else {
      if (UNLIKELY(!CalculateCSSValueToFloat(
              value, text_attributes_->computed_line_height, length_context_,
              parser_configs_, true))) {
        return false;
      }
      text_attributes_->line_height_factor =
          DefaultComputedStyle::DEFAULT_LINE_HEIGHT_FACTOR;
      return base::FloatsNotEqual(text_attributes_->computed_line_height,
                                  old_value);
    }
  }
  return false;
}

bool ComputedCSSStyle::SetXAutoFontSize(const tasm::CSSValue& value,
                                        const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_is_auto_font_size = text_attributes_->is_auto_font_size;
  auto old_auto_font_size_min_size = text_attributes_->auto_font_size_min_size;
  auto old_is_auto_font_size_max_size =
      text_attributes_->auto_font_size_max_size;
  auto old_auto_font_size_step_granularity =
      text_attributes_->auto_font_size_step_granularity;
  if (reset) {
    text_attributes_->is_auto_font_size =
        DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE;
    text_attributes_->auto_font_size_min_size =
        DefaultComputedStyle::DEFAULT_FLOAT;
    text_attributes_->auto_font_size_max_size =
        DefaultComputedStyle::DEFAULT_FLOAT;
    text_attributes_->auto_font_size_step_granularity =
        DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE_STEP_GRANULARITY;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDXAutoFontSize)
            .c_str(),
        tasm::ARRAY_TYPE)

    auto arr = value.GetValue().Array();
    CSS_HANDLER_FAIL_IF_NOT(
        arr->size() == 7, parser_configs_.enable_css_strict_mode,
        tasm::SIZE_ERROR,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDXAutoFontSize)
            .c_str(),
        arr->size())

    text_attributes_->is_auto_font_size = arr->get(0).Bool();
    if (UNLIKELY(!CalculateCSSValueToFloat(
            tasm::CSSValue(arr->get(1), static_cast<tasm::CSSValuePattern>(
                                            arr->get(2).Number())),
            text_attributes_->auto_font_size_min_size, length_context_,
            parser_configs_, true))) {
      return false;
    }
    if (UNLIKELY(!CalculateCSSValueToFloat(
            tasm::CSSValue(arr->get(3), static_cast<tasm::CSSValuePattern>(
                                            arr->get(4).Number())),
            text_attributes_->auto_font_size_max_size, length_context_,
            parser_configs_, true))) {
      return false;
    }
    if (UNLIKELY(!CalculateCSSValueToFloat(
            tasm::CSSValue(arr->get(5), static_cast<tasm::CSSValuePattern>(
                                            arr->get(6).Number())),
            text_attributes_->auto_font_size_step_granularity, length_context_,
            parser_configs_, true))) {
      return false;
    }
  }
  return old_is_auto_font_size != text_attributes_->is_auto_font_size ||
         old_auto_font_size_min_size !=
             text_attributes_->auto_font_size_min_size ||
         old_is_auto_font_size_max_size !=
             text_attributes_->auto_font_size_max_size ||
         old_auto_font_size_step_granularity !=
             text_attributes_->auto_font_size_step_granularity;
}

bool ComputedCSSStyle::SetXAutoFontSizePresetSizes(const tasm::CSSValue& value,
                                                   const bool reset) {
  PrepareOptionalForTextAttributes();

  auto old_value =
      text_attributes_->auto_font_size_preset_sizes
          ? *text_attributes_->auto_font_size_preset_sizes
          : DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE_PRESET_SIZES();
  if (reset) {
    text_attributes_->auto_font_size_preset_sizes.reset();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.IsArray(),
                            parser_configs_.enable_css_strict_mode,
                            tasm::TYPE_MUST_BE,
                            tasm::CSSProperty::GetPropertyName(
                                tasm::kPropertyIDXAutoFontSizePresetSizes)
                                .c_str(),
                            tasm::ARRAY_TYPE)

    auto arr = value.GetValue().Array();
    CSS_HANDLER_FAIL_IF_NOT(arr->size() % 2 == 0,
                            parser_configs_.enable_css_strict_mode,
                            tasm::SIZE_ERROR,
                            tasm::CSSProperty::GetPropertyName(
                                tasm::kPropertyIDXAutoFontSizePresetSizes)
                                .c_str(),
                            arr->size())

    std::vector<float> dest;
    for (size_t i = 0; i < arr->size() / 2; i++) {
      float preset_size;
      if (UNLIKELY(!CalculateCSSValueToFloat(
              tasm::CSSValue(arr->get(2 * i),
                             static_cast<tasm::CSSValuePattern>(
                                 arr->get(2 * i + 1).Number())),
              preset_size, length_context_, parser_configs_, true))) {
        return false;
      }
      dest.push_back(preset_size);
    }
    if (dest.size() > 0) {
      text_attributes_->auto_font_size_preset_sizes = dest;
    } else {
      text_attributes_->auto_font_size_preset_sizes.reset();
    }
  }

  return old_value !=
         (text_attributes_->auto_font_size_preset_sizes
              ? *text_attributes_->auto_font_size_preset_sizes
              : DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE_PRESET_SIZES());
}

bool ComputedCSSStyle::SetPerspective(const tasm::CSSValue& value,
                                      const bool reset) {
  CSSStyleUtils::PrepareOptional(perspective_data_);
  auto old_value = perspective_data_;
  if (reset) {
    perspective_data_.reset();
  } else {
    auto length =
        CSSStyleUtils::ToLength(value, length_context_, parser_configs_);
    if (length.second) {
      perspective_data_->length_ = length.first;
      perspective_data_->pattern_ = value.GetPattern();
    }
  }
  return !(old_value == perspective_data_);
}

bool ComputedCSSStyle::SetLetterSpacing(const tasm::CSSValue& value,
                                        const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value = text_attributes_->letter_spacing;
  if (reset) {
    text_attributes_->letter_spacing =
        DefaultComputedStyle::DEFAULT_LETTER_SPACING;
  } else if (UNLIKELY(!CalculateCSSValueToFloat(
                 value, text_attributes_->letter_spacing, length_context_,
                 parser_configs_, true))) {
    return false;
  }
  return base::FloatsNotEqual(text_attributes_->letter_spacing, old_value);
}
// transform

bool ComputedCSSStyle::SetTransform(const tasm::CSSValue& value,
                                    const bool reset) {
  return CSSStyleUtils::ComputeTransform(value, reset, transform_raw_,
                                         length_context_, parser_configs_);
}

bool ComputedCSSStyle::SetTransformOrigin(const tasm::CSSValue& value,
                                          const bool reset) {
  auto old_value = transform_origin_
                       ? *transform_origin_
                       : DefaultComputedStyle::DEFAULT_TRANSFORM_ORIGIN();
  if (reset) {
    transform_origin_.reset();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDTransformOrigin)
            .c_str(),
        tasm::ARRAY_TYPE)

    auto arr = value.GetValue().Array();
    CSS_HANDLER_FAIL_IF_NOT(
        arr->size() >= 2, parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDTransformOrigin)
            .c_str(),
        tasm::ARRAY_TYPE)

    CSSStyleUtils::PrepareOptional(transform_origin_);
    auto parse_result_x = CSSStyleUtils::ToLength(
        tasm::CSSValue(
            arr->get(TransformOriginData::INDEX_X),
            static_cast<CSSValuePattern>(
                arr->get(TransformOriginData::INDEX_X_UNIT).Number())),
        length_context_, parser_configs_);
    if (parse_result_x.second) {
      transform_origin_->x = parse_result_x.first;
    }
    if (arr->size() == 4) {
      auto parse_result_y = CSSStyleUtils::ToLength(
          tasm::CSSValue(
              arr->get(TransformOriginData::INDEX_Y),
              static_cast<CSSValuePattern>(
                  arr->get(TransformOriginData::INDEX_Y_UNIT).Number())),
          length_context_, parser_configs_);
      if (parse_result_y.second) {
        transform_origin_->y = parse_result_y.first;
      }
    }
  }
  return !(old_value == transform_origin_);
}

// animation
bool ComputedCSSStyle::SetAnimation(const tasm::CSSValue& value,
                                    const bool reset) {
  auto old_value = animation_data_;
  if (reset) {
    animation_data_.reset();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray() || value.IsMap(),
        parser_configs_.enable_css_strict_mode, tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDAnimation).c_str(),
        tasm::ARRAY_OR_MAP_TYPE)

    if (!animation_data_) {
      animation_data_ = std::vector<AnimationData>();
    }
    animation_data_->clear();
    if (value.IsArray()) {
      auto group = value.GetValue().Array();
      for (size_t i = 0; i < group->size(); i++) {
        if (animation_data_->size() < i + 1) {
          animation_data_->push_back(AnimationData());
        }
        CSSStyleUtils::ComputeAnimation(group->get(i), animation_data_->at(i),
                                        "animation must is invalid.",
                                        parser_configs_);
      }
    } else {
      animation_data_->push_back(AnimationData());
      CSSStyleUtils::ComputeAnimation(
          value.GetValue(), animation_data_->front(),
          "animation must is invalid.", parser_configs_);
    }
  }
  return old_value != animation_data_;
}

bool ComputedCSSStyle::SetAnimationName(const tasm::CSSValue& value,
                                        const bool reset) {
  auto reset_func = [](AnimationData& anim) { anim.name = base::String(); };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeStringStyle(
        tasm::CSSValue(value, CSSValuePattern::STRING), reset, anim.name,
        base::String(), "animation-name must be a string!",
        this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationTimingFunction(const tasm::CSSValue& value,
                                                  const bool reset) {
  auto reset_func = [](AnimationData& anim) { anim.timing_func.Reset(); };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeTimingFunction(value, reset, anim.timing_func,
                                                this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationIterationCount(const tasm::CSSValue& value,
                                                  const bool reset) {
  auto reset_func = [](AnimationData& anim) { anim.iteration_count = 1; };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeIntStyle(
        tasm::CSSValue(value, CSSValuePattern::NUMBER), reset,
        anim.iteration_count, 0, "animation-iteration-count must be a number!",
        this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationDuration(const tasm::CSSValue& value,
                                            const bool reset) {
  auto reset_func = [](AnimationData& anim) {
    anim.duration = DefaultComputedStyle::DEFAULT_LONG;
  };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeLongStyle(
        tasm::CSSValue(value, CSSValuePattern::NUMBER), reset, anim.duration,
        DefaultComputedStyle::DEFAULT_LONG,
        "animation-duration must be a long!", this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationFillMode(const tasm::CSSValue& value,
                                            const bool reset) {
  auto reset_func = [](AnimationData& anim) {
    anim.fill_mode = AnimationFillModeType::kNone;
  };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeEnumStyle<AnimationFillModeType>(
        tasm::CSSValue(value, CSSValuePattern::ENUM), reset, anim.fill_mode,
        AnimationFillModeType::kNone, "animation-fill-mode must be a string!",
        this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationDelay(const tasm::CSSValue& value,
                                         const bool reset) {
  auto reset_func = [](AnimationData& anim) {
    anim.delay = DefaultComputedStyle::DEFAULT_LONG;
  };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeLongStyle(
        tasm::CSSValue(value, CSSValuePattern::NUMBER), reset, anim.delay,
        DefaultComputedStyle::DEFAULT_LONG, "animation-delay must be a float!",
        this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationDirection(const tasm::CSSValue& value,
                                             const bool reset) {
  auto reset_func = [](AnimationData& anim) {
    anim.direction = AnimationDirectionType::kNormal;
  };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeEnumStyle<AnimationDirectionType>(
        tasm::CSSValue(value, CSSValuePattern::ENUM), reset, anim.direction,
        AnimationDirectionType::kNormal, "animation-direction must be a !",
        this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

bool ComputedCSSStyle::SetAnimationPlayState(const tasm::CSSValue& value,
                                             const bool reset) {
  auto reset_func = [](AnimationData& anim) {
    anim.play_state = AnimationPlayStateType::kRunning;
  };
  auto compute_func = [this](const lepus::Value& value, AnimationData& anim,
                             bool reset) -> bool {
    return CSSStyleUtils::ComputeEnumStyle<AnimationPlayStateType>(
        tasm::CSSValue(value, CSSValuePattern::ENUM), reset, anim.play_state,
        AnimationPlayStateType::kRunning,
        "animation-play-state must be a enum!", this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(
      animation_data_, value, reset_func, compute_func, reset, parser_configs_);
}

// layout animation

bool ComputedCSSStyle::SetLayoutAnimationCreateDelay(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeLongStyle(
      value, reset, layout_animation_data_->create_ani.delay,
      DefaultComputedStyle::DEFAULT_LONG,
      "layout-animation-create-delay must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationCreateDuration(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeLongStyle(
      value, reset, layout_animation_data_->create_ani.duration,
      DefaultComputedStyle::DEFAULT_LONG,
      "layout-animation-create-duration must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationCreateProperty(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeEnumStyle<starlight::AnimationPropertyType>(
      value, reset, layout_animation_data_->create_ani.property,
      starlight::AnimationPropertyType::kOpacity,
      "layout-animation-create-property must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationCreateTimingFunction(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return SetLayoutAnimationTimingFunctionInternal(
      value, reset, layout_animation_data_->create_ani.timing_function,
      parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationUpdateDelay(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeLongStyle(
      value, reset, layout_animation_data_->update_ani.delay,
      DefaultComputedStyle::DEFAULT_LONG,
      "layout-animation-update-delay must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationUpdateDuration(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeLongStyle(
      value, reset, layout_animation_data_->update_ani.duration,
      DefaultComputedStyle::DEFAULT_LONG,
      "layout-animation-update-duration must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationUpdateTimingFunction(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return SetLayoutAnimationTimingFunctionInternal(
      value, reset, layout_animation_data_->update_ani.timing_function,
      parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationDeleteDuration(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeLongStyle(
      value, reset, layout_animation_data_->delete_ani.duration,
      DefaultComputedStyle::DEFAULT_LONG,
      "layout-animation-delete-duration must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationDeleteDelay(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeLongStyle(
      value, reset, layout_animation_data_->delete_ani.delay,
      DefaultComputedStyle::DEFAULT_LONG,
      "layout-animation-delete-delay must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationDeleteProperty(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return CSSStyleUtils::ComputeEnumStyle<starlight::AnimationPropertyType>(
      value, reset, layout_animation_data_->delete_ani.property,
      starlight::AnimationPropertyType::kOpacity,
      "layout-animation-delete-property must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetLayoutAnimationDeleteTimingFunction(
    const tasm::CSSValue& value, const bool reset) {
  CSSStyleUtils::PrepareOptional(layout_animation_data_);
  return SetLayoutAnimationTimingFunctionInternal(
      value, reset, layout_animation_data_->delete_ani.timing_function,
      parser_configs_);
}

// TODO(baiqiang): Remove Transition shorthand setter
bool ComputedCSSStyle::SetTransition(const tasm::CSSValue& value,
                                     const bool reset) {
  auto old_value = transition_data_;
  if (reset) {
    transition_data_.reset();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDTransition).c_str(),
        tasm::ARRAY_TYPE)

    if (!transition_data_) {
      transition_data_ = std::vector<TransitionData>();
    }
    transition_data_->clear();
    auto group = value.GetValue().Array();
    BASE_STATIC_STRING_DECL(kProperty, "property");
    BASE_STATIC_STRING_DECL(kDuration, "duration");
    BASE_STATIC_STRING_DECL(kTiming, "timing");
    BASE_STATIC_STRING_DECL(kDelay, "delay");
    for (size_t i = 0; i < group->size(); i++) {
      if (transition_data_->size() < i + 1) {
        transition_data_->push_back(TransitionData());
      }
      auto dict = group->get(i).Table();
      (*transition_data_)[i].property = static_cast<AnimationPropertyType>(
          dict->GetValue(kProperty).Number());
      (*transition_data_)[i].duration = dict->GetValue(kDuration).Number();
      if (dict->Contains(kTiming)) {
        DCHECK(dict->GetValue(kTiming).IsArray());
        CSSStyleUtils::ComputeTimingFunction(
            dict->GetValue(kTiming).Array()->get(0), reset,
            (*transition_data_)[i].timing_func, parser_configs_);
      }
      if (dict->Contains(kDelay)) {
        (*transition_data_)[i].delay = dict->GetValue(kDelay).Number();
      }
    }
  }
  return old_value != transition_data_;
}

bool ComputedCSSStyle::SetTransitionProperty(const tasm::CSSValue& value,
                                             const bool reset) {
  auto reset_func = [](TransitionData& transition) {
    transition.property = AnimationPropertyType::kNone;
  };
  auto compute_func = [](const lepus::Value& value, TransitionData& transition,
                         bool reset) {
    AnimationPropertyType old = transition.property;
    transition.property = static_cast<AnimationPropertyType>(value.Number());
    return old != transition.property;
  };
  return CSSStyleUtils::SetAnimationProperty(transition_data_, value,
                                             reset_func, compute_func, reset,
                                             parser_configs_);
}

bool ComputedCSSStyle::SetTransitionTimingFunction(const tasm::CSSValue& value,
                                                   const bool reset) {
  auto reset_func = [](TransitionData& transition) {
    transition.timing_func.Reset();
  };
  auto compute_func = [this](const lepus::Value& value,
                             TransitionData& transition, bool reset) {
    return CSSStyleUtils::ComputeTimingFunction(
        value, reset, transition.timing_func, this->parser_configs_);
  };
  return CSSStyleUtils::SetAnimationProperty(transition_data_, value,
                                             reset_func, compute_func, reset,
                                             parser_configs_);
}

bool ComputedCSSStyle::SetTransitionDuration(const tasm::CSSValue& value,
                                             const bool reset) {
  auto reset_func = [](TransitionData& transition) { transition.duration = 0; };
  auto compute_func = [](const lepus::Value& value, TransitionData& transition,
                         bool reset) {
    long old = transition.duration;
    transition.duration = value.Number();
    return old != transition.duration;
  };
  return CSSStyleUtils::SetAnimationProperty(transition_data_, value,
                                             reset_func, compute_func, reset,
                                             parser_configs_);
}

bool ComputedCSSStyle::SetTransitionDelay(const tasm::CSSValue& value,
                                          const bool reset) {
  auto reset_func = [](TransitionData& transition) { transition.delay = 0; };
  auto compute_func = [](const lepus::Value& value, TransitionData& transition,
                         bool reset) {
    long old = transition.delay;
    transition.delay = value.Number();
    return old != transition.delay;
  };
  return CSSStyleUtils::SetAnimationProperty(transition_data_, value,
                                             reset_func, compute_func, reset,
                                             parser_configs_);
}

bool ComputedCSSStyle::SetEnterTransitionName(const lynx::tasm::CSSValue& value,
                                              bool reset) {
  return CSSStyleUtils::ComputeHeroAnimation(
      value, reset, enter_transition_data_,
      "enter-transition-name must is invalid.", parser_configs_);
}

bool ComputedCSSStyle::SetExitTransitionName(lynx::tasm::CSSValue const& value,
                                             bool reset) {
  return CSSStyleUtils::ComputeHeroAnimation(
      value, reset, exit_transition_data_,
      "exit-transition-name must is invalid.", parser_configs_);
}

bool ComputedCSSStyle::SetPauseTransitionName(lynx::tasm::CSSValue const& value,
                                              bool reset) {
  return CSSStyleUtils::ComputeHeroAnimation(
      value, reset, pause_transition_data_,
      "pause-transition-name must is invalid.", parser_configs_);
}

bool ComputedCSSStyle::SetResumeTransitionName(
    lynx::tasm::CSSValue const& value, bool reset) {
  return CSSStyleUtils::ComputeHeroAnimation(
      value, reset, resume_transition_data_,
      "resume-transition-name must is invalid.", parser_configs_);
}

bool ComputedCSSStyle::SetLineSpacing(const tasm::CSSValue& value,
                                      const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value = text_attributes_->line_spacing;
  if (reset) {
    text_attributes_->line_spacing = DefaultComputedStyle::DEFAULT_LINE_SPACING;
  } else if (UNLIKELY(!CalculateCSSValueToFloat(
                 value, text_attributes_->line_spacing, length_context_,
                 parser_configs_, true))) {
    return false;
  }
  return base::FloatsNotEqual(text_attributes_->line_spacing, old_value);
}

bool ComputedCSSStyle::SetColor(const tasm::CSSValue& value, const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value_color = text_attributes_->color;
  auto old_value_gradient = text_attributes_->text_gradient;
  if (reset) {
    text_attributes_->color = DefaultColor::DEFAULT_TEXT_COLOR;
    text_attributes_->text_gradient = lepus::Value();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsNumber() || value.IsArray(),
        parser_configs_.enable_css_strict_mode, tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDColor).c_str(),
        tasm::ARRAY_OR_NUMBER_TYPE)

    if (value.IsNumber()) {
      text_attributes_->color =
          static_cast<uint32_t>(value.GetValue().Number());
      text_attributes_->text_gradient = lepus::Value();
    } else {
      text_attributes_->color = DefaultColor::DEFAULT_TEXT_COLOR;
      text_attributes_->text_gradient = value.GetValue();
    }
  }
  return old_value_color != text_attributes_->color ||
         old_value_gradient != text_attributes_->text_gradient;
}

bool ComputedCSSStyle::SetBackground(const tasm::CSSValue& value,
                                     const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetBackgroundColor(const tasm::CSSValue& value,
                                          const bool reset) {
  CSSStyleUtils::PrepareOptional(background_data_);
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset, background_data_->color, DefaultColor::DEFAULT_COLOR,
      "background-color must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetBackgroundImage(const tasm::CSSValue& value,
                                          const bool reset) {
  return SetBackgroundOrMaskImage(background_data_, value, reset);
}

bool ComputedCSSStyle::SetBackgroundSize(const tasm::CSSValue& value,
                                         const bool reset) {
  return SetBackgroundOrMaskSize(background_data_, length_context_,
                                 parser_configs_, value, reset);
}

bool ComputedCSSStyle::SetBackgroundClip(const lynx::tasm::CSSValue& value,
                                         bool reset) {
  return SetBackgroundOrMaskClip(background_data_, value, reset);
}

bool ComputedCSSStyle::SetBackgroundPosition(const tasm::CSSValue& value,
                                             const bool reset) {
  return SetBackgroundOrMaskPosition(background_data_, length_context_,
                                     parser_configs_, value, reset);
}

bool ComputedCSSStyle::SetBackgroundRepeat(const tasm::CSSValue& value,
                                           const bool reset) {
  return SetBackgroundOrMaskRepeat(background_data_, value, reset);
}

bool ComputedCSSStyle::SetBackgroundOrigin(const tasm::CSSValue& value,
                                           const bool reset) {
  return SetBackgroundOrMaskOrigin(background_data_, value, reset);
}

bool ComputedCSSStyle::SetFilter(const tasm::CSSValue& value,
                                 const bool reset) {
  if (!value.IsArray()) {
    // CSSParser enabled, only grayscale supported.
    auto last_filter = filter_;
    if (reset) {
      filter_.reset();
    } else {
      CSSStyleUtils::PrepareOptional(filter_);
      (*filter_).type = FilterType::kGrayscale;
      (*filter_).amount =
          NLength::MakePercentageNLength(value.GetValue().Number() * 100);
    }
    return last_filter != filter_;
  }

  return CSSStyleUtils::ComputeFilter(value, reset, filter_, length_context_,
                                      parser_configs_);
}

bool ComputedCSSStyle::SetBorderTopColor(const tasm::CSSValue& value,
                                         const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->color_top,
      DefaultColor::DEFAULT_BORDER_COLOR, "border-top-color must be a number",
      parser_configs_);
}

bool ComputedCSSStyle::SetBorderRightColor(const tasm::CSSValue& value,
                                           const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->color_right,
      DefaultColor::DEFAULT_BORDER_COLOR, "border-right-color must be a number",
      parser_configs_);
}

bool ComputedCSSStyle::SetBorderBottomColor(const tasm::CSSValue& value,
                                            const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->color_bottom,
      DefaultColor::DEFAULT_BORDER_COLOR,
      "border-bottom-color must be a number", parser_configs_);
}

bool ComputedCSSStyle::SetBorderLeftColor(const tasm::CSSValue& value,
                                          const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->color_left,
      DefaultColor::DEFAULT_BORDER_COLOR, "border-left-color must be a number",
      parser_configs_);
}

bool ComputedCSSStyle::SetBorderTopStyle(const tasm::CSSValue& value,
                                         const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return CSSStyleUtils::ComputeEnumStyle<BorderStyleType>(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->style_top,
      DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE),
      "border-top-style must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetBorderRightStyle(const tasm::CSSValue& value,
                                           const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return CSSStyleUtils::ComputeEnumStyle<BorderStyleType>(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->style_right,
      DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE),
      "border-right-style must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetBorderBottomStyle(const tasm::CSSValue& value,
                                            const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return CSSStyleUtils::ComputeEnumStyle<BorderStyleType>(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->style_bottom,
      DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE),
      "border-bottom-style must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetBorderLeftStyle(const tasm::CSSValue& value,
                                          const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_,
      css_align_with_legacy_w3c_);
  return CSSStyleUtils::ComputeEnumStyle<BorderStyleType>(
      value, reset,
      layout_computed_style_.surround_data_.border_data_->style_left,
      DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE),
      "border-left-style must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetOutlineColor(const lynx::tasm::CSSValue& value,
                                       bool reset) {
  CSSStyleUtils::PrepareOptional(outline_);
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset, outline_->color, DefaultColor::DEFAULT_OUTLINE_COLOR,
      "outline-color must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetOutlineStyle(const lynx::tasm::CSSValue& value,
                                       bool reset) {
  CSSStyleUtils::PrepareOptional(outline_);
  return CSSStyleUtils::ComputeEnumStyle(
      value, reset, outline_->style,
      DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE),
      "outline-style must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetOutlineWidth(const lynx::tasm::CSSValue& value,
                                       bool reset) {
  CSSStyleUtils::PrepareOptional(outline_);
  float old_value = outline_->width;
  if (reset) {
    outline_->width = DefaultComputedStyle::DEFAULT_FLOAT;
  } else {
    if (UNLIKELY(!CSSStyleUtils::CalculateLength(
            value, outline_->width, length_context_, parser_configs_))) {
      return false;
    }
  }
  return base::FloatsNotEqual(old_value, outline_->width);
}

bool ComputedCSSStyle::SetVisibility(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<VisibilityType>(
      value, reset, visibility_, DefaultComputedStyle::DEFAULT_VISIBILITY,
      "visibility must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetBoxShadow(const tasm::CSSValue& value,
                                    const bool reset) {
  return CSSStyleUtils::ComputeShadowStyle(value, reset, box_shadow_,
                                           length_context_, parser_configs_);
}

bool ComputedCSSStyle::SetBorderColor(const tasm::CSSValue& value,
                                      const bool reset) {
  return tasm::UnitHandler::CSSWarning(false,
                                       parser_configs_.enable_css_strict_mode,
                                       "Set Border Color will never be called");
}

bool ComputedCSSStyle::SetFontFamily(const tasm::CSSValue& value,
                                     const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeStringStyle(
      value, reset, text_attributes_->font_family, base::String(),
      "font family must be a string!", parser_configs_);
}

bool ComputedCSSStyle::SetCaretColor(const lynx::tasm::CSSValue& value,
                                     bool reset) {
  return CSSStyleUtils::ComputeStringStyle(
      value, reset, caret_color_, base::String(),
      "caret-color must be a string!", parser_configs_);
}

bool ComputedCSSStyle::SetTextShadow(const tasm::CSSValue& value,
                                     const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeShadowStyle(value, reset,
                                           text_attributes_->text_shadow,
                                           length_context_, parser_configs_);
}

bool ComputedCSSStyle::SetWhiteSpace(const tasm::CSSValue& value,
                                     const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<WhiteSpaceType>(
      value, reset, text_attributes_->white_space,
      DefaultComputedStyle::DEFAULT_WHITE_SPACE, "white-space must be an enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetFontWeight(const tasm::CSSValue& value,
                                     const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<starlight::FontWeightType>(
      value, reset, text_attributes_->font_weight,
      DefaultComputedStyle::DEFAULT_FONT_WEIGHT, "font weight must be an enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetWordBreak(const tasm::CSSValue& value,
                                    const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<WordBreakType>(
      value, reset, text_attributes_->word_break,
      DefaultComputedStyle::DEFAULT_WORD_BREAK, "word-break must be an enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetFontStyle(const tasm::CSSValue& value,
                                    const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<starlight::FontStyleType>(
      value, reset, text_attributes_->font_style,
      DefaultComputedStyle::DEFAULT_FONT_STYLE, "font style must be an enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetTextAlign(const tasm::CSSValue& value,
                                    const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<starlight::TextAlignType>(
      value, reset, text_attributes_->text_align,
      DefaultComputedStyle::DEFAULT_TEXT_ALIGN, "text align must be an enum!",
      parser_configs_);
}

bool ComputedCSSStyle::SetTextOverflow(const tasm::CSSValue& value,
                                       const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<starlight::TextOverflowType>(
      value, reset, text_attributes_->text_overflow,
      DefaultComputedStyle::DEFAULT_TEXT_OVERFLOW,
      "text overflow must be an enum!", parser_configs_);
}

bool ComputedCSSStyle::SetTextDecoration(const tasm::CSSValue& value,
                                         const bool reset) {
  PrepareOptionalForTextAttributes();

  const auto get_flags = [this]() {
    uint32_t flags = 0;
    if (text_attributes_->underline_decoration) {
      flags |= static_cast<int>(TextDecorationType::kUnderLine);
    }
    if (text_attributes_->line_through_decoration) {
      flags |= static_cast<int>(TextDecorationType::kLineThrough);
    }
    if (text_attributes_->text_decoration_style) {
      flags |= static_cast<int>(text_attributes_->text_decoration_style);
    }
    return flags;
  };
  uint32_t old_flags = get_flags();
  uint32_t old_color = text_attributes_->text_decoration_color;

  if (reset) {
    text_attributes_->underline_decoration =
        DefaultComputedStyle::DEFAULT_BOOLEAN;
    text_attributes_->line_through_decoration =
        DefaultComputedStyle::DEFAULT_BOOLEAN;
    text_attributes_->text_decoration_style =
        static_cast<unsigned int>(TextDecorationType::kSolid);
    text_attributes_->text_decoration_color = 0;
  } else {
    text_attributes_->underline_decoration =
        DefaultComputedStyle::DEFAULT_BOOLEAN;
    text_attributes_->line_through_decoration =
        DefaultComputedStyle::DEFAULT_BOOLEAN;
    text_attributes_->text_decoration_style =
        DefaultComputedStyle::DEFAULT_TEXT_DECORATION_STYLE;
    text_attributes_->text_decoration_color = 0;
    auto result = value.GetValue().Array();
    for (size_t i = 0; i < result->size(); i++) {
      int decoration = static_cast<int>(result->get(i).Number());
      if (decoration & static_cast<int>(TextDecorationType::kColor)) {
        text_attributes_->text_decoration_color =
            static_cast<uint32_t>(result->get(i + 1).Number());
        i++;
        continue;
      }
      if (decoration & static_cast<int>(TextDecorationType::kUnderLine)) {
        text_attributes_->underline_decoration = true;
      } else if (decoration &
                 static_cast<int>(TextDecorationType::kLineThrough)) {
        text_attributes_->line_through_decoration = true;
      } else if (decoration) {
        text_attributes_->text_decoration_style = decoration;
      }
    }
  }
  uint32_t new_flags = get_flags();
  uint32_t new_color = text_attributes_->text_decoration_color;
  return (old_flags != new_flags) || (old_color != new_color);
}

bool ComputedCSSStyle::SetTextDecorationColor(const tasm::CSSValue& value,
                                              const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value_color = text_attributes_->decoration_color;
  if (reset) {
    text_attributes_->decoration_color = DefaultColor::DEFAULT_COLOR;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsNumber(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDTextDecorationColor)
            .c_str(),
        tasm::NUMBER_TYPE)

    if (value.IsNumber()) {
      text_attributes_->decoration_color =
          static_cast<unsigned int>(value.GetValue().Number());
    }
  }
  return old_value_color != text_attributes_->decoration_color;
}

bool ComputedCSSStyle::SetZIndex(const tasm::CSSValue& value,
                                 const bool reset) {
  return CSSStyleUtils::ComputeIntStyle(
      value, reset, z_index_, 0, "z-index must be a number!", parser_configs_);
}

bool ComputedCSSStyle::SetBorderRadius(const tasm::CSSValue& value,
                                       const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  auto old_value = layout_computed_style_.surround_data_.border_data_;
  if (reset) {
    layout_computed_style_.surround_data_.border_data_->radius_x_top_left =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_y_top_left =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_x_top_right =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_y_top_right =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_x_bottom_left =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_y_bottom_left =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_x_bottom_right =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
    layout_computed_style_.surround_data_.border_data_->radius_y_bottom_right =
        DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDBorderRadius)
            .c_str(),
        tasm::ARRAY_TYPE)

    auto container = value.GetValue().Array();
    for (int i = 0; i < 4; i++) {
      auto parse_result = CSSStyleUtils::ToLength(
          tasm::CSSValue(
              container->get(i * 4),
              static_cast<CSSValuePattern>(container->get(i * 4 + 1).Number())),
          length_context_, parser_configs_);

      CSS_HANDLER_FAIL_IF_NOT(
          parse_result.second, parser_configs_.enable_css_strict_mode,
          tasm::SET_PROPERTY_ERROR,
          tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDBorderRadius)
              .c_str())

      if (i == 0) {
        layout_computed_style_.surround_data_.border_data_->radius_x_top_left =
            std::move(parse_result.first);
      } else if (i == 1) {
        layout_computed_style_.surround_data_.border_data_->radius_x_top_right =
            std::move(parse_result.first);
      } else if (i == 2) {
        layout_computed_style_.surround_data_.border_data_
            ->radius_x_bottom_right = std::move(parse_result.first);
      } else if (i == 3) {
        layout_computed_style_.surround_data_.border_data_
            ->radius_x_bottom_left = std::move(parse_result.first);
      }

      parse_result = CSSStyleUtils::ToLength(
          tasm::CSSValue(
              container->get(i * 4 + 2),
              static_cast<CSSValuePattern>(container->get(i * 4 + 3).Number())),
          length_context_, parser_configs_);

      CSS_HANDLER_FAIL_IF_NOT(
          parse_result.second, parser_configs_.enable_css_strict_mode,
          tasm::SET_PROPERTY_ERROR,
          tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDBorderRadius)
              .c_str())

      if (i == 0) {
        layout_computed_style_.surround_data_.border_data_->radius_y_top_left =
            std::move(parse_result.first);
      } else if (i == 1) {
        layout_computed_style_.surround_data_.border_data_->radius_y_top_right =
            std::move(parse_result.first);
      } else if (i == 2) {
        layout_computed_style_.surround_data_.border_data_
            ->radius_y_bottom_right = std::move(parse_result.first);
      } else if (i == 3) {
        layout_computed_style_.surround_data_.border_data_
            ->radius_y_bottom_left = std::move(parse_result.first);
      }
    }
  }
  return old_value->radius_x_top_left != layout_computed_style_.surround_data_
                                             .border_data_->radius_x_top_left ||
         old_value->radius_y_top_left != layout_computed_style_.surround_data_
                                             .border_data_->radius_y_top_left ||
         old_value->radius_x_top_right !=
             layout_computed_style_.surround_data_.border_data_
                 ->radius_x_top_right ||
         old_value->radius_y_top_right !=
             layout_computed_style_.surround_data_.border_data_
                 ->radius_y_top_right ||
         old_value->radius_x_bottom_left !=
             layout_computed_style_.surround_data_.border_data_
                 ->radius_x_bottom_left ||
         old_value->radius_y_bottom_left !=
             layout_computed_style_.surround_data_.border_data_
                 ->radius_y_bottom_left ||
         old_value->radius_x_bottom_right !=
             layout_computed_style_.surround_data_.border_data_
                 ->radius_x_bottom_right ||
         old_value->radius_y_bottom_right !=
             layout_computed_style_.surround_data_.border_data_
                 ->radius_y_bottom_right;
}

bool ComputedCSSStyle::SetBorderTopLeftRadius(const tasm::CSSValue& value,
                                              const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  auto old_value = layout_computed_style_.surround_data_.border_data_;

  return SetBorderRadiusHelper(layout_computed_style_.surround_data_
                                   .border_data_->radius_x_top_left,
                               layout_computed_style_.surround_data_
                                   .border_data_->radius_y_top_left,
                               length_context_,
                               tasm::kPropertyIDBorderTopLeftRadius, value,
                               reset, parser_configs_) &&
         (old_value->radius_x_top_left !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_left ||
          old_value->radius_y_top_left != layout_computed_style_.surround_data_
                                              .border_data_->radius_y_top_left);
}

bool ComputedCSSStyle::SetBorderTopRightRadius(const tasm::CSSValue& value,
                                               const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  auto old_value = layout_computed_style_.surround_data_.border_data_;

  return SetBorderRadiusHelper(layout_computed_style_.surround_data_
                                   .border_data_->radius_x_top_right,
                               layout_computed_style_.surround_data_
                                   .border_data_->radius_y_top_right,
                               length_context_,
                               tasm::kPropertyIDBorderTopRightRadius, value,
                               reset, parser_configs_) &&
         (old_value->radius_x_top_right !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_right ||
          old_value->radius_y_top_right !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_right);
}

bool ComputedCSSStyle::SetBorderBottomRightRadius(const tasm::CSSValue& value,
                                                  const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  auto old_value = layout_computed_style_.surround_data_.border_data_;

  return SetBorderRadiusHelper(layout_computed_style_.surround_data_
                                   .border_data_->radius_x_bottom_right,
                               layout_computed_style_.surround_data_
                                   .border_data_->radius_y_bottom_right,
                               length_context_,
                               tasm::kPropertyIDBorderBottomRightRadius, value,
                               reset, parser_configs_) &&
         (old_value->radius_x_bottom_right !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_right ||
          old_value->radius_y_bottom_right !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_right);
}

bool ComputedCSSStyle::SetBorderBottomLeftRadius(const tasm::CSSValue& value,
                                                 const bool reset) {
  CSSStyleUtils::PrepareOptional(
      layout_computed_style_.surround_data_.border_data_);
  auto old_value = layout_computed_style_.surround_data_.border_data_;

  return SetBorderRadiusHelper(layout_computed_style_.surround_data_
                                   .border_data_->radius_x_bottom_left,
                               layout_computed_style_.surround_data_
                                   .border_data_->radius_y_bottom_left,
                               length_context_,
                               tasm::kPropertyIDBorderBottomLeftRadius, value,
                               reset, parser_configs_) &&
         (old_value->radius_x_bottom_left !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_left ||
          old_value->radius_y_bottom_left !=
              layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_left);
}

bool ComputedCSSStyle::SetBorderStyle(const tasm::CSSValue& value,
                                      const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetAdaptFontSize(const tasm::CSSValue& value,
                                        const bool reset) {
  return CSSStyleUtils::ComputeStringStyle(
      value, reset, adapt_font_size_, base::String(),
      "adapt-font-size must be a string!", parser_configs_);
}

bool ComputedCSSStyle::SetOutline(const tasm::CSSValue& value,
                                  const bool reset) {
  return tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
}

bool ComputedCSSStyle::SetVerticalAlign(const tasm::CSSValue& value,
                                        const bool reset) {
  PrepareOptionalForTextAttributes();
  auto old_value_type = text_attributes_->vertical_align;
  auto old_value_length = text_attributes_->vertical_align_length;
  if (reset) {
    text_attributes_->vertical_align =
        DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN;
    text_attributes_->vertical_align_length =
        DefaultComputedStyle::DEFAULT_FLOAT;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(
        value.IsArray(), parser_configs_.enable_css_strict_mode,
        tasm::TYPE_MUST_BE,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDVerticalAlign)
            .c_str(),
        tasm::ARRAY_TYPE)

    auto arr = value.GetValue().Array();
    CSS_HANDLER_FAIL_IF_NOT(
        arr->size() == 4, parser_configs_.enable_css_strict_mode,
        tasm::SIZE_ERROR,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDVerticalAlign)
            .c_str(),
        arr->size())

    CSS_HANDLER_FAIL_IF_NOT(
        static_cast<CSSValuePattern>(arr->get(1).Number()) ==
            CSSValuePattern::ENUM,
        parser_configs_.enable_css_strict_mode, tasm::TYPE_ERROR,
        tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDVerticalAlign)
            .c_str())

    text_attributes_->vertical_align =
        static_cast<starlight::VerticalAlignType>(arr->get(0).Number());
    if (text_attributes_->vertical_align == VerticalAlignType::kLength) {
      auto pattern = static_cast<tasm::CSSValuePattern>(arr->get(3).Number());
      std::pair<NLength, bool> result =
          CSSStyleUtils::ToLength(tasm::CSSValue(arr->get(2), pattern),
                                  length_context_, parser_configs_);
      CSS_HANDLER_FAIL_IF_NOT(
          (result.second && result.first.IsUnit()),
          parser_configs_.enable_css_strict_mode, tasm::TYPE_ERROR,
          tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDVerticalAlign)
              .c_str())

      text_attributes_->vertical_align_length = result.first.GetRawValue();
    } else if (text_attributes_->vertical_align ==
               VerticalAlignType::kPercent) {
      CSS_HANDLER_FAIL_IF_NOT(
          arr->get(2).IsNumber(), parser_configs_.enable_css_strict_mode,
          tasm::TYPE_ERROR,
          tasm::CSSProperty::GetPropertyName(tasm::kPropertyIDVerticalAlign)
              .c_str())

      text_attributes_->vertical_align_length = arr->get(2).Number();
    } else {
      text_attributes_->vertical_align_length = 0.0f;
    }
  }
  return old_value_type != text_attributes_->vertical_align ||
         old_value_length != text_attributes_->vertical_align_length;
}

bool ComputedCSSStyle::SetContent(const tasm::CSSValue& value,
                                  const bool reset) {
  return CSSStyleUtils::ComputeStringStyle(
      value, reset, content_, base::String(), "content must be a string!",
      parser_configs_);
}

bool ComputedCSSStyle::SetListMainAxisGap(const tasm::CSSValue& value,
                                          const bool reset) {
  return CSSStyleUtils::ComputeLengthStyle(
      value, reset, length_context_, layout_computed_style_.list_main_axis_gap_,
      DefaultLayoutStyle::SL_DEFAULT_ZEROLENGTH(), parser_configs_);
}

bool ComputedCSSStyle::SetListCrossAxisGap(const tasm::CSSValue& value,
                                           const bool reset) {
  return CSSStyleUtils::ComputeLengthStyle(
      value, reset, length_context_,
      layout_computed_style_.list_cross_axis_gap_,
      DefaultLayoutStyle::SL_DEFAULT_ZEROLENGTH(), parser_configs_);
}

bool ComputedCSSStyle::SetClipPath(const tasm::CSSValue& value,
                                   const bool reset) {
  // ref to old array
  fml::RefPtr<lepus::CArray> last_path = clip_path_;
  clip_path_ = lepus::CArray::Create();

  fml::RefPtr<lepus::CArray> raw_array;
  BasicShapeType type = BasicShapeType::kUnknown;
  if (reset || !value.IsArray() || value.GetValue().Array()->size() == 0) {
    // if not reset, it means value is invalid and launch warning.
    LynxWarning(reset, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
                "clip-path must be an array")
  } else {
    raw_array = value.GetValue().Array();
    type = static_cast<starlight::BasicShapeType>(raw_array->get(0).Number());
  }

  switch (type) {
    case BasicShapeType::kUnknown:
      // Unknown type, reset the clip-path.
      break;
    case BasicShapeType::kCircle:
      if (raw_array->size() != 7) {
        LOGW("Error in parsing basic shape circle.");
        return false;
      }
      CSSStyleUtils::ComputeBasicShapeCircle(raw_array, reset, clip_path_,
                                             length_context_, parser_configs_);
      break;
    case BasicShapeType::kEllipse:
      if (raw_array->size() != 9) {
        LOGW("Error in parsing basic shape circle.");
        return false;
      }
      CSSStyleUtils::ComputeBasicShapeEllipse(raw_array, reset, clip_path_,
                                              length_context_, parser_configs_);
      break;
    case BasicShapeType::kPath:
      if (raw_array->size() != 2) {
        LOGW("Error in parsing basic shape path.");
        return false;
      }
      CSSStyleUtils::ComputeBasicShapePath(raw_array, reset, clip_path_);
      break;
    case BasicShapeType::kSuperEllipse:
      if (raw_array->size() != 11) {
        LOGW("Error in parsing super ellipse.");
        return false;
      }
      CSSStyleUtils::ComputeSuperEllipse(raw_array, reset, clip_path_,
                                         length_context_, parser_configs_);
      break;
    case BasicShapeType::kInset:
      constexpr int INSET_ARRAY_LENGTH_RECT = 9;
      constexpr int INSET_ARRAY_LENGTH_ROUND = 25;
      constexpr int INSET_ARRAY_LENGTH_SUPER_ELLIPSE = 27;
      if (raw_array->size() != INSET_ARRAY_LENGTH_RECT &&
          raw_array->size() != INSET_ARRAY_LENGTH_ROUND &&
          raw_array->size() != INSET_ARRAY_LENGTH_SUPER_ELLIPSE) {
        LOGW("Error in parsing basic shape inset.");
        return false;
      }
      CSSStyleUtils::ComputeBasicShapeInset(raw_array, reset, clip_path_,
                                            length_context_, parser_configs_);
      break;
  }
  // Check last path equals to current path.
  return last_path.get() == nullptr || *last_path != *clip_path_;
}

// TODO(liyanbo): this will replace by drawInfo.
// getter

lepus_value ComputedCSSStyle::OpacityToLepus() { return lepus_value(opacity_); }

lepus_value ComputedCSSStyle::PositionToLepus() {
  return lepus_value(static_cast<int>(layout_computed_style_.position_));
}

lepus_value ComputedCSSStyle::OverflowToLepus() {
  return lepus_value(static_cast<int>(overflow_));
}

lepus_value ComputedCSSStyle::OverflowXToLepus() {
  return lepus_value(static_cast<int>(overflow_x_));
}
lepus_value ComputedCSSStyle::OverflowYToLepus() {
  return lepus_value(static_cast<int>(overflow_y_));
}

lepus_value ComputedCSSStyle::FontSizeToLepus() {
  if (text_attributes_) {
    return lepus_value(text_attributes_->font_size);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::LineHeightToLepus() {
  if (text_attributes_) {
    return lepus_value(text_attributes_->computed_line_height);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::XAutoFontSizeToLepus() {
  if (text_attributes_) {
    auto arr = lepus::CArray::Create();
    arr->emplace_back(text_attributes_->is_auto_font_size);
    arr->emplace_back(text_attributes_->auto_font_size_min_size);
    arr->emplace_back(text_attributes_->auto_font_size_max_size);
    arr->emplace_back(text_attributes_->auto_font_size_step_granularity);
    return lepus::Value(std::move(arr));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::XAutoFontSizePresetSizesToLepus() {
  if (text_attributes_ && text_attributes_->auto_font_size_preset_sizes) {
    base::InsertionSort((*text_attributes_->auto_font_size_preset_sizes).data(),
                        (*text_attributes_->auto_font_size_preset_sizes).size(),
                        [](float a, float b) { return a < b; });
    auto arr = lepus::CArray::Create();
    for (const auto& preset_size :
         *text_attributes_->auto_font_size_preset_sizes) {
      arr->emplace_back(preset_size);
    }
    return lepus::Value(std::move(arr));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::PerspectiveToLepus() {
  auto array = lepus::CArray::Create();
  NLength& length = perspective_data_->length_;
  tasm::CSSValuePattern pattern = perspective_data_->pattern_;
  array->emplace_back(length.GetRawValue());
  if (length.IsUnit()) {
    if (pattern == tasm::CSSValuePattern::NUMBER) {
      array->emplace_back(static_cast<int>(PerspectiveLengthUnit::NUMBER));
    } else {
      array->emplace_back(static_cast<int>(PerspectiveLengthUnit::PX));
    }
  } else {
    array->emplace_back(static_cast<int>(PerspectiveLengthUnit::DEFAULT));
  }
  return lepus_value(std::move(array));
}

lepus_value ComputedCSSStyle::LetterSpacingToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<double>(text_attributes_->letter_spacing));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::LineSpacingToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<double>(text_attributes_->line_spacing));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::ColorToLepus() {
  if (text_attributes_) {
    if (text_attributes_->text_gradient.IsArray()) {
      return text_attributes_->text_gradient;
    } else {
      return lepus_value(text_attributes_->color);
    }
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BackgroundToLepus() { return lepus_value(); }

lepus_value ComputedCSSStyle::BackgroundColorToLepus() {
  if (background_data_) {
    return lepus_value(background_data_->color);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BackgroundImageToLepus() {
  return BackgroundOrMaskImageToLepus(background_data_, length_context_,
                                      parser_configs_);
}

lepus_value ComputedCSSStyle::BackgroundSizeToLepus() {
  return BackgroundOrMaskSizeToLepus(background_data_);
}

lepus_value ComputedCSSStyle::BackgroundClipToLepus() {
  return BackgroundOrMaskClipToLepus(background_data_);
}

lepus_value ComputedCSSStyle::BackgroundOriginToLepus() {
  return BackgroundOrMaskOriginToLepus(background_data_);
}

lepus_value ComputedCSSStyle::BackgroundPositionToLepus() {
  return BackgroundOrMaskPositionToLepus(background_data_);
}

lepus_value ComputedCSSStyle::BackgroundRepeatToLepus() {
  return BackgroundOrMaskRepeatToLepus(background_data_);
}

lepus_value ComputedCSSStyle::FilterToLepus() {
  return CSSStyleUtils::FilterToLepus(filter_);
}

lepus_value ComputedCSSStyle::BorderTopColorToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(
        layout_computed_style_.surround_data_.border_data_->color_top);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderRightColorToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(
        layout_computed_style_.surround_data_.border_data_->color_right);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderBottomColorToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(
        layout_computed_style_.surround_data_.border_data_->color_bottom);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderLeftColorToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(
        layout_computed_style_.surround_data_.border_data_->color_left);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderTopWidthToLepus() {
  return lepus_value(
      static_cast<double>(layout_computed_style_.GetBorderTopWidth()));
}

lepus_value ComputedCSSStyle::BorderRightWidthToLepus() {
  return lepus_value(
      static_cast<double>(layout_computed_style_.GetBorderRightWidth()));
}

lepus_value ComputedCSSStyle::BorderBottomWidthToLepus() {
  return lepus_value(
      static_cast<double>(layout_computed_style_.GetBorderBottomWidth()));
}

lepus_value ComputedCSSStyle::BorderLeftWidthToLepus() {
  return lepus_value(
      static_cast<double>(layout_computed_style_.GetBorderLeftWidth()));
}

lepus_value ComputedCSSStyle::ListMainAxisGapToLepus() {
  return lepus_value(
      static_cast<double>(layout_computed_style_.GetListMainAxisGap()));
}

lepus_value ComputedCSSStyle::ListCrossAxisGapToLepus() {
  return lepus_value(
      static_cast<double>(layout_computed_style_.GetListCrossAxisGap()));
}

lepus_value ComputedCSSStyle::TransformToLepus() {
  if (transform_raw_) {
    return CSSStyleUtils::TransformToLepus(*transform_raw_);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TransformOriginToLepus() {
  auto array = lepus::CArray::Create();
  CSSStyleUtils::AddLengthToArray(array, transform_origin_->x);
  CSSStyleUtils::AddLengthToArray(array, transform_origin_->y);
  return lepus_value(std::move(array));
}

lepus_value ComputedCSSStyle::AnimationToLepus() {
  if (!animation_data_) {
    return lepus::Value();
  }
  auto array_wrap = lepus::CArray::Create();
  std::for_each(
      animation_data_->begin(), animation_data_->end(),
      [&array_wrap](AnimationData& anim) {
        array_wrap->emplace_back(CSSStyleUtils::AnimationDataToLepus(anim));
      });
  return lepus_value(std::move(array_wrap));
}

lepus_value ComputedCSSStyle::AnimationNameToLepus() {
  if (!animation_data_) {
    return lepus::Value();
  }
  auto array_wrap = lepus::CArray::Create();
  std::for_each(animation_data_->begin(), animation_data_->end(),
                [&array_wrap](AnimationData& anim) {
                  array_wrap->emplace_back(anim.name);
                });
  return lepus_value(std::move(array_wrap));
}

lepus_value ComputedCSSStyle::AnimationDurationToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  return lepus_value(static_cast<double>(animation_data_->front().duration));
}

lepus_value ComputedCSSStyle::AnimationTimingFunctionToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  auto array = lepus::CArray::Create();
  array->emplace_back(
      static_cast<int>(animation_data_->front().timing_func.timing_func));
  array->emplace_back(
      static_cast<int>(animation_data_->front().timing_func.steps_type));
  array->emplace_back(animation_data_->front().timing_func.x1);
  array->emplace_back(animation_data_->front().timing_func.y1);
  array->emplace_back(animation_data_->front().timing_func.x2);
  array->emplace_back(animation_data_->front().timing_func.y2);
  return lepus_value(std::move(array));
}

lepus_value ComputedCSSStyle::AnimationDelayToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  return lepus_value(static_cast<double>(animation_data_->front().delay));
}

lepus_value ComputedCSSStyle::AnimationIterationCountToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  return lepus_value(animation_data_->front().iteration_count);
}

lepus_value ComputedCSSStyle::AnimationDirectionToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  return lepus_value(static_cast<int>(animation_data_->front().direction));
}

lepus_value ComputedCSSStyle::AnimationFillModeToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  return lepus_value(static_cast<int>(animation_data_->front().fill_mode));
}

lepus_value ComputedCSSStyle::AnimationPlayStateToLepus() {
  DCHECK(animation_data_ && !animation_data_->empty());
  return lepus_value(static_cast<int>(animation_data_->front().play_state));
}

lepus_value ComputedCSSStyle::LayoutAnimationCreateDurationToLepus() {
  return lepus_value(
      static_cast<double>(layout_animation_data_->create_ani.duration));
}

lepus_value ComputedCSSStyle::LayoutAnimationCreateTimingFunctionToLepus() {
  return LayoutAnimationTimingFunctionToLepusHelper(
      layout_animation_data_->create_ani.timing_function);
}
lepus_value ComputedCSSStyle::LayoutAnimationCreateDelayToLepus() {
  return lepus_value(
      static_cast<double>(layout_animation_data_->create_ani.delay));
}
lepus_value ComputedCSSStyle::LayoutAnimationCreatePropertyToLepus() {
  return lepus_value(
      static_cast<int>(layout_animation_data_->create_ani.property));
}
lepus_value ComputedCSSStyle::LayoutAnimationDeleteDurationToLepus() {
  return lepus_value(
      static_cast<double>(layout_animation_data_->delete_ani.duration));
}
lepus_value ComputedCSSStyle::LayoutAnimationDeleteTimingFunctionToLepus() {
  return LayoutAnimationTimingFunctionToLepusHelper(
      layout_animation_data_->delete_ani.timing_function);
}
lepus_value ComputedCSSStyle::LayoutAnimationDeleteDelayToLepus() {
  return lepus_value(
      static_cast<double>(layout_animation_data_->delete_ani.delay));
}
lepus_value ComputedCSSStyle::LayoutAnimationDeletePropertyToLepus() {
  return lepus_value(
      static_cast<int>(layout_animation_data_->delete_ani.property));
}
lepus_value ComputedCSSStyle::LayoutAnimationUpdateDurationToLepus() {
  return lepus_value(
      static_cast<double>(layout_animation_data_->update_ani.duration));
}
lepus_value ComputedCSSStyle::LayoutAnimationUpdateTimingFunctionToLepus() {
  return LayoutAnimationTimingFunctionToLepusHelper(
      layout_animation_data_->update_ani.timing_function);
}
lepus_value ComputedCSSStyle::LayoutAnimationUpdateDelayToLepus() {
  return lepus_value(
      static_cast<double>(layout_animation_data_->update_ani.delay));
}

lepus_value ComputedCSSStyle::TransitionToLepus() {
  if (!transition_data_) {
    return lepus::Value();
  }
  auto array_wrap = lepus::CArray::Create();
  for (const auto& it : *transition_data_) {
    auto array = lepus::CArray::Create();
    if (it.property == AnimationPropertyType::kNone) {
      break;
    }
    array->reserve(9);
    array->emplace_back(static_cast<int>(it.property));
    array->emplace_back(static_cast<double>(it.duration));
    array->emplace_back(static_cast<int>(it.timing_func.timing_func));
    array->emplace_back(static_cast<int>(it.timing_func.steps_type));
    array->emplace_back(static_cast<double>(it.timing_func.x1));
    array->emplace_back(static_cast<double>(it.timing_func.y1));
    array->emplace_back(static_cast<double>(it.timing_func.x2));
    array->emplace_back(static_cast<double>(it.timing_func.y2));
    array->emplace_back(static_cast<double>(it.delay));
    array_wrap->emplace_back(std::move(array));
  }
  return lepus::Value(std::move(array_wrap));
}

lepus_value ComputedCSSStyle::TransitionPropertyToLepus() {
  tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
  return lepus_value();
}
lepus_value ComputedCSSStyle::TransitionDurationToLepus() {
  tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
  return lepus_value();
}
lepus_value ComputedCSSStyle::TransitionDelayToLepus() {
  tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
  return lepus_value();
}
lepus_value ComputedCSSStyle::TransitionTimingFunctionToLepus() {
  tasm::UnitHandler::CSSMethodUnreachable(
      parser_configs_.enable_css_strict_mode);
  return lepus_value();
}

lepus_value ComputedCSSStyle::EnterTransitionNameToLepus() {
  return CSSStyleUtils::AnimationDataToLepus(*enter_transition_data_);
}

lepus_value ComputedCSSStyle::ExitTransitionNameToLepus() {
  return CSSStyleUtils::AnimationDataToLepus(*exit_transition_data_);
}
lepus_value ComputedCSSStyle::PauseTransitionNameToLepus() {
  return CSSStyleUtils::AnimationDataToLepus(*pause_transition_data_);
}

lepus_value ComputedCSSStyle::ResumeTransitionNameToLepus() {
  return CSSStyleUtils::AnimationDataToLepus(*resume_transition_data_);
}

lepus_value ComputedCSSStyle::VisibilityToLepus() {
  return lepus_value(static_cast<int>(visibility_));
}

lepus_value ComputedCSSStyle::BorderTopStyleToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(static_cast<int>(
        layout_computed_style_.surround_data_.border_data_->style_top));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderRightStyleToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(static_cast<int>(
        layout_computed_style_.surround_data_.border_data_->style_right));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderBottomStyleToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(static_cast<int>(
        layout_computed_style_.surround_data_.border_data_->style_bottom));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderLeftStyleToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    return lepus_value(static_cast<int>(
        layout_computed_style_.surround_data_.border_data_->style_left));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::OutlineColorToLepus() {
  return lepus_value(outline_->color);
}
lepus_value ComputedCSSStyle::OutlineStyleToLepus() {
  return lepus_value(static_cast<int>(outline_->style));
}
lepus_value ComputedCSSStyle::OutlineWidthToLepus() {
  return lepus_value(static_cast<int>(outline_->width));
}

lepus_value ComputedCSSStyle::BorderColorToLepus() { return lepus_value(); }

lepus_value ComputedCSSStyle::FontFamilyToLepus() {
  if (text_attributes_) {
    return lepus_value(text_attributes_->font_family);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::CaretColorToLepus() {
  return lepus::Value(caret_color_);
}

lepus_value ComputedCSSStyle::DirectionToLepus() {
  return lepus::Value(static_cast<int>(
      layout_computed_style_.direction_ == DirectionType::kLynxRtl
          ? DirectionType::kRtl
          : layout_computed_style_.direction_));
}

lepus_value ComputedCSSStyle::WhiteSpaceToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->white_space));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::WordBreakToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->word_break));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BoxShadowToLepus() {
  if (box_shadow_) {
    return ShadowDataToLepus(*box_shadow_);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TextShadowToLepus() {
  if (text_attributes_ && text_attributes_->text_shadow) {
    return ShadowDataToLepus(*text_attributes_->text_shadow);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::FontWeightToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->font_weight));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::FontStyleToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->font_style));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TextAlignToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->text_align));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TextOverflowToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->text_overflow));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TextDecorationToLepus() {
  if (text_attributes_) {
    auto array = lepus::CArray::Create();
    int flags = 0;
    if (text_attributes_->underline_decoration) {
      flags |= static_cast<int>(TextDecorationType::kUnderLine);
    }
    if (text_attributes_->line_through_decoration) {
      flags |= static_cast<int>(TextDecorationType::kLineThrough);
    }
    array->emplace_back(static_cast<int32_t>(flags));
    array->emplace_back(
        static_cast<int32_t>(text_attributes_->text_decoration_style));
    // if not set the text-decoration-color, use the color as the default value
    array->emplace_back(
        static_cast<int32_t>(text_attributes_->text_decoration_color));
    return lepus_value{std::move(array)};
  } else {
    return lepus::Value{lepus::CArray::Create()};
  }
}

lepus_value ComputedCSSStyle::TextDecorationColorToLepus() {
  if (text_attributes_) {
    return lepus_value(text_attributes_->decoration_color);
  } else {
    return lepus_value();
  }
}

bool ComputedCSSStyle::InheritLineHeight(const ComputedCSSStyle& from) {
  DCHECK(from.text_attributes_.has_value());

  PrepareOptionalForTextAttributes();
  bool factor_different =
      base::FloatsNotEqual(text_attributes_->line_height_factor,
                           from.text_attributes_->line_height_factor);
  text_attributes_->line_height_factor =
      from.text_attributes_->line_height_factor;

  auto old_computed_value = text_attributes_->computed_line_height;
  if (text_attributes_->line_height_factor !=
      DefaultComputedStyle::DEFAULT_LINE_HEIGHT_FACTOR) {
    // Inherit the factor, when the line height is in factor form.
    text_attributes_->computed_line_height =
        text_attributes_->line_height_factor *
        length_context_.cur_node_font_size_;
  } else {
    text_attributes_->computed_line_height =
        from.text_attributes_->computed_line_height;
  }
  return factor_different ||
         base::FloatsNotEqual(text_attributes_->computed_line_height,
                              old_computed_value);
}

bool ComputedCSSStyle::InheritLineSpacing(const ComputedCSSStyle& from) {
  DCHECK(from.text_attributes_.has_value());
  if (!from.text_attributes_.has_value() ||
      (text_attributes_.has_value() &&
       text_attributes_->line_spacing == from.text_attributes_->line_spacing)) {
    return false;
  }
  PrepareOptionalForTextAttributes();
  text_attributes_->line_spacing = from.text_attributes_->line_spacing;
  return true;
}

bool ComputedCSSStyle::InheritLetterSpacing(const ComputedCSSStyle& from) {
  DCHECK(from.text_attributes_.has_value());
  if (!from.text_attributes_.has_value() ||
      (text_attributes_.has_value() &&
       text_attributes_->letter_spacing ==
           from.text_attributes_->letter_spacing)) {
    return false;
  }
  PrepareOptionalForTextAttributes();
  text_attributes_->letter_spacing = from.text_attributes_->letter_spacing;
  return true;
}

lepus_value ComputedCSSStyle::ZIndexToLepus() { return lepus_value(z_index_); }

lepus_value ComputedCSSStyle::VerticalAlignToLepus() {
  if (text_attributes_) {
    auto arr = lepus::CArray::Create();
    arr->emplace_back(static_cast<int>(text_attributes_->vertical_align));
    arr->emplace_back(text_attributes_->vertical_align_length);
    return lepus::Value(std::move(arr));
  } else {
    return lepus::Value();
  }
}

lepus_value ComputedCSSStyle::BorderRadiusToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    auto container = lepus::CArray::Create();
    container->reserve(16);
    CSSStyleUtils::AddLengthToArray(
        container,
        layout_computed_style_.surround_data_.border_data_->radius_x_top_left);
    CSSStyleUtils::AddLengthToArray(
        container,
        layout_computed_style_.surround_data_.border_data_->radius_y_top_left);
    CSSStyleUtils::AddLengthToArray(
        container,
        layout_computed_style_.surround_data_.border_data_->radius_x_top_right);
    CSSStyleUtils::AddLengthToArray(
        container,
        layout_computed_style_.surround_data_.border_data_->radius_y_top_right);
    CSSStyleUtils::AddLengthToArray(
        container, layout_computed_style_.surround_data_.border_data_
                       ->radius_x_bottom_right);
    CSSStyleUtils::AddLengthToArray(
        container, layout_computed_style_.surround_data_.border_data_
                       ->radius_y_bottom_right);
    CSSStyleUtils::AddLengthToArray(
        container, layout_computed_style_.surround_data_.border_data_
                       ->radius_x_bottom_left);
    CSSStyleUtils::AddLengthToArray(
        container, layout_computed_style_.surround_data_.border_data_
                       ->radius_y_bottom_left);
    return lepus::Value(std::move(container));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderTopLeftRadiusToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    auto array = lepus::CArray::Create();
    CSSStyleUtils::AddLengthToArray(
        array,
        layout_computed_style_.surround_data_.border_data_->radius_x_top_left);
    CSSStyleUtils::AddLengthToArray(
        array,
        layout_computed_style_.surround_data_.border_data_->radius_y_top_left);
    return lepus::Value(std::move(array));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderTopRightRadiusToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    auto array = lepus::CArray::Create();
    CSSStyleUtils::AddLengthToArray(
        array,
        layout_computed_style_.surround_data_.border_data_->radius_x_top_right);
    CSSStyleUtils::AddLengthToArray(
        array,
        layout_computed_style_.surround_data_.border_data_->radius_y_top_right);
    return lepus::Value(std::move(array));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderBottomRightRadiusToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    auto array = lepus::CArray::Create();
    CSSStyleUtils::AddLengthToArray(
        array, layout_computed_style_.surround_data_.border_data_
                   ->radius_x_bottom_right);
    CSSStyleUtils::AddLengthToArray(
        array, layout_computed_style_.surround_data_.border_data_
                   ->radius_y_bottom_right);
    return lepus::Value(std::move(array));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::BorderBottomLeftRadiusToLepus() {
  if (layout_computed_style_.surround_data_.border_data_) {
    auto array = lepus::CArray::Create();
    CSSStyleUtils::AddLengthToArray(
        array, layout_computed_style_.surround_data_.border_data_
                   ->radius_x_bottom_left);
    CSSStyleUtils::AddLengthToArray(
        array, layout_computed_style_.surround_data_.border_data_
                   ->radius_y_bottom_left);
    return lepus::Value(std::move(array));
  } else {
    return lepus_value();
  }
}

bool ComputedCSSStyle::SetCursor(const tasm::CSSValue& value,
                                 const bool reset) {
  auto old_value = cursor_;
  if (reset) {
    cursor_.reset();
    return true;
  } else {
    CSSStyleUtils::PrepareOptional(cursor_);
    cursor_ = value.GetValue();
    return old_value != cursor_;
  }
}

lepus_value ComputedCSSStyle::CursorToLepus() {
  if (cursor_ && cursor_->IsArray()) {
    return *cursor_;
  } else {
    return lepus_value();
  }
}

bool ComputedCSSStyle::SetTextIndent(const tasm::CSSValue& value,
                                     const bool reset) {
  PrepareOptionalForTextAttributes();
  if (reset) {
    text_attributes_->text_indent = DefaultLayoutStyle::SL_DEFAULT_ZEROLENGTH();
    return true;
  } else {
    auto old_value = text_attributes_->text_indent;
    auto ret = CSSStyleUtils::ToLength(value, length_context_, parser_configs_);
    if (!ret.second) {
      return false;
    }
    text_attributes_->text_indent = ret.first;
    return old_value != text_attributes_->text_indent;
  }
}

lepus_value ComputedCSSStyle::TextIndentToLepus() {
  if (text_attributes_) {
    auto array = lepus::CArray::Create();
    CSSStyleUtils::AddLengthToArray(array, text_attributes_->text_indent);
    return lepus_value(std::move(array));
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TextStrokeToLepus() {
  // Parse failed or reset, return an empty array.
  return lepus_value();
}

lepus_value ComputedCSSStyle::TextStrokeColorToLepus() {
  if (text_attributes_) {
    return lepus_value(text_attributes_->text_stroke_color);
  } else {
    return lepus_value();
  }
}

lepus_value ComputedCSSStyle::TextStrokeWidthToLepus() {
  if (text_attributes_) {
    return lepus_value(text_attributes_->text_stroke_width);
  } else {
    return lepus_value();
  }
}

lepus::Value ComputedCSSStyle::ClipPathToLepus() {
  // Parse failed or reset, return an empty array.
  return clip_path_.get() ? lepus::Value(clip_path_)
                          : lepus::Value(lepus::CArray::Create());
}

bool ComputedCSSStyle::SetMaskImage(const tasm::CSSValue& value,
                                    const bool reset) {
  return SetBackgroundOrMaskImage(mask_data_, value, reset);
}

bool ComputedCSSStyle::SetMaskSize(const tasm::CSSValue& value,
                                   const bool reset) {
  return SetBackgroundOrMaskSize(mask_data_, length_context_, parser_configs_,
                                 value, reset);
}

bool ComputedCSSStyle::SetMaskOrigin(const tasm::CSSValue& value,
                                     const bool reset) {
  return SetBackgroundOrMaskOrigin(mask_data_, value, reset);
}

bool ComputedCSSStyle::SetMaskClip(const tasm::CSSValue& value,
                                   const bool reset) {
  return SetBackgroundOrMaskClip(mask_data_, value, reset);
}

bool ComputedCSSStyle::SetMaskPosition(const tasm::CSSValue& value,
                                       const bool reset) {
  return SetBackgroundOrMaskPosition(mask_data_, length_context_,
                                     parser_configs_, value, reset);
}

bool ComputedCSSStyle::SetMaskRepeat(const tasm::CSSValue& value,
                                     const bool reset) {
  return SetBackgroundOrMaskRepeat(mask_data_, value, reset);
}

bool ComputedCSSStyle::SetMask(const tasm::CSSValue& value, const bool reset) {
  return false;
}

lepus_value ComputedCSSStyle::MaskImageToLepus() {
  return BackgroundOrMaskImageToLepus(mask_data_, length_context_,
                                      parser_configs_);
}

lepus_value ComputedCSSStyle::MaskSizeToLepus() {
  return BackgroundOrMaskSizeToLepus(mask_data_);
}

lepus_value ComputedCSSStyle::MaskClipToLepus() {
  return BackgroundOrMaskClipToLepus(mask_data_);
}

lepus_value ComputedCSSStyle::MaskOriginToLepus() {
  return BackgroundOrMaskOriginToLepus(mask_data_);
}

lepus_value ComputedCSSStyle::MaskPositionToLepus() {
  return BackgroundOrMaskPositionToLepus(mask_data_);
}

lepus_value ComputedCSSStyle::MaskRepeatToLepus() {
  return BackgroundOrMaskRepeatToLepus(mask_data_);
}

bool ComputedCSSStyle::SetImageRendering(const tasm::CSSValue& value,
                                         const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<ImageRenderingType>(
      value, reset, image_rendering_, ImageRenderingType::kAuto,
      "image-rendering must be a number!", parser_configs_);
}

lepus_value ComputedCSSStyle::ImageRenderingToLepus() {
  return lepus_value(static_cast<int>(image_rendering_));
}

bool ComputedCSSStyle::SetHyphens(const tasm::CSSValue& value,
                                  const bool reset) {
  PrepareOptionalForTextAttributes();
  return CSSStyleUtils::ComputeEnumStyle<HyphensType>(
      value, reset, text_attributes_->hyphens,
      DefaultComputedStyle::DEFAULT_HYPHENS, "hyphens must be an enum!",
      parser_configs_);
}

lepus_value ComputedCSSStyle::HyphensToLepus() {
  if (text_attributes_) {
    return lepus_value(static_cast<int>(text_attributes_->hyphens));
  } else {
    return lepus_value();
  }
}

bool ComputedCSSStyle::SetXAppRegion(const tasm::CSSValue& value,
                                     const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<XAppRegionType>(
      value, reset, app_region_, XAppRegionType::kNone,
      "app region must be an enum!", parser_configs_);
}

lepus_value ComputedCSSStyle::XAppRegionToLepus() {
  return lepus_value(static_cast<int>(app_region_));
}

bool ComputedCSSStyle::SetXAnimationColorInterpolation(
    const tasm::CSSValue& value, const bool reset) {
  return CSSStyleUtils::ComputeEnumStyle<XAnimationColorInterpolationType>(
      value, reset, new_animator_interpolation_,
      XAnimationColorInterpolationType::kAuto,
      "-x-animation-color-interpolation must be an enum", parser_configs_);
}

bool ComputedCSSStyle::SetXHandleColor(const tasm::CSSValue& value,
                                       const bool reset) {
  return CSSStyleUtils::ComputeUIntStyle(
      value, reset, handle_color_, DefaultColor::DEFAULT_COLOR,
      "-x-handle-color must be a number!", parser_configs_);
}

lepus_value ComputedCSSStyle::XHandleColorToLepus() {
  return lepus_value(handle_color_);
}

bool ComputedCSSStyle::SetXHandleSize(const tasm::CSSValue& value,
                                      const bool reset) {
  float old_value = handle_size_;
  if (reset) {
    handle_size_ = DefaultComputedStyle::DEFAULT_FLOAT;
  } else {
    if (UNLIKELY(!CalculateCSSValueToFloat(value, handle_size_, length_context_,
                                           parser_configs_))) {
      return false;
    }
  }
  return base::FloatsNotEqual(old_value, handle_size_);
}

lepus_value ComputedCSSStyle::XHandleSizeToLepus() {
  return lepus_value(handle_size_);
}

}  // namespace starlight
}  // namespace lynx
