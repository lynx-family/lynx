// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_DEVTOOL_COORDINATES_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_DEVTOOL_COORDINATES_H_

#include <vector>

#include "base/include/float_comparison.h"

namespace lynx {
namespace tasm {
namespace harmony {

// DevTool uses logical pixels; ArkUI positions, snapshots and native input use
// physical pixels. Keep the conversion at these platform boundaries.
class HarmonyDevToolCoordinates {
 public:
  explicit HarmonyDevToolCoordinates(float density)
      : density_(base::FloatsLargerOrEqual(0.f, density) ? 1.f : density) {}

  float PhysicalToLogical(float value) const { return value / density_; }
  float LogicalToPhysical(float value) const { return value * density_; }

  // UIBase returns transformed corners relative to the root in physical pixels.
  // The shared inspector subtracts the root quad for lynxview mode.
  void QuadToWindow(std::vector<float>& quad, float root_x_px,
                    float root_y_px) const {
    for (size_t i = 0; i + 1 < quad.size(); i += 2) {
      quad[i] = PhysicalToLogical(quad[i] + root_x_px);
      quad[i + 1] = PhysicalToLogical(quad[i + 1] + root_y_px);
    }
  }

  // HitTest expects root-relative logical pixels.
  void WindowToRoot(float point[2], float root_x_px, float root_y_px) const {
    point[0] -= PhysicalToLogical(root_x_px);
    point[1] -= PhysicalToLogical(root_y_px);
  }

 private:
  float density_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_DEVTOOL_COORDINATES_H_
