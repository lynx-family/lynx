// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_ELEMENT_UTILS_H_
#define CORE_RENDERER_DOM_FIBER_ELEMENT_UTILS_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {

fml::RefPtr<Element> GetComposeContentOrFiberElementFromValue(
    const lepus::Value& value);

fml::RefPtr<Element> GetComposeMountRootOrFiberElementFromValue(
    const lepus::Value& value);

base::String ConvertTextContent(const lepus::Value& value);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_ELEMENT_UTILS_H_
