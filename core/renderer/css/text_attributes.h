// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_CSS_TEXT_ATTRIBUTES_H_
#define CORE_RENDERER_CSS_TEXT_ATTRIBUTES_H_

#include <optional>
#include <vector>

#include "base/include/flex_optional.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/style/color.h"
#include "core/style/default_computed_style.h"
#include "core/style/shadow_data.h"

namespace lynx {
namespace tasm {
class CssMeasureContext;
struct CSSParserConfigs;
}  // namespace tasm

namespace starlight {

enum class TextPropertyID : uint8_t {
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

struct AutoFontSizeLineRange {
  int32_t start_line;
  int32_t end_line;
  float min_size;
  float max_size;

  bool operator==(const AutoFontSizeLineRange& rhs) const {
    return start_line == rhs.start_line && end_line == rhs.end_line &&
           min_size == rhs.min_size && max_size == rhs.max_size;
  }
};

class TextAttributes {
 public:
  TextAttributes(float default_font_size) : font_size{default_font_size} {}

  base::flex_optional<base::InlineVector<ShadowData, 1>> text_shadow;
  base::flex_optional<base::InlineVector<float, 6>> auto_font_size_preset_sizes;
  base::flex_optional<std::vector<AutoFontSizeLineRange>>
      auto_font_size_line_ranges;
  NLength text_indent{DefaultLayoutStyle::SL_DEFAULT_ZEROLENGTH()};
  base::String font_family;

  bool clone_text_gradient = false;
  base::flex_optional<lepus::Value> text_gradient;

  float vertical_align_length{DefaultComputedStyle::DEFAULT_FLOAT};
  float font_size;
  float computed_line_height{DefaultComputedStyle::DEFAULT_LINE_HEIGHT};
  float line_height_factor{DefaultComputedStyle::DEFAULT_LINE_HEIGHT_FACTOR};
  float letter_spacing{DefaultComputedStyle::DEFAULT_LETTER_SPACING};
  float line_spacing{DefaultComputedStyle::DEFAULT_LINE_SPACING};
  float text_stroke_width{DefaultComputedStyle::DEFAULT_FLOAT};
  float auto_font_size_min_size{DefaultComputedStyle::DEFAULT_FLOAT};
  float auto_font_size_max_size{DefaultComputedStyle::DEFAULT_FLOAT};
  float auto_font_size_step_granularity{
      DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE_STEP_GRANULARITY};
  base::flex_optional<uint32_t> text_stroke_color;
  base::flex_optional<uint32_t> color;
  base::flex_optional<uint32_t> decoration_color;
  base::flex_optional<uint32_t> text_decoration_color;
  base::flex_optional<float> text_decoration_thickness;
  base::flex_optional<float> text_decoration_width;
  base::flex_optional<float> text_decoration_gap;
  uint8_t text_decoration_style{
      DefaultComputedStyle::DEFAULT_TEXT_DECORATION_STYLE};
  // TODO(linxs) this type has changed.
  starlight::WhiteSpaceType white_space{
      DefaultComputedStyle::DEFAULT_WHITE_SPACE};
  starlight::TextOverflowType text_overflow{
      DefaultComputedStyle::DEFAULT_TEXT_OVERFLOW};
  starlight::FontWeightType font_weight{
      DefaultComputedStyle::DEFAULT_FONT_WEIGHT};
  starlight::FontStyleType font_style{DefaultComputedStyle::DEFAULT_FONT_STYLE};
  starlight::VerticalAlignType vertical_align{
      DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN};
  starlight::TextAlignType text_align{DefaultComputedStyle::DEFAULT_TEXT_ALIGN};
  starlight::WordBreakType word_break{DefaultComputedStyle::DEFAULT_WORD_BREAK};
  starlight::HyphensType hyphens{DefaultComputedStyle::DEFAULT_HYPHENS};
  bool enable_font_scaling{DefaultComputedStyle::DEFAULT_BOOLEAN};
  bool underline_decoration{DefaultComputedStyle::DEFAULT_BOOLEAN};
  bool line_through_decoration{DefaultComputedStyle::DEFAULT_BOOLEAN};
  bool is_auto_font_size{DefaultComputedStyle::DEFAULT_AUTO_FONT_SIZE};
  fml::RefPtr<lepus::CArray> font_variation_settings{nullptr};
  fml::RefPtr<lepus::CArray> font_feature_settings{nullptr};
  starlight::FontOpticalSizingType font_optical_sizing{
      DefaultComputedStyle::DEFAULT_FONT_OPTICAL_SIZING};

  void Reset() {}

  bool operator==(const TextAttributes& rhs) const {
    return font_size == rhs.font_size && color == rhs.color &&
           text_gradient == rhs.text_gradient &&
           decoration_color == rhs.decoration_color &&
           white_space == rhs.white_space &&
           text_overflow == rhs.text_overflow &&
           font_weight == rhs.font_weight && font_style == rhs.font_style &&
           font_family == rhs.font_family &&
           vertical_align_length == rhs.vertical_align_length &&
           computed_line_height == rhs.computed_line_height &&
           line_height_factor == rhs.line_height_factor &&
           letter_spacing == rhs.letter_spacing &&
           line_spacing == rhs.line_spacing &&
           text_stroke_width == rhs.text_stroke_width &&
           text_decoration_thickness == rhs.text_decoration_thickness &&
           text_decoration_width == rhs.text_decoration_width &&
           text_decoration_gap == rhs.text_decoration_gap &&
           auto_font_size_min_size == rhs.auto_font_size_min_size &&
           auto_font_size_max_size == rhs.auto_font_size_max_size &&
           auto_font_size_step_granularity ==
               rhs.auto_font_size_step_granularity &&
           text_stroke_color == rhs.text_stroke_color &&
           text_shadow == rhs.text_shadow && text_align == rhs.text_align &&
           word_break == rhs.word_break && hyphens == rhs.hyphens &&
           enable_font_scaling == rhs.enable_font_scaling &&
           underline_decoration == rhs.underline_decoration &&
           line_through_decoration == rhs.line_through_decoration &&
           is_auto_font_size == rhs.is_auto_font_size &&
           text_decoration_color == rhs.text_decoration_color &&
           text_decoration_style == rhs.text_decoration_style &&
           text_indent == rhs.text_indent &&
           auto_font_size_preset_sizes == rhs.auto_font_size_preset_sizes &&
           auto_font_size_line_ranges == rhs.auto_font_size_line_ranges &&
           vertical_align == rhs.vertical_align &&
           font_optical_sizing == rhs.font_optical_sizing &&
           RefPtrEqual(font_variation_settings, rhs.font_variation_settings) &&
           RefPtrEqual(font_feature_settings, rhs.font_feature_settings);
  }

  static bool RefPtrEqual(const fml::RefPtr<lepus::CArray>& lhs,
                          const fml::RefPtr<lepus::CArray>& rhs) {
    if (lhs == rhs) return true;
    if (!lhs || !rhs) return false;
    return *lhs == *rhs;
  }

  bool operator!=(const TextAttributes& rhs) const { return !(*this == rhs); }

  void Apply(const TextAttributes& rhs);

  void ProcessRadialGradientIfNeeded(
      const tasm::CssMeasureContext& length_context,
      const tasm::CSSParserConfigs& parser_configs);
};

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_CSS_TEXT_ATTRIBUTES_H_
