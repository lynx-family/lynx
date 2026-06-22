// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inline_view_shadow_node.h"

#include "clay/third_party/txt/src/txt/placeholder_run.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/shadow/shadow_layout_context.h"

namespace clay {

InlineViewShadowNode::InlineViewShadowNode(ShadowNodeOwner* owner,
                                           std::string tag, int id)
    : ShadowNode(owner, tag, id) {}

void InlineViewShadowNode::PreLayout(PreLayoutContext* context) {
  auto* context_text = static_cast<PreShadowLayoutContextText*>(context);
  if (GetEndIndex() > 0) {
    context_text->CollectInlineViews(this);
  }
}

void InlineViewShadowNode::TextLayout(LayoutContext* context) {
  if (GetEndIndex() == 0) {
    ResetTextLayout();
    return;
  }
  TextParagraphBuilder* builder =
      static_cast<LayoutContextText*>(context)->builder();
  builder->PushStyle(text_style_.value());
  txt::PlaceholderRun placeholder(
      Width(), Height(), txt::PlaceholderAlignment::kBaseline,
      txt::TextBaseline::kAlphabetic, Height() + baseline_offset_);
  start_glyph_ =
      static_cast<LayoutContextText*>(context)->TextSizeIncludingPlaceholders();
  end_glyph_ = start_glyph_ + 1;
  placeholder_index_ =
      static_cast<LayoutContextText*>(context)->AddPlaceholder(placeholder);
  builder->Pop();
}

void InlineViewShadowNode::ResetTextLayout() {
  placeholder_index_ = -1;
  start_glyph_ = 0;
  end_glyph_ = 0;
}

MeasureResult InlineViewShadowNode::MeasureNativeNode(
    const MeasureConstraint& constraint) {
  return owner_->MeasureNativeNode(this, constraint);
}

void InlineViewShadowNode::AlignNativeNode(float top, float left) {
  return owner_->AlignNativeNode(this, top, left);
}

}  // namespace clay
