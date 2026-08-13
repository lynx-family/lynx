// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/harmony/text_layout_harmony.h"

#include "core/renderer/dom/element.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_measurer_harmony.h"

namespace lynx {
namespace tasm {

void TextLayoutHarmony::DispatchLayoutBefore(Element* element) {
  if (text_measurer_ != nullptr) {
    text_measurer_->DispatchLayoutBefore(element);
  }
}

LayoutResult TextLayoutHarmony::Measure(Element* element, float width,
                                        int width_mode, float height,
                                        int height_mode) {
  return text_measurer_ != nullptr
             ? text_measurer_->Measure(element, width, width_mode, height,
                                       height_mode)
             : LayoutResult{};
}

void TextLayoutHarmony::Align(Element* element) {
  if (text_measurer_ != nullptr) {
    text_measurer_->Align(element);
  }
}

void TextLayoutHarmony::Destroy(Element* element) {
  if (text_measurer_ != nullptr) {
    text_measurer_->Destroy(element);
  }
}

}  // namespace tasm
}  // namespace lynx
