// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/text_attributes.h"

#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/tasm/config.h"
#include "core/style/color.h"
#include "core/style/default_computed_style.h"

namespace lynx {
namespace starlight {
TextAttributes::TextAttributes(float default_font_size)
    : font_size(default_font_size),
      color(DefaultColor::DEFAULT_TEXT_COLOR),
      decoration_color(DefaultColor::DEFAULT_TEXT_COLOR),
      white_space(DefaultComputedStyle::DEFAULT_WHITE_SPACE),
      text_overflow(DefaultComputedStyle::DEFAULT_TEXT_OVERFLOW),
      font_weight(DefaultComputedStyle::DEFAULT_FONT_WEIGHT),
      font_style(DefaultComputedStyle::DEFAULT_FONT_STYLE),
      computed_line_height(DefaultComputedStyle::DEFAULT_LINE_HEIGHT),
      line_height_factor(DefaultComputedStyle::DEFAULT_LINE_HEIGHT_FACTOR),
      enable_font_scaling(DefaultComputedStyle::DEFAULT_BOOLEAN),
      letter_spacing(DefaultComputedStyle::DEFAULT_LETTER_SPACING),
      line_spacing(DefaultComputedStyle::DEFAULT_LINE_SPACING),
      text_align(DefaultComputedStyle::DEFAULT_TEXT_ALIGN),
      word_break(DefaultComputedStyle::DEFAULT_WORD_BREAK),
      underline_decoration(DefaultComputedStyle::DEFAULT_BOOLEAN),
      line_through_decoration(DefaultComputedStyle::DEFAULT_BOOLEAN),
      text_decoration_color(DefaultColor::DEFAULT_COLOR),
      text_decoration_style(
          DefaultComputedStyle::DEFAULT_TEXT_DECORATION_STYLE),
      text_stroke_width(DefaultComputedStyle::DEFAULT_FLOAT),
      text_stroke_color(DefaultColor::DEFAULT_COLOR),
      vertical_align(DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN),
      vertical_align_length(DefaultComputedStyle::DEFAULT_FLOAT),
      text_indent(DefaultLayoutStyle::SL_DEFAULT_ZEROLENGTH()),
      is_auto_font_size(DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE),
      auto_font_size_min_size(DefaultComputedStyle::DEFAULT_FLOAT),
      auto_font_size_max_size(DefaultComputedStyle::DEFAULT_FLOAT),
      auto_font_size_step_granularity(
          DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE_STEP_GRANULARITY),
      hyphens(DefaultComputedStyle::DEFAULT_HYPHENS) {
  text_shadow.reset();
  auto_font_size_preset_sizes.reset();
}

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
  text_shadow = rhs.text_shadow ? *rhs.text_shadow : text_shadow;
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
}

}  // namespace starlight
}  // namespace lynx
