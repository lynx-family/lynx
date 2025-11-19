// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/computed_css_style_css_text_helper.h"

#include <cstdint>
#include <sstream>
#include <string>

#include "base/include/value/base_string.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/transforms/transform_operations.h"
#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace tasm {

namespace {
constexpr const char kDefaultZIndex[] = "0";
constexpr const char kDefaultFilter[] = "none";
constexpr const char kDirectionLtr[] = "ltr";
constexpr const char kDirectionRtl[] = "rtl";
constexpr const char kDefaultBackgroundPosition[] = "0% 0%";
constexpr const char kDefaultBackgroundPositionDimensionValue[] = "50%";
constexpr const char* kPxMark = "px";
constexpr const char* kPercentageMark = "%";
constexpr const char kDefaultBorderRadius[] = "0px 0px";
constexpr CSSPropertyID kMarginIDs[] = {
    kPropertyIDMarginTop, kPropertyIDMarginRight, kPropertyIDMarginBottom,
    kPropertyIDMarginLeft};
constexpr CSSPropertyID kPaddingsIDs[] = {
    kPropertyIDPaddingTop, kPropertyIDPaddingRight, kPropertyIDPaddingBottom,
    kPropertyIDPaddingLeft};
constexpr CSSPropertyID kBorderWidthIDs[] = {
    kPropertyIDBorderTopWidth, kPropertyIDBorderRightWidth,
    kPropertyIDBorderBottomWidth, kPropertyIDBorderLeftWidth};
constexpr CSSPropertyID kBorderColorIDs[] = {
    kPropertyIDBorderTopColor, kPropertyIDBorderRightColor,
    kPropertyIDBorderBottomColor, kPropertyIDBorderLeftColor};

std::string NumericLengthToString(
    const starlight::NLength::BaseLength& length) {
  if (!length.HasValue()) {
    return std::string("0px");
  } else if (length.ContainsFixedValue() && !length.ContainsPercentage()) {
    return CSSDecoder::NumberToString(length.GetFixedPart()) + kPxMark;
  } else if (!length.ContainsFixedValue() && length.ContainsPercentage()) {
    return CSSDecoder::NumberToString(length.GetPercentagePart()) +
           kPercentageMark;
  } else {
    // Ignore scenario when both fixed part and percentage part exists.
  }

  return "0px";
}
}  // namespace

const ComputedCSSStyleCssTextHelper::StyleStringValueGetter*
ComputedCSSStyleCssTextHelper::GetterFuncMap() {
  static const StyleStringValueGetter* getter_func_map = []() {
    static StyleStringValueGetter map[tasm::CSSPropertyID::kPropertyEnd] = {
        nullptr};
#define DECLARE_GET_COMPUTED_VALUE_SUPPORTED_GETTER(name) \
  map[tasm::CSSPropertyID::kPropertyID##name] =           \
      &ComputedCSSStyleCssTextHelper::name##CSSText;
    FOREACH_GET_COMPUTED_VALUE_SUPPORTED_PROPERTY(
        DECLARE_GET_COMPUTED_VALUE_SUPPORTED_GETTER)
#undef DECLARE_GET_COMPUTED_VALUE_SUPPORTED_GETTER
#undef FOREACH_GET_COMPUTED_VALUE_SUPPORTED_PROPERTY
    return map;
  }();

  return getter_func_map;
}

base::String ComputedCSSStyleCssTextHelper::ColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->text_attributes_) {
    if (computed_css_style->text_attributes_->text_gradient.has_value() &&
        computed_css_style->text_attributes_->text_gradient->IsArray()) {
      return base::String();
    } else {
      return Uint32ToRGBString(
          computed_css_style->text_attributes_->color.has_value()
              ? *computed_css_style->text_attributes_->color
              : starlight::DefaultColor::DEFAULT_TEXT_COLOR);
    }
  }

  return base::String();
}

base::String ComputedCSSStyleCssTextHelper::BackgroundColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->background_data_) {
    return Uint32ToRGBString(computed_css_style->background_data_->color);
  } else {
    return base::String();
  }
}

