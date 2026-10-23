// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_

#include "base/include/value/base_value.h"
#include "core/renderer/dom/fragment/custom_draw_functions.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}
namespace tasm {

// Calls the runtime during full content drawing. Cache construction and State
// observation belong to the producer; this bridge retains no draw cache.
CustomDrawFunctions ComposeDrawBehindFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_
