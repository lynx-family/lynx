// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SERVICES_FSP_TRACING_FSP_CONFIG_H_
#define CORE_SERVICES_FSP_TRACING_FSP_CONFIG_H_

#include <bitset>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>

#include "lynx/base/include/closure.h"
#include "lynx/base/include/geometry/rect.h"

namespace lynx {
namespace tasm {
namespace timing {
// Config fields for axial mode;
struct FSPAxialConfig {
  // The threshold of minimum-milisecond interval between two snapshots.
  //
  // If the value is zero or negative, the tracer will not check for
  // interval.
  double minimum_diff_interval_ms{1.0};

  // The threshold of maximium acceptable pixel differences per-milisecond.
  // Any content changes exceeding this threshold will delay the FSP
  // calculation.
  double acceptable_pixel_difference_per_ms{2.0};

  // The threshold of maximum acceptable pixel differences per snapshot.
  // Any content changes exceeding this threshold will delay the FSP
  // calculation.
  double acceptable_pixel_difference_per_snapshot{50.0};

  // The threshold of minimum acceptable pixel projection completion rate in x
  // axis. The FSP tracer will wait until the completion rate of the
  // pixel projection is greater than this value before marking the timing
  // of FSP.
  //
  // If the value is zero or negative, the tracer will not check for
  // completion rate.
  double minimum_completion_rate_x{0.0};

  // The threshold of minimum acceptable pixel projection completion rate in y
  // axis. The FSP tracer will wait until the completion rate of the
  // pixel projection is greater than this value before marking the timing
  // of FSP.
  //
  // If the value is zero or negative, the tracer will not check for
  // completion rate.
  double minimum_completion_rate_y{0.0};
};

// Config fields for area mode;
struct FSPAreaConfig {
  // The threshold of minimum-milisecond interval between two snapshots.
  //
  // If the value is zero or negative, the tracer will not check for
  // interval.
  double minimum_diff_interval_ms{1.0};

  // The threshold of maximum acceptable area(in square pixels) differences
  // per-milisecond. Any content changes exceeding this threshold will delay the
  // FSP calculation.
  double acceptable_difference_per_ms{500.0};

  // The threshold of maximum acceptable area(in square pixels) differences per
  // snapshot. Any content changes exceeding this threshold will delay the FSP
  // calculation.
  double acceptable_difference_per_snapshot{500.0};

  // The threshold of minimum acceptable area completion rate
  // The FSP tracer will wait until the completion rate of the
  // area projection is greater than this value before marking the timing
  // of FSP.
  //
  // If the value is zero or negative, the tracer will not check for
  // completion rate.
  double minimum_completion_rate{0.0};
};

class FSPTracer;

struct FSPConfig {
  FSPConfig(std::nullopt_t null = std::nullopt) : v(std::nullopt) {}
  FSPConfig(FSPAxialConfig axialConfig) : v(axialConfig) {}
  FSPConfig(FSPAreaConfig areaConfig) : v(areaConfig) {}

  operator bool() { return !std::holds_alternative<std::nullopt_t>(v); }

 private:
  friend class FSPTracer;

  std::variant<std::nullopt_t, FSPAxialConfig, FSPAreaConfig> v;
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_FSP_CONFIG_H_
