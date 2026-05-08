// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/text_render.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/inline_image_view.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/component/view.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/rendering/render_image.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/shadow/icu_substitute.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_truncation_shadow_node.h"
#include "clay/ui/shadow/measure_utils.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {
namespace utils = attribute_utils;

constexpr float kLayoutTolerance = 1.f;
static constexpr float kDefaultFontSizeInDip = 14.f;

clay::Value TextRender::GetTextInfo(const char* text,
                                    const clay::Value& params) {
  const auto& info = utils::GetMap(params);
  TextStyle text_style;
  auto pixel_ratio = utils::GetInt(utils::GetMapItem(info, "pixelRatio"));
  if (pixel_ratio == 0) {
    pixel_ratio = 1;
  }
  auto font_size_str = utils::GetCString(utils::GetMapItem(info, "fontSize"));
  char* endptr;
  double num = std::strtod(font_size_str.c_str(), &endptr);
  if (endptr == font_size_str.c_str()) {
    // TODO(ZhuChengCheng) There may be problems here:
    // The front end receive this size with unit 'px', which should be the same
    // meaning in the css, i.e. measuring precess. thus, the unit here shuld be
    // the same as the platform unit, i.e. Android px, iOS pt.
    text_style.font_size = kDefaultFontSizeInDip * pixel_ratio;
  } else {
    text_style.font_size = num * pixel_ratio;
  }
  double layout_width = std::numeric_limits<double>::max();
  auto it_max_width = info.find("maxWidth");
  if (it_max_width != info.end()) {
    auto layout_width_str = utils::GetCString(it_max_width->second);
    layout_width = std::strtod(layout_width_str.c_str(), &endptr);
    if (endptr == layout_width_str.c_str()) {
      layout_width = std::numeric_limits<double>::max();
    }
  }

  auto it_max_len = info.find("maxLine");
  if (it_max_len != info.end()) {
    text_style.max_lines = utils::GetDouble(it_max_len->second);
  } else {
    text_style.max_lines = 1;
  }
  auto builder = std::make_unique<TextParagraphBuilder>(true, text_style);
  builder->AddText(UnicodeUtil::Utf8ToUtf16(text));
  auto paragraph = Build(std::move(builder));
  paragraph->Layout(layout_width);
  auto line_metrics = paragraph->GetLineMetrics();
  clay::Value::Array content_array(line_metrics.size());
  for (size_t i = 0; i < content_array.size(); i++) {
    auto value = clay::Value(
        std::string(text + line_metrics[i].start_index,
                    line_metrics[i].end_index - line_metrics[i].start_index));
    content_array[i] = std::move(value);
  }
  clay::Value::Map ret;
  ret.emplace("width", clay::Value(std::min(
                           layout_width, paragraph->GetMaxIntrinsicWidth())));
  ret.emplace("content", clay::Value(std::move(content_array)));
  return clay::Value(std::move(ret));
}

