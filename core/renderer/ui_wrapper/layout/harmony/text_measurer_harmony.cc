// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/harmony/text_measurer_harmony.h"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/float_comparison.h"
#include "base/include/string/string_utils.h"
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

using InlineElementSizeList = std::vector<std::pair<int32_t, LayoutResult>>;

void LoadCustomFontsIfNeeded(
    const base::String& font_family,
    const std::shared_ptr<harmony::FontFaceManager>& font_manager,
    const std::shared_ptr<harmony::FontCollectionHarmony>& font_collection) {
  if (font_manager == nullptr || font_collection == nullptr) {
    return;
  }

  std::vector<std::string> raw_font_families;
  base::SplitString(font_family.str(), ',', raw_font_families);
  for (const auto& raw_font_family : raw_font_families) {
    std::vector<harmony::FontFace::FontSrcData> font_src_data;
    if (!font_manager->TryGetFontSrcData(raw_font_family, font_src_data)) {
      continue;
    }
    for (const auto& font_src : font_src_data) {
      const auto& custom_font_family = font_src.unique_custom_font_family;
      if (custom_font_family.empty() ||
          font_collection->GetFontLoadingState(custom_font_family) !=
              harmony::FontCollectionHarmony::FontLoadingState::kUndefined) {
        continue;
      }
      font_manager->LoadFontWithUrl(
          custom_font_family, font_src.src, font_src.type,
          [font_collection](const std::string& loaded_font_family,
                            int32_t ret_code, std::vector<uint8_t>& data,
                            size_t length) {
            if (ret_code != 0 || length == 0) {
              return;
            }
            font_collection->SetLoadingFontState(
                loaded_font_family,
                harmony::FontCollectionHarmony::FontLoadingState::kLoading);
            font_collection->RegisterFontBuffer(
                loaded_font_family.c_str(), data, length,
                [font_collection, loaded_font_family]() {
                  font_collection->SetLoadingFontState(
                      loaded_font_family, harmony::FontCollectionHarmony::
                                              FontLoadingState::kLoaded);
                  // TODO: Request a new layout after asynchronous font loading
                  // completes.
                });
          });
    }
  }
}

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
    LoadCustomFontsIfNeeded(attributes->font_family, font_manager,
                            font_manager->GetFontCollection());
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

bool HasInlineElement(Element* element) {
  if (element == nullptr) {
    return false;
  }
  for (auto* child = element->first_render_child(); child != nullptr;
       child = child->next_render_sibling()) {
    if (child->is_image() || child->is_view()) {
      return true;
    }
    if ((child->is_text() || child->is_wrapper()) && HasInlineElement(child)) {
      return true;
    }
  }
  return false;
}

void MeasureInlineElements(Element* element,
                           const starlight::Constraints& constraints,
                           InlineElementSizeList& sizes) {
  if (element == nullptr) {
    return;
  }
  for (auto* child = element->first_render_child(); child != nullptr;
       child = child->next_render_sibling()) {
    if (child->is_image() || child->is_view()) {
      if (child->slnode() != nullptr) {
        const FloatSize size =
            child->slnode()->UpdateMeasureByPlatform(constraints, true);
        sizes.emplace_back(
            child->impl_id(),
            LayoutResult{size.width_, size.height_, size.baseline_});
      }
    } else if (child->is_text() || child->is_wrapper()) {
      MeasureInlineElements(child, constraints, sizes);
    }
  }
}

OH_Drawing_PlaceholderVerticalAlignment ResolvePlaceholderVerticalAlignment(
    Element* element, const LayoutResult& size, float line_height,
    float* baseline_offset) {
  auto* style = element->computed_css_style();
  auto vertical_align = starlight::DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN;
  float vertical_align_length = 0.f;
  if (style != nullptr) {
    const auto& attributes = style->GetTextAttributes();
    if (attributes.has_value()) {
      vertical_align = attributes->vertical_align;
      vertical_align_length = attributes->vertical_align_length;
    }
  }

  switch (vertical_align) {
    case starlight::VerticalAlignType::kBaseline:
      *baseline_offset = size.baseline_ - size.height_;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kSub:
      *baseline_offset = -size.height_ * 0.1f;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kSuper:
      *baseline_offset = size.height_ * 0.1f;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kTop:
    case starlight::VerticalAlignType::kTextTop:
      return ALIGNMENT_TOP_OF_ROW_BOX;
    case starlight::VerticalAlignType::kMiddle:
    case starlight::VerticalAlignType::kCenter:
      return ALIGNMENT_CENTER_OF_ROW_BOX;
    case starlight::VerticalAlignType::kBottom:
    case starlight::VerticalAlignType::kTextBottom:
      return ALIGNMENT_BOTTOM_OF_ROW_BOX;
    case starlight::VerticalAlignType::kLength:
      *baseline_offset = vertical_align_length;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kPercent:
      *baseline_offset = line_height * vertical_align_length * 0.01f;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    default:
      return ALIGNMENT_ABOVE_BASELINE;
  }
}

