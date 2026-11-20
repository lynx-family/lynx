// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_RECORDING_DECODER_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_RECORDING_DECODER_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace lynx::lepus {
class Value;
}
namespace lynx::tasm {
class DisplayList;
enum class ComposeDrawPhase;

struct DrawRecordingError {
  enum class Code {
    kVersion = 1,
    kMalformed = 2,
    kUnsupported = 3,
    kOverflow = 4,
    kCapacity = 5
  };
  static constexpr size_t kNoCommand = std::numeric_limits<size_t>::max();
  Code code{Code::kMalformed};
  size_t command_index{kNoCommand};
  std::string name_space;
};

namespace detail {
// Compile into standalone storage under fixed SDK limits. Every error leaves
// both outputs unchanged; success transfers the complete list and shadow
// effect.
std::optional<DrawRecordingError> DecodeDrawDisplayList(
    const lepus::Value& input, double pixels, ComposeDrawPhase phase,
    DisplayList* output, bool* has_outset_shadow);
}  // namespace detail
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_RECORDING_DECODER_H_