void TextRender::MeasureText(const std::string& text, bool show_content,
                             const std::optional<double>& max_width,
                             const std::optional<uint32_t>& max_length,
                             const TextStyle& text_style, float* measured_width,
                             float* measured_height,
                             std::vector<std::string>* measured_content) {
  auto builder = std::make_unique<TextParagraphBuilder>(true, text_style);
  builder->PushStyle(text_style);
  LayoutContextText measure_context;
  measure_context.SetBuilder(builder.get());
  uint32_t max_length_value = max_length.value_or(0);
  float text_indent_value = text_style.text_indent.value_or(0.f);
  if (max_length_value > 0) {
    measure_context.SetMaxLength(max_length_value);
  }
  if (text_indent_value > 0.f) {
    measure_context.SetTextIndent(text_indent_value);
  }
  measure_context.AddText(UnicodeUtil::Utf8ToUtf16(text),
                          text_indent_value > 0.f);
  auto paragraph = Build(std::move(builder));
  double layout_width = max_width.has_value()
                            ? max_width.value()
                            : std::numeric_limits<double>::max();
  paragraph->Layout(layout_width);
  if (measured_width) {
    *measured_width = std::min(layout_width, paragraph->GetMaxIntrinsicWidth());
  }
  if (measured_height) {
    if (text_style.line_spacing.has_value() &&
        paragraph->GetLineMetrics().size() > 0) {
      auto line_spacing =
          std::max(text_style.line_spacing.value() -
                       paragraph->GetLineMetrics().begin()->ascent -
                       paragraph->GetLineMetrics().begin()->descent,
                   0.0);
      *measured_height = std::ceil(paragraph->GetHeight() - line_spacing);
    } else {
      *measured_height = std::ceil(paragraph->GetHeight());
    }
  }
  if (show_content && measured_content) {
    size_t indent = text_indent_value > 0.f ? 1 : 0;
    auto line_metrics = paragraph->GetLineMetrics();
    if (!line_metrics.empty() && indent) {
      auto first_line = line_metrics.begin();
      measured_content->emplace_back(
          text.c_str() + first_line->start_index,
          first_line->end_index - first_line->start_index - indent);
      line_metrics.erase(line_metrics.begin());
    }
    std::unique_ptr<txt::LineMetrics> last_line;
    if (!line_metrics.empty() && max_length_value > 0 &&
        max_length_value < text.length()) {
      last_line = std::make_unique<txt::LineMetrics>(line_metrics.back());
      line_metrics.pop_back();
    }
    for (auto& line_metric : line_metrics) {
      measured_content->emplace_back(
          text.c_str() + line_metric.start_index - indent,
          line_metric.end_index - line_metric.start_index);
    }
    if (last_line) {
      measured_content->emplace_back(
          text.c_str() + last_line->start_index - indent,
          last_line->end_index - last_line->start_index - 1 /*ellipsis size*/);
    }
  }
}

TextAlignment TextRender::EffectAlign() {
  auto text_align =
      measure_node_->text_style_->text_align.value_or(TextAlignment::kStart);
  auto text_direction =
      measure_node_->text_style_->text_direction.value_or(TextDirection::kLtr);
  if (text_align == TextAlignment::kStart) {
    return (text_direction == TextDirection::kLtr) ? TextAlignment::kLeft
                                                   : TextAlignment::kRight;
  } else if (text_align == TextAlignment::kEnd) {
    return (text_direction == TextDirection::kLtr) ? TextAlignment::kRight
                                                   : TextAlignment::kLeft;
  } else {
    return text_align;
  }
}

void TextRender::Measure(const MeasureConstraint& constraint,
                         ShadowLayoutContextMeasure* context) {
  inline_truncation_hidden_count_ = -1;
  BuildTextLayout(constraint, context);

  if (constraint.width_mode == TextMeasureMode::kIndefinite &&
      measure_node_->text_style_) {
    // Do second layout if the width mode is indefinite and the
    // alignment is not left. Because in this case, the paragraph_
    // won't have the actual line and nothing will be painted.
    if (measure_node_->GetResolvedTextAlign() != TextAlignment::kLeft) {
      measure_node_->SetNeedSecondLayout(true);
    }
  }

  if (constraint.width_mode == TextMeasureMode::kAtMost &&
      context->measured_width_ + kLayoutTolerance < context->layout_width_) {
    // Do second layout if actual text width is less than constraints.
    // For example, given at most 200px width and actually 100px is needed,
    // use 100px to layout again. Otherwise text align will be problem.
    // Note: Here maybe some optimizations to avoid second layout.
    measure_node_->SetNeedSecondLayout(true);
  }

  if (cache_paragraph_) {
    HandleAutoSize(constraint, context);
    HandleInlineTruncation(constraint, context);
  }
}

std::vector<LineInfo> TextRender::GetLineInfo() {
  auto res = std::vector<LineInfo>{};
  if (cache_paragraph_ == nullptr) {
    return res;
  }
  auto line_metrics = cache_paragraph_->GetLineMetrics();
  for (auto line_metric : line_metrics) {
    res.emplace_back(LineInfo{static_cast<int>(line_metric.start_index),
                              static_cast<int>(line_metric.end_index), 0});
  }
  if (res.size() == 0) {
    return res;
  }
  int text_length = static_cast<int>(end_glyph_position_);
  if (res.back().end < text_length) {
    res.back().ellipsis_count = text_length - res.back().end + 1;
  }
  if (inline_truncation_hidden_count_ >= 0) {
    res.back().ellipsis_count = inline_truncation_hidden_count_;
  }
  return res;
}

