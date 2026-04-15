// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_OFFSET_CALCULATOR_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_OFFSET_CALCULATOR_H_

#include <memory>
#include <optional>
#include <string>

struct OH_Drawing_Path;

namespace lynx {
namespace tasm {
namespace harmony {

struct OffsetMotionState {
  float x{0.0f};
  float y{0.0f};
  // Tangent angle in degrees relative to +X axis.
  float deg{0.0f};
};

class LynxOffsetCalculator {
 public:
  LynxOffsetCalculator() = default;
  ~LynxOffsetCalculator() = default;

  LynxOffsetCalculator(const LynxOffsetCalculator&) = delete;
  LynxOffsetCalculator& operator=(const LynxOffsetCalculator&) = delete;

  // Unified public API: given an SVG path string and normalized progress [0,1],
  // returns the point and tangent angle along the path, or std::nullopt on
  // error.
  std::optional<OffsetMotionState> PointAtProgress(
      const std::string& svg_path_string, float progress);

 private:
  void Reset();
  bool UpdatePathString(const std::string& svg_path_string);
  std::optional<OffsetMotionState> GetMotionState(float progress) const;

  // Cached state for the last successfully parsed path string.
  std::string cached_path_string_;
  std::shared_ptr<OH_Drawing_Path> cached_path_;
  float cached_path_length_{0.0f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_OFFSET_CALCULATOR_H_
