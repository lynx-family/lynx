// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_STYLE_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_STYLE_HARMONY_H_

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_text_typography.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_attributes.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/shadow_layer_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_gradient_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr const char* ELLIPSIS = u8"\u2026";

class ParagraphStyleHarmony {
 public:
  ParagraphStyleHarmony() : typo_style_(OH_Drawing_CreateTypographyStyle()) {}

  ~ParagraphStyleHarmony() {
    if (typo_style_ != nullptr) {
      OH_Drawing_DestroyTypographyStyle(typo_style_);
    }
  }

 public:
  OH_Drawing_TypographyStyle* GetRawStruct() const { return typo_style_; }

  void SetTextDirection(OH_Drawing_TextDirection dir) const {
    OH_Drawing_SetTypographyTextDirection(typo_style_, dir);
  }

  void SetTextAlign(OH_Drawing_TextAlign align) const {
    OH_Drawing_SetTypographyTextAlign(typo_style_, align);
  }

  void SetTextMaxLines(int32_t max_line) const {
    if (max_line <= 0) {
      max_line = INT32_MAX;
    }
    OH_Drawing_SetTypographyTextMaxLines(typo_style_, max_line);
  }

  void SetTextBreakStrategy(OH_Drawing_BreakStrategy strategy) const {
    OH_Drawing_SetTypographyTextBreakStrategy(typo_style_, strategy);
  }

  void SetTextWordBreakType(starlight::WordBreakType word_break) const {
    switch (word_break) {
      case starlight::WordBreakType::kBreakAll:
        OH_Drawing_SetTypographyTextWordBreakType(
            typo_style_, OH_Drawing_WordBreakType::WORD_BREAK_TYPE_BREAK_ALL);
        break;
      default:
        OH_Drawing_SetTypographyTextWordBreakType(
            typo_style_, OH_Drawing_WordBreakType::WORD_BREAK_TYPE_NORMAL);
        break;
    }
  }

  void SetTextEllipsisModal(OH_Drawing_EllipsisModal modal) const {
    OH_Drawing_SetTypographyTextEllipsisModal(typo_style_, modal);
  }

  void SetTextEllipsis(const char* ellipsis) const {
    OH_Drawing_SetTypographyTextEllipsis(typo_style_, ellipsis);
  }

  void SetLineSpacingScale(double scale) const {
    OH_Drawing_SetTypographyTextLineStyleSpacingScale(typo_style_, scale);
  }

  void SetTextUseLineStyle(bool b) const {
    OH_Drawing_SetTypographyTextUseLineStyle(typo_style_, b);
  }

  starlight::TextAlignType GetEffectiveAlignment() {
    OH_Drawing_TextAlign text_align =
        OH_Drawing_TypographyStyleGetEffectiveAlignment(typo_style_);
    OH_Drawing_TextDirection text_direction =
        OH_Drawing_TypographyGetTextDirection(typo_style_);
    switch (text_align) {
      case TEXT_ALIGN_LEFT:
        return starlight::TextAlignType::kLeft;
      case TEXT_ALIGN_RIGHT:
        return starlight::TextAlignType::kRight;
      case TEXT_ALIGN_CENTER:
        return starlight::TextAlignType::kCenter;
      case TEXT_ALIGN_JUSTIFY:
        return starlight::TextAlignType::kJustify;
      case TEXT_ALIGN_START:
        return text_direction == OH_Drawing_TextDirection::TEXT_DIRECTION_LTR
                   ? starlight::TextAlignType::kLeft
                   : starlight::TextAlignType::kRight;
      case TEXT_ALIGN_END:
        return text_direction == OH_Drawing_TextDirection::TEXT_DIRECTION_LTR
                   ? starlight::TextAlignType::kRight
                   : starlight::TextAlignType::kLeft;
    }
  }