std::unique_ptr<txt::Paragraph> TextRender::LayoutParagraph(
    double layout_width) {
  TRACE_EVENT("clay", "TextRender::LayoutParagraph");
#ifndef CLAY_ENABLE_TTTEXT
  ReprocessAttributeIfNeeded(layout_width);
#endif
  auto text_style = measure_node_->text_style_.value();
  if (text_style.white_space == WhiteSpace::kNoWrap) {
    text_style.max_lines = 1;
    if (text_style.overflow != TextOverflow::kEllipsis) {
      layout_width = std::numeric_limits<float>::infinity();
    }
    text_style.text_align = TextAlignment::kLeft;
  }
  auto raw_text = measure_node_->GetRawText();
  if (!text_style.text_direction.has_value() && !raw_text.empty() &&
      icu_substitute::IsRTLCharacter(std::u16string(1, raw_text.front()))) {
    text_style.text_direction = TextDirection::kRtl;
    truncation_direction_ = TextDirection::kRtl;
  } else {
    truncation_direction_ =
        text_style.text_direction.value_or(TextDirection::kLtr);
  }
  auto builder = std::make_unique<TextParagraphBuilder>(true, text_style);
  LayoutContextText context_text;
  if (measure_node_->max_length_.has_value()) {
    context_text.SetMaxLength(measure_node_->max_length_.value());
  }
  context_text.SetBuilder(builder.get());
  context_text.SetTextIndent(
      measure_node_->text_style_->text_indent.value_or(0));
  measure_node_->TextLayout(&context_text);
  measure_node_->SetInlineEmojiInfo(context_text.TakeInlineEmojiInfo());
  auto paragraph = Build(std::move(builder));
  TRACE_EVENT("clay", "TextRender::Layout");
  paragraph->Layout(layout_width);
  end_glyph_position_ = context_text.TextSizeIncludingPlaceholders();
  return paragraph;
}

void TextRender::BuildTextLayout(const MeasureConstraint& constraint,
                                 LayoutContext* context) {
  TRACE_EVENT("clay", "TextRender::BuildTextLayout");
  auto* context_measure = static_cast<ShadowLayoutContextMeasure*>(context);
  auto layout_width = context_measure->layout_width_;

  const bool force_rebuild = update_flag_ != TextUpdateFlag::kUpdateFlagNone;
  const bool should_layout =
      force_rebuild || prev_layout_width_ != layout_width;

  if (force_rebuild) {
    measure_node_->PreLayout(nullptr);
  }

  std::unique_ptr<txt::Paragraph> paragraph;
  if (should_layout) {
    if (measure_node_->text_indent_ != 0) {
      if (measure_node_->text_indent_use_percent_) {
        measure_node_->text_style_->text_indent =
            measure_node_->text_indent_ * layout_width;
      } else {
        measure_node_->text_style_->text_indent = measure_node_->text_indent_;
      }
    }
    if (measure_node_->text_style_->white_space == WhiteSpace::kNoWrap) {
      if (measure_node_->text_style_->text_align == TextAlignment::kCenter) {
        measure_node_->SetTextPaintAlign(TextAlignment::kCenter);
      } else if (measure_node_->GetResolvedTextAlign() ==
                 TextAlignment::kRight) {
        measure_node_->SetTextPaintAlign(TextAlignment::kRight);
      } else {
        FML_DCHECK(measure_node_->GetResolvedTextAlign() ==
                   TextAlignment::kLeft);
        measure_node_->SetTextPaintAlign(TextAlignment::kLeft);
      }
    } else {
      measure_node_->SetTextPaintAlign(TextAlignment::kLeft);
    }
    paragraph = LayoutParagraph(layout_width);
    cache_paragraph_ = std::move(paragraph);
    measured_width_ = std::ceil(cache_paragraph_->GetMaxIntrinsicWidth());
    if (measure_node_->text_style_->line_spacing.has_value() &&
        cache_paragraph_->GetLineMetrics().size() > 0) {
#ifndef CLAY_ENABLE_TTTEXT
      auto line_spacing =
          std::max(measure_node_->text_style_->line_spacing.value() -
                       cache_paragraph_->GetLineMetrics().begin()->ascent -
                       cache_paragraph_->GetLineMetrics().begin()->descent,
                   0.0);
      measured_height_ =
          std::ceil(cache_paragraph_->GetHeight() - line_spacing);
#else
      measured_height_ = std::ceil(cache_paragraph_->GetHeight());
#endif
    } else {
      measured_height_ = std::ceil(cache_paragraph_->GetHeight());
    }
  }

  if (context_measure) {
    context_measure->measured_width_ = measured_width_;
    context_measure->measured_height_ = measured_height_;
  }

  update_flag_ = TextUpdateFlag::kUpdateFlagNone;
  prev_layout_width_ = layout_width;
}