base::String ComputedCSSStyleCssTextHelper::BorderColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  base::Vector<base::String> border_colors;
  border_colors.reserve(4);
  for (auto border_color_id : kBorderColorIDs) {
    border_colors.emplace_back(
        ComputedCSSStyleCssTextHelper::GetComputedStyleByPropertyID(
            border_color_id, computed_css_style, ref_layout_result));
  }

  return ConcatStringsWithWhitespace(border_colors);
}

base::String ComputedCSSStyleCssTextHelper::BorderTopColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return Uint32ToRGBString(computed_css_style->layout_computed_style_
                                 .surround_data_.border_data_->color_top);
  }
  return Uint32ToRGBString(starlight::DefaultColor::DEFAULT_BORDER_COLOR);
}

base::String ComputedCSSStyleCssTextHelper::BorderRightColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return Uint32ToRGBString(computed_css_style->layout_computed_style_
                                 .surround_data_.border_data_->color_right);
  }
  return Uint32ToRGBString(starlight::DefaultColor::DEFAULT_BORDER_COLOR);
}

base::String ComputedCSSStyleCssTextHelper::BorderBottomColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return Uint32ToRGBString(computed_css_style->layout_computed_style_
                                 .surround_data_.border_data_->color_bottom);
  }
  return Uint32ToRGBString(starlight::DefaultColor::DEFAULT_BORDER_COLOR);
}

base::String ComputedCSSStyleCssTextHelper::BorderLeftColorCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return Uint32ToRGBString(computed_css_style->layout_computed_style_
                                 .surround_data_.border_data_->color_left);
  }
  return Uint32ToRGBString(starlight::DefaultColor::DEFAULT_BORDER_COLOR);
}

base::String ComputedCSSStyleCssTextHelper::LeftCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(ref_layout_result.offset_.X());
}

base::String ComputedCSSStyleCssTextHelper::TopCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(ref_layout_result.offset_.Y());
}

base::String ComputedCSSStyleCssTextHelper::BottomCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(ref_layout_result.offset_.Y() +
                            ref_layout_result.size_.height_);
}

base::String ComputedCSSStyleCssTextHelper::RightCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(ref_layout_result.offset_.X() +
                            ref_layout_result.size_.width_);
}

base::String ComputedCSSStyleCssTextHelper::WidthCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(ref_layout_result.size_.width_);
}

base::String ComputedCSSStyleCssTextHelper::HeightCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(ref_layout_result.size_.height_);
}

base::String ComputedCSSStyleCssTextHelper::TransformCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->transform_raw_) {
    auto transform_operations = transforms::TransformOperations(
        ref_layout_result, *(computed_css_style->transform_raw_));
    return transform_operations.CssText();
  } else {
    return base::String();
  }
}

base::String ComputedCSSStyleCssTextHelper::TransformOriginCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  base::Vector<base::String> tokens;
  tokens.reserve(2);
  tokens.emplace_back(FloatToPixelString(
      starlight::NLengthToLayoutUnit(
          computed_css_style->transform_origin_->x,
          starlight::LayoutUnit(ref_layout_result.size_.width_))
          .ToFloat()));
  tokens.emplace_back(FloatToPixelString(
      starlight::NLengthToLayoutUnit(
          computed_css_style->transform_origin_->y,
          starlight::LayoutUnit(ref_layout_result.size_.height_))
          .ToFloat()));
  return ConcatStringsWithWhitespace(tokens);
}

base::String ComputedCSSStyleCssTextHelper::OpacityCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return base::String(CSSDecoder::NumberToString(computed_css_style->opacity_));
}

base::String ComputedCSSStyleCssTextHelper::ZIndexCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->HasZIndex()) {
    return base::String(
        CSSDecoder::NumberToString(computed_css_style->GetZIndex()));
  }
  return BASE_STATIC_STRING(kDefaultZIndex);
}

base::String ComputedCSSStyleCssTextHelper::FilterCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (!computed_css_style->filter_.has_value()) {
    return BASE_STATIC_STRING(kDefaultFilter);
  }

  std::ostringstream filter_sstream;
  if (computed_css_style->filter_->type == starlight::FilterType::kBlur) {
    filter_sstream << "blur(";
    filter_sstream << CSSDecoder::NumberToString(
        starlight::NLengthToLayoutUnit(computed_css_style->filter_->amount,
                                       starlight::LayoutUnit::Indefinite())
            .ToFloat());
    filter_sstream << "px)";
  } else if (computed_css_style->filter_->type ==
             starlight::FilterType::kGrayscale) {
    filter_sstream << "grayscale(";
    filter_sstream << computed_css_style->filter_->amount.NumericLength()
                              .GetPercentagePart() /
                          100;
    filter_sstream << ")";
  } else {
    filter_sstream << "none";
  }

  return base::String(filter_sstream.str());
}

