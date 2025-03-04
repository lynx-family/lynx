// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_CONTEXT_DATA_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_CONTEXT_DATA_H_

#include <array>

namespace lynx {
namespace tasm {

namespace layout {
enum MeasureMode : int {
  Indefinite = 0,
  Definite = 1,
  AtMost = 2,
};

typedef std::array<float, 13> LayoutInfoArray;

class LayoutInfo {
 public:
  static constexpr int32_t kWidth = 0;
  static constexpr int32_t kHeight = 1;
  static constexpr int32_t kLeft = 2;
  ;
  static constexpr int32_t kTop = 3;
  static constexpr int32_t kMarginLeft = 4;
  static constexpr int32_t kMarginTop = 5;
  static constexpr int32_t kMarginRight = 6;
  static constexpr int32_t kMarginBottom = 7;
  static constexpr int32_t kPaddingLeft = 8;
  static constexpr int32_t kPaddingTop = 9;
  static constexpr int32_t kPaddingRight = 10;
  static constexpr int32_t kPaddingBottom = 11;
  static constexpr int32_t kIsUpdatedListElement = 12;
};

struct Viewport {
  float width;
  float height;
  int width_mode;
  int height_mode;

  inline void UpdateViewport(float width, int width_mode, float height,
                             int height_mode) {
    this->width = width;
    this->height = height;
    this->width_mode = width_mode;
    this->height_mode = height_mode;
  }
};

struct CalculatedViewport {
  float width = 0;
  float height = 0;
};
}  // namespace layout

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_CONTEXT_DATA_H_
