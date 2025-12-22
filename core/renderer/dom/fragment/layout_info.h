// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_LAYOUT_INFO_H_
#define CORE_RENDERER_DOM_FRAGMENT_LAYOUT_INFO_H_

#include <algorithm>

#include "base/include/flex_optional.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"
#include "core/renderer/starlight/types/layout_directions.h"
#include "core/renderer/starlight/types/layout_result.h"

namespace lynx {
namespace tasm {

struct BorderRadiusInfo {
  float x_top_left = 0.f;
  float y_top_left = 0.f;
  float x_top_right = 0.f;
  float y_top_right = 0.f;
  float x_bottom_right = 0.f;
  float y_bottom_right = 0.f;
  float x_bottom_left = 0.f;
  float y_bottom_left = 0.f;
};

struct LayoutInfoForDraw {
  starlight::LayoutResultForRendering layout_result;
  base::flex_optional<BorderRadiusInfo> border_radius_info;

  float GetBorderBoxX() const { return 0; }

  float GetBorderBoxY() const { return 0; }

  float GetBorderBoxWidth() const { return layout_result.size_.width_; }

  float GetBorderBoxHeight() const { return layout_result.size_.height_; }

  float GetPaddingBoxX() const {
    return GetBorderBoxX() + layout_result.border_[starlight::Direction::kLeft];
  }

  float GetPaddingBoxY() const {
    return GetBorderBoxY() + layout_result.border_[starlight::Direction::kTop];
  }

  float GetPaddingBoxWidth() const {
    return std::max(GetBorderBoxWidth() -
                        layout_result.border_[starlight::Direction::kLeft] -
                        layout_result.border_[starlight::Direction::kRight],
                    0.f);
  }

  float GetPaddingBoxHeight() const {
    return std::max(GetBorderBoxHeight() -
                        layout_result.border_[starlight::Direction::kTop] -
                        layout_result.border_[starlight::Direction::kBottom],
                    0.f);
  }

  float GetContentBoxX() const {
    return GetPaddingBoxX() +
           layout_result.padding_[starlight::Direction::kLeft];
  }

  float GetContentBoxY() const {
    return GetPaddingBoxY() +
           layout_result.padding_[starlight::Direction::kTop];
  }

  float GetContentBoxWidth() const {
    return std::max(GetPaddingBoxWidth() -
                        layout_result.padding_[starlight::Direction::kLeft] -
                        layout_result.padding_[starlight::Direction::kRight],
                    0.f);
  }

  float GetContentBoxHeight() const {
    return std::max(GetPaddingBoxHeight() -
                        layout_result.padding_[starlight::Direction::kTop] -
                        layout_result.padding_[starlight::Direction::kBottom],
                    0.f);
  }

  RoundedRectangle GenerateContentRectangle() const;
  RoundedRectangle GeneratePaddingRectangle() const;
  RoundedRectangle GenerateBorderRectangle() const;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_LAYOUT_INFO_H_