  void SetTextAlignToParagraphStyle(TextAlign text_align) {
    if (text_align == starlight::TextAlignType::kLeft) {
      SetTextAlign(OH_Drawing_TextAlign::TEXT_ALIGN_LEFT);
    } else if (text_align == starlight::TextAlignType::kCenter) {
      SetTextAlign(OH_Drawing_TextAlign::TEXT_ALIGN_CENTER);
    } else if (text_align == starlight::TextAlignType::kRight) {
      SetTextAlign(OH_Drawing_TextAlign::TEXT_ALIGN_RIGHT);
    } else if (text_align == starlight::TextAlignType::kJustify) {
      SetTextAlign(OH_Drawing_TextAlign::TEXT_ALIGN_JUSTIFY);
    } else if (text_align == starlight::TextAlignType::kEnd) {
      SetTextAlign(OH_Drawing_TextAlign::TEXT_ALIGN_END);
    } else {
      SetTextAlign(OH_Drawing_TextAlign::TEXT_ALIGN_START);
    }
  }
  void SetDirectionToParagraphStyle(Direction direction) {
    if (direction == starlight::DirectionType::kLtr) {
      SetTextDirection(OH_Drawing_TextDirection::TEXT_DIRECTION_LTR);
    } else if (direction == starlight::DirectionType::kRtl ||
               direction == starlight::DirectionType::kLynxRtl) {
      SetTextDirection(OH_Drawing_TextDirection::TEXT_DIRECTION_RTL);
    }
  }

  void SetEllipsis(const char* ellipsis) {
    OH_Drawing_SetTypographyTextEllipsisModal(
        typo_style_, OH_Drawing_EllipsisModal::ELLIPSIS_MODAL_TAIL);
    OH_Drawing_SetTypographyTextEllipsis(typo_style_, ellipsis);
  }

  void SetWhiteSpace(starlight::WhiteSpaceType white_space) {
    white_space_ = white_space;
  }

  starlight::WhiteSpaceType GetWhiteSpace() { return white_space_; }

  void SetTextIndent(const std::optional<TextIndent>& indent) {
    indent_ = indent;
  }

  const std::optional<TextIndent>& GetTextIndent() { return indent_; }

 private:
  OH_Drawing_TypographyStyle* typo_style_;
  starlight::WhiteSpaceType white_space_{starlight::WhiteSpaceType::kNormal};
  std::optional<TextIndent> indent_;
};

class TextStyleHarmony {
 public:
  TextStyleHarmony() : text_style_(OH_Drawing_CreateTextStyle()) {}
  ~TextStyleHarmony() {
    if (text_style_ != nullptr) {
      OH_Drawing_DestroyTextStyle(text_style_);
    }
    if (foreground_pen_ != nullptr) {
      OH_Drawing_PenDestroy(foreground_pen_);
    }
    if (foreground_brush_ != nullptr) {
      OH_Drawing_BrushDestroy(foreground_brush_);
    }
  }

  void SetDefaultFontFamily(const std::string& default_font_family) {
    if (!default_font_family.empty()) {
      const char* default_font[] = {default_font_family.c_str()};
      OH_Drawing_SetTextStyleFontFamilies(text_style_, 1, default_font);
    }
  }

  OH_Drawing_TextStyle* GetRawStruct() const { return text_style_; }

  void SetColor(uint32_t color) {
    color_ = color;
    OH_Drawing_SetTextStyleColor(text_style_, color);
  }

  void SetFontSize(double size) {
    OH_Drawing_SetTextStyleFontSize(text_style_, size);
  }

  void SetFontWeight(OH_Drawing_FontWeight weight) const {
    OH_Drawing_SetTextStyleFontWeight(text_style_, weight);
  }

  void SetBaseLine(OH_Drawing_TextBaseline baseline) const {
    OH_Drawing_SetTextStyleBaseLine(text_style_, baseline);
  }

  void SetFontHeight(double height) const {
    OH_Drawing_SetTextStyleFontHeight(text_style_, height);
  }

  const std::vector<std::string>& GetFontFamilies() const {
    return font_families_;
  }

  void SetFontStyle(OH_Drawing_FontStyle style) const {
    OH_Drawing_SetTextStyleFontStyle(text_style_, style);
  }

