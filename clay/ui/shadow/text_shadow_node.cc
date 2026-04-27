// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/text_shadow_node.h"

#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "clay/fml/logging.h"
#include "clay/public/value.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/painter/gradient_factory.h"
#include "clay/ui/rendering/text/render_text.h"
#include "clay/ui/shadow/base_text_shadow_node.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_truncation_shadow_node.h"
#include "clay/ui/shadow/measure_utils.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/vertical_align_style.h"
#if defined(CLAY_ENABLE_SKSHAPER)
#include "clay/third_party/txt/src/skia/paragraph_skia.h"
#endif
#if defined(CLAY_ENABLE_TTTEXT)
#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#endif
#include "base/trace/native/trace_event.h"

namespace clay {

namespace {

static constexpr float kDefaultFontSizeInDip = 14.f;

}  // namespace

TextShadowNode::TextShadowNode(ShadowNodeOwner* owner, std::string tag, int id)
    : BaseTextShadowNode(owner, tag, id) {
  text_render_ = std::make_unique<TextRender>(this);
}

TextShadowNode::~TextShadowNode() { text_render_ = nullptr; }

void TextShadowNode::OnLayout(float width, TextMeasureMode width_mode,
                              float height, TextMeasureMode height_mode,
                              const std::array<float, 4>& paddings,
                              const std::array<float, 4>& borders) {
  BaseTextShadowNode::OnLayout(width, width_mode, height, height_mode, paddings,
                               borders);
  auto paragraph = text_render_->GetCacheParagraph();
  if (!paragraph) {
    return;
  }
  if (HasEvent(event_attr::kEventLayout)) {
    std::vector<LineInfo> lines_info = text_render_->GetLineInfo();
    auto lines_size = lines_info.size();
    clay::Value::Array array(lines_size);
    for (size_t i = 0; i < lines_size; i++) {
      auto map = CreateClayMap({"ellipsisCount", "end", "start"},
                               lines_info[i].ellipsis_count, lines_info[i].end,
                               lines_info[i].start);
      array[i] = clay::Value(std::move(map));
    }
    clay::Value::Map map;
    map["lineCount"] = clay::Value(static_cast<int>(lines_size));
    map["lines"] = clay::Value(std::move(array));
    map["size"] = clay::Value(CreateClayMap(
        {"width", "height"}, paragraph->GetMaxWidth(), paragraph->GetHeight()));
    owner_->GetViewContext()->GetPageView()->SendCustomEvent(
        id(), event_attr::kEventLayout, std::move(map));
  }

  UpdateBundleData();
}

void TextShadowNode::MarkDirty() {
  DidUpdateStyle();
  ShadowNode::MarkDirty();
}

void TextShadowNode::UpdateBundleData() {
  if (!owner_ || !text_render_->GetCacheParagraph()) {
    return;
  }

  if (!bundle_) {
    CreateTextBundle();
  }

  auto paragraph = text_render_->GetCacheParagraph();
  // Traverse the tree structure to create textinfo
  CreateTextInfo(paragraph);

  auto bundle = static_cast<TextUpdateBundle*>(bundle_.get());
  bundle->SetParagraph(text_render_->MoveParagraph());
  bundle->SetText(GetRawText());
  bundle->SetTextPaintAlign(text_paint_align_);
  bundle->SetInlineEmojiInfo(inline_emoji_info_);
  if (text_style_) {
    std::map<int, std::shared_ptr<ColorSource>> gradient_shader_map;
    std::map<int, std::pair<size_t, size_t>> gradient_shader_range_map;
    CreateGradientShaderMap(this, paragraph, gradient_shader_map,
                            gradient_shader_range_map);
    if (gradient_shader_map.size() > 0) {
      bundle->SetGradientShaderMap(gradient_shader_map,
                                   gradient_shader_range_map);
    }
    std::unordered_map<int, TextStroke> text_stroke_map;
    CreateTextStrokeMap(this, text_stroke_map);
    if (text_stroke_map.size() > 0) {
      bundle->SetTextStrokeMap(text_stroke_map);
    }
  }
}

void TextShadowNode::CreateGradientShaderMap(
    ShadowNode* node, txt::Paragraph* paragraph,
    std::map<int, std::shared_ptr<ColorSource>>& map,
    std::map<int, std::pair<size_t, size_t>>& range_map) {
  if (!paragraph || !node) {
    return;
  }
  if (node->IsTextShadowNode()) {
    auto text_node = static_cast<TextShadowNode*>(node);
    if (text_node->text_style_->text_gradient &&
        text_node->text_style_->foreground_id.has_value()) {
      auto gradient_shader = GradientFactory::CreateShader(
          text_node->text_style_->text_gradient.value(),
          FloatRect(0, 0, paragraph->GetLongestLine(), paragraph->GetHeight()));
      map[text_node->text_style_->foreground_id.value()] = gradient_shader;
      size_t text_length = 0;
#if defined(CLAY_ENABLE_TTTEXT)
      auto* impl = static_cast<txt::ParagraphTTText*>(paragraph);
      text_length = impl->GetTextSize();
#endif
      range_map.emplace(text_node->text_style_->foreground_id.value(),
                        std::pair{0, text_length});
    }
  }
  if (node->IsInlineTextShadowNode()) {
    auto inline_text_node = static_cast<InlineTextShadowNode*>(node);
    if (inline_text_node->text_style_ &&
        inline_text_node->text_style_->text_gradient &&
        inline_text_node->text_style_->foreground_id.has_value()) {
      auto boxes = paragraph->GetRectsForRange(
          inline_text_node->range_in_paragraph_.front().start(),
          inline_text_node->range_in_paragraph_.back().end(),
          RectHeightStyle::kTight, RectWidthStyle::kTight);
      if (!boxes.empty()) {
        auto res_rect = boxes.front().rect;
        for (auto box : boxes) {
          res_rect.Join(box.rect);
        }
        auto gradient_shader = GradientFactory::CreateShader(
            inline_text_node->text_style_->text_gradient.value(),
            FloatRect(res_rect));
        map[inline_text_node->text_style_->foreground_id.value()] =
            gradient_shader;
        range_map.emplace(
            inline_text_node->text_style_->foreground_id.value(),
            std::pair{
                inline_text_node->range_in_paragraph_.front().start(),
                inline_text_node->range_in_paragraph_.back().end(),
            });
      }
    }
  }
  if (node->IsInlineViewShadowNode()) {
    return;
  }
  for (auto* child : node->GetChildren()) {
    CreateGradientShaderMap(child, paragraph, map, range_map);
  }
}

void TextShadowNode::CreateTextStrokeMap(
    ShadowNode* node, std::unordered_map<int, TextStroke>& map) {
  if (node->IsBaseTextShadowNode()) {
    auto text_node = static_cast<BaseTextShadowNode*>(node);
    if ((text_node->stroke_color_ || text_node->stroke_width_ > 0) &&
        text_node->text_style_->foreground_id.has_value()) {
      TextStroke text_stroke;
      text_stroke.fill_color =
          text_node->text_style_->text_color.value_or(Color(0xFF000000));
      text_stroke.stroke_color =
          text_node->stroke_color_.value_or(text_stroke.fill_color);
      text_stroke.width = text_node->stroke_width_;
      map[text_node->text_style_->foreground_id.value()] = text_stroke;
    }
  }
  for (auto* child : node->GetChildren()) {
    CreateTextStrokeMap(child, map);
  }
}

void TextShadowNode::ProcessParagraph(
    const MeasureConstraint& constraint,
    ShadowLayoutContextMeasure* context_measure, txt::Paragraph* paragraph) {
  LayoutInlineText(paragraph);

  if (context_measure && owner_) {
    auto line_metrics = paragraph->GetLineMetrics();
#ifndef CLAY_ENABLE_TTTEXT
    auto line_spacing = std::max(
        text_style_->line_spacing.value_or(0) -
            (line_metrics.size() > 0
                 ? line_metrics.begin()->ascent - line_metrics.begin()->descent
                 : 0),
        0.0);
    static_cast<TextUpdateBundle*>(bundle_.get())
        ->SetLineSpacingOffset(-line_spacing / 2);
#endif
  }
}

// Collect the inline-image views in this phase.
void TextShadowNode::PreLayout(PreLayoutContext* context) {
  PreShadowLayoutContextText context_text;
  BaseTextShadowNode::PreLayout(&context_text);
  inline_images_ = context_text.TakeInlineImages();
  inline_views_ = context_text.TakeInlineViews();
}

void TextShadowNode::TextLayout(LayoutContext* context) {
  BaseTextShadowNode::TextLayout(context);
}

void TextShadowNode::LayoutInlineText(txt::Paragraph* paragraph) {
  for (auto child : children_) {
    if (child->IsInlineTextShadowNode() && owner_) {
      static_cast<InlineTextShadowNode*>(child)->LayoutRange(paragraph);
    }
  }
}

MeasureResult TextShadowNode::Measure(const MeasureConstraint& constraint) {
  MeasureResult result;
  TRACE_EVENT("clay", "TextShadowNode::Measure");
  CreateTextBundle();
  if (!constraint.IsValid()) {
    // Even when the `MeasureConstraint` value is zero, the text still needs to
    // be measured Compatible with the end-side rendering behavior.
    FML_DLOG(WARNING) << "Invalid measure metrics.";
  }
  width_mode_ = constraint.width_mode;
  auto context = CreateLayoutContext(constraint);

#ifndef CLAY_ENABLE_TTTEXT
  UpdateLineHeight();
#endif

  // inline view measure
  MeasureInlineView(constraint);

  ResetEndIndex();
  text_render_->Measure(constraint, &context);

  if (auto paragraph = text_render_->GetCacheParagraph()) {
    ProcessParagraph(constraint, &context, paragraph);
  }

  if (constraint.width_mode == TextMeasureMode::kDefinite) {
    result.width = *constraint.width;
  } else if (constraint.width_mode == TextMeasureMode::kIndefinite) {
    result.width = std::ceil(context.measured_width_);
  } else {
    result.width =
        std::min(std::ceil(static_cast<float>(context.measured_width_)),
                 *constraint.width);
  }

  auto desired_height = context.measured_height_;
  // if TextMeasureMode is kAtMost and the desired height triggers
  // flex shrink, lynx will calculate the ratio of every flex element
  // and relayout the paragraph by invoking a new Measure while
  // constraint.width_mode == TextMeasureMode::kDefinite
  switch (constraint.height_mode) {
    case TextMeasureMode::kAtMost:
    case TextMeasureMode::kIndefinite: {
      result.height = desired_height;
      break;
    }
    case TextMeasureMode::kDefinite: {
      result.height = *constraint.height;
      break;
    }
  }

  if (need_second_layout_) {
    need_second_layout_ = false;
    auto constraint =
        MeasureConstraint({result.width, TextMeasureMode::kDefinite,
                           result.height, TextMeasureMode::kDefinite});
    context = CreateLayoutContext(constraint);
    text_render_->Measure(constraint, &context);
  }
  return result;
}

ShadowLayoutContextMeasure TextShadowNode::CreateLayoutContext(
    const MeasureConstraint& constraint) {
  ShadowLayoutContextMeasure context;
  switch (constraint.width_mode) {
    case TextMeasureMode::kIndefinite:
      context.layout_width_ = std::numeric_limits<float>::infinity();
      break;
    case TextMeasureMode::kAtMost:
    case TextMeasureMode::kDefinite:
      context.layout_width_ = *constraint.width;
      break;
  }
  return context;
}

void TextShadowNode::DidUpdateStyle() {
  MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagStyle);
}

