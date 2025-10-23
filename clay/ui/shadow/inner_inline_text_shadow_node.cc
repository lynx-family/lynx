// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inner_inline_text_shadow_node.h"

#include <string>

#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/rendering/text/render_inline_text.h"

namespace clay {

InnerInlineTextShadowNode::InnerInlineTextShadowNode(ShadowNodeOwner* owner,
                                                     std::string tag, int id)
    : InlineTextShadowNode(owner, tag, id) {}

void InnerInlineTextShadowNode::LayoutRange(txt::Paragraph* paragraph) {
  fml::TaskRunner::RunNowOrPostTask(
      inline_text_view_->page_view()->GetTaskRunner(), [this, paragraph]() {
        if (auto render_object = inline_text_view_->render_object()) {
          static_cast<RenderInlineText*>(render_object)->ClearTextBox();
          for (const auto& range : range_in_paragraph_) {
            auto boxes = paragraph->GetRectsForRange(
                range.start(), range.end(),
                txt::Paragraph::RectHeightStyle::kTight,
                txt::Paragraph::RectWidthStyle::kTight);
            for (auto& box : boxes) {
              static_cast<RenderInlineText*>(render_object)
                  ->AddTextBox(box.rect);
            }
          }
        } else {
          FML_DLOG(ERROR)
              << "The view corresponding to the node cannot be found";
        }
      });
  for (auto child : children_) {
    if (child->IsInlineTextShadowNode()) {
      static_cast<InnerInlineTextShadowNode*>(child)->LayoutRange(paragraph);
    }
  }
}

}  // namespace clay
