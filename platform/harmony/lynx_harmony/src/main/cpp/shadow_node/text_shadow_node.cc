// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text_shadow_node.h"

#include <cmath>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/style/color.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_placeholder_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_truncation_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_measure_cache.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr float TEXT_MAX_LAYOUT_WIDTH = INT16_MAX;

TextShadowNode::TextShadowNode(int sign, const std::string& tag)
    : BaseTextShadowNode(sign, tag) {
  paragraph_style_ = std::make_unique<ParagraphStyleHarmony>();
  SetCustomMeasureFunc(this);
}

TextShadowNode::~TextShadowNode() = default;

void TextShadowNode::OnContextReady() {
  BaseTextShadowNode::OnContextReady();
  const auto font_face_manager = context_->GetFontFaceManager();
  font_collection_ =
      font_face_manager
          ? font_face_manager->GetFontCollection()
          : FontCollectionHarmony::MakeSharedFontCollectionHarmony();
}

LayoutResult TextShadowNode::Measure(float width, MeasureMode width_mode,
                                     float height, MeasureMode height_mode,
                                     bool final_measure) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEXT_SHADOW_NODE_MEASURE);
  HandleWhiteSpace();
  MeasureChildrenNode(width, width_mode, height, height_mode, final_measure);
  paragraph_builder_ = std::make_unique<ParagraphBuilderHarmony>(
      paragraph_style_.get(), font_collection_.get());
  origin_char_count_ = ellipsis_count_ = 0;
  inline_truncation_shadow_node_ = FindInlineTruncationNode();
  if (inline_truncation_shadow_node_ == nullptr &&
      text_props_->text_overflow == starlight::TextOverflowType::kEllipsis) {
    paragraph_style_->SetEllipsis(ELLIPSIS);
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TEXT_SHADOW_NODE_LAYOUT);
    paragraph_ = LayoutNode(this, paragraph_builder_.get(), width, width_mode,
                            height, height_mode, final_measure);
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                TEXT_SHADOW_NODE_HANDLE_TEXT_OVERFLOW_AND_TRUNCATION);
    paragraph_ = HandleTextOverflowAndTruncation(std::move(paragraph_), width,
                                                 width_mode, height,
                                                 height_mode, final_measure);
  }
  LayoutResult result = LayoutResult{paragraph_->GetMeasuredWidth(),
                                     paragraph_->GetMeasuredHeight(),
                                     paragraph_->GetMeasuredBaseline()};
  return result;
}

/**
 * Rebuild paragraph to changed the size related shader effect style only. Use
 * the layout result generated before to calculate gradient's size.
 *
 * Will be removed once HarmonyOS support updating paints of one specific
 * TextStyle object.
 */
fml::RefPtr<ParagraphHarmony> TextShadowNode::ReBuildParagraph(
    ParagraphBuilderHarmony* builder, const LayoutResult result,
    const float layout_max_width, const float offset) {
  auto paragraph = builder->CreateParagraph(font_collection_, result.width_);
  paragraph->Layout(layout_max_width);
  paragraph->SetTranslateLeftOffset(offset);
  paragraph->SetMeasuredSize(result.width_, result.height_, result.baseline_);
  return paragraph;
}

void TextShadowNode::HandleWhiteSpace() {
  if (paragraph_style_->GetWhiteSpace() == WhiteSpaceType::kNowrap) {
    if (text_props_->text_overflow == starlight::TextOverflowType::kEllipsis) {
      paragraph_style_->SetTextMaxLines(1);
      paragraph_style_->SetTextWordBreakType(
          starlight::WordBreakType::kBreakAll);
    }
  } else if (text_props_->max_line > 0) {
    paragraph_style_->SetTextMaxLines(text_props_->max_line);
  }
}

