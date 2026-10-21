// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_layout_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"

namespace lynx {
namespace tasm {
namespace compose_layout_utils {
namespace {

FloatSize MeasureChild(starlight::LayoutObject& node,
                       const starlight::Constraints& constraints,
                       bool final_measure) {
  // Platform measurement interprets its constraints as a containing block and
  // downgrades exact sizes of auto-sized children to at-most. Compose passes
  // constraints for this node itself, so preserve exact axes and cap preferred
  // CSS sizes on bounded axes before entering the layout algorithm.
  auto percent_base = constraints;
  for (auto axis : {starlight::kHorizontal, starlight::kVertical}) {
    if (!IsSLIndefiniteMode(constraints[axis].Mode())) {
      // Compose fill consumes a finite maximum even when the incoming
      // constraint is not exact. Its CSS percentage needs that finite base.
      percent_base[axis] =
          starlight::OneSideConstraint::Definite(constraints[axis].Size());
    }
  }
  auto resolved =
      starlight::property_utils::GenerateDefaultConstraints(node, percent_base);
  for (auto axis : {starlight::kHorizontal, starlight::kVertical}) {
    const auto& limit = constraints[axis];
    if (IsSLDefiniteMode(limit.Mode())) {
      resolved[axis] = limit;
    } else if (!IsSLIndefiniteMode(limit.Mode()) &&
               (IsSLIndefiniteMode(resolved[axis].Mode()) ||
                resolved[axis].Size() > limit.Size())) {
      resolved[axis] = starlight::OneSideConstraint(
          limit.Size(), IsSLDefiniteMode(resolved[axis].Mode())
                            ? SLMeasureModeDefinite
                            : SLMeasureModeAtMost);
    }
  }
  node.GetBoxInfo()->InitializeBoxInfo(resolved, node, node.GetLayoutConfigs());
  return node.UpdateMeasure(resolved, final_measure);
}

}  // namespace

SizeRangeMeasureResult MeasureSizeRange(starlight::LayoutObject& child,
                                        const starlight::Constraints& incoming,
                                        const std::array<double, 4>& limits,
                                        double layouts_unit_per_px,
                                        bool final_measure) {
  starlight::Constraints constraints;
  std::array<float, 2> minimum{};
  std::array<float, 2> maximum{};
  const double scale = layouts_unit_per_px;
  auto pixels = [scale](double value) {
    return static_cast<float>(
        std::min(std::round(value * scale),
                 static_cast<double>(std::numeric_limits<int32_t>::max())));
  };
  for (auto axis : {starlight::kHorizontal, starlight::kVertical}) {
    const auto& input = incoming[axis];
    const float input_max = IsSLIndefiniteMode(input.Mode())
                                ? std::numeric_limits<float>::infinity()
                                : input.Size();
    const float input_min = IsSLDefiniteMode(input.Mode()) ? input_max : 0;
    const float requested_max = limits[axis + 2] < 0
                                    ? std::numeric_limits<float>::infinity()
                                    : pixels(limits[axis + 2]);
    maximum[axis] = std::clamp(requested_max, input_min, input_max);
    const float requested_min = limits[axis] < 0
                                    ? std::numeric_limits<float>::infinity()
                                    : pixels(limits[axis]);
    float target_min = std::min(requested_min, requested_max);
    // Compose maps an infinite minimum to zero only after capping it by max.
    if (!std::isfinite(target_min) ||
        target_min == static_cast<float>(std::numeric_limits<int32_t>::max())) {
      target_min = 0;
    }
    minimum[axis] = std::clamp(target_min, input_min, input_max);
    constraints[axis] =
        minimum[axis] == maximum[axis]
            ? starlight::OneSideConstraint::Definite(maximum[axis])
        : std::isfinite(maximum[axis])
            ? starlight::OneSideConstraint::AtMost(maximum[axis])
            : starlight::OneSideConstraint::Indefinite();
  }
  auto measured = MeasureChild(child, constraints, final_measure);
  // Native modes encode exact / at-most / unbounded. Re-measure a preferred
  // child that misses a positive minimum with that axis made exact.
  // TODO: Carry positive minima directly when custom Compose measure policies
  // can receive a complete min/max constraint interval.
  bool needs_minimum = false;
  if (measured.width_ < minimum[0]) {
    constraints[starlight::kHorizontal] =
        starlight::OneSideConstraint::Definite(minimum[0]);
    needs_minimum = true;
  }
  if (measured.height_ < minimum[1]) {
    constraints[starlight::kVertical] =
        starlight::OneSideConstraint::Definite(minimum[1]);
    needs_minimum = true;
  }
  if (needs_minimum) {
    measured = MeasureChild(child, constraints, final_measure);
  }
  const FloatSize result(std::clamp(measured.width_, minimum[0], maximum[0]),
                         std::clamp(measured.height_, minimum[1], maximum[1]));
  return {result, FloatSize((result.width_ - measured.width_) / 2,
                            (result.height_ - measured.height_) / 2)};
}

}  // namespace compose_layout_utils
}  // namespace tasm
}  // namespace lynx
