// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/renderer/dom/fragment/custom_draw_handler.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}
namespace tasm {

enum class ComposeDrawPhase { kBehind, kForeground, kClip };

// Calls the runtime during full content drawing. Cache construction and State
// observation belong to the producer. Cache-aware callbacks return
// [canReuse, encodedRecording]. Native retains the compiled DisplayList and
// uses the supplied recording when compiling for the first time or a new pixel
// scale. Foreground runs after native content.
fml::RefPtr<CustomDrawHandler> ComposeDrawBehindFunctions(
    runtime::MTSRuntime* runtime, lepus::Value callback,
    ComposeDrawPhase phase = ComposeDrawPhase::kBehind,
    bool supports_reuse = false);
fml::RefPtr<CustomDrawHandler> ComposeDrawForegroundFunctions(
    runtime::MTSRuntime* runtime, lepus::Value callback,
    bool supports_reuse = false);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_BEHIND_H_
