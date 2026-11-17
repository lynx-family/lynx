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

enum class ComposeDrawPhase { kBehind, kForeground, kClip };

// Cache control is a callback result, outside the versioned recording envelope.
inline constexpr char kComposeDrawReuseLast[] = "lynx.draw.reuse-last";

// Calls the runtime during full content drawing. Cache construction and State
// observation belong to the producer. Native cache-aware bindings retain an
// immutable compiled DisplayList and pass a fourth Boolean indicating whether
// that binding's target, phase, size and pixel scale still match. A miss asks
// for a full v1 recording. Foreground runs after native content.
CustomDrawFunctions ComposeDrawBehindFunctions(
    runtime::MTSRuntime* runtime, lepus::Value callback,
    ComposeDrawPhase phase = ComposeDrawPhase::kBehind,
    bool supports_reuse = false);
CustomDrawFunctions ComposeDrawForegroundFunctions(runtime::MTSRuntime* runtime,
                                                   lepus::Value callback,
                                                   bool supports_reuse = false);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_
