// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inner_text_shadow_node.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/painter/gradient_factory.h"
#include "clay/ui/shadow/inner_inline_text_shadow_node.h"

namespace clay {

namespace {
constexpr float kLayoutTolerance = 1.f;
}

void InnerTextShadowNode::ProcessParagraph(
    ShadowLayoutContextMeasure* context_measure,
    std::unique_ptr<txt::Paragraph> paragraph) {
  // layout inline text
  for (auto child : children_) {
    if (child->IsInlineTextShadowNode()) {
      static_cast<InnerInlineTextShadowNode*>(child)->LayoutRange(
          paragraph.get());
    }
  }

  if (context_measure) {
    measured_width_ = std::ceil(paragraph->GetMaxIntrinsicWidth());
    measured_height_ = std::ceil(paragraph->GetHeight());
  }
  SetCacheParagraph(std::move(paragraph));
}

void InnerTextShadowNode::TextLayout(LayoutContext* context) {
  auto* context_measure = static_cast<ShadowLayoutContextMeasure*>(context);
  auto layout_width = context_measure->layout_width_;

  const bool force_rebuild = update_flag_ != kUpdateFlagNone;
  const bool should_layout =
      force_rebuild || prev_layout_width_ != layout_width;

  if (force_rebuild) {
    PreLayout(nullptr);
  }

  std::unique_ptr<txt::Paragraph> paragraph;
  if (should_layout) {
    auto builder = std::make_unique<TextParagraphBuilder>(false, text_style_);
    LayoutContextText context_text;
    context_text.SetBuilder(builder.get());
    builder->PushStyle(text_style_.value());
    BaseTextShadowNode::TextLayout(&context_text);
    builder->Pop();
    paragraph = Build(std::move(builder));
    paragraph->Layout(layout_width);
    ProcessParagraph(context_measure, std::move(paragraph));
  }

  if (context_measure) {
    context_measure->measured_width_ = measured_width_;
    context_measure->measured_height_ = measured_height_;
  }

  update_flag_ = kUpdateFlagNone;
  prev_layout_width_ = layout_width;
}

MeasureResult InnerTextShadowNode::Measure(
    const MeasureConstraint& constraint) {
  MeasureResult result;
  if (!constraint.IsValid()) {
    FML_DLOG(WARNING) << "Invalid measure metrics.";
    result.width = 0;
    result.height = 0;
    if (text_view_) {
      text_view_->SetWidth(result.width);
      text_view_->SetHeight(result.height);
    } else {
      FML_DLOG(ERROR) << "The text view is null";
    }
    return result;
  }
  constraint_ = constraint;

  ShadowLayoutContextMeasure context;
  switch (constraint.width_mode) {
    case TextMeasureMode::kIndefinite:
      context.layout_width_ = std::numeric_limits<float>::infinity();
      break;
    case TextMeasureMode::kDefinite:
    case TextMeasureMode::kAtMost:
      context.layout_width_ = *constraint.width;
      break;
  }

  TextLayout(&context);

  if (constraint.width_mode == TextMeasureMode::kIndefinite) {
    if (GetResolvedTextAlign() != TextAlignment::kLeft) {
      // Do second layout if the width mode is indefinite and the alignment is
      // not left. Because in this case, the paragraph_ won't have the actual
      // line and nothing will be painted.
      context.layout_width_ = context.measured_width_;
      TextLayout(&context);
    }
  }

  if (constraint.width_mode == TextMeasureMode::kAtMost &&
      context.measured_width_ + kLayoutTolerance < context.layout_width_) {
    context.layout_width_ = context.measured_width_;
    // Do second layout if actual text width is less than constraints.
    // For example, given at most 200px width and actually 100px is needed,
    // use 100px to layout again. Otherwise text align will be problem.
    // Note: Here maybe some optimizations to avoid second layout.
    TextLayout(&context);
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
  switch (constraint.height_mode) {
    case TextMeasureMode::kIndefinite: {
      result.height = desired_height;
      break;
    }
    case TextMeasureMode::kDefinite: {
      result.height = *constraint.height;
      break;
    }
    case TextMeasureMode::kAtMost: {
      result.height = std::min(std::ceil(static_cast<float>(desired_height)),
                               *constraint.height);
      break;
    }
  }

  if (text_view_) {
    text_view_->SetWidth(result.width);
    text_view_->SetHeight(result.height);
    text_view_->SetParagraph(GetCacheParagraph(), GetRawText());
    if (text_style_->text_gradient) {
      text_view_->GetRenderText()->SetGradient(text_style_->text_gradient);
    } else {
      text_view_->GetRenderText()->SetGradient(std::nullopt);
    }
  } else {
    FML_DLOG(ERROR) << "The view corresponding to the node cannot be found";
  }

  return result;
}

}  // namespace clay
