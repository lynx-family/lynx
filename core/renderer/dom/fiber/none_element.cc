// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/none_element.h"

namespace lynx {
namespace tasm {

constexpr const static char kNoneElementTag[] = "view";

NoneElement::NoneElement(ElementManager* manager)
    : FiberElement(manager, BASE_STATIC_STRING(kNoneElementTag)) {
  is_layout_only_ = true;
  BASE_STATIC_STRING_DECL(kAbsolute, "absolute");
  BASE_STATIC_STRING_DECL(kNone, "none");
  SetStyle(kPropertyIDPosition, lepus::Value(kAbsolute));
  SetStyle(kPropertyIDDisplay, lepus::Value(kNone));
}

NoneElement::NoneElement(const NoneElement& element, bool clone_resolved_props)
    : FiberElement(element, clone_resolved_props) {
  is_layout_only_ = true;
}

}  // namespace tasm
}  // namespace lynx
