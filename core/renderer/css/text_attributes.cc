// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/text_attributes.h"

#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/tasm/config.h"
#include "core/style/color.h"

namespace lynx {
namespace starlight {

void TextAttributes::Apply(const TextAttributes& rhs) {
  font_size = rhs.font_size;
  color = rhs.color;
  decoration_color = rhs.decoration_color;
  text_gradient = rhs.text_gradient;
  white_space = rhs.white_space;
  text_overflow = rhs.text_overflow;
  font_weight = rhs.font_weight;
  font_style = rhs.font_style;
  font_family = rhs.font_family;
  computed_line_height = rhs.computed_line_height;
  line_height_factor = rhs.line_height_factor;
  enable_font_scaling = rhs.enable_font_scaling;
  letter_spacing = rhs.letter_spacing;
  line_spacing = rhs.line_spacing;
  text_align = rhs.text_align;
  word_break = rhs.word_break;
  underline_decoration = rhs.underline_decoration;
  line_through_decoration = rhs.line_through_decoration;
  text_shadow = rhs.text_shadow ? rhs.text_shadow : text_shadow;
  vertical_align = rhs.vertical_align;
  vertical_align_length = rhs.vertical_align_length;
  text_indent = rhs.text_indent;
  is_auto_font_size = rhs.is_auto_font_size;
  auto_font_size_min_size = rhs.auto_font_size_min_size;
  auto_font_size_max_size = rhs.auto_font_size_max_size;
  auto_font_size_step_granularity = rhs.auto_font_size_step_granularity;
  auto_font_size_preset_sizes = rhs.auto_font_size_preset_sizes
                                    ? *rhs.auto_font_size_preset_sizes
                                    : auto_font_size_preset_sizes;
  hyphens = rhs.hyphens;
  font_variation_settings = rhs.font_variation_settings;
  font_feature_settings = rhs.font_feature_settings;
  font_optical_sizing = rhs.font_optical_sizing;
}

void TextAttributes::ProcessRadialGradientIfNeeded(
    const tasm::CssMeasureContext& length_context,
    const tasm::CSSParserConfigs& parser_configs) {
  if (text_gradient.has_value() && text_gradient->IsArray()) {
    // For radial gradients, compute the gradient data before setting the
    // property.
    if (text_gradient->Array()->size() > 1 &&
        text_gradient->Array()->get(0).Number() ==
            static_cast<uint32_t>(
                starlight::BackgroundImageType::kRadialGradient)) {
      // Clone the gradient data to avoid modifying the original data during
      // computation.
      if (!clone_text_gradient) {
        text_gradient =
            base::make_flex_optional(lepus::Value::Clone(*(text_gradient)));
        clone_text_gradient = true;
      }
      auto& gradient_data = text_gradient->Array()->get(1);
      starlight::CSSStyleUtils::ComputeRadialGradient(
          gradient_data, length_context, parser_configs);
    }
  }
}

}  // namespace starlight
}  // namespace lynx
