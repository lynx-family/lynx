// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/inline_text_view.h"

#include <memory>

#include "clay/ui/rendering/text/render_inline_text.h"

namespace clay {

InlineTextView::InlineTextView(int id, PageView* page_view)
    : WithTypeInfo(id, "inline-text", std::make_unique<RenderInlineText>(),
                   page_view) {}

InlineTextView::~InlineTextView() = default;

BaseView* InlineTextView::GetDeepestViewInPos(
    txt::Paragraph::PositionWithAffinity text_pos) {
  for (const auto& child : children_) {
    if (child->Is<InlineTextView>()) {
      auto view =
          static_cast<InlineTextView*>(child)->GetDeepestViewInPos(text_pos);
      if (view != nullptr) {
        return view;
      }
    }
  }
  for (const auto& range : range_in_paragraph_) {
    auto pos = text_pos.position;
    if (text_pos.affinity == txt::Paragraph::UPSTREAM && pos > 0) {
      pos--;
    }
    if (pos >= range.start() && pos < range.end()) {
      return this;
    }
  }
  return nullptr;
}

}  // namespace clay
