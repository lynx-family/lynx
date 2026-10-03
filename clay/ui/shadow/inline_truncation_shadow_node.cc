// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/inline_truncation_shadow_node.h"

#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/shadow/base_text_shadow_node.h"

namespace clay {

InlineTruncationShadowNode::InlineTruncationShadowNode(ShadowNodeOwner* owner,
                                                       std::string tag, int id)
    : BaseTextShadowNode(owner, tag, id) {}

void InlineTruncationShadowNode::TextLayout(LayoutContext* context) {
  if (need_layout_) {
    LayoutTruncation(context);
  }
}

void InlineTruncationShadowNode::LayoutTruncation(LayoutContext* context) {
  auto* text_context = static_cast<LayoutContextText*>(context);
  start_glyph_ = text_context->TextSizeIncludingPlaceholders();
  // Keep the ellipsis in the surrounding text style, before applying the
  // custom marker's own color, font, or inline children.
  text_context->AddText(ellipsis_);
  BaseTextShadowNode::TextLayout(context);
  end_glyph_ = text_context->TextSizeIncludingPlaceholders();
}

void InlineTruncationShadowNode::UpdateTruncatedSize(float width,
                                                     float height) {
  styles_.width = width;
  styles_.height = height;
}

FloatSize InlineTruncationShadowNode::CalculateTruncatedSize() {
  TRACE_EVENT("clay",
              "InlineTruncationShadowNode::CalculateTruncatedStringWidth");
  auto builder = std::make_unique<TextParagraphBuilder>(
      true, Parent() ? Parent()->text_style_ : text_style_);
  LayoutContextText context;
  context.SetBuilder(builder.get());
  LayoutTruncation(&context);
  auto paragraph = Build(std::move(builder));
  paragraph->Layout(std::numeric_limits<float>::infinity());
  return {static_cast<float>(paragraph->GetMaxIntrinsicWidth()),
          static_cast<float>(paragraph->GetHeight())};
}

}  // namespace clay
