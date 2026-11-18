// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_draw_behind.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/runtime/mts_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx::tasm {
namespace {
[[maybe_unused]] const char* PhaseLabel(ComposeDrawPhase phase) {
  switch (phase) {
    case ComposeDrawPhase::kBehind:
      return "drawBehind";
    case ComposeDrawPhase::kForeground:
      return "drawForeground";
    case ComposeDrawPhase::kClip:
      return "scopedClip";
  }
}

void LogError(ComposeDrawPhase phase, const DrawRecordingError& error) {
  LOGE(PhaseLabel(phase) << " recording rejected: code="
                         << static_cast<int>(error.code)
                         << " command=" << error.command_index
                         << " namespace=" << error.name_space);
}

}  // namespace

CustomDrawFunctions ComposeDrawBehindFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback,
                                               ComposeDrawPhase phase,
                                               bool supports_reuse) {
  CustomDrawFunctions functions;
  auto draw = [runtime, callback, phase, supports_reuse,
               cached = std::shared_ptr<const DisplayList>{},
               cached_pixels = 0., cached_has_shadow = false](
                  Fragment& fragment, DisplayListBuilder& builder) mutable {
    auto* target = fragment.element();
    const double pixels = target->element_manager()
                              ->GetLynxEnvConfig()
                              .PhysicalPixelsPerLayoutUnit();
    const lepus::Value element{fml::RefPtr<Element>(target)};
    const lepus::Value width(target->width() * pixels);
    const lepus::Value height(target->height() * pixels);
    const lepus::Value* args[] = {&element, &width, &height};
    const auto value = runtime->GetMTSContext()
                           ->CallClosureArgs(callback, args, 3)
                           .ToLepusValue();
    if (value.IsEmpty()) return;
    DisplayList decoded;
    const DisplayList* drawing = &decoded;
    bool has_shadow = false;
    const bool can_reuse = supports_reuse && value.GetProperty(0).Bool();
    if (can_reuse && cached && cached_pixels == pixels) {
      drawing = cached.get();
      has_shadow = cached_has_shadow;
    } else {
      cached.reset();
      const auto recording = supports_reuse ? value.GetProperty(1) : value;
      if (const auto error = detail::DecodeDrawDisplayList(
              recording, pixels, phase, &decoded, &has_shadow)) {
        LogError(phase, *error);
        return;
      }
    }
    if (!builder.AppendDrawingContent(*drawing)) {
      LogError(phase, {DrawRecordingError::Code::kCapacity,
                       DrawRecordingError::kNoCommand,
                       {}});
      return;
    }
    if (has_shadow) fragment.MarkCustomOutsetShadowDrawn();
    if (supports_reuse) {
      if (drawing == &decoded)
        cached = std::make_shared<const DisplayList>(std::move(decoded));
      cached_pixels = pixels;
      cached_has_shadow = has_shadow;
    }
  };
  if (phase == ComposeDrawPhase::kForeground)
    functions.draw_foreground = std::move(draw);
  else
    functions.draw_behind = std::move(draw);
  return functions;
}

CustomDrawFunctions ComposeDrawForegroundFunctions(runtime::MTSRuntime* runtime,
                                                   lepus::Value callback,
                                                   bool supports_reuse) {
  return ComposeDrawBehindFunctions(runtime, std::move(callback),
                                    ComposeDrawPhase::kForeground,
                                    supports_reuse);
}
}  // namespace lynx::tasm
