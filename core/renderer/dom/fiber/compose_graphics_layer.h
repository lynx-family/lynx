// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_GRAPHICS_LAYER_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_GRAPHICS_LAYER_H_

#include "base/include/value/base_value.h"
#include "core/renderer/dom/fragment/custom_draw_functions.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}  // namespace runtime
namespace tasm {

// Creates a layer-property hook that evaluates a Compose graphicsLayer closure
// with (Element, physical width, physical height) and records its transform
// in the DisplayList. The Element is the hook's own target, including a
// generated Modifier frame when the layer precedes another operation.
CustomDrawFunctions GraphicsLayerDrawFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_GRAPHICS_LAYER_H_