template <typename PlaceholderInfoList>
void AppendInlineElement(harmony::ParagraphBuilderHarmony& builder,
                         Element* element, TextElement* inherited_text,
                         const InlineElementSizeList& sizes, float density,
                         PlaceholderInfoList& placeholder_infos) {
  const auto size_it = std::find_if(
      sizes.begin(), sizes.end(),
      [element](const auto& item) { return item.first == element->impl_id(); });
  if (size_it == sizes.end()) {
    return;
  }

  float line_height = 0.f;
  if (inherited_text != nullptr &&
      inherited_text->computed_css_style() != nullptr) {
    const auto& attributes =
        inherited_text->computed_css_style()->GetTextAttributes();
    if (attributes.has_value() &&
        attributes->computed_line_height !=
            starlight::DefaultComputedStyle::DEFAULT_LINE_HEIGHT) {
      line_height = attributes->computed_line_height;
    }
  }

  float baseline_offset = 0.f;
  const auto alignment = ResolvePlaceholderVerticalAlignment(
      element, size_it->second, line_height, &baseline_offset);
  harmony::PlaceholderHarmony placeholder{size_it->second.width_ * density,
                                          size_it->second.height_ * density,
                                          alignment, baseline_offset * density};
  const int32_t char_index = builder.GetCharCount();
  if (builder.AddPlaceholder(placeholder, element->impl_id())) {
    placeholder_infos.push_back(
        {element->impl_id(), char_index, size_it->second, line_height});
  }
}

template <typename PlaceholderInfoList>
void AppendTextSubtree(harmony::ParagraphBuilderHarmony& builder,
                       TextElement* element, float density,
                       harmony::LynxContext* context,
                       const InlineElementSizeList& inline_sizes,
                       PlaceholderInfoList& placeholder_infos) {
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
                          context, inline_sizes, placeholder_infos);
      } else if (child->is_image() || child->is_view()) {
        AppendInlineElement(builder, child, inherited_text, inline_sizes,
                            density, placeholder_infos);
      } else if (child->is_wrapper()) {
        self(self, child, inherited_text);
      }
    }
  };
  append_children(append_children, element, element);
  builder.PopTextStyle();
}

template <typename PlaceholderInfo>
float CalculatePlaceholderTop(Element* element,
                              const PlaceholderInfo& placeholder_info,
                              harmony::LineMetricsHarmony& line_metrics,
                              float density) {
  float baseline_offset = 0.f;
  ResolvePlaceholderVerticalAlignment(element, placeholder_info.size,
                                      placeholder_info.line_height,
                                      &baseline_offset);

  const float height = placeholder_info.size.height_ * density;
  baseline_offset *= density;
  auto vertical_align = starlight::DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN;
  if (element != nullptr && element->computed_css_style() != nullptr) {
    const auto& attributes = element->computed_css_style()->GetTextAttributes();
    if (attributes.has_value()) {
      vertical_align = attributes->vertical_align;
    }
  }

  switch (vertical_align) {
    case starlight::VerticalAlignType::kBaseline:
    case starlight::VerticalAlignType::kSub:
    case starlight::VerticalAlignType::kSuper:
    case starlight::VerticalAlignType::kLength:
    case starlight::VerticalAlignType::kPercent:
      return line_metrics.Top() + line_metrics.Ascent() - height -
             baseline_offset;
    case starlight::VerticalAlignType::kTop:
    case starlight::VerticalAlignType::kTextTop:
      return line_metrics.Top();
    case starlight::VerticalAlignType::kMiddle:
      return line_metrics.Top() + line_metrics.Ascent() -
             (line_metrics.XHeight() + height) / 2.f;
    case starlight::VerticalAlignType::kCenter:
      return line_metrics.Top() + (line_metrics.Height() - height) / 2.f;
    case starlight::VerticalAlignType::kBottom:
    case starlight::VerticalAlignType::kTextBottom:
      return line_metrics.Top() + line_metrics.Height() - height;
    default:
      return line_metrics.Top() + line_metrics.Ascent() - height;
  }
}

