// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inline_text_shadow_node.h"

#include <string>
#include <utility>

#include "clay/ui/common/isolate.h"
#include "clay/ui/rendering/text/render_inline_text.h"

namespace clay {

InlineTextShadowNode::InlineTextShadowNode(ShadowNodeOwner* owner,
                                           std::string tag, int id)
    : BaseTextShadowNode(owner, tag, id) {
#if defined(CLAY_ENABLE_TTTEXT)
  text_style_->text_color.reset();
#endif
}

InlineTextShadowNode::~InlineTextShadowNode() = default;

void InlineTextShadowNode::AddTextRange(size_t start_utf32, size_t end_utf32) {
  range_in_paragraph_.emplace_back(start_utf32, end_utf32);
}

void InlineTextShadowNode::LayoutRange(txt::Paragraph* paragraph) {
  for (auto child : children_) {
    if (child->IsInlineTextShadowNode()) {
      static_cast<InlineTextShadowNode*>(child)->LayoutRange(paragraph);
    }
  }
}

void InlineTextShadowNode::TextLayout(LayoutContext* context) {
  range_in_paragraph_.clear();
#if defined(CLAY_ENABLE_TTTEXT)
  const bool use_default_text_color =
      !text_style_->text_color.has_value() &&
      text_style_->stroke_width.value_or(0.f) <= 0.f;
  if (use_default_text_color) {
    text_style_->text_color = Color::kBlack();
  }
#endif
  BaseTextShadowNode::TextLayout(context);
#if defined(CLAY_ENABLE_TTTEXT)
  if (use_default_text_color) {
    text_style_->text_color.reset();
  }
#endif
}

}  // namespace clay
