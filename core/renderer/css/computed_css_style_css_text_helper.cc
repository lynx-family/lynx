// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/computed_css_style_css_text_helper.h"

#include <cstdint>
#include <string>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/transforms/transform_operations.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace tasm {

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

}  // namespace tasm
}  // namespace lynx