#ifndef CLAY_ENABLE_TTTEXT
void TextRender::ReprocessAttributeIfNeeded(double layout_width) {
  // Determine if the baseline_shift property needs to be set
  for (auto* child : measure_node_->GetChildren()) {
    if (!child->IsRawTextShadowNode() &&
        child->GetVerticalAlign().has_value()) {
      if (child->GetVerticalAlign()->type ==
          VerticalAlignType::kVerticalAlignLength) {
        child->SetBaselineOffset(child->GetVerticalAlign()->length);
        continue;
      }
      TextStyle style = measure_node_->text_style_.value();
      // According to lynx's current implementation of the vertical-align
      // attribute,
      // the largest text among all texts is aligned. Maybe it is more
      // reasonable to align the largest text on the same line.
      style.font_size = GetMaxFontSize();
      auto parent_paragraph = LayoutXCharacter(layout_width, style);
      auto line_metrics = parent_paragraph->GetLineMetrics();
      if (line_metrics.empty()) {
        return;
      }
      auto parent_run_metrics = line_metrics.front().run_metrics;
      if (parent_run_metrics.empty()) {
        return;
      }
      auto box = parent_paragraph->GetRectsForRange(
          0, 1, txt::Paragraph::RectHeightStyle::kTight,
          txt::Paragraph::RectWidthStyle::kTight);
      if (box.empty()) {
        return;
      }
      FontMetrics parent_metrics{
          0 - parent_paragraph->GetLineMetrics().front().ascent,
          parent_paragraph->GetLineMetrics().front().descent,
          parent_run_metrics.begin()->second.font_metrics.fXHeight,
          measure_node_->text_style_->line_height.value_or(0.f),
          0.f,
          static_cast<float>(parent_paragraph->GetHeight()),
          box[0].rect.Top(),
          box[0].rect.Bottom()};
      if (child->IsInlineTextShadowNode()) {
        static_cast<InlineTextShadowNode*>(child)->EnsureDefaultStyle();
        TextStyle child_text_style =
            static_cast<InlineTextShadowNode*>(child)->text_style_.value();
        auto child_paragraph = LayoutXCharacter(layout_width, child_text_style);
        auto child_line_metrics = child_paragraph->GetLineMetrics();
        if (child_line_metrics.empty()) {
          continue;
        }
        auto child_run_metrics = child_line_metrics.front().run_metrics;
        if (child_run_metrics.empty()) {
          continue;
        }
        child->SetBaselineOffset(
            parent_metrics,
            child_run_metrics.begin()->second.font_metrics.fDescent,
            child_run_metrics.begin()->second.font_metrics.fAscent);
      } else {
        // inline-image need to get real width and height from lynx
        if (child->IsInlineImageShadowNode()) {
          child->UpdateLayoutStylesFromLynx();
        }
        child->SetBaselineOffset(
            parent_metrics, 0,
            0 - (child->Height() + child->MarginTop() + child->MarginBottom()));
      }
    }
  }
}
#endif

std::shared_ptr<txt::Paragraph> TextRender::LayoutXCharacter(
    double layout_width, const TextStyle& style) {
  auto builder = std::make_unique<TextParagraphBuilder>(true, style);
  builder->PushStyle(style);
  // To get the x-height we default the layout "x"
  std::u16string text = u"x";
  builder->AddText(text);
  auto paragraph = Build(std::move(builder));
  paragraph->Layout(layout_width);
  return paragraph;
}