base::String ComputedCSSStyleCssTextHelper::DirectionCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return computed_css_style->GetDirection() == starlight::DirectionType::kRtl ||
                 computed_css_style->GetDirection() ==
                     starlight::DirectionType::kLynxRtl
             ? BASE_STATIC_STRING(kDirectionRtl)
             : BASE_STATIC_STRING(kDirectionLtr);
}

base::Vector<base::String>
ComputedCSSStyleCssTextHelper::ParseBackgroundPositionArray(
    starlight::ComputedCSSStyle* computed_css_style) {
  base::Vector<base::String> position_tokens;
  const auto& position =
      computed_css_style->background_data_->image_data->position;
  uint32_t token_count = static_cast<uint32_t>(position.size()) / 2;
  position_tokens.reserve(token_count);

  for (uint32_t idx = 0; idx < token_count; idx++) {
    base::Vector<base::String> tokens;
    const auto& x_length = position[idx * 2];
    const auto& y_length = position[idx * 2 + 1];

    if (x_length.IsUnit() || x_length.IsPercent()) {
      tokens.emplace_back(
          base::String(NumericLengthToString(x_length.NumericLength())));
    } else {
      tokens.emplace_back(
          BASE_STATIC_STRING(kDefaultBackgroundPositionDimensionValue));
    }

    if (y_length.IsUnit() || y_length.IsPercent()) {
      tokens.emplace_back(
          base::String(NumericLengthToString(y_length.NumericLength())));
    } else {
      tokens.emplace_back(
          BASE_STATIC_STRING(kDefaultBackgroundPositionDimensionValue));
    }

    position_tokens.emplace_back(ConcatStringsWithWhitespace(tokens));
  }

  return position_tokens;
}

base::String ComputedCSSStyleCssTextHelper::BackgroundPositionCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (!computed_css_style->background_data_.has_value() ||
      !computed_css_style->background_data_->image_data.has_value() ||
      computed_css_style->background_data_->image_data->image_count == 0) {
    return BASE_STATIC_STRING(kDefaultBackgroundPosition);
  }

  const auto& image_data = *computed_css_style->background_data_->image_data;
  auto image_count = image_data.image_count;

  if (image_data.position.empty()) {
    std::ostringstream position_stream;
    for (uint32_t idx = 0; idx < image_count; idx++) {
      position_stream << kDefaultBackgroundPosition;
      if (idx < image_count - 1) {
        position_stream << ", ";
      }
    }
    return base::String(position_stream.str());
  }

  auto position_tokens = ParseBackgroundPositionArray(computed_css_style);
  std::ostringstream position_stream;
  const size_t token_count = position_tokens.size();

  if (token_count == 0) {
    for (uint32_t idx = 0; idx < image_count; idx++) {
      position_stream << kDefaultBackgroundPosition;
      if (idx < image_count - 1) {
        position_stream << ", ";
      }
    }
  } else {
    for (uint32_t idx = 0; idx < image_count; idx++) {
      position_stream << position_tokens[idx % token_count].str();
      if (idx < image_count - 1) {
        position_stream << ", ";
      }
    }
  }

  return base::String(position_stream.str());
}

base::String ComputedCSSStyleCssTextHelper::PaddingCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  base::Vector<base::String> padding_tokens;
  padding_tokens.reserve(4);
  for (auto id : kPaddingsIDs) {
    padding_tokens.emplace_back(GetComputedStyleByPropertyID(
        id, computed_css_style, ref_layout_result));
  }
  return ConcatStringsWithWhitespace(padding_tokens);
}

base::String ComputedCSSStyleCssTextHelper::PaddingLeftCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.padding_[starlight::Direction::kLeft]);
}

base::String ComputedCSSStyleCssTextHelper::PaddingRightCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.padding_[starlight::Direction::kRight]);
}

base::String ComputedCSSStyleCssTextHelper::PaddingTopCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.padding_[starlight::Direction::kTop]);
}

