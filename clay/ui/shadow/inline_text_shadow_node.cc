// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inline_text_shadow_node.h"

#include <cstring>
#include <string>
#include <utility>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/rendering/text/render_inline_text.h"

namespace clay {

InlineTextShadowNode::InlineTextShadowNode(ShadowNodeOwner* owner,
                                           std::string tag, int id)
    : BaseTextShadowNode(owner, tag, id) {
  text_style_->text_color.reset();
}

InlineTextShadowNode::~InlineTextShadowNode() = default;

void InlineTextShadowNode::SetAttribute(const char* attr_c,
                                        const clay::Value& value) {
  if (tag_ == "x-inline-text" && std::strcmp(attr_c, "no-trim") == 0) {
    const bool no_trim = attribute_utils::GetBool(value, true);
    if (no_trim_ != no_trim) {
      no_trim_ = no_trim;
      MarkDirty();
    }
    return;
  }
  BaseTextShadowNode::SetAttribute(attr_c, value);
}

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