void TextShadowNode::Align() {
  if (auto paragraph = text_render_->GetCacheParagraph()) {
    AlignNativeNode(paragraph);
  }
}

void TextShadowNode::UpdateLineHeight() {
  EnsureDefaultStyle();
  float line_height = 0.f;
  float font_size = text_style_->font_size.value_or(kDefaultFontSizeInDip *
                                                    Logical2ClayPixelRatio());
  CollectMaxLineHeight(line_height, font_size);
  if (line_height > 0) {
    text_style_->line_height = line_height / font_size;
  }
}

void TextShadowNode::SetUpdateFlag(TextUpdateFlag flag) {
  if (text_render_) {
    text_render_->SetUpdateFlag(flag);
  }
}

void TextShadowNode::AddUpdateFlag(TextUpdateFlag flag) {
  if (text_render_) {
    text_render_->AddUpdateFlag(flag);
  }
}

double TextShadowNode::GetBaseline() const {
  if (text_render_ && text_render_->GetCacheParagraph()) {
    return text_render_->GetCacheParagraph()->GetAlphabeticBaseline();
  }
  return 0;
}

txt::Paragraph* TextShadowNode::GetCacheParagraph() {
  if (text_render_) {
    return std::move(text_render_->GetCacheParagraph());
  } else {
    return nullptr;
  }
}
void TextShadowNode::SetCacheParagraph(
    std::unique_ptr<txt::Paragraph> paragraph) {
  if (text_render_) {
    text_render_->SetCacheParagraph(std::move(paragraph));
  }
}

TextAlignment TextShadowNode::GetResolvedTextAlign() {
  auto align = text_style_->text_align.value_or(TextAlignment::kStart);
  auto direction = text_style_->text_direction.value_or(TextDirection::kLtr);
  if (align == TextAlignment::kStart) {
    if (direction == TextDirection::kRtl) {
      return TextAlignment::kRight;
    } else {
      return TextAlignment::kLeft;
    }
  } else if (align == TextAlignment::kEnd) {
    if (direction == TextDirection::kRtl) {
      return TextAlignment::kLeft;
    } else {
      return TextAlignment::kRight;
    }
  }
  return align;
}

}  // namespace clay
