// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_TEXT_STYLE_H_
#define CLAY_UI_COMPONENT_TEXT_TEXT_STYLE_H_

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "clay/gfx/style/color.h"
#include "clay/gfx/style/shadow.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/painter/gradient.h"
#include "clay/ui/shadow/vertical_align_style.h"

namespace clay {

using TextMeasureMode = MeasureMode;

enum class TextAlignment { kLeft = 0, kCenter, kRight, kStart, kEnd, kJustify };

enum class TextDirection {
  kNormal = 0,
  kRtl = 2,
  kLtr = 3,
};

enum class FontStyle {
  kNormal = 0,
  kItalic,
  kOblique,
};

enum class FontWeight {
  kNormal = 0,
  kBold,
  k100,
  k200,
  k300,
  k400,
  k500,
  k600,
  k700,
  k800,
  k900,
};

// Multiple decorations can be applied at once. Ex: Underline and overline is
// (0x1 | 0x2)
// Note(Xietong): Use enum instead of enum class to do bit operation.
enum TextDecorationLine : uint8_t {
  kTextDecorationLineNone = 0x0,
  kTextDecorationLineUnderline = 0x1,
  kTextDecorationLineOverline = 0x2,
  kTextDecorationLineLineThrough = 0x4,
};

enum class TextDecorationStyle {
  kSolid = 1 << 2,
  kDouble = 1 << 3,
  kDotted = 1 << 4,
  kDashed = 1 << 5,
  kWavy = 1 << 6,
};

enum TextOverflow {
  kClip,
  kEllipsis,
};

enum WordBreak : uint8_t {
  kNormal = 0,
  kBreakAll = 1,
  kKeepAll = 2,
};

enum class WhiteSpace {
  kNormal = 0,
  kNoWrap = 1,
};

struct TextDecoration {
  // Support bit combination of TextDecorationLine
  uint8_t line;
  TextDecorationStyle style = TextDecorationStyle::kSolid;
  std::optional<Color> color;

  bool operator!=(const TextDecoration& oth) const {
    return line != oth.line || style != oth.style || color != oth.color;
  }
  bool operator==(const TextDecoration& oth) const {
    return line == oth.line && style == oth.style && color == oth.color;
  }
};

struct TextStyle {
  std::optional<bool> enable_font_scaling;
  std::optional<bool> half_leading;
  std::optional<float> font_size;
  std::optional<float> baseline_shift;
  std::optional<float> line_height;
  std::optional<float> line_spacing;
  std::optional<float> letter_spacing;
  std::optional<float> text_indent;
  std::optional<TextAlignment> text_align;
  std::optional<TextDirection> text_direction;
  std::optional<FontWeight> font_weight;
  std::optional<FontStyle> font_style;
  std::optional<Color> text_color;
  std::optional<Color> background_color;
  std::optional<std::string> font_family;
  std::optional<TextDecoration> text_decoration;
  std::optional<Gradient> text_gradient;
  std::optional<std::vector<Shadow>> text_shadows;
  std::optional<uint32_t> max_lines;
  std::optional<TextOverflow> overflow;
  std::optional<std::u16string> ellipsis;
  std::optional<WordBreak> word_break;
  std::optional<WhiteSpace> white_space;
  std::optional<int> foreground_id;
  std::optional<bool> keep_trailing_spaces;

  // Strut properties. strut_enabled must be set to true for the rest of the
  // properties to take effect.
  // TODO(garyq): Break the strut properties into a separate class.
  std::optional<bool> strut_enabled;
  std::optional<FontWeight> strut_font_weight;
  std::optional<FontStyle> strut_font_style;
  std::vector<std::string> strut_font_families;
  std::optional<double> strut_font_size;
  std::optional<double> strut_height;
  // Negative to use font's default leading. [0,inf)
  // to use custom leading as a ratio of font size.
  std::optional<double> strut_leading;
  std::optional<VerticalAlignType> align_type;
  std::optional<bool> enable_text_bounds;

  bool operator==(const TextStyle& style) const {
    return this->enable_font_scaling == style.enable_font_scaling &&
           this->half_leading == style.half_leading &&
           this->font_size == style.font_size &&
           this->baseline_shift == style.baseline_shift &&
           this->line_height == style.line_height &&
           this->line_spacing == style.line_spacing &&
           this->letter_spacing == style.letter_spacing &&
           this->text_indent == style.text_indent &&
           this->text_align == style.text_align &&
           this->text_direction == style.text_direction &&
           this->font_weight == style.font_weight &&
           this->font_style == style.font_style &&
           this->text_color == style.text_color &&
           this->background_color == style.background_color &&
           this->font_family == style.font_family &&
           this->text_decoration == style.text_decoration &&
           this->text_gradient == style.text_gradient &&
           this->text_shadows == style.text_shadows &&
           this->max_lines == style.max_lines &&
           this->overflow == style.overflow &&
           this->ellipsis == style.ellipsis &&
           this->word_break == style.word_break &&
           this->white_space == style.white_space &&
           this->foreground_id == style.foreground_id &&
           this->keep_trailing_spaces == style.keep_trailing_spaces &&
           this->align_type == style.align_type &&
           this->enable_text_bounds == style.enable_text_bounds;
  }
};

}  // namespace clay

namespace std {
template <>
struct hash<clay::TextStyle> {
  size_t operator()(const clay::TextStyle& text_style) const {
    int prime = 31;
    size_t result =
        prime + std::clamp(text_style.font_size
                               ? static_cast<int>(*text_style.font_size)
                               : 0,
                           0, 99999);
    result = result * prime +
             std::clamp(text_style.baseline_shift
                            ? static_cast<int>(*text_style.baseline_shift)
                            : 0,
                        0, 99999);
    result = result * prime +
             std::clamp(text_style.line_height
                            ? static_cast<int>(*text_style.line_height)
                            : 0,
                        0, 99999);
    result = result * prime +
             std::clamp(text_style.line_spacing
                            ? static_cast<int>(*text_style.line_spacing)
                            : 0,
                        0, 99999);
    result = result * prime +
             std::clamp(text_style.letter_spacing
                            ? static_cast<int>(*text_style.letter_spacing)
                            : 0,
                        0, 99999);
    result = result * prime +
             std::clamp(text_style.text_indent
                            ? static_cast<int>(*text_style.text_indent)
                            : 0,
                        0, 99999);
    result =
        result * prime +
        (text_style.text_align ? static_cast<int>(*text_style.text_align) : 0);
    result =
        result * prime + (text_style.text_direction
                              ? static_cast<int>(*text_style.text_direction)
                              : 0);
    result = result * prime + (text_style.font_weight
                                   ? static_cast<int>(*text_style.font_weight)
                                   : 0);
    result =
        result * prime +
        (text_style.font_style ? static_cast<int>(*text_style.font_style) : 0);
    result = result * prime +
             (text_style.text_color ? text_style.text_color->Value() : 0);
    result = result * prime + (text_style.background_color
                                   ? text_style.background_color->Value()
                                   : 0);
    result =
        result * prime + (text_style.max_lines ? *text_style.max_lines : 0);
    result = result * prime +
             (text_style.overflow ? static_cast<int>(*text_style.overflow) : 0);
    result =
        result * prime +
        (text_style.align_type ? static_cast<int>(*text_style.align_type) : 0);
    result =
        result * prime + (text_style.enable_text_bounds
                              ? static_cast<int>(*text_style.enable_text_bounds)
                              : 0);
    return result;
  }
};
}  // namespace std

#endif  // CLAY_UI_COMPONENT_TEXT_TEXT_STYLE_H_