float TextRender::GetMaxFontSize() {
  auto res = measure_node_->text_style_->font_size.value();
  for (auto* child : measure_node_->GetChildren()) {
    if (child->IsInlineTextShadowNode()) {
      static_cast<InlineTextShadowNode*>(child)->EnsureDefaultStyle();
      res = std::max(res, static_cast<InlineTextShadowNode*>(child)
                              ->text_style_->font_size.value());
    }
  }
  return res;
}

void TextRender::HandleAutoSize(const MeasureConstraint& constraint,
                                ShadowLayoutContextMeasure* context) {
  if (measure_node_->enable_auto_font_size_) {
    if (!CheckTextFullyDisplayed(constraint, context)) {
      TryShrinkFontSize(constraint, context);
    } else {
      TryExpandFontSize(constraint, context);
    }
  }
}

bool TextRender::CheckTextFullyDisplayed(
    const MeasureConstraint& constraint,
    ShadowLayoutContextMeasure* context_measure) {
  bool exceed_maxlines = measure_node_->text_style_->max_lines.has_value() &&
                         cache_paragraph_->DidExceedMaxLines();
  bool height_has_overflow =
      constraint.height_mode != TextMeasureMode::kIndefinite &&
      context_measure->measured_height_ > constraint.height;
  bool width_has_overflow = false;
  if (measure_node_->text_style_->white_space &&
      measure_node_->text_style_->white_space == WhiteSpace::kNoWrap) {
    width_has_overflow =
        cache_paragraph_->GetMinIntrinsicWidth() > constraint.width;
  }
  return !(exceed_maxlines || height_has_overflow || width_has_overflow);
}

void TextRender::TryShrinkFontSize(
    const MeasureConstraint& constraint,
    ShadowLayoutContextMeasure* context_measure) {
  // FIXME: if the number of auto_font_size_preset_sizes_ is too much, we
  // should use dichotomy to find target
  if (!measure_node_->auto_font_size_preset_sizes_.empty()) {
    if (measure_node_->text_style_->font_size <=
        measure_node_->auto_font_size_preset_sizes_.front()) {
      return;
    } else {
      auto sizes = measure_node_->auto_font_size_preset_sizes_.size();
      for (size_t i = 0; i < sizes; i++) {
        if (measure_node_->text_style_->font_size <
            measure_node_->auto_font_size_preset_sizes_[sizes - i - 1])
          continue;
        measure_node_->text_style_->font_size =
            measure_node_->auto_font_size_preset_sizes_[sizes - i - 1];
        update_flag_ = TextUpdateFlag::kUpdateFlagStyle;
        BuildTextLayout(constraint, context_measure);
        if (CheckTextFullyDisplayed(constraint, context_measure)) {
          return;
        }
      }
    }
    return;
  }

  if (measure_node_->text_style_->font_size <=
      measure_node_->auto_font_size_min_size_) {
    measure_node_->text_style_->font_size =
        measure_node_->auto_font_size_min_size_;
  } else if (measure_node_->text_style_->font_size >=
                 measure_node_->auto_font_size_max_size_ &&
             measure_node_->auto_font_size_max_size_ >
                 measure_node_->auto_font_size_min_size_) {
    measure_node_->text_style_->font_size =
        measure_node_->auto_font_size_max_size_;
  }
  while (measure_node_->text_style_->font_size >=
         measure_node_->auto_font_size_min_size_) {
    measure_node_->text_style_->font_size =
        measure_node_->text_style_->font_size.value_or(
            kDefaultFontSizeInDip * measure_node_->Logical2ClayPixelRatio()) -
        measure_node_->auto_font_size_step_granularity_;
    FlexInlineFontSize(true, measure_node_->text_style_->font_size.value(),
                       measure_node_);
    if (measure_node_->text_style_->font_size >=
        measure_node_->auto_font_size_min_size_) {
      update_flag_ = TextUpdateFlag::kUpdateFlagStyle;
      BuildTextLayout(constraint, context_measure);
      if (CheckTextFullyDisplayed(constraint, context_measure)) {
        return;
      }
    }
  }
}

