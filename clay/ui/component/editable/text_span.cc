// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/text_span.h"

#include <memory>
#include <utility>

namespace clay {

TextSpan::TextSpan(std::u16string text, TextStyle text_style,
                   std::vector<std::shared_ptr<TextSpan>> children)
    : text_(text), text_style_(text_style), children_(std::move(children)) {}

void TextSpan::Build(TextParagraphBuilder& builder) {
  if (!text_.empty()) {
    builder.PushStyle(text_style_);
    builder.AddText(text_);
  }
  if (!children_.empty()) {
    for (auto& child : children_) {
      child->Build(builder);
    }
  }
  if (!text_.empty()) {
    builder.Pop();
  }
}

}  // namespace clay