fml::RefPtr<ParagraphHarmony> TextShadowNode::HandleTextOverflowAndTruncation(
    fml::RefPtr<ParagraphHarmony> paragraph, float width,
    MeasureMode width_mode, float height, MeasureMode height_mode,
    bool final_measure) {
  float layout_max_height = ConstructTextHeightConstraint(height_mode, height);
  if (height_mode != MeasureMode::Indefinite && text_props_.has_value() &&
      static_cast<int64_t>(text_props_->line_height) != kLineHeightNormal) {
    const float line_height = static_cast<float>(text_props_->line_height);
    const float line_height_float_error =
        (std::ceil(line_height - base::EPSILON_PRECISE) - line_height) *
        paragraph->GetLineCount();
    if (line_height_float_error > 0.f) {
      layout_max_height = static_cast<float>(
          std::ceil(layout_max_height + line_height_float_error));
    }
  }
  if (!IsTextOverflow(paragraph.get(), layout_max_height)) {
    // not overflow
    return paragraph;
  }
  auto truncation_line_index =
      FindTruncationLineIndex(paragraph.get(), layout_max_height);
  if (inline_truncation_shadow_node_ == nullptr &&
      truncation_line_index + 1 >= paragraph->GetLineCount()) {
    // no truncation and need ellipsis at last line,
    // ellipsis has been added.
    return paragraph;
  }
  float layout_max_width = width_mode == MeasureMode::Indefinite
                               ? TEXT_MAX_LAYOUT_WIDTH
                               : width * ScaleDensity();
  bool need_truncation = true;
  fml::RefPtr<ParagraphHarmony> truncation_paragraph = nullptr;
  if (inline_truncation_shadow_node_ == nullptr) {
    need_truncation = false;
  } else {
    auto truncation_paragraph_builder =
        std::make_unique<ParagraphBuilderHarmony>(paragraph_style_.get(),
                                                  font_collection_.get());
    truncation_paragraph = LayoutNode(
        inline_truncation_shadow_node_, truncation_paragraph_builder.get(),
        width, width_mode, height, height_mode, final_measure);
    if (truncation_paragraph->GetLineCount() > 1) {
      // truncation text longer than one line
      need_truncation = false;
    }
  }
  if (!need_truncation) {
    if (text_props_->text_overflow != starlight::TextOverflowType::kEllipsis) {
      return paragraph;
    }
    paragraph_style_->SetTextMaxLines(truncation_line_index + 1);
    paragraph_builder_ = std::make_unique<ParagraphBuilderHarmony>(
        paragraph_style_.get(), font_collection_.get());
    AppendToParagraph(*paragraph_builder_, width * ScaleDensity(),
                      height * ScaleDensity());
    paragraph = paragraph_builder_->CreateParagraph(font_collection_, width);
    paragraph_style_->SetTextMaxLines(text_props_->max_line);
  } else {
    auto truncation_line_metrics =
        paragraph->GetLineMetrics(truncation_line_index);
    int line_start_index = truncation_line_metrics.StartIndex();
    int line_end_index = truncation_line_metrics.EndIndex();
    float truncation_width = truncation_paragraph->GetLongestLine();
    for (; line_end_index > line_start_index; line_end_index--) {
      auto box = paragraph->GetRectsForRange(line_start_index, line_end_index);
      if (box.GetCount() == 0) continue;
      if (base::FloatsLargerOrEqual(
              layout_max_width - box.GetRight(box.GetCount() - 1),
              truncation_width)) {
        break;
      }
    }
    int max_char_count = line_end_index;
    paragraph_builder_->Reset();
    paragraph_builder_->SetMaxCharCount(max_char_count);
    ellipsis_count_ = origin_char_count_ - max_char_count;
    AppendToParagraph(*paragraph_builder_, width * ScaleDensity(),
                      height * ScaleDensity());
    paragraph_builder_->SetMaxCharCount(std::numeric_limits<int32_t>::max());
    inline_truncation_shadow_node_->AppendToParagraph(
        *paragraph_builder_, width * ScaleDensity(), height * ScaleDensity());
    paragraph = paragraph_builder_->CreateParagraph(font_collection_, width);
  }
  paragraph->Layout(layout_max_width);
  LayoutResult result =
      GetLayoutResult(paragraph.get(), width, width_mode, height, height_mode);
  paragraph->SetMeasuredSize(result.width_, result.height_, result.baseline_);
  return paragraph;
}