template <typename PlaceholderInfoList>
void AlignInlineElements(Element* element, harmony::TextBoxHarmony& rects,
                         const std::vector<int32_t>& placeholders,
                         const PlaceholderInfoList& placeholder_infos,
                         size_t placeholder_count, float density,
                         float translate_left_offset,
                         harmony::ParagraphHarmony& paragraph) {
  if (element == nullptr) {
    return;
  }
  for (auto* child = element->first_render_child(); child != nullptr;
       child = child->next_render_sibling()) {
    if (child->is_image() || child->is_view()) {
      const auto placeholder_it =
          std::find(placeholders.begin(),
                    placeholders.begin() + placeholder_count, child->impl_id());
      if (placeholder_it != placeholders.begin() + placeholder_count &&
          child->slnode() != nullptr) {
        const size_t index = placeholder_it - placeholders.begin();
        float top = rects.GetTop(index);
        const auto info_it =
            std::find_if(placeholder_infos.begin(), placeholder_infos.end(),
                         [child](const auto& info) {
                           return info.sign == child->impl_id();
                         });
        if (info_it != placeholder_infos.end()) {
          for (int32_t i = 0; i < paragraph.GetLineCount(); ++i) {
            auto line_metrics = paragraph.GetLineMetrics(i);
            if (info_it->char_index < line_metrics.EndIndex() &&
                info_it->char_index >= line_metrics.StartIndex()) {
              top = CalculatePlaceholderTop(child, *info_it, line_metrics,
                                            density);
              break;
            }
          }
        }
        child->slnode()->AlignmentByPlatform(
            top / density,
            (rects.GetLeft(index) + translate_left_offset) / density);
      }
    } else if (child->is_text() || child->is_wrapper()) {
      AlignInlineElements(child, rects, placeholders, placeholder_infos,
                          placeholder_count, density, translate_left_offset,
                          paragraph);
    }
  }
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
  static_cast<TextElement*>(element)->set_need_layout_children(
      HasInlineElement(element));
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
  InlineElementSizeList inline_sizes;
  if (text_element->need_layout_children()) {
    starlight::Constraints constraints;
    constraints[starlight::kHorizontal] = starlight::OneSideConstraint(
        width, static_cast<SLMeasureMode>(width_mode));
    constraints[starlight::kVertical] = starlight::OneSideConstraint(
        height, static_cast<SLMeasureMode>(height_mode));
    MeasureInlineElements(element, constraints, inline_sizes);
  }

  harmony::ParagraphStyleHarmony paragraph_style;
  ApplyParagraphStyle(text_element, density, paragraph_style);
  harmony::ParagraphBuilderHarmony builder(&paragraph_style,
                                           font_collection_.get());
  std::vector<InlinePlaceholderInfo> placeholder_infos;
  AppendTextSubtree(builder, text_element, density, context_, inline_sizes,
                    placeholder_infos);

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
  const int32_t id = element->impl_id();
  auto& stored_paragraph = paragraphs_[id];
  stored_paragraph = std::move(paragraph);
  inline_placeholders_[id] = std::move(placeholder_infos);
  if (element->EnableFragmentLayerRender()) {
    text_element->SetTextBundle(
        reinterpret_cast<intptr_t>(stored_paragraph.get()));
  }
  return result;
}

void TextMeasurerHarmony::Align(Element* element) {
  if (element == nullptr || !element->is_text()) {
    return;
  }
  const auto paragraph_it = paragraphs_.find(element->impl_id());
  if (paragraph_it == paragraphs_.end() || paragraph_it->second == nullptr) {
    return;
  }

  const float density = context_ != nullptr ? context_->ScaledDensity() : 1.f;
  const auto& paragraph = paragraph_it->second;
  const auto placeholder_info_it =
      inline_placeholders_.find(element->impl_id());
  if (placeholder_info_it == inline_placeholders_.end()) {
    return;
  }
  auto rects = paragraph->GetRectsForPlaceholders();
  const auto& placeholders = paragraph->GetPlaceholders();
  const size_t count =
      std::min(static_cast<size_t>(rects.GetCount()), placeholders.size());
  AlignInlineElements(element, rects, placeholders, placeholder_info_it->second,
                      count, density, paragraph->GetTranslateLeftOffset(),
                      *paragraph);
}

void TextMeasurerHarmony::Destroy(Element* element) {
  if (element != nullptr) {
    paragraphs_.erase(element->impl_id());
    inline_placeholders_.erase(element->impl_id());
  }
}

fml::RefPtr<harmony::ParagraphHarmony> TextMeasurerHarmony::GetParagraph(
    int32_t id) const {
  const auto it = paragraphs_.find(id);
  return it != paragraphs_.end() ? it->second : nullptr;
}

}  // namespace tasm
}  // namespace lynx