  void SetLocale(const char* locale) const {
    OH_Drawing_SetTextStyleLocale(text_style_, locale);
  }

  void SetThicknessScale(double scale) const {
    OH_Drawing_SetTextStyleDecorationThicknessScale(text_style_, scale);
  }

  void SetLetterSpacing(double spacing) const {
    OH_Drawing_SetTextStyleLetterSpacing(text_style_, spacing);
  }

  void SetWordSpacing(double spacing) const {
    OH_Drawing_SetTextStyleWordSpacing(text_style_, spacing);
  }

  void SetHalfLeading(bool enable) const {
    OH_Drawing_SetTextStyleHalfLeading(text_style_, enable);
  }

  void SetEllipsis(const char* ellipsis) const {
    OH_Drawing_SetTextStyleEllipsis(text_style_, ellipsis);
  }

  void SetEllipsisModal(OH_Drawing_EllipsisModal modal) const {
    OH_Drawing_SetTextStyleEllipsisModal(text_style_, modal);
  }

  void SetTextHalfLeading(bool b) const {
    OH_Drawing_SetTextStyleHalfLeading(text_style_, b);
  }

  void SetFontFamiliesToStyle(const std::string& in_family) {
    font_families_.clear();
    base::SplitString(in_family, ',', font_families_);

    // trim '\'' and whitespace
    for (auto& item : font_families_) {
      while (item.front() == '\'' || item.front() == '\"' ||
             item.front() == ' ') {
        item.erase(0, 1);
      }
      while (item.back() == '\'' || item.back() == '\"' || item.back() == ' ') {
        item.pop_back();
      }
    }

    size_t size = font_families_.size();

    if (size <= 0) {
      OH_Drawing_SetTextStyleFontFamilies(text_style_, 0, nullptr);
      return;
    }

    std::unique_ptr<const char*[]> font_families =
        std::make_unique<const char*[]>(size);

    for (size_t i = 0; i < size; ++i) {
      font_families[i] = font_families_[i].c_str();
    }

    OH_Drawing_SetTextStyleFontFamilies(text_style_, static_cast<int>(size),
                                        font_families.get());
  }

  void SetFontWeightToStyle(FontWeight font_weight) {
    OH_Drawing_FontWeight style_font_weight = FONT_WEIGHT_400;
    switch (font_weight) {
      case FontWeight::kNormal:
      case FontWeight::k400: {
        style_font_weight = FONT_WEIGHT_400;
      } break;
      case FontWeight::kBold:
      case FontWeight::k700: {
        style_font_weight = FONT_WEIGHT_700;
      } break;
      case FontWeight::k100: {
        style_font_weight = FONT_WEIGHT_100;
      } break;
      case FontWeight::k200: {
        style_font_weight = FONT_WEIGHT_200;
      } break;
      case FontWeight::k300: {
        style_font_weight = FONT_WEIGHT_300;
      } break;
      case FontWeight::k500: {
        style_font_weight = FONT_WEIGHT_500;
      } break;
      case FontWeight::k600: {
        style_font_weight = FONT_WEIGHT_600;
      } break;
      case FontWeight::k800: {
        style_font_weight = FONT_WEIGHT_800;
      } break;
      case FontWeight::k900: {
        style_font_weight = FONT_WEIGHT_900;
      } break;
      default:
        break;
    }
    SetFontWeight(style_font_weight);
  }

  void SetFontStyleToStyle(FontStyle font_style) {
    OH_Drawing_FontStyle style_font_style = FONT_STYLE_NORMAL;
    switch (font_style) {
      case FontStyle::kNormal: {
        style_font_style = FONT_STYLE_NORMAL;
      } break;
      case FontStyle::kItalic: {
        style_font_style = FONT_STYLE_ITALIC;
      } break;
      case FontStyle::kOblique: {
        style_font_style = FONT_STYLE_OBLIQUE;
      } break;
      default:
        break;
    }
    SetFontStyle(style_font_style);
  }