int32_t TextShadowNode::FindTruncationLineIndex(ParagraphHarmony* paragraph,
                                                float layout_max_height) {
  int32_t max_lines = paragraph->GetLineCount();
  int32_t truncation_line_index = max_lines - 1;
  for (; truncation_line_index > 0; truncation_line_index--) {
    auto line_metrics = paragraph->GetLineMetrics(truncation_line_index);
    if (base::FloatsLargerOrEqual(layout_max_height,
                                  line_metrics.Top() + line_metrics.Height())) {
      break;
    }
  }
  return truncation_line_index;
}

InlineTruncationShadowNode* TextShadowNode::FindInlineTruncationNode() {
  for (auto* child : GetChildren()) {
    if (static_cast<BaseTextShadowNode*>(child)->IsInlineTruncation()) {
      return static_cast<InlineTruncationShadowNode*>(child);
    }
  }
  return nullptr;
}

float TextShadowNode::ConstructTextWidthConstraint(MeasureMode width_mode,
                                                   float width) {
  if (paragraph_style_->GetWhiteSpace() == starlight::WhiteSpaceType::kNowrap &&
      text_props_->text_overflow == starlight::TextOverflowType::kClip) {
    return TEXT_MAX_LAYOUT_WIDTH;
  }

  return width_mode == MeasureMode::Indefinite ? TEXT_MAX_LAYOUT_WIDTH
                                               : width * ScaleDensity();
}

float TextShadowNode::ConstructTextHeightConstraint(MeasureMode height_mode,
                                                    float height) {
  return height_mode == MeasureMode::Indefinite ? TEXT_MAX_LAYOUT_WIDTH
                                                : height * ScaleDensity();
}

void TextShadowNode::HandleMaxWidthMode(BaseTextShadowNode* shadow_node,
                                        ParagraphHarmony* paragraph,
                                        float width, MeasureMode width_mode,
                                        float layout_max_width,
                                        float text_max_width) {
  if (width_mode == MeasureMode::Definite &&
      paragraph_style_->GetWhiteSpace() != starlight::WhiteSpaceType::kNowrap) {
    return;
  }

  float offset = 0.f;
  if (paragraph_style_->GetWhiteSpace() == starlight::WhiteSpaceType::kNowrap &&
      width_mode == MeasureMode::Definite && width > text_max_width) {
    const auto props = shadow_node->GetTextProps();
    if (props == nullptr ||
        props->text_overflow != starlight::TextOverflowType::kEllipsis) {
      offset = CalcTextTranslateOffset(layout_max_width, width);
    }
  } else {
    offset = CalcTextTranslateOffset(layout_max_width, text_max_width);
  }

  paragraph->SetTranslateLeftOffset(offset);
}

float TextShadowNode::CalcTextTranslateOffset(float layout_max_width,
                                              float text_width) {
  float offset = 0.f;
  starlight::TextAlignType text_align =
      paragraph_style_->GetEffectiveAlignment();

  if (text_align == starlight::TextAlignType::kCenter) {
    offset = -(layout_max_width - text_width) / 2;
  } else if (text_align == starlight::TextAlignType::kRight) {
    offset = -(layout_max_width - text_width);
  }
  return offset;
}

