// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/raw_text_element.h"

namespace lynx {
namespace tasm {

RawTextElement::RawTextElement(ElementManager* manager)
    : FiberElement(manager, BASE_STATIC_STRING(kRawTextTag)) {}

void RawTextElement::SetText(const lepus::Value& text) {
  SetAttribute(BASE_STATIC_STRING(kTextAttr), text);
}

}  // namespace tasm
}  // namespace lynx
