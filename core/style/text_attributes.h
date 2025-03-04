// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_STYLE_TEXT_ATTRIBUTES_H_
#define CORE_STYLE_TEXT_ATTRIBUTES_H_

#include <optional>
#include <tuple>
#include <vector>

#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/style/shadow_data.h"

namespace lynx {
namespace starlight {

enum TextPropertyID {
  kTextProperIDFontSize = 1,
  kTextProperIDColor = 2,
  kTextProperIDWhiteSpace = 3,
  kTextProperIDTextOverflow = 4,
  kTextProperIDFontWeight = 5,
  kTextProperIDFontStyle = 6,
  kTextProperIDLineHeight = 7,
  kTextProperIDEnableFontScaling = 8,
  kTextProperIDLetterSpacing = 9,
  kTextProperIDLineSpacing = 10,
  kTextProperIDTextAlign = 11,
  kTextProperIDWordBreak = 12,
  kTextProperIDUnderline = 13,
  kTextProperIDLineThrough = 14,
  kTextProperIDHasTextShadow = 15,
  kTextProperIDShadowHOffset = 16,
  kTextProperIDShadowVOffset = 17,
  kTextProperIDShadowBlur = 18,
  kTextProperIDShadowColor = 19,
  kTextProperIDVerticalAlign = 20,
  kTextProperIDVerticalAlignLength = 21,
  kTextProperIDTextIndent = 22,

  kTextProperIDEnd = 0xFF,
};

class TextAttributes {
 public:
  TextAttributes(float default_font_size);

  float font_size;
  unsigned int color;
  unsigned int decoration_color;
  lepus::Value text_gradient;
  // TODO(linxs) this type has changed.
  starlight::WhiteSpaceType white_space;
  starlight::TextOverflowType text_overflow;
  starlight::FontWeightType font_weight;
  starlight::FontStyleType font_style;
  base::String font_family;
  float computed_line_height;
  float line_height_factor;
  bool enable_font_scaling;
  float letter_spacing;
  float line_spacing;
  starlight::TextAlignType text_align;
  starlight::WordBreakType word_break;
  bool underline_decoration;
  bool line_through_decoration;
  uint32_t text_decoration_color;
  uint32_t text_decoration_style;
  float text_stroke_width;
  unsigned int text_stroke_color;
  std::optional<std::vector<ShadowData>> text_shadow;
  starlight::VerticalAlignType vertical_align;
  double vertical_align_length;
  NLength text_indent;
  bool is_auto_font_size;
  float auto_font_size_min_size;
  float auto_font_size_max_size;
  float auto_font_size_step_granularity;
  std::optional<std::vector<float>> auto_font_size_preset_sizes;
  starlight::HyphensType hyphens;

  void Reset() {}

  bool operator==(const TextAttributes& rhs) const {
    return std::tie(font_size, color, decoration_color, white_space,
                    text_overflow, font_weight, font_style, font_family,
                    computed_line_height, line_height_factor,
                    enable_font_scaling, letter_spacing, line_spacing,
                    text_shadow, text_align, word_break, underline_decoration,
                    line_through_decoration, text_decoration_color,
                    text_decoration_style, text_indent, is_auto_font_size,
                    auto_font_size_min_size, auto_font_size_max_size,
                    auto_font_size_step_granularity,
                    auto_font_size_preset_sizes, hyphens) ==
           std::tie(rhs.font_size, rhs.color, rhs.decoration_color,
                    rhs.white_space, rhs.text_overflow, rhs.font_weight,
                    rhs.font_style, rhs.font_family, rhs.computed_line_height,
                    rhs.line_height_factor, rhs.enable_font_scaling,
                    rhs.letter_spacing, rhs.line_spacing, rhs.text_shadow,
                    rhs.text_align, rhs.word_break, rhs.underline_decoration,
                    rhs.line_through_decoration, rhs.text_decoration_color,
                    rhs.text_decoration_style, rhs.text_indent,
                    rhs.is_auto_font_size, rhs.auto_font_size_min_size,
                    rhs.auto_font_size_max_size,
                    rhs.auto_font_size_step_granularity,
                    rhs.auto_font_size_preset_sizes, hyphens);
  }

  bool operator!=(const TextAttributes& rhs) const { return !(*this == rhs); }

  void Apply(const TextAttributes& rhs);
};

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_STYLE_TEXT_ATTRIBUTES_H_