  void SetTextDecorationStyle(int32_t style) {
    OH_Drawing_TextDecorationStyle decoration_style;
    switch (static_cast<starlight::TextDecorationType>(style)) {
      case starlight::TextDecorationType::kSolid:
        decoration_style = TEXT_DECORATION_STYLE_SOLID;
        break;
      case starlight::TextDecorationType::kDouble:
        decoration_style = TEXT_DECORATION_STYLE_DOUBLE;
        break;
      case starlight::TextDecorationType::kDotted:
        decoration_style = TEXT_DECORATION_STYLE_DOTTED;
        break;
      case starlight::TextDecorationType::kDashed:
        decoration_style = TEXT_DECORATION_STYLE_DASHED;
        break;
      case starlight::TextDecorationType::kWavy:
        decoration_style = TEXT_DECORATION_STYLE_WAVY;
        break;
      default:
        decoration_style = TEXT_DECORATION_STYLE_SOLID;
        break;
    }
    OH_Drawing_SetTextStyleDecorationStyle(text_style_, decoration_style);
  }

  void SetTextDecoration(int32_t line) {
    if (line ==
        static_cast<int32_t>(starlight::TextDecorationType::kUnderLine)) {
      OH_Drawing_SetTextStyleDecoration(
          text_style_, OH_Drawing_TextDecoration::TEXT_DECORATION_UNDERLINE);
    } else if (line == static_cast<int32_t>(
                           starlight::TextDecorationType::kLineThrough)) {
      OH_Drawing_SetTextStyleDecoration(
          text_style_, OH_Drawing_TextDecoration::TEXT_DECORATION_LINE_THROUGH);
    } else {
      OH_Drawing_SetTextStyleDecoration(
          text_style_, OH_Drawing_TextDecoration::TEXT_DECORATION_NONE);
    }
  }

  void SetTextDecorationColor(uint32_t color) {
    OH_Drawing_SetTextStyleDecorationColor(text_style_, color);
  }

  void SetStrokeWidth(float width) { stroke_width_ = width; }

  void SetStrokeColor(uint32_t color) { stroke_color_ = color; }

  void UpdateTextPaint(float width, float height, float scale_density);

  void SetShadowLayer(const std::optional<ShadowData>& shadow_data) {
    if (shadow_data.has_value()) {
      shadow_layer_ = std::make_unique<ShadowLayerHarmony>(*shadow_data);
    } else {
      shadow_layer_ = nullptr;
    }
  }

  BackgroundGradientLayer* GradientColor() const {
    return gradient_color_.get();
  }

  const fml::RefPtr<ShaderEffect>& GradientShaderEffect() const {
    return gradient_shader_effect_;
  }

  void SetGradientColor(std::unique_ptr<BackgroundGradientLayer> gradient);

 private:
  void EnsureForegroundPenAndBrush() {
    if (foreground_pen_ == nullptr) {
      foreground_pen_ = OH_Drawing_PenCreate();
      OH_Drawing_PenSetAntiAlias(foreground_pen_, true);
    } else {
      OH_Drawing_PenReset(foreground_pen_);
      OH_Drawing_PenSetAntiAlias(foreground_pen_, true);
    }
    if (foreground_brush_ == nullptr) {
      foreground_brush_ = OH_Drawing_BrushCreate();
      OH_Drawing_BrushSetAntiAlias(foreground_brush_, true);
    } else {
      OH_Drawing_BrushReset(foreground_brush_);
      OH_Drawing_BrushSetAntiAlias(foreground_brush_, true);
    }
  }

  OH_Drawing_TextStyle* text_style_;
  std::vector<std::string> font_families_;
  OH_Drawing_Pen* foreground_pen_{nullptr};
  OH_Drawing_Brush* foreground_brush_{nullptr};
  std::unique_ptr<ShadowLayerHarmony> shadow_layer_{nullptr};
  uint32_t color_{0};
  std::optional<uint32_t> stroke_color_;
  float stroke_width_{-1};
  std::unique_ptr<BackgroundGradientLayer> gradient_color_ = nullptr;
  fml::RefPtr<ShaderEffect> gradient_shader_effect_ = nullptr;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_STYLE_HARMONY_H_