base::String ComputedCSSStyleCssTextHelper::PaddingBottomCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.padding_[starlight::Direction::kBottom]);
}

base::String ComputedCSSStyleCssTextHelper::MarginCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  base::Vector<base::String> margin_tokens;
  margin_tokens.reserve(4);
  for (auto id : kMarginIDs) {
    margin_tokens.emplace_back(GetComputedStyleByPropertyID(
        id, computed_css_style, ref_layout_result));
  }
  return ConcatStringsWithWhitespace(margin_tokens);
}

base::String ComputedCSSStyleCssTextHelper::MarginTopCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.margin_[starlight::Direction::kTop]);
}

base::String ComputedCSSStyleCssTextHelper::MarginBottomCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.margin_[starlight::Direction::kBottom]);
}

base::String ComputedCSSStyleCssTextHelper::MarginLeftCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.margin_[starlight::Direction::kLeft]);
}

base::String ComputedCSSStyleCssTextHelper::MarginRightCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  return FloatToPixelString(
      ref_layout_result.margin_[starlight::Direction::kRight]);
}

base::String ComputedCSSStyleCssTextHelper::BorderWidthCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  base::Vector<base::String> border_width_tokens;
  border_width_tokens.reserve(4);
  for (auto id : kBorderWidthIDs) {
    border_width_tokens.emplace_back(GetComputedStyleByPropertyID(
        id, computed_css_style, ref_layout_result));
  }
  return ConcatStringsWithWhitespace(border_width_tokens);
}

base::String ComputedCSSStyleCssTextHelper::BorderLeftWidthCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return FloatToPixelString(computed_css_style->layout_computed_style_
                                  .surround_data_.border_data_->width_left);
  }
  return FloatToPixelString(GetDefaultBorderWidth(computed_css_style));
}

base::String ComputedCSSStyleCssTextHelper::BorderRightWidthCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return FloatToPixelString(computed_css_style->layout_computed_style_
                                  .surround_data_.border_data_->width_right);
  }
  return FloatToPixelString(GetDefaultBorderWidth(computed_css_style));
}

base::String ComputedCSSStyleCssTextHelper::BorderTopWidthCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return FloatToPixelString(computed_css_style->layout_computed_style_
                                  .surround_data_.border_data_->width_top);
  }
  return FloatToPixelString(GetDefaultBorderWidth(computed_css_style));
}

base::String ComputedCSSStyleCssTextHelper::BorderBottomWidthCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return FloatToPixelString(computed_css_style->layout_computed_style_
                                  .surround_data_.border_data_->width_bottom);
  }
  return FloatToPixelString(GetDefaultBorderWidth(computed_css_style));
}

base::String ComputedCSSStyleCssTextHelper::BorderRadiusCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  std::ostringstream border_radius_stream;
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    auto [top_left_rx, top_left_ry] = BorderRadiusPairCSSTextComponents(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_top_left,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_top_left,
        ref_layout_result);
    auto [top_right_rx, top_right_ry] = BorderRadiusPairCSSTextComponents(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_top_right,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_top_right,
        ref_layout_result);
    auto [bottom_right_rx, bottom_right_ry] = BorderRadiusPairCSSTextComponents(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_bottom_right,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_bottom_right,
        ref_layout_result);
    auto [bottom_left_rx, bottom_left_ry] = BorderRadiusPairCSSTextComponents(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_bottom_left,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_bottom_left,
        ref_layout_result);
    border_radius_stream << top_left_rx.str() << " " << top_right_rx.str()
                         << " " << bottom_right_rx.str() << " "
                         << bottom_left_rx.str() << " / " << top_left_ry.str()
                         << " " << top_right_ry.str() << " "
                         << bottom_right_ry.str() << " "
                         << bottom_left_ry.str();
  } else {
    border_radius_stream << "0px 0px 0px 0px / 0px 0px 0px 0px";
  }

  return border_radius_stream.str();
}

base::String ComputedCSSStyleCssTextHelper::BorderTopRightRadiusCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return BorderRadiusPairCSSText(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_top_right,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_top_right,
        ref_layout_result);
  }

  return BASE_STATIC_STRING(kDefaultBorderRadius);
}

