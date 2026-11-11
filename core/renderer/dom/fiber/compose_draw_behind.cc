// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_draw_behind.h"

#include <cmath>
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
const char* PhaseLabel(ComposeDrawPhase phase) {
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

bool Active(const Element& target) {
  return target.IsAttached() && target.HasCustomDrawFunctions() &&
         !(target.dirty() & Element::kDirtyCustomDraw);
}

struct DrawExecutionState {
  DrawExecutionState() = default;
  DrawExecutionState(const DrawExecutionState&) : DrawExecutionState() {}

  bool in_flight{false};
  bool reentered{false};
};

struct CacheKey {
  const Element* target{nullptr};
  int32_t target_id{0};
  const CustomDrawFunctions* binding{nullptr};
  uint64_t detach_generation{0};
  uint64_t binding_generation{0};
  ComposeDrawPhase phase{ComposeDrawPhase::kBehind};
  float width{0};
  float height{0};
  double pixels{0};

  bool operator==(const CacheKey& other) const {
    return target == other.target && target_id == other.target_id &&
           binding == other.binding &&
           detach_generation == other.detach_generation &&
           binding_generation == other.binding_generation &&
           phase == other.phase && width == other.width &&
           height == other.height && pixels == other.pixels;
  }
};

CacheKey CurrentKey(const Fragment& fragment, ComposeDrawPhase phase) {
  auto* target = fragment.element();
  return {target,
          target->impl_id(),
          fragment.GetCustomDrawFunctions(),
          target->CustomDrawDetachGeneration(),
          fragment.CustomDrawBindingGeneration(),
          phase,
          target->width(),
          target->height(),
          target->element_manager()
              ->GetLynxEnvConfig()
              .PhysicalPixelsPerLayoutUnit()};
}

}  // namespace

CustomDrawFunctions ComposeDrawBehindFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback,
                                               ComposeDrawPhase phase,
                                               bool supports_reuse) {
  CustomDrawFunctions functions;
  auto draw = [runtime, callback, phase, supports_reuse,
               cached = std::shared_ptr<const DisplayList>{}, key = CacheKey{},
               cached_has_shadow = false, execution = DrawExecutionState{}](
                  Fragment& fragment, DisplayListBuilder& builder) mutable {
    if (execution.in_flight) {
      cached.reset();
      execution.reentered = true;
      return;
    }
    execution.in_flight = true;
    execution.reentered = false;
    struct DrawingScope {
      bool& in_flight;
      ~DrawingScope() { in_flight = false; }
    } scope{execution.in_flight};
    auto previous = std::move(cached);
    cached.reset();
    auto* target = fragment.element();
    if (!Active(*target)) return;
    const auto current_key = CurrentKey(fragment, phase);
    const double pixels = current_key.pixels;
    if (!std::isfinite(pixels) || pixels <= 0) return;
    const lepus::Value element{fml::RefPtr<Element>(target)};
    const lepus::Value width(current_key.width * pixels);
    const lepus::Value height(current_key.height * pixels);
    const lepus::Value can_reuse(supports_reuse && previous != nullptr &&
                                 key == current_key);
    const lepus::Value* args[] = {&element, &width, &height, &can_reuse};
    const auto value =
        runtime->GetMTSContext()
            ->CallClosureArgs(callback, args, supports_reuse ? 4 : 3)
            .ToLepusValue();
    // A resolver may remove/rebind or detach its target while evaluating.
    if (value.IsEmpty() || execution.reentered || !Active(*target) ||
        !(current_key == CurrentKey(fragment, phase)))
      return;
    DisplayList decoded;
    const DisplayList* drawing = &decoded;
    bool has_shadow = false;
    if (value.IsString() && value.StringView() == kComposeDrawReuseLast) {
      if (!can_reuse.Bool()) {
        LogError(phase, {});
        return;
      }
      drawing = previous.get();
      has_shadow = cached_has_shadow;
    } else {
      // The reader observes each runtime property once and owns every result.
      if (const auto error = detail::DecodeDrawDisplayList(
              value, pixels, phase, detail::DecodeLimits{}, builder, &decoded,
              &has_shadow)) {
        LogError(phase, *error);
        return;
      }
    }
    if (execution.reentered || !Active(*target) ||
        !(current_key == CurrentKey(fragment, phase)))
      return;
    if (!builder.AppendDrawingContent(*drawing)) {
      LogError(phase, {DrawRecordingError::Code::kCapacity,
                       DrawRecordingError::kNoCommand,
                       {}});
      return;
    }
    if (has_shadow) fragment.MarkCustomOutsetShadowDrawn();
    if (supports_reuse && !execution.reentered && Active(*target) &&
        current_key == CurrentKey(fragment, phase)) {
      cached = drawing == &decoded
                   ? std::make_shared<const DisplayList>(std::move(decoded))
                   : std::move(previous);
      key = current_key;
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
