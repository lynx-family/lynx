// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_TEXTRA_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_TEXTRA_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "core/public/text_layout_impl.h"
#include "core/renderer/css/css_property_bitset.h"

namespace lynx {
namespace tasm {
class TextElement;
class FiberElement;

namespace text {
class TextLayoutAPI;
class ParagraphBuilder;
class Paragraph;
class ParagraphListener;
}  // namespace text

class TextLayoutTextra : public TextLayoutImpl {
 public:
  explicit TextLayoutTextra(intptr_t textra);
  ~TextLayoutTextra() override;

  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode) override;

  void Align(Element* element) override;

  void DispatchLayoutBefore(Element* element) override;

  void Destroy(Element* element) override;

  void BuildParagraphRecursively(Element* element, bool& has_inline_view);

 private:
  void ApplyTextStyle(TextElement* element);
  void ApplyTextStyle(Element* element, const CSSIDBitset& property_bits);
  void ApplyParagraphStyle(TextElement* element);
  void ProcessChildStyleAndProps(Element* child, bool& has_inline_view);
  void BuildInlineTruncation(Element* element, bool& has_inline_view);
  void HandleInlineImageProps(Element* child);
  void HandleInlineViewProps(Element* child);
  void EnsureParagraphListener(Element* element);
  void DestroyParagraph(int32_t id);
  //  void MeasureChildrenRecursively(Element* element,starlight::Constraints&
  //  constraints);

  // Textra owns teardown ordering for the API today so it can release
  // ParagraphBuilder/Paragraph state before destroying the underlying API.
  // TODO(linxiaosong): align API destruction with the TextService boundary,
  // since the implementation is created by TextService.
  text::TextLayoutAPI* api_{nullptr};
  text::ParagraphBuilder* paragraph_builder_{nullptr};
  bool building_inline_truncation_{false};
  Element* truncation_{nullptr};
  std::unordered_map<int32_t, text::Paragraph*> paragraphs_;
  std::unordered_map<int32_t, std::unique_ptr<text::ParagraphListener>>
      paragraph_listeners_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_TEXTRA_H_
