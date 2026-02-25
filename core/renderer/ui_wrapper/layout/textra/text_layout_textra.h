// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_TEXTRA_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_TEXTRA_H_

#include <cstdint>
#include <unordered_map>

#include "core/public/text_layout_impl.h"

namespace lynx {
namespace tasm {
class TextElement;
class FiberElement;

namespace text {
class TextLayoutAPI;
class ParagraphBuilder;
class Paragraph;
}  // namespace text

class TextLayoutTextra : public TextLayoutImpl {
 public:
  explicit TextLayoutTextra(intptr_t textra);
  ~TextLayoutTextra() override = default;

  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode) override;

  void Align(Element* element) override;

  void DispatchLayoutBefore(Element* element) override;

  void BuildParagraphRecursively(Element* element);

 private:
  void ApplyTextStyle(TextElement* element);
  void ApplyParagraphStyle(TextElement* element);
  void ProcessChildStyleAndProps(Element* child);
  void HandleInlineImageProps(Element* child);
  void HandleInlineViewProps(Element* child);
  //  void MeasureChildrenRecursively(Element* element,starlight::Constraints&
  //  constraints);

  text::TextLayoutAPI* api_{nullptr};
  text::ParagraphBuilder* paragraph_builder_{nullptr};
  std::unordered_map<int32_t, text::Paragraph*> paragraphs_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_TEXTRA_H_
