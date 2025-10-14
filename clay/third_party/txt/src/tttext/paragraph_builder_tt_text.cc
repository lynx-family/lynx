// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "paragraph_builder_tt_text.h"

#include <textra/layout_region.h>
#include "clay/fml/logging.h"
#include "tttext/tttext_headers.h"

namespace txt {
ParagraphBuilderTTText::ParagraphBuilderTTText(
    const ParagraphStyle& style,
    const std::shared_ptr<FontCollection>& font_collection)
    : font_collection_(font_collection) {
  ToTTParaStyle(style);
  ParagraphBuilderTTText::PushStyle(style.GetTextStyle());
}
ParagraphBuilderTTText::~ParagraphBuilderTTText() = default;
void ParagraphBuilderTTText::PushStyle(const TextStyle& style) {
  text_style_stack_.push_back(style);
  run_style_stack_.push_back(ToTTStyle(style));
}
void ParagraphBuilderTTText::Pop() {
  text_style_stack_.pop_back();
  run_style_stack_.pop_back();
}
const TextStyle& ParagraphBuilderTTText::PeekStyle() {
  return text_style_stack_.back();
}
void ParagraphBuilderTTText::AddText(const std::u16string& text) {
  CreateParagraph();
  paragraph_->AddTextRun(run_style_stack_.back(), text);
}
void ParagraphBuilderTTText::AddPlaceholder(PlaceholderRun& span) {
  CreateParagraph();
  paragraph_->AddPlaceholder(run_style_stack_.back(), span, false);
}
std::unique_ptr<Paragraph> ParagraphBuilderTTText::Build() {
  CreateParagraph();
  return std::move(paragraph_);
}
void ParagraphBuilderTTText::AddText(const char* text, size_t len) {
  CreateParagraph();
  paragraph_->AddTextRun(run_style_stack_.back(), std::string(text, len));
}

tttext::Style ParagraphBuilderTTText::ToTTStyle(const TextStyle& text_style) {
  tttext::Style style;
  tttext::Weight weight;
  switch (text_style.font_weight) {
    case FontWeight::w100:
      weight = tttext::FontStyle::kThin_Weight;
      break;
    case FontWeight::w200:
      weight = tttext::FontStyle::kExtraLight_Weight;
      break;
    case FontWeight::w300:
      weight = tttext::FontStyle::kLight_Weight;
      break;
    case FontWeight::w400:
      weight = tttext::FontStyle::kNormal_Weight;
      break;
    case FontWeight::w500:
      weight = tttext::FontStyle::kMedium_Weight;
      break;
    case FontWeight::w600:
      weight = tttext::FontStyle::kSemiBold_Weight;
      break;
    case FontWeight::w700:
      weight = tttext::FontStyle::kBold_Weight;
      break;
    case FontWeight::w800:
      weight = tttext::FontStyle::kExtraBold_Weight;
      break;
    case FontWeight::w900:
      weight = tttext::FontStyle::kBlack_Weight;
      break;
  }
  tttext::FontDescriptor fd{{text_style.font_families},
                            {weight, tttext::FontStyle::kNormal_Width,
                             tttext::FontStyle::kUpright_Slant},
                            0};
  style.SetTextSize(text_style.font_size);
  style.SetFontDescriptor(fd);
  style.SetForegroundColor(tttext::TTColor(text_style.color));
#if defined(ENABLE_SKITY)
  if (text_style.has_foreground) {
    style.SetForegroundColor(tttext::TTColor(text_style.foreground.GetColor()));
  }
  if (text_style.has_background) {
    style.SetBackgroundColor(tttext::TTColor(text_style.background.GetColor()));
  }
#else
  if (text_style.has_foreground) {
    style.SetForegroundColor(tttext::TTColor(text_style.foreground.getColor()));
  }
  if (text_style.has_background) {
    style.SetBackgroundColor(tttext::TTColor(text_style.background.getColor()));
  }
#endif  // ENABLE_SKITY
  if (text_style.font_style != FontStyle::normal) {
    style.SetItalic(true);
  }
  style.SetDecorationType(
      static_cast<tttext::DecorationType>(text_style.decoration));
  style.SetDecorationColor(tttext::TTColor(text_style.decoration_color
                                               ? text_style.decoration_color
                                               : text_style.color));
  style.SetDecorationStyle(
      static_cast<tttext::LineType>(text_style.decoration_style));
  style.SetDecorationThicknessMultiplier(
      text_style.decoration_thickness_multiplier);
  style.SetWordSpacing(text_style.word_spacing);
  style.SetLetterSpacing(text_style.letter_spacing);
  style.SetBaselineOffset(text_style.text_baseline_shift);
  switch (text_style.word_break) {
    case kNormal:
      style.SetWordBreak(tttext::WordBreakType::kNormal);
      break;
    case kBreakAll:
      style.SetWordBreak(tttext::WordBreakType::kBreakAll);
      break;
    case kKeepAll:
      style.SetWordBreak(tttext::WordBreakType::kKeepAll);
      break;
  }

  std::vector<tttext::TextShadow> shadow_list;
  for (const txt::TextShadow& txt_shadow : text_style.text_shadows) {
    tttext::TextShadow shadow;
    shadow.offset_[0] = txt_shadow.offset.x;
    shadow.offset_[1] = txt_shadow.offset.y;
    shadow.blur_radius_ = txt_shadow.blur_sigma;
    shadow.color_ = txt_shadow.color;
    shadow_list.emplace_back(shadow);
  }
  style.SetTextShadowList(shadow_list);
#if defined(CLAY_ENABLE_TTTEXT)
  style.SetVerticalAlignment(text_style.align_type);
#endif  // CLAY_ENABLE_TTTEXT
  return style;
}
void ParagraphBuilderTTText::ToTTParaStyle(const ParagraphStyle& para_style) {
  auto& tt_para_style = paragraph_style_;
  auto text_style = para_style.GetTextStyle();
  switch (para_style.text_align) {
    case TextAlign::left:
      tt_para_style.SetHorizontalAlign(
          tttext::ParagraphHorizontalAlignment::kLeft);
      break;
    case TextAlign::right:
      tt_para_style.SetHorizontalAlign(
          tttext::ParagraphHorizontalAlignment::kRight);
      break;
    case TextAlign::center:
      tt_para_style.SetHorizontalAlign(
          tttext::ParagraphHorizontalAlignment::kCenter);
      break;
    case TextAlign::justify:
      tt_para_style.SetHorizontalAlign(
          tttext::ParagraphHorizontalAlignment::kJustify);
      break;
    case TextAlign::start:
      if (para_style.text_direction == TextDirection::ltr) {
        tt_para_style.SetHorizontalAlign(
            tttext::ParagraphHorizontalAlignment::kLeft);
      } else {
        tt_para_style.SetHorizontalAlign(
            tttext::ParagraphHorizontalAlignment::kRight);
      }
      break;
    case TextAlign::end:
      if (para_style.text_direction == TextDirection::rtl) {
        tt_para_style.SetHorizontalAlign(
            tttext::ParagraphHorizontalAlignment::kLeft);
      } else {
        tt_para_style.SetHorizontalAlign(
            tttext::ParagraphHorizontalAlignment::kRight);
      }
      break;
  }
  tt_para_style.SetMaxLines(para_style.max_lines);
  tt_para_style.SetEllipsis(para_style.ellipsis);
  tt_para_style.SetWriteDirection(para_style.text_direction ==
                                          TextDirection::ltr
                                      ? tttext::WriteDirection::kLTR
                                      : tttext::WriteDirection::kRTL);
  tt_para_style.SetLineHeightOverride(para_style.has_height_override);
  tt_para_style.SetLineHeightInPercent(para_style.height);
  tt_para_style.EnableTextBounds(para_style.enable_text_bounds);
  tt_para_style.SetDefaultStyle(ToTTStyle(text_style));
}
void ParagraphBuilderTTText::CreateParagraph() {
  if (paragraph_ == nullptr) {
    paragraph_ =
        std::make_unique<ParagraphTTText>(font_collection_, paragraph_style_);
  }
}
}  // namespace txt
