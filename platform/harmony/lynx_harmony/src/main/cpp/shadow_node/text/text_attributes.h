// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_ATTRIBUTES_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_ATTRIBUTES_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/hash_combine.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/shadow_layer_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

class BackgroundGradientLayer;

using FontWeight = starlight::FontWeightType;
using FontStyle = starlight::FontStyleType;
using TextAlign = starlight::TextAlignType;
using Direction = starlight::DirectionType;
using TextOverflow = starlight::TextOverflowType;
using WhiteSpaceType = starlight::WhiteSpaceType;
static constexpr int64_t kLineHeightNormal = 9223372036854775807l;

struct LynxVerticalAlignStyle {
  starlight::VerticalAlignType vertical_align_type{
      starlight::VerticalAlignType::kDefault};
  float baseline_offset{0.f};

  bool operator==(const LynxVerticalAlignStyle& rhs) const {
    return vertical_align_type == rhs.vertical_align_type &&
           baseline_offset == rhs.baseline_offset;
  }
};

struct TextIndent {
  float value{0};
  starlight::PlatformLengthUnit unit{starlight::PlatformLengthUnit::NUMBER};

  bool operator==(const TextIndent& rhs) const {
    return value == rhs.value && unit == rhs.unit;
  }

  float GetValue(float width) const {
    return unit == starlight::PlatformLengthUnit::NUMBER ? value
                                                         : value * width;
  }
};

struct TextProps {
  uint32_t color{0};
  double font_size{-1};
  double line_height{kLineHeightNormal};
  FontWeight font_weight{starlight::FontWeightType::kNormal};
  FontStyle font_style{starlight::FontStyleType::kNormal};
  double letter_spacing{0};
  std::string font_family;
  mutable size_t loading_font_sign{
      0};  // indicate font loading unique id: family+id
  LynxVerticalAlignStyle vertical_align_style_;
  int32_t max_line{0};
  TextAlign text_align{starlight::TextAlignType::kLeft};
  Direction direction{starlight::DirectionType::kNormal};
  TextOverflow text_overflow{starlight::TextOverflowType::kClip};
  WhiteSpaceType white_space{WhiteSpaceType::kNormal};
  int32_t text_decoration_line{0};
  int32_t text_decoration_style{0};
  uint32_t text_decoration_color{0};
  starlight::WordBreakType word_break{starlight::WordBreakType::kNormal};
  uint32_t stroke_color{0};
  float stroke_width{0};
  std::optional<TextIndent> text_indent;
  BackgroundGradientLayer* gradient_color{nullptr};
  std::optional<ShadowData> shadow_data;

  bool operator==(const TextProps& rhs) const {
    return std::tie(color, font_size, line_height, font_style, letter_spacing,
                    font_family, loading_font_sign, vertical_align_style_,
                    max_line, text_align, direction, text_overflow, white_space,
                    text_decoration_color, text_decoration_line,
                    text_decoration_style, word_break, stroke_color,
                    stroke_width, text_indent, gradient_color, shadow_data) ==
           std::tie(rhs.color, rhs.font_size, rhs.line_height, rhs.font_style,
                    rhs.letter_spacing, rhs.font_family, rhs.loading_font_sign,
                    rhs.vertical_align_style_, rhs.max_line, rhs.text_align,
                    rhs.direction, rhs.text_overflow, rhs.white_space,
                    rhs.text_decoration_color, rhs.text_decoration_line,
                    rhs.text_decoration_style, rhs.word_break, rhs.stroke_color,
                    rhs.stroke_width, rhs.text_indent, rhs.gradient_color,
                    rhs.shadow_data);
  }
};

struct TextFragment {
  std::optional<std::string> text_content;
  std::optional<std::string> image_src;
  std::optional<TextProps> text_props;

  bool operator==(const TextFragment& rhs) const {
    return std::tie(text_content, image_src, text_props) ==
           std::tie(rhs.text_content, rhs.image_src, rhs.text_props);
  }
};

using AttributedString = std::vector<TextFragment>;

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

namespace std {
template <>
struct hash<lynx::tasm::harmony::LynxVerticalAlignStyle> {
  size_t operator()(
      const lynx::tasm::harmony::LynxVerticalAlignStyle& vertical_align) const {
    auto seed = lynx::fml::HashCombine();
    lynx::fml::HashCombineSeed(seed, vertical_align.vertical_align_type,
                               vertical_align.baseline_offset);
    return seed;
  }
};

template <>
struct hash<lynx::tasm::harmony::TextIndent> {
  size_t operator()(const lynx::tasm::harmony::TextIndent& indent) const {
    auto seed = lynx::fml::HashCombine();
    lynx::fml::HashCombineSeed(seed, indent.value, indent.unit);
    return seed;
  }
};

template <>
struct hash<lynx::tasm::harmony::TextProps> {
  size_t operator()(const lynx::tasm::harmony::TextProps& text_props) const {
    auto seed = lynx::fml::HashCombine();
    lynx::fml::HashCombineSeed(
        seed, text_props.color, text_props.font_size, text_props.line_height,
        text_props.font_weight, text_props.font_style,
        text_props.letter_spacing, text_props.font_family,
        text_props.loading_font_sign, text_props.vertical_align_style_,
        text_props.max_line, text_props.text_align, text_props.direction,
        text_props.text_overflow, text_props.white_space,
        text_props.text_decoration_color, text_props.text_decoration_line,
        text_props.text_decoration_style, text_props.word_break,
        text_props.stroke_color, text_props.stroke_width,
        text_props.text_indent, text_props.gradient_color,
        text_props.shadow_data);
    return seed;
  }
};

template <>
struct hash<lynx::tasm::harmony::TextFragment> {
  size_t operator()(
      const lynx::tasm::harmony::TextFragment& text_fragment) const {
    auto seed = lynx::fml::HashCombine();

    lynx::fml::HashCombineSeed(seed, text_fragment.text_content.has_value(),
                               text_fragment.text_props);
    return seed;
  }
};

template <>
struct hash<lynx::tasm::harmony::AttributedString> {
  std::size_t operator()(
      const lynx::tasm::harmony::AttributedString& attrStr) const {
    auto seed = lynx::fml::HashCombine();
    for (const auto& frag : attrStr) {
      lynx::fml::HashCombineSeed(seed, frag);
    }
    return seed;
  }
};

template <>
struct hash<vector<string>> {
  size_t operator()(const vector<string>& vec) const {
    auto seed = lynx::fml::HashCombine();
    for (const auto& str : vec) {
      lynx::fml::HashCombineSeed(seed, str);
    }
    return seed;
  }
};

}  // namespace std
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_ATTRIBUTES_H_