void TextShadowNode::Align() {
  auto rects = paragraph_->GetRectsForPlaceholders();
  size_t count = rects.GetCount();
  std::stack<const ShadowNode*> to_align;
  to_align.push(this);
  float dx = 0, dy = 0;
  const auto& placeholders = paragraph_->GetPlaceholders();
  while (!to_align.empty()) {
    const ShadowNode* node = to_align.top();
    to_align.pop();
    if (node->IsPlaceholder()) {
      const auto* placeholder_node =
          static_cast<const InlinePlaceholderShadowNode*>(node);
      auto iterator = std::find(placeholders.begin(), placeholders.end(),
                                placeholder_node->Signature());
      if (iterator != placeholders.end()) {
        size_t index = iterator - placeholders.begin();
        if (index < count) {
          dx = rects.GetLeft(index) + paragraph_->GetTranslateLeftOffset();
          dy = rects.GetTop(index);
          int32_t placeholder_index = placeholder_node->Index();
          for (int i = 0; i < paragraph_->GetLineCount(); ++i) {
            LineMetricsHarmony line_metric = paragraph_->GetLineMetrics(i);
            if (placeholder_index < line_metric.EndIndex() &&
                placeholder_index >= line_metric.StartIndex()) {
              // The placeholder rect obtained after the second layout of the
              // same text is inaccurate. Temporarily recalculate the vertical
              // position of the placeholder.
              dy = placeholder_node->CalcPlaceholderTopOffset(&line_metric);
              break;
            }
          }
          node->AlignTo(dx, dy);
        }
      }
    }
    const auto& next_to_align = node->GetChildren();
    for (auto i = next_to_align.rbegin(); i != next_to_align.rend(); ++i) {
      to_align.push(*i);
    }
  }
}

void TextShadowNode::OnAppendToParagraph(ParagraphBuilderHarmony& builder,
                                         float width, float height) {
  LoadFontFamilyIfNeeded(GetRawFontFamilies(), font_collection_.get());
  // override word break
  BaseTextShadowNode::OnAppendToParagraph(builder, width, height);
}

void TextShadowNode::OnLayoutBefore() {
  ShadowNode::OnLayoutBefore();
  UpdateWordBreakType(word_break_);
  if (text_props_.has_value() &&
      static_cast<int64_t>(text_props_->line_height) != kLineHeightNormal) {
    UpdateLineHeight(this, text_props_->line_height);
  }

  uint32_t color = starlight::DefaultColor::DEFAULT_TEXT_COLOR;
  std::shared_ptr<BackgroundGradientLayer> gradient = nullptr;
  if (text_props_.has_value()) {
    if (text_props_->gradient_color != nullptr) {
      color = 0;
      gradient = style_.GradientColorShared();
    } else if (text_props_->color.has_value()) {
      color = text_props_->color.value();
    }
  }
  UpdateInheritedTextStyle(color, std::move(gradient), {});
}

void TextShadowNode::UpdateLayout(float left, float top, float width,
                                  float height) {
  // TODO(renzhongyue): to be implemented.
  ShadowNode::UpdateLayout(left, top, width, height);
}

fml::RefPtr<fml::RefCountedThreadSafeStorage> TextShadowNode::getExtraBundle() {
  DispatchLayoutEvent();
  return paragraph_;
}

