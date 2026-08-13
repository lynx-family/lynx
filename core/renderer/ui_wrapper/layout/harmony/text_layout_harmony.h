// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_LAYOUT_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_LAYOUT_HARMONY_H_

#include "core/public/text_layout_impl.h"

namespace lynx {
namespace tasm {

class TextMeasurerHarmony;

class TextLayoutHarmony final : public TextLayoutImpl {
 public:
  explicit TextLayoutHarmony(TextMeasurerHarmony* text_measurer)
      : text_measurer_(text_measurer) {}
  ~TextLayoutHarmony() override = default;

  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode) override;
  void Align(Element* element) override;
  void DispatchLayoutBefore(Element* element) override;
  void Destroy(Element* element) override;

 private:
  TextMeasurerHarmony* text_measurer_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_LAYOUT_HARMONY_H_
