// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_GEOMETRY_LENGTH_H_
#define GFX_GEOMETRY_LENGTH_H_

#include <cstdint>

namespace lynx {
namespace gfx {

enum class LengthUnit : uint8_t {
  kNumber = 0,
  kPercent,
};

// Percentage values use CSS percentage points, so 50% is represented as
// {50, kPercent}, not {0.5, kPercent}.
struct LengthValue {
  float value{0.0f};
  LengthUnit unit{LengthUnit::kNumber};

  constexpr float Resolve(float percentage_base) const {
    return unit == LengthUnit::kPercent ? percentage_base * value / 100.0f
                                        : value;
  }
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_GEOMETRY_LENGTH_H_