void TextShadowNode::OnPropsUpdate(const std::string& name,
                                   const lepus::Value& value) {
  char const* attr = name.c_str();
  PrepareTextProps();
  if (base::StringEqual(attr, kTextMaxLine)) {
    int32_t line = -1;
    if (value.IsString() && base::StringToInt(value.StdString(), &line, 10)) {
      // value is valid string
    } else if (value.IsNumber()) {
      line = value.Number();
    } else {
      LOGE(
          "TextShadowNode setMaxLine with invalid value:" << value.StdString());
    }
    text_props_->max_line = line;
    paragraph_style_->SetTextMaxLines(line);
  } else if (base::StringEqual(attr, kTextAlign)) {
    text_props_->text_align =
        static_cast<starlight::TextAlignType>(value.Number());
    paragraph_style_->SetTextAlignToParagraphStyle(text_props_->text_align);
  } else if (base::StringEqual(attr, kDirection)) {
    text_props_->direction =
        static_cast<starlight::DirectionType>(value.Number());
    ;
    paragraph_style_->SetDirectionToParagraphStyle(text_props_->direction);
  } else if (base::StringEqual(attr, kTextOverflow)) {
    text_props_->text_overflow =
        static_cast<starlight::TextOverflowType>(value.Number());
  } else if (base::StringEqual(attr, kWhiteSpace)) {
    text_props_->white_space = static_cast<WhiteSpaceType>(value.Number());
    paragraph_style_->SetWhiteSpace(text_props_->white_space);
  } else if (base::StringEqual(attr, kTextIndent)) {
    const auto& arr = value.Array();
    text_props_->text_indent = TextIndent();
    if (arr->size() == 2) {
      text_props_->text_indent->value =
          static_cast<float>(arr->get(0).Number() * ScaleDensity());
      text_props_->text_indent->unit =
          static_cast<starlight::PlatformLengthUnit>(arr->get(1).Number());
    }
    paragraph_style_->SetTextIndent(text_props_->text_indent);
  } else {
    BaseTextShadowNode::OnPropsUpdate(name, value);
  }
  MarkDirty();
}

fml::RefPtr<ParagraphHarmony> TextShadowNode::LayoutNode(
    BaseTextShadowNode* shadow_node, ParagraphBuilderHarmony* builder,
    float width, MeasureMode width_mode, float height, MeasureMode height_mode,
    bool final_measure) {
  // TODO(zhouzhuangzhuang):temporarily remove the text cache because the cache
  // operation takes too much time.
  fml::RefPtr<ParagraphHarmony> paragraph = nullptr;
  builder->Reset();
  shadow_node->AppendToParagraph(*builder, 0, 0);
  paragraph = builder->CreateParagraph(font_collection_, width);
  if (origin_char_count_ == 0) {
    origin_char_count_ = builder->GetCharCount();
  }
  float layout_max_width = ConstructTextWidthConstraint(width_mode, width);
  paragraph->Layout(layout_max_width);
  HandleMaxWidthMode(shadow_node, paragraph.get(), width * ScaleDensity(),
                     width_mode, layout_max_width, paragraph->GetLongestLine());
  LayoutResult result =
      GetLayoutResult(paragraph.get(), width, width_mode, height, height_mode);
  paragraph->SetMeasuredSize(result.width_, result.height_, result.baseline_);
  if (builder->NeedsRebuilding()) {
    builder->Reset();
    shadow_node->AppendToParagraph(*builder, result.width_ * ScaleDensity(),
                                   result.height_ * ScaleDensity());
    paragraph = ReBuildParagraph(builder, result, layout_max_width,
                                 paragraph->GetTranslateLeftOffset());
  }
  paragraph->SetText(paragraph_builder_->GetText());

  return paragraph;
}

LayoutResult TextShadowNode::GetLayoutResult(ParagraphHarmony* paragraph,
                                             float width,
                                             MeasureMode width_mode,
                                             float height,
                                             MeasureMode height_mode) {
  LayoutResult result = LayoutResult{
      static_cast<float>(paragraph->GetLongestLine() / ScaleDensity()),
      static_cast<float>(paragraph->GetHeight() / ScaleDensity()),
      static_cast<float>(paragraph->GetBaseline() / ScaleDensity())};

  if (paragraph_style_->GetTextIndent()) {
    float first_line_width =
        (paragraph->GetLineWidth(0) + paragraph->GetIndent()) / ScaleDensity();
    if (base::FloatsLarger(first_line_width, result.width_)) {
      result.width_ = first_line_width;
    }
  }

  if (width_mode == MeasureMode::AtMost &&
      paragraph_style_->GetEffectiveAlignment() ==
          starlight::TextAlignType::kJustify) {
    float max_right = 0.f;
    uint32_t line_count = paragraph->GetLineCount();
    for (uint32_t i = 0; i < line_count; ++i) {
      auto line_metric = paragraph->GetLineMetrics(i);
      float right =
          static_cast<float>(line_metric.Left() + line_metric.Width());
      right /= ScaleDensity();
      if (base::FloatsLarger(right, max_right)) {
        max_right = right;
      }
    }
    if (base::FloatsLarger(max_right, result.width_)) {
      result.width_ = max_right;
    }
  }

  if (width_mode == MeasureMode::Definite) {
    result.width_ = width;
  }
  if (height_mode == MeasureMode::Definite) {
    result.height_ = height;
  }
  if (width_mode == MeasureMode::AtMost &&
      base::FloatsLarger(result.width_, width)) {
    result.width_ = width;
  }
  return result;
}