void TextRender::TryExpandFontSize(
    const MeasureConstraint& constraint,
    ShadowLayoutContextMeasure* context_measure) {
  // FIXME: if the number of auto_font_size_preset_sizes_ is too much, we
  // should use dichotomy to find target
  if (!measure_node_->auto_font_size_preset_sizes_.empty()) {
    if (measure_node_->text_style_->font_size >=
        measure_node_->auto_font_size_preset_sizes_.back()) {
      return;
    } else {
      for (auto auto_font_size_preset_size :
           measure_node_->auto_font_size_preset_sizes_) {
        if (measure_node_->text_style_->font_size >=
            auto_font_size_preset_size) {
          continue;
        }
        auto pre_paragraph = std::move(cache_paragraph_);
        auto original_font_size = measure_node_->text_style_->font_size;
        measure_node_->text_style_->font_size = auto_font_size_preset_size;
        update_flag_ = TextUpdateFlag::kUpdateFlagStyle;
        BuildTextLayout(constraint, context_measure);
        if (!CheckTextFullyDisplayed(constraint, context_measure)) {
          measure_node_->text_style_->font_size = original_font_size;
          cache_paragraph_ = std::move(pre_paragraph);
          return;
        }
      }
    }
    return;
  }

  if (measure_node_->text_style_->font_size <=
      measure_node_->auto_font_size_min_size_) {
    measure_node_->text_style_->font_size =
        measure_node_->auto_font_size_min_size_;
  } else if (measure_node_->text_style_->font_size >=
                 measure_node_->auto_font_size_max_size_ &&
             measure_node_->auto_font_size_max_size_ >
                 measure_node_->auto_font_size_min_size_) {
    measure_node_->text_style_->font_size =
        measure_node_->auto_font_size_max_size_;
  }
  while (measure_node_->text_style_->font_size <=
         measure_node_->auto_font_size_max_size_) {
    std::unique_ptr<txt::Paragraph> pre_paragraph = std::move(cache_paragraph_);
    measure_node_->text_style_->font_size =
        measure_node_->text_style_->font_size.value_or(
            kDefaultFontSizeInDip * measure_node_->Logical2ClayPixelRatio()) +
        measure_node_->auto_font_size_step_granularity_;
    FlexInlineFontSize(false, measure_node_->text_style_->font_size.value(),
                       measure_node_);
    if (measure_node_->text_style_->font_size <=
        measure_node_->auto_font_size_max_size_) {
      update_flag_ = TextUpdateFlag::kUpdateFlagStyle;
      BuildTextLayout(constraint, context_measure);
      if (!CheckTextFullyDisplayed(constraint, context_measure)) {
        measure_node_->text_style_->font_size =
            measure_node_->text_style_->font_size.value_or(
                kDefaultFontSizeInDip *
                measure_node_->Logical2ClayPixelRatio()) -
            measure_node_->auto_font_size_step_granularity_;
        cache_paragraph_ = std::move(pre_paragraph);
        return;
      }
    } else {
      cache_paragraph_ = std::move(pre_paragraph);
    }
  }
}

void TextRender::FlexInlineFontSize(bool shrink_or_expand, float font_size,
                                    ShadowNode* shadow_node) {
  for (auto* child : shadow_node->GetChildren()) {
    if (child->IsInlineTextShadowNode()) {
      auto inline_text_child = static_cast<InlineTextShadowNode*>(child);
      if ((shrink_or_expand &&
           inline_text_child->text_style_->font_size.value() > font_size) ||
          (!shrink_or_expand &&
           inline_text_child->text_style_->font_size.value() < font_size)) {
        static_cast<InlineTextShadowNode*>(child)->text_style_->font_size =
            font_size;
      }
      FlexInlineFontSize(shrink_or_expand, font_size, child);
    } else if (child->IsInlineViewShadowNode()) {
      FlexInlineFontSize(shrink_or_expand, font_size, child);
    }
  }
}

