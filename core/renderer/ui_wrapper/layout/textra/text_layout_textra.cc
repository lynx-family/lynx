// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"

namespace lynx {
namespace tasm {

TextLayoutTextra::TextLayoutTextra(intptr_t api) {}

TextLayoutTextra::~TextLayoutTextra() = default;

LayoutResult TextLayoutTextra::Measure(Element* element, float width,
                                       int width_mode, float height,
                                       int height_mode) {
  return {};
}

void TextLayoutTextra::Align(Element* element) {}

void TextLayoutTextra::DispatchLayoutBefore(Element* element) {}

}  // namespace tasm
}  // namespace lynx