void TextShadowNode::UpdateLineHeight(
    lynx::tasm::harmony::BaseTextShadowNode* node, double line_height) {
  if (node == nullptr) {
    return;
  }
  if (node->IsPlaceholder()) {
    static_cast<InlinePlaceholderShadowNode*>(node)->SetLineHeight(line_height);
    return;
  }
  for (auto* child : node->GetChildren()) {
    UpdateLineHeight(static_cast<BaseTextShadowNode*>(child), line_height);
  }
}

void TextShadowNode::DispatchLayoutEvent() {
  if (paragraph_ == nullptr ||
      std::find(events_.begin(), events_.end(), kLayout) == events_.end()) {
    return;
  }

  BASE_STATIC_STRING_DECL(kLineCount, "lineCount");
  BASE_STATIC_STRING_DECL(kStart, "start");
  BASE_STATIC_STRING_DECL(kEnd, "end");
  BASE_STATIC_STRING_DECL(kEllipsisCount, "ellipsisCount");
  BASE_STATIC_STRING_DECL(kLines, "lines");
  BASE_STATIC_STRING_DECL(kSize, "size");
  BASE_STATIC_STRING_DECL(kDetail, "detail");

  // Retrieve the line count from the paragraph
  uint32_t line_count = paragraph_->GetLineCount();
  auto param = lepus::Dictionary::Create();
  param->SetValue(kLineCount, line_count);

  // Create an array to hold information about each line
  auto lines = lepus::CArray::Create();
  for (uint32_t i = 0; i < line_count; ++i) {
    auto line_metric = paragraph_->GetLineMetrics(i);
    auto line_info = lepus::Dictionary::Create();
    int32_t end =
        (i == line_count - 1) ? origin_char_count_ : line_metric.EndIndex();
    int32_t ellipsis_count =
        (i == line_count - 1)
            ? (ellipsis_count_ == 0
                   ? origin_char_count_ - line_metric.EndIndex()
                   : ellipsis_count_)
            : 0;

    line_info->SetValue(kStart, line_metric.StartIndex());
    line_info->SetValue(kEnd, end);
    line_info->SetValue(kEllipsisCount, ellipsis_count);

    lines->emplace_back(std::move(line_info));
  }
  param->SetValue(kLines, std::move(lines));

  auto size = lepus::Dictionary::Create();
  size->SetValue(kWidth, paragraph_->GetLongestLine() / ScaleDensity());
  size->SetValue(kHeight, paragraph_->GetHeight() / ScaleDensity());
  param->SetValue(kSize, std::move(size));

  CustomEvent event{Signature(), kLayout, kDetail.str(),
                    lepus::Value(std::move(param))};
  context_->RunOnUIThread([this, event]() { context_->SendEvent(event); });
}

bool TextShadowNode::IsTextOverflow(ParagraphHarmony* paragraph,
                                    float layout_max_height) {
  return paragraph->DidExceedMaxLines() ||
         base::FloatsLarger(paragraph->GetHeight(), layout_max_height) ||
         base::FloatsLarger(paragraph->GetLongestLine() / ScaleDensity(),
                            paragraph->GetMeasuredWidth());
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
