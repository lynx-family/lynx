// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/harmony/text_measurer_harmony.h"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/string/unicode_decode_utils.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/style/color.h"
#include "core/style/default_computed_style.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/font/font_face_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/font_collection_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"

namespace lynx {
namespace tasm {

namespace {

constexpr float kTextMaxLayoutWidth = std::numeric_limits<int16_t>::max();

base::UnicodeDecodeProperty DecodePropertyForTextElement(TextElement* element) {
  if (element == nullptr || element->computed_css_style() == nullptr) {
    return base::UnicodeDecodeProperty::kDefault;
  }
  const auto& attributes = element->computed_css_style()->GetTextAttributes();
  if (!attributes.has_value()) {
    return base::UnicodeDecodeProperty::kDefault;
  }
  switch (attributes->word_break) {
    case starlight::WordBreakType::kBreakAll:
      return base::UnicodeDecodeProperty::kInsertZeroWidthChar;
    case starlight::WordBreakType::kKeepAll:
      return base::UnicodeDecodeProperty::kCjkInsertWordJoiner;
    default:
      return base::UnicodeDecodeProperty::kDefault;
  }
}

void AppendDecodedText(harmony::ParagraphBuilderHarmony& builder,
                       const base::String& text,
                       base::UnicodeDecodeProperty decode_property) {
  if (text.empty()) {
    return;
  }
  const std::string decoded = base::UnicodeDecodeUtils::Decode(
      std::string_view(text.c_str(), text.length()), decode_property);
  builder.AddText(decoded.c_str());
}

void ApplyTextStyle(TextElement* element, float density,
                    harmony::LynxContext* context,
                    harmony::TextStyleHarmony& text_style) {
  if (element == nullptr || element->computed_css_style() == nullptr) {
    return;
  }

  auto* style = element->computed_css_style();
  const auto& attributes = style->GetTextAttributes();
  const auto font_manager =
      context != nullptr ? context->GetFontFaceManager() : nullptr;
  if (font_manager != nullptr) {
    text_style.SetDefaultFontFamily(font_manager->GetDefaultFontFamily());
  }

  text_style.SetFontSize(style->GetFontSize() * density);
  text_style.SetHalfLeading(true);
  if (!attributes.has_value()) {
    text_style.SetColor(starlight::DefaultColor::DEFAULT_TEXT_COLOR);
    text_style.UpdateTextPaint(0.f, 0.f, density);
    return;
  }

  text_style.SetColor(attributes->color.has_value()
                          ? *attributes->color
                          : starlight::DefaultColor::DEFAULT_TEXT_COLOR);
  text_style.SetFontWeightToStyle(attributes->font_weight);
  text_style.SetFontStyleToStyle(attributes->font_style);
  if (base::FloatsNotEqual(
          attributes->letter_spacing,
          starlight::DefaultComputedStyle::DEFAULT_LETTER_SPACING)) {
    text_style.SetLetterSpacing(attributes->letter_spacing * density);
  }

  if (attributes->computed_line_height !=
          starlight::DefaultComputedStyle::DEFAULT_LINE_HEIGHT &&
      style->GetFontSize() > 0.f) {
    text_style.SetFontHeight(attributes->computed_line_height /
                             style->GetFontSize());
  }

  if (!attributes->font_family.empty() && font_manager != nullptr) {
    text_style.SetCustomFontFamilyVector(
        font_manager->GetCustomFamiliesFromRawString(
            attributes->font_family.str()));
  }

  if (attributes->underline_decoration) {
    text_style.SetTextDecoration(
        static_cast<int32_t>(starlight::TextDecorationType::kUnderLine));
  } else if (attributes->line_through_decoration) {
    text_style.SetTextDecoration(
        static_cast<int32_t>(starlight::TextDecorationType::kLineThrough));
  }
  if (attributes->text_decoration_color.has_value()) {
    text_style.SetTextDecorationColor(*attributes->text_decoration_color);
  }
  text_style.SetTextDecorationStyle(attributes->text_decoration_style);
  text_style.UpdateTextPaint(0.f, 0.f, density);
}

void ApplyParagraphStyle(TextElement* element, float density,
                         harmony::ParagraphStyleHarmony& paragraph_style) {
  if (element == nullptr || element->computed_css_style() == nullptr) {
    return;
  }

  auto* style = element->computed_css_style();
  const auto& attributes = style->GetTextAttributes();
  const auto direction = style->GetDirection();
  paragraph_style.SetDirectionToParagraphStyle(
      direction == starlight::DirectionType::kRtl ||
              direction == starlight::DirectionType::kLynxRtl
          ? starlight::DirectionType::kRtl
          : starlight::DirectionType::kLtr);

  if (!attributes.has_value()) {
    return;
  }

  paragraph_style.SetTextAlignToParagraphStyle(attributes->text_align);
  paragraph_style.SetWhiteSpace(attributes->white_space);
  paragraph_style.SetTextWordBreakType(attributes->word_break);

  int32_t max_lines = starlight::DefaultComputedStyle::DEFAULT_TEXT_MAX_LINE;
  if (const auto* text_props = element->text_props();
      text_props != nullptr && text_props->text_max_line.has_value()) {
    max_lines = *text_props->text_max_line;
  }
  if (attributes->white_space == starlight::WhiteSpaceType::kNowrap &&
      attributes->text_overflow == starlight::TextOverflowType::kEllipsis) {
    max_lines = 1;
  }
  paragraph_style.SetTextMaxLines(max_lines);
  if (attributes->text_overflow == starlight::TextOverflowType::kEllipsis) {
    paragraph_style.SetEllipsis(harmony::ELLIPSIS);
  }

  const auto& indent = attributes->text_indent;
  if (indent.IsUnit() && indent.GetRawValue() != 0.f) {
    harmony::TextIndent text_indent;
    text_indent.value = indent.GetRawValue() * density;
    text_indent.unit = starlight::PlatformLengthUnit::NUMBER;
    paragraph_style.SetTextIndent(text_indent);
  } else if (indent.IsPercent() && indent.GetRawValue() != 0.f) {
    harmony::TextIndent text_indent;
    text_indent.value = indent.GetRawValue() / 100.f;
    text_indent.unit = starlight::PlatformLengthUnit::PERCENTAGE;
    paragraph_style.SetTextIndent(text_indent);
  }
}

void AppendTextSubtree(harmony::ParagraphBuilderHarmony& builder,
                       TextElement* element, float density,
                       harmony::LynxContext* context) {
  harmony::TextStyleHarmony text_style;
  ApplyTextStyle(element, density, context, text_style);
  builder.PushTextStyle(text_style);

  const auto decode_property = DecodePropertyForTextElement(element);
  AppendDecodedText(builder, element->content(), decode_property);

  auto append_children = [&](auto&& self, Element* parent,
                             TextElement* inherited_text) -> void {
    for (auto* child = parent->first_render_child(); child != nullptr;
         child = child->next_render_sibling()) {
      if (child->is_raw_text()) {
        AppendDecodedText(builder,
                          static_cast<RawTextElement*>(child)->content(),
                          DecodePropertyForTextElement(inherited_text));
      } else if (child->is_text()) {
        AppendTextSubtree(builder, static_cast<TextElement*>(child), density,
                          context);
      } else if (child->is_wrapper()) {
        self(self, child, inherited_text);
      }
    }
  };
  append_children(append_children, element, element);
  builder.PopTextStyle();
}

}  // namespace

TextMeasurerHarmony::TextMeasurerHarmony(harmony::LynxContext* context)
    : context_(context) {
  const auto font_manager =
      context_ != nullptr ? context_->GetFontFaceManager() : nullptr;
  font_collection_ =
      font_manager != nullptr
          ? font_manager->GetFontCollection()
          : harmony::FontCollectionHarmony::MakeSharedFontCollectionHarmony();
}

TextMeasurerHarmony::~TextMeasurerHarmony() = default;

void TextMeasurerHarmony::DispatchLayoutBefore(Element* element) {
  if (element == nullptr || !element->is_text()) {
    return;
  }
  // Inline platform children are not part of the first Layout-in-Element
  // implementation. Keep child layout disabled until placeholder support is
  // provided by this manager directly.
  static_cast<TextElement*>(element)->set_need_layout_children(false);
}

LayoutResult TextMeasurerHarmony::Measure(Element* element, float width,
                                          int width_mode, float height,
                                          int height_mode) {
  if (element == nullptr || !element->is_text() ||
      font_collection_ == nullptr) {
    return {};
  }

  auto* text_element = static_cast<TextElement*>(element);
  const float density = context_ != nullptr ? context_->ScaledDensity() : 1.f;
  harmony::ParagraphStyleHarmony paragraph_style;
  ApplyParagraphStyle(text_element, density, paragraph_style);
  harmony::ParagraphBuilderHarmony builder(&paragraph_style,
                                           font_collection_.get());
  AppendTextSubtree(builder, text_element, density, context_);

  auto paragraph = builder.CreateParagraph(font_collection_, width);
  const auto measure_width_mode = static_cast<SLMeasureMode>(width_mode);
  const auto measure_height_mode = static_cast<SLMeasureMode>(height_mode);
  const float max_width = IsSLIndefiniteMode(measure_width_mode)
                              ? kTextMaxLayoutWidth
                              : width * density;
  paragraph->Layout(max_width);

  LayoutResult result{static_cast<float>(paragraph->GetLongestLine() / density),
                      static_cast<float>(paragraph->GetHeight() / density),
                      static_cast<float>(paragraph->GetBaseline() / density)};
  if (paragraph_style.GetTextIndent() && paragraph->GetLineCount() > 0) {
    result.width_ = std::max(
        result.width_,
        (paragraph->GetLineWidth(0) + paragraph->GetIndent()) / density);
  }
  if (IsSLDefiniteMode(measure_width_mode)) {
    result.width_ = width;
  } else if (IsSLAtMostMode(measure_width_mode)) {
    result.width_ = std::min(result.width_, width);
  }
  if (IsSLDefiniteMode(measure_height_mode)) {
    result.height_ = height;
  } else if (IsSLAtMostMode(measure_height_mode)) {
    result.height_ = std::min(result.height_, height);
  }

  const auto& attributes =
      text_element->computed_css_style()->GetTextAttributes();
  const auto white_space = attributes.has_value()
                               ? attributes->white_space
                               : starlight::WhiteSpaceType::kNormal;
  const auto text_overflow = attributes.has_value()
                                 ? attributes->text_overflow
                                 : starlight::TextOverflowType::kClip;
  if (!IsSLDefiniteMode(measure_width_mode) ||
      white_space == starlight::WhiteSpaceType::kNowrap) {
    float text_width = paragraph->GetLongestLine();
    if (IsSLDefiniteMode(measure_width_mode) && width * density > text_width &&
        text_overflow != starlight::TextOverflowType::kEllipsis) {
      text_width = width * density;
    }
    const auto alignment = paragraph_style.GetEffectiveAlignment();
    if (alignment == starlight::TextAlignType::kCenter) {
      paragraph->SetTranslateLeftOffset(-(max_width - text_width) / 2.f);
    } else if (alignment == starlight::TextAlignType::kRight) {
      paragraph->SetTranslateLeftOffset(-(max_width - text_width));
    }
  }

  paragraph->SetMeasuredSize(result.width_, result.height_, result.baseline_);
  paragraph->SetText(builder.GetText());
  paragraphs_[element->impl_id()] = std::move(paragraph);
  return result;
}

void TextMeasurerHarmony::Align(Element* element) {
  // Inline placeholders are intentionally unsupported in this first version.
}

void TextMeasurerHarmony::Destroy(Element* element) {
  if (element != nullptr) {
    paragraphs_.erase(element->impl_id());
  }
}

fml::RefPtr<harmony::ParagraphHarmony> TextMeasurerHarmony::GetParagraph(
    int32_t id) const {
  const auto it = paragraphs_.find(id);
  return it != paragraphs_.end() ? it->second : nullptr;
}

}  // namespace tasm
}  // namespace lynx
