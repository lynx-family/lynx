// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_draw_behind.h"

#include <optional>
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

class ComposeDrawHandler final : public CustomDrawHandler {
 public:
  ComposeDrawHandler(runtime::MTSRuntime* runtime, lepus::Value callback,
                     ComposeDrawPhase phase, bool supports_reuse)
      : CustomDrawHandler(FlagsForPhase(phase)),
        runtime_(runtime),
        callback_(std::move(callback)),
        phase_(phase),
        supports_reuse_(supports_reuse) {}

  void Draw(Fragment& fragment, DisplayListBuilder& builder,
            Phase /*phase*/) override {
    auto* target = fragment.element();
    const double pixels = target->element_manager()
                              ->GetLynxEnvConfig()
                              .PhysicalPixelsPerLayoutUnit();
    const lepus::Value element{fml::RefPtr<Element>(target)};
    const lepus::Value width(target->width() * pixels);
    const lepus::Value height(target->height() * pixels);
    const lepus::Value* args[] = {&element, &width, &height};
    auto recording = runtime_->GetMTSContext()
                         ->CallClosureArgs(callback_, args, 3)
                         .ToLepusValue();
    if (recording.IsEmpty()) return;
    DisplayList decoded;
    const DisplayList* drawing = &decoded;
    bool has_shadow = false;
    const bool can_reuse = supports_reuse_ && recording.GetProperty(0).Bool();
    if (can_reuse && cached_ && cached_pixels_ == pixels) {
      drawing = &*cached_;
      has_shadow = cached_has_shadow_;
    } else {
      cached_.reset();
      if (supports_reuse_) recording = recording.GetProperty(1);
      if (const auto error = detail::DecodeDrawDisplayList(
              recording, pixels, phase_, &decoded, &has_shadow)) {
        LogError(phase_, *error);
        return;
      }
    }
    if (!builder.AppendDrawingContent(*drawing)) {
      LogError(phase_, {DrawRecordingError::Code::kCapacity,
                        DrawRecordingError::kNoCommand,
                        {}});
      return;
    }
    if (has_shadow) fragment.MarkCustomOutsetShadowDrawn();
    if (supports_reuse_) {
      if (drawing == &decoded) cached_.emplace(std::move(decoded));
      cached_pixels_ = pixels;
      cached_has_shadow_ = has_shadow;
    }
  }

 private:
  static uint8_t FlagsForPhase(ComposeDrawPhase phase) {
    uint8_t flags =
        (phase == ComposeDrawPhase::kForeground ? kForeground : kBehind) |
        kMayReenter;
    if (phase == ComposeDrawPhase::kClip) {
      flags |= kRequiresLayer;
    }
    return flags;
  }

  runtime::MTSRuntime* runtime_;
  lepus::Value callback_;
  std::optional<DisplayList> cached_;
  double cached_pixels_{0.};
  ComposeDrawPhase phase_;
  bool supports_reuse_{false};
  bool cached_has_shadow_{false};
};

}  // namespace

fml::RefPtr<CustomDrawHandler> ComposeDrawBehindFunctions(
    runtime::MTSRuntime* runtime, lepus::Value callback, ComposeDrawPhase phase,
    bool supports_reuse) {
  return fml::MakeRefCounted<ComposeDrawHandler>(runtime, std::move(callback),
                                                 phase, supports_reuse);
}

fml::RefPtr<CustomDrawHandler> ComposeDrawForegroundFunctions(
    runtime::MTSRuntime* runtime, lepus::Value callback, bool supports_reuse) {
  return ComposeDrawBehindFunctions(runtime, std::move(callback),
                                    ComposeDrawPhase::kForeground,
                                    supports_reuse);
}
}  // namespace lynx::tasm
