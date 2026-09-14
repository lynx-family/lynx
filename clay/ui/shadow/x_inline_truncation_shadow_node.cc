// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/x_inline_truncation_shadow_node.h"

#include <utility>

#include "clay/ui/component/text/layout_context.h"

namespace clay {

XInlineTruncationShadowNode::XInlineTruncationShadowNode(ShadowNodeOwner* owner,
                                                         std::string tag,
                                                         int id)
    : InlineTruncationShadowNode(owner, std::move(tag), id) {}

void XInlineTruncationShadowNode::AppendTruncationPrefix(
    LayoutContextText* context) {
  auto* parent = Parent();
  if (!parent || !parent->IsBaseTextShadowNode() || !parent->text_style_ ||
      parent->text_style_->overflow != TextOverflow::kEllipsis) {
    return;
  }

  auto* builder = context->builder();
  builder->PushStyle(parent->text_style_.value());
  context->AddText(u"\u2026");
  builder->Pop();
}

}  // namespace clay
