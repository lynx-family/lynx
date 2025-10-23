// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_INNER_INLINE_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_INNER_INLINE_TEXT_SHADOW_NODE_H_

#include <string>

#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"

namespace clay {

class InnerInlineTextShadowNode : public InlineTextShadowNode {
 public:
  explicit InnerInlineTextShadowNode(InlineTextView* inline_text_view)
      : InnerInlineTextShadowNode(nullptr, "inner-inline-text", -1) {
    inline_text_view_ = inline_text_view;
  }

  InnerInlineTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~InnerInlineTextShadowNode() = default;

  void LayoutRange(txt::Paragraph* paragraph);

 private:
  InlineTextView* inline_text_view_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_INNER_INLINE_TEXT_SHADOW_NODE_H_