void TextRender::HandleInlineTruncation(const MeasureConstraint& constraint,
                                        ShadowLayoutContextMeasure* context) {
  for (auto child : measure_node_->GetChildren()) {
    if (child->IsInlineTruncationShadowNode()) {
      auto truncation_node = static_cast<InlineTruncationShadowNode*>(child);
      if (cache_paragraph_ &&
          (cache_paragraph_->DidExceedMaxLines() ||
           (constraint.height_mode != MeasureMode::kIndefinite &&
            cache_paragraph_->GetHeight() > constraint.height &&
            cache_paragraph_->GetLineMetrics().size() > 1))) {
        FloatSize truncation_size = truncation_node->CalculateTruncatedSize();
        if (truncation_size.width() > constraint.width) {
          truncation_node->SetNeedMount(false);
          return;
        }
        truncation_node->SetNeedLayout(true);
        truncation_node->SetNeedMount(true);
#ifndef CLAY_ENABLE_TTTEXT
        auto end_dx = truncation_direction_ == TextDirection::kRtl
                          ? truncation_size.width()
                          : cache_paragraph_->GetMaxIntrinsicWidth() -
                                truncation_size.width();
#else
        auto end_dx =
            cache_paragraph_->GetMaxIntrinsicWidth() - truncation_size.width();
#endif
        auto line_metrics = cache_paragraph_->GetLineMetrics();
        if (line_metrics.empty()) {
          truncation_node->SetNeedMount(false);
          return;
        }
        auto last_line_height = line_metrics.back().height;
        auto end_dy = constraint.height_mode == MeasureMode::kIndefinite
                          ? cache_paragraph_->GetHeight()
                          : std::min<double>(cache_paragraph_->GetHeight(),
                                             constraint.height.value_or(0)) -
                                last_line_height + kLayoutTolerance;
        auto end_glyph_index =
            cache_paragraph_->GetGlyphPositionAtCoordinate(end_dx, end_dy);
        auto end_glyph_boxes = cache_paragraph_->GetRectsForRange(
            end_glyph_index.position - 1, end_glyph_index.position,
            txt::Paragraph::RectHeightStyle::kTight,
            txt::Paragraph::RectWidthStyle::kTight);
        size_t display_glyph_num = end_glyph_index.position;
        if (!end_glyph_boxes.empty()) {
          auto glyph_box = end_glyph_boxes.front();
          if (glyph_box.rect.Right() >
              prev_layout_width_ - truncation_size.width()) {
            display_glyph_num = end_glyph_index.position - 1;
          }
        }
        const size_t visible_glyph_num = display_glyph_num;
        ProcessTruncationContent(display_glyph_num, measure_node_);
        inline_truncation_hidden_count_ =
            static_cast<int>(end_glyph_position_ > visible_glyph_num
                                 ? end_glyph_position_ - visible_glyph_num
                                 : 0);
        measure_node_->text_style_->overflow = TextOverflow::kClip;
        update_flag_ = TextUpdateFlag::kUpdateFlagStyle;
        BuildTextLayout(constraint, context);
        for (auto truncation_child : child->GetChildren()) {
          if (truncation_child->IsInlineTextShadowNode()) {
            static_cast<InlineTextShadowNode*>(truncation_child)
                ->LayoutRange(cache_paragraph_.get());
          }
        }
        truncation_node->UpdateTruncatedSize(truncation_size.width(),
                                             truncation_size.height());
        truncation_node->SetNeedLayout(false);
        return;
      } else {
        truncation_node->SetNeedMount(false);
      }
      return;
    }
  }
}

void TextRender::ProcessTruncationContent(size_t& display_glyph_num,
                                          ShadowNode* node) {
  for (auto* child : node->GetChildren()) {
    if (child->IsRawTextShadowNode()) {
      auto* raw_text_node = static_cast<RawTextShadowNode*>(child);
      auto layout_text_length = raw_text_node->GetLayoutTextLength();
      if (layout_text_length < display_glyph_num) {
        display_glyph_num = display_glyph_num - layout_text_length;
      } else {
        raw_text_node->SetEndIndex(
            raw_text_node->GetRawEndIndexForLayoutTextLength(
                display_glyph_num));
        measure_node_->MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagChildren);
        display_glyph_num = 0;
      }
    } else if (child->IsInlineTextShadowNode()) {
      ProcessTruncationContent(display_glyph_num, child);
    } else if (child->IsInlineImageShadowNode() ||
               child->IsInlineViewShadowNode()) {
      if (display_glyph_num > 0) {
        display_glyph_num--;
      } else {
        child->SetEndIndex(0);
      }
    }
  }
}

}  // namespace clay
