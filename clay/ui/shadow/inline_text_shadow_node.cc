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
    : BaseTextShadowNode(owner, tag, id) {}

InlineTextShadowNode::~InlineTextShadowNode() = default;

void InlineTextShadowNode::AddTextRange(size_t start, size_t end) {
  range_in_paragraph_.emplace_back(start, end);
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
  BaseTextShadowNode::TextLayout(context);
}

}  // namespace clay
