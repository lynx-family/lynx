// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_PARAGRAPH_BUILDER_TT_TEXT_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_PARAGRAPH_BUILDER_TT_TEXT_H_

#include <memory>
#include <vector>
#include "paragraph_tt_text.h"
#ifdef ENABLE_SKITY
#include "txt/font_collection_skity.h"
#else
#include "txt/font_collection.h"
#endif
#include <textra/paragraph_style.h>
#include "txt/paragraph_builder.h"

namespace ttoffice {
namespace textlayout {
class Paragraph;
class Style;
}  // namespace textlayout
}  // namespace ttoffice
namespace txt {
class ParagraphBuilderTTText : public ParagraphBuilder {
 public:
  ParagraphBuilderTTText(
      const ParagraphStyle& style,
      const std::shared_ptr<FontCollection>& font_collection);

  ~ParagraphBuilderTTText() override;

  void PushStyle(const TextStyle& style) override;
  void Pop() override;
  const TextStyle& PeekStyle() override;
  void AddText(const std::u16string& text) override;
  void AddPlaceholder(PlaceholderRun& span) override;
  std::unique_ptr<Paragraph> Build() override;

  void AddText(const char* text, size_t len);
  tttext::ParagraphStyle GetTTParagraphStyle() { return paragraph_style_; }

 private:
  tttext::Style ToTTStyle(const TextStyle& text_style);
  void ToTTParaStyle(const ParagraphStyle& para_style);
  void CreateParagraph();

 private:
  std::shared_ptr<FontCollection> font_collection_;
  std::vector<TextStyle> text_style_stack_;
  std::vector<tttext::Style> run_style_stack_;
  std::unique_ptr<ParagraphTTText> paragraph_;
  tttext::ParagraphStyle paragraph_style_;
};
}  // namespace txt
#endif  // CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_PARAGRAPH_BUILDER_TT_TEXT_H_