base::String ComputedCSSStyleCssTextHelper::BorderTopLeftRadiusCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return BorderRadiusPairCSSText(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_top_left,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_top_left,
        ref_layout_result);
  }

  return BASE_STATIC_STRING(kDefaultBorderRadius);
}

base::String ComputedCSSStyleCssTextHelper::BorderBottomLeftRadiusCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return BorderRadiusPairCSSText(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_bottom_left,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_bottom_left,
        ref_layout_result);
  }

  return BASE_STATIC_STRING(kDefaultBorderRadius);
}

base::String ComputedCSSStyleCssTextHelper::BorderBottomRightRadiusCSSText(
    starlight::ComputedCSSStyle* computed_css_style,
    starlight::LayoutResultForRendering ref_layout_result) {
  if (computed_css_style->layout_computed_style_.surround_data_.border_data_
          .has_value()) {
    return BorderRadiusPairCSSText(
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_x_bottom_right,
        computed_css_style->layout_computed_style_.surround_data_.border_data_
            ->radius_y_bottom_right,
        ref_layout_result);
  }

  return BASE_STATIC_STRING(kDefaultBorderRadius);
}

base::String ComputedCSSStyleCssTextHelper::BorderRadiusPairCSSText(
    const starlight::NLength& rx, const starlight::NLength& ry,
    const starlight::LayoutResultForRendering& ref_layout_result) {
  base::Vector<base::String> tokens;
  tokens.reserve(2);
  auto [rx_str, ry_str] =
      BorderRadiusPairCSSTextComponents(rx, ry, ref_layout_result);
  tokens.emplace_back(rx_str);
  tokens.emplace_back(ry_str);
  return ConcatStringsWithWhitespace(tokens);
}

std::tuple<base::String, base::String>
ComputedCSSStyleCssTextHelper::BorderRadiusPairCSSTextComponents(
    const starlight::NLength& rx, const starlight::NLength& ry,
    const starlight::LayoutResultForRendering& ref_layout_result) {
  return std::make_tuple(
      FloatToPixelString(
          starlight::NLengthToLayoutUnit(
              rx, starlight::LayoutUnit(ref_layout_result.size_.width_))
              .ToFloat()),
      FloatToPixelString(
          starlight::NLengthToLayoutUnit(
              ry, starlight::LayoutUnit(ref_layout_result.size_.height_))
              .ToFloat()));
}

base::String ComputedCSSStyleCssTextHelper::FloatToPixelString(float value) {
  std::string num_value = CSSDecoder::NumberToString(value);

  return base::String(num_value + "px");
}

base::String ComputedCSSStyleCssTextHelper::Uint32ToRGBString(
    uint32_t color_value) {
  std::string result;
  result.reserve(24);
  uint32_t alpha = (color_value >> 24) & 0xFF;
  uint32_t red = (color_value >> 16) & 0xFF;
  uint32_t green = (color_value >> 8) & 0xFF;
  uint32_t blue = color_value & 0xFF;

  if (alpha != 255) {
    result += "argb(";
  } else {
    result += "rgb(";
  }
  result += std::to_string(red);
  result += ", ";
  result += std::to_string(green);
  result += ", ";
  result += std::to_string(blue);
  if (alpha != 255) {
    result += ", ";
    result += std::to_string(alpha / 255);
  }
  result += ')';
  return base::String(result);
}

base::String ComputedCSSStyleCssTextHelper::ConcatStringsWithWhitespace(
    const base::Vector<base::String>& strings) {
  if (strings.empty()) {
    return base::String();
  }
  if (strings.size() == 1) {
    return strings[0];
  }
  size_t total_length = 0;
  for (const auto& str : strings) {
    total_length += str.length();
  }
  total_length += strings.size() - 1;
  std::string result;
  result.reserve(total_length);
  for (size_t i = 0; i < strings.size(); ++i) {
    if (i > 0) {
      result += ' ';
    }
    result += strings[i].c_str();
  }
  return base::String(result);
}

float ComputedCSSStyleCssTextHelper::GetDefaultBorderWidth(
    starlight::ComputedCSSStyle* computed_css_style) {
  return computed_css_style->css_align_with_legacy_w3c_
             ? starlight::DefaultLayoutStyle::W3C_DEFAULT_BORDER
             : starlight::DefaultLayoutStyle::SL_DEFAULT_BORDER;
}

}  // namespace tasm
}  // namespace lynx
