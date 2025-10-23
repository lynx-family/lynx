// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_TEXT_PARAGRAPH_BUILDER_H_
#define CLAY_UI_COMPONENT_TEXT_TEXT_PARAGRAPH_BUILDER_H_

#include <memory>
#include <string>

#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/third_party/txt/src/txt/paragraph_builder.h"
#include "clay/ui/component/text/text_style.h"

namespace clay {

class TextParagraphBuilder;

std::unique_ptr<txt::Paragraph> Build(
    std::unique_ptr<TextParagraphBuilder> builder);

class TextParagraphBuilder {
 public:
  // Use TextStyle as ParagraphStyle for now.
  explicit TextParagraphBuilder(
      bool use_skia, const std::optional<TextStyle>& paragraph_style);
  ~TextParagraphBuilder();

  void PushStyle(const TextStyle& style);
  void Pop();
  void AddText(const std::u16string& text);
  void AddPlaceholder(txt::PlaceholderRun& placeholder);

 private:
  friend std::unique_ptr<txt::Paragraph> Build(
      std::unique_ptr<TextParagraphBuilder> builder);

  std::unique_ptr<txt::ParagraphBuilder> builder_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_TEXT_PARAGRAPH_BUILDER_H_
