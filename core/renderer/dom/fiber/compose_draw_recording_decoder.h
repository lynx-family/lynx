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
class DisplayListBuilder;
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
// Private SDK work/allocation limits, never wire fields. Value occurrences
// are charged even though opaque payloads are validated without being copied.
struct DecodeLimits {
  DecodeLimits();
  size_t max_commands;
  size_t max_paths;
  size_t max_paints;
  size_t max_resources;
  size_t max_path_depth;
  size_t max_state_depth;
  size_t max_value_depth;
  size_t max_payload_bytes;
  size_t max_snapshot_nodes{262144};
  size_t max_owned_value_nodes{262144};
  size_t max_items;
  size_t max_data_bytes{INT32_MAX};
  size_t max_boxes{INT32_MAX};
  size_t max_gradient_stops{static_cast<size_t>(INT32_MAX) / 8};
};

// Compile into standalone final storage, accounting for destination's prefix.
// Neither destination nor Fragment state is mutated. Every error leaves both
// output parameters unchanged; success transfers the complete list and effect.
std::optional<DrawRecordingError> DecodeDrawDisplayList(
    const lepus::Value& input, double pixels, ComposeDrawPhase phase,
    const DecodeLimits& limits, const DisplayListBuilder& destination,
    DisplayList* output, bool* has_outset_shadow);
}  // namespace detail
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_DRAW_RECORDING_DECODER_H_
