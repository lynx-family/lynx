// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inline_image_shadow_node.h"

#include "clay/ui/component/inline_image_view.h"
#include "clay/ui/rendering/render_image.h"
#include "clay/ui/shadow/shadow_layout_context.h"

namespace clay {

InlineImageShadowNode::InlineImageShadowNode(ShadowNodeOwner* owner,
                                             std::string tag, int id)
    : ShadowNode(owner, tag, id) {}

void InlineImageShadowNode::PreLayout(PreLayoutContext* context) {
  auto* context_text = static_cast<PreShadowLayoutContextText*>(context);
  if (GetEndIndex() > 0) {
    context_text->CollectInlineImages(this);
  }
}

void InlineImageShadowNode::TextLayout(LayoutContext* context) {
  if (GetEndIndex() == 0) {
    placeholder_index_ = -1;
    return;
  }
  TextParagraphBuilder* builder =
      static_cast<LayoutContextText*>(context)->builder();
  builder->PushStyle(text_style_.value());
  const auto vertical_align = GetVerticalAlign();
  const auto placeholder_alignment =
      vertical_align.has_value() && vertical_align->type == kVerticalAlignMiddle
          ? txt::PlaceholderAlignment::kMiddle
          : txt::PlaceholderAlignment::kBaseline;
  txt::PlaceholderRun placeholder(
      Width() + MarginLeft() + MarginRight(),
      Height() + MarginTop() + MarginBottom(), placeholder_alignment,
      txt::TextBaseline::kAlphabetic,
      Height() + MarginTop() + MarginBottom() + baseline_offset_);
  auto* text_context = static_cast<LayoutContextText*>(context);
  start_glyph_ = text_context->TextSizeIncludingPlaceholders();
  end_glyph_ = start_glyph_ + 1;
  placeholder_index_ = text_context->AddPlaceholder(placeholder);
  builder->Pop();
}

}  // namespace clay
