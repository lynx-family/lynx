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
  txt::PlaceholderRun placeholder(
      Width(), Height() + MarginTop() + MarginBottom(),
      txt::PlaceholderAlignment::kBaseline, txt::TextBaseline::kAlphabetic,
      Height() + MarginTop() + MarginBottom() + baseline_offset_);
  start_glyph_ =
      static_cast<LayoutContextText*>(context)->TextSizeIncludingPlaceholders();
  end_glyph_ = start_glyph_ + 1;
  placeholder_index_ =
      static_cast<LayoutContextText*>(context)->AddPlaceholder(placeholder);
}

}  // namespace clay
