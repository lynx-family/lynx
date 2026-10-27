// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/text_paragraph_builder.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/gfx/graphics_context.h"
#include "clay/third_party/txt/src/txt/paragraph_style.h"
#include "clay/third_party/txt/src/txt/text_style.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/shadow/vertical_align_style.h"

namespace clay {
namespace {

static const std::u16string kDefaultEllipsis = u"\u2026";

#if defined(CLAY_ENABLE_TTTEXT)
ttoffice::tttext::CharacterVerticalAlignment ToTTTextAlign(
    VerticalAlignType type) {
  if (type == VerticalAlignType::kVerticalAlignTop) {
    return ttoffice::tttext::CharacterVerticalAlignment::kTop;
  } else if (type == VerticalAlignType::kVerticalAlignCenter) {
    return ttoffice::tttext::CharacterVerticalAlignment::kMiddle;
  } else if (type == VerticalAlignType::kVerticalAlignBottom) {
    return ttoffice::tttext::CharacterVerticalAlignment::kBottom;
  }
  return ttoffice::tttext::CharacterVerticalAlignment::kBaseLine;
}
#endif

txt::FontWeight ToTxtFontWeight(FontWeight font_weight) {
  static const std::unordered_map<FontWeight, txt::FontWeight> kMapping = {
      {FontWeight::kNormal, txt::FontWeight::w400},
      {FontWeight::kBold, txt::FontWeight::w700},
      {FontWeight::k100, txt::FontWeight::w100},  // Thin
      {FontWeight::k200, txt::FontWeight::w200},  // Extra-Light
      {FontWeight::k300, txt::FontWeight::w300},  // Light
      {FontWeight::k400, txt::FontWeight::w400},  // Normal/Regular
      {FontWeight::k500, txt::FontWeight::w500},  // Medium
      {FontWeight::k600, txt::FontWeight::w600},  // Semi-bold
      {FontWeight::k700, txt::FontWeight::w700},  // Bold
      {FontWeight::k800, txt::FontWeight::w800},  // Extra-Bold
      {FontWeight::k900, txt::FontWeight::w900},  // Black
  };
  return kMapping.at(font_weight);
}

txt::FontStyle ToTxtFontStyle(FontStyle font_style) {
  if (font_style == FontStyle::kNormal) {
    return txt::FontStyle::normal;
  }
  if (font_style == FontStyle::kItalic) {
    return txt::FontStyle::italic;
  }
  return txt::FontStyle::oblique;
}

txt::TextAlign ToTxtAlign(TextAlignment align) {
  if (align == TextAlignment::kLeft) {
    return txt::TextAlign::left;
  } else if (align == TextAlignment::kCenter) {
    return txt::TextAlign::center;
  } else if (align == TextAlignment::kRight) {
    return txt::TextAlign::right;
  } else if (align == TextAlignment::kStart) {
    return txt::TextAlign::start;
  } else if (align == TextAlignment::kEnd) {
    return txt::TextAlign::end;
  } else if (align == TextAlignment::kJustify) {
    return txt::TextAlign::justify;
  }

  FML_DCHECK(false) << "unsupported TextAlignment: " << static_cast<int>(align);
  return txt::TextAlign::start;
}

txt::TextDirection ToTxtDirection(TextDirection direction) {
  if (direction == TextDirection::kLtr) {
    return txt::TextDirection::ltr;
  } else if (direction == TextDirection::kRtl) {
    return txt::TextDirection::rtl;
  }
  FML_DCHECK(false);
  return txt::TextDirection::ltr;
}

txt::TextDecorationStyle ToTxtDecorationStyle(TextDecorationStyle style) {
  static const std::unordered_map<TextDecorationStyle, txt::TextDecorationStyle>
      kRDToTxt{
          {TextDecorationStyle::kSolid, txt::TextDecorationStyle::kSolid},
          {TextDecorationStyle::kDouble, txt::TextDecorationStyle::kDouble},
          {TextDecorationStyle::kDotted, txt::TextDecorationStyle::kDotted},
          {TextDecorationStyle::kDashed, txt::TextDecorationStyle::kDashed},
          {TextDecorationStyle::kWavy, txt::TextDecorationStyle::kWavy},
      };

  const auto itr = kRDToTxt.find(style);
  FML_DCHECK(itr != kRDToTxt.end());
  if (itr != kRDToTxt.end()) {
    return itr->second;
  }

  FML_UNREACHABLE();
}

txt::TextDecoration ToTxtDecorationLine(uint8_t line) {
  int res = txt::TextDecoration::kNone;
  if (line & kTextDecorationLineUnderline) {
    res |= txt::TextDecoration::kUnderline;
  }
  if (line & kTextDecorationLineOverline) {
    res |= txt::TextDecoration::kOverline;
  }
  if (line & kTextDecorationLineLineThrough) {
    res |= txt::TextDecoration::kLineThrough;
  }
  return static_cast<txt::TextDecoration>(res);
}

txt::TextShadow ToTextShadow(const Shadow& shadow) {
  return txt::TextShadow(
      shadow.color, {shadow.offset_x, shadow.offset_y},
      GraphicsContext::ConvertRadiusToSigma(shadow.blur_radius));
}

void ApplyParagraphStyle(const TextStyle& clay_style,
                         txt::ParagraphStyle& txt_style) {
  if (clay_style.font_weight) {
    txt_style.font_weight = ToTxtFontWeight(clay_style.font_weight.value());
  }

  if (clay_style.font_style) {
    txt_style.font_style = ToTxtFontStyle(clay_style.font_style.value());
  }

  if (clay_style.font_size) {
    txt_style.font_size = clay_style.font_size.value();
  }

  if (clay_style.text_align) {
    txt_style.text_align = ToTxtAlign(clay_style.text_align.value());
  }

  if (clay_style.text_direction) {
    txt_style.text_direction =
        ToTxtDirection(clay_style.text_direction.value());
  }

  if (clay_style.max_lines) {
    txt_style.max_lines = static_cast<size_t>(*clay_style.max_lines);

    if (clay_style.overflow == TextOverflow::kEllipsis) {
      if (clay_style.ellipsis) {
        txt_style.ellipsis = *clay_style.ellipsis;
      } else {
        txt_style.ellipsis = kDefaultEllipsis;
      }
    }
  }

  if (clay_style.line_height) {
    txt_style.height = clay_style.line_height.value();
    txt_style.has_height_override = true;
    // TODO(WeiGuoliang): lxblxb kEvenLeading not found
    txt_style.text_height_behavior = txt::TextHeightBehavior::kAll;
  }

  if (clay_style.keep_trailing_spaces) {
    txt_style.keep_trailing_spaces = clay_style.keep_trailing_spaces.value();
  }

  if (clay_style.strut_enabled) {
    txt_style.strut_enabled = clay_style.strut_enabled.value();

    if (clay_style.strut_font_size) {
      txt_style.strut_font_size = clay_style.strut_font_size.value();
    }

    if (clay_style.strut_font_style) {
      txt_style.strut_font_style =
          ToTxtFontStyle(clay_style.strut_font_style.value());
    }

    if (clay_style.strut_font_weight) {
      txt_style.strut_font_weight =
          ToTxtFontWeight(clay_style.strut_font_weight.value());
    }

    if (clay_style.strut_height) {
      txt_style.strut_height = clay_style.strut_height.value();
    }

    txt_style.strut_font_families = clay_style.strut_font_families;
    txt_style.strut_has_height_override = true;
    txt_style.strut_half_leading = true;
    txt_style.force_strut_height = true;
  }

#if defined(CLAY_ENABLE_TTTEXT)
  if (clay_style.enable_text_bounds) {
    txt_style.enable_text_bounds = clay_style.enable_text_bounds.value();
  }
#endif
}

void ApplyTextStyle(const TextStyle& clay_style, txt::TextStyle& txt_style) {
  if (clay_style.text_color && !clay_style.text_gradient) {
    txt_style.has_foreground = false;
    txt_style.color = ToSk(clay_style.text_color.value());
  }

  if (clay_style.foreground_id) {
    // This attribute if for gradient and text-stroke.
    txt_style.has_foreground_id = true;
    txt_style.foreground_id = clay_style.foreground_id.value();
  }

  if (clay_style.background_color) {
    PAINT_SET_COLOR(txt_style.background,
                    ToSk(clay_style.background_color.value()));
    txt_style.has_background = true;
  }

  if (clay_style.font_weight) {
    txt_style.font_weight = ToTxtFontWeight(clay_style.font_weight.value());
  }

  if (clay_style.font_style) {
    txt_style.font_style = ToTxtFontStyle(clay_style.font_style.value());
  }

  if (clay_style.font_size) {
    txt_style.font_size = clay_style.font_size.value();
  }

  if (clay_style.baseline_shift) {
    txt_style.text_baseline_shift = clay_style.baseline_shift.value();
  }

  if (clay_style.half_leading) {
    txt_style.half_leading = clay_style.half_leading.value();
  }

  if (clay_style.letter_spacing) {
    txt_style.letter_spacing = clay_style.letter_spacing.value();
  }

  if (clay_style.line_height) {
    txt_style.height = clay_style.line_height.value();
    txt_style.has_height_override = true;
  }

  if (clay_style.line_spacing) {
    txt_style.line_spacing = clay_style.line_spacing.value();
  }

  if (clay_style.text_decoration) {
    txt_style.decoration_style =
        ToTxtDecorationStyle(clay_style.text_decoration->style);
    txt_style.decoration =
        ToTxtDecorationLine(clay_style.text_decoration->line);
    txt_style.decoration_color = clay_style.text_decoration->color.has_value()
                                     ? clay_style.text_decoration->color.value()
                                     : txt_style.color;
  }

  if (clay_style.text_shadows) {
    txt_style.text_shadows.clear();
    for (const Shadow& shadow : clay_style.text_shadows.value()) {
      txt_style.text_shadows.emplace_back(ToTextShadow(shadow));
    }
  }
  if (clay_style.word_break) {
    switch (clay_style.word_break.value()) {
      case kNormal:
        txt_style.word_break = txt::kNormal;
        break;
      case kBreakAll:
        txt_style.word_break = txt::kBreakAll;
        break;
      case kKeepAll:
        txt_style.word_break = txt::kKeepAll;
        break;
    }
  }

  if (clay_style.font_family) {
    txt_style.font_families.insert(txt_style.font_families.begin(),
                                   clay_style.font_family.value());
  }

#if defined(CLAY_ENABLE_TTTEXT)
  if (clay_style.align_type) {
    txt_style.align_type = ToTTTextAlign(clay_style.align_type.value());
    txt_style.enable_text_bounds_ = true;
  }
#endif
}

}  // namespace

std::unique_ptr<txt::Paragraph> Build(
    std::unique_ptr<TextParagraphBuilder> builder) {
  return std::unique_ptr<txt::Paragraph>(
      static_cast<txt::Paragraph*>(builder->builder_->Build().release()));
}

TextParagraphBuilder::TextParagraphBuilder(
    bool use_skia, const std::optional<TextStyle>& paragraph_style) {
  txt::ParagraphStyle style;

  if (paragraph_style) {
    ApplyParagraphStyle(paragraph_style.value(), style);
  }

#if defined(CLAY_ENABLE_SKSHAPER)
  builder_ = txt::ParagraphBuilder::CreateSkiaBuilder(
      style, Isolate::Instance().GetTxtFontCollection());
#endif
#if defined(CLAY_ENABLE_MINIKIN)
  builder_ = txt::ParagraphBuilder::CreateTxtBuilder(
      style, Isolate::Instance().GetTxtFontCollection());
#endif  // CLAY_ENABLE_MINIKIN
#if defined(CLAY_ENABLE_TTTEXT)
  builder_ = txt::ParagraphBuilder::CreateTTTextBuilder(
      style, Isolate::Instance().GetTxtFontCollection());
#endif
}

TextParagraphBuilder::~TextParagraphBuilder() = default;

void TextParagraphBuilder::PushStyle(const TextStyle& style) {
  txt::TextStyle txt_style = builder_->PeekStyle();
  FML_DCHECK(style.font_size.has_value());
  ApplyTextStyle(style, txt_style);
  builder_->PushStyle(txt_style);
}

void TextParagraphBuilder::Pop() { builder_->Pop(); }

void TextParagraphBuilder::AddText(const std::u16string& text) {
  builder_->AddText(text);
}

void TextParagraphBuilder::AddPlaceholder(txt::PlaceholderRun& placeholder) {
  builder_->AddPlaceholder(placeholder);
}

}  // namespace clay
