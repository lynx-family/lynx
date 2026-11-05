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
  auto placeholder_style = text_style_.value();
  auto placeholder_alignment = tag_ == "x-inline-image"
                                   ? txt::PlaceholderAlignment::kMiddle
                                   : txt::PlaceholderAlignment::kBaseline;
#ifdef CLAY_ENABLE_TTTEXT
  if (baseline_offset_.has_value()) {
    // Preprocessing already encoded middle in the placeholder baseline.
    // Keep the node style intact for subsequent property updates.
    placeholder_style.align_type = kVerticalAlignBaseline;
    placeholder_style.baseline_shift = 0.f;
    placeholder_alignment = txt::PlaceholderAlignment::kBaseline;
  }
#endif
  builder->PushStyle(placeholder_style);
  txt::PlaceholderRun placeholder(
      Width(), Height() + MarginTop() + MarginBottom(), placeholder_alignment,
      txt::TextBaseline::kAlphabetic,
      Height() + MarginTop() + MarginBottom() + baseline_offset_.value_or(0.0));
  auto* text_context = static_cast<LayoutContextText*>(context);
  start_glyph_ = text_context->TextSizeIncludingPlaceholders();
  end_glyph_ = start_glyph_ + 1;
  placeholder_index_ = text_context->AddPlaceholder(placeholder);
  builder->Pop();
}

}  // namespace clay
