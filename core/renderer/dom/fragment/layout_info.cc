// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/layout_info.h"

namespace lynx {
namespace tasm {

RoundedRectangle LayoutInfoForDraw::GenerateContentRectangle() const {
  RoundedRectangle rect;
  rect.SetX(GetContentBoxX());
  rect.SetY(GetContentBoxY());
  rect.SetWidth(GetContentBoxWidth());
  rect.SetHeight(GetContentBoxHeight());

  auto border_left_width = layout_result.border_[starlight::Direction::kLeft];
  auto border_top_width = layout_result.border_[starlight::Direction::kTop];
  auto border_right_width = layout_result.border_[starlight::Direction::kRight];
  auto border_bottom_width =
      layout_result.border_[starlight::Direction::kBottom];

  auto padding_left_width = layout_result.padding_[starlight::Direction::kLeft];
  auto padding_top_width = layout_result.padding_[starlight::Direction::kTop];
  auto padding_right_width =
      layout_result.padding_[starlight::Direction::kRight];
  auto padding_bottom_width =
      layout_result.padding_[starlight::Direction::kBottom];

  if (border_radius_info.has_value()) {
    rect.SetRadiusXTopLeft(std::max(
        border_radius_info->x_top_left - border_left_width - padding_left_width,
        0.f));
    rect.SetRadiusXTopRight(std::max(border_radius_info->x_top_right -
                                         border_right_width -
                                         padding_right_width,
                                     0.f));
    rect.SetRadiusXBottomRight(std::max(border_radius_info->x_bottom_right -
                                            border_right_width -
                                            padding_right_width,
                                        0.f));
    rect.SetRadiusXBottomLeft(std::max(border_radius_info->x_bottom_left -
                                           border_left_width -
                                           padding_left_width,
                                       0.f));
    rect.SetRadiusYTopLeft(std::max(
        border_radius_info->y_top_left - border_top_width - padding_top_width,
        0.f));
    rect.SetRadiusYTopRight(std::max(
        border_radius_info->y_top_right - border_top_width - padding_top_width,
        0.f));
    rect.SetRadiusYBottomRight(std::max(border_radius_info->y_bottom_right -
                                            border_bottom_width -
                                            padding_bottom_width,
                                        0.f));
    rect.SetRadiusYBottomLeft(std::max(border_radius_info->y_bottom_left -
                                           border_bottom_width -
                                           padding_bottom_width,
                                       0.f));
  }
  return rect;
}

RoundedRectangle LayoutInfoForDraw::GeneratePaddingRectangle() const {
  RoundedRectangle rect;
  rect.SetX(GetPaddingBoxX());
  rect.SetY(GetPaddingBoxY());
  rect.SetWidth(GetPaddingBoxWidth());
  rect.SetHeight(GetPaddingBoxHeight());

  auto border_left_width = layout_result.border_[starlight::Direction::kLeft];
  auto border_top_width = layout_result.border_[starlight::Direction::kTop];
  auto border_right_width = layout_result.border_[starlight::Direction::kRight];
  auto border_bottom_width =
      layout_result.border_[starlight::Direction::kBottom];

  if (border_radius_info.has_value()) {
    rect.SetRadiusXTopLeft(
        std::max(border_radius_info->x_top_left - border_left_width, 0.f));
    rect.SetRadiusXTopRight(
        std::max(border_radius_info->x_top_right - border_right_width, 0.f));
    rect.SetRadiusXBottomRight(
        std::max(border_radius_info->x_bottom_right - border_right_width, 0.f));
    rect.SetRadiusXBottomLeft(
        std::max(border_radius_info->x_bottom_left - border_left_width, 0.f));
    rect.SetRadiusYTopLeft(
        std::max(border_radius_info->y_top_left - border_top_width, 0.f));
    rect.SetRadiusYTopRight(
        std::max(border_radius_info->y_top_right - border_top_width, 0.f));
    rect.SetRadiusYBottomRight(std::max(
        border_radius_info->y_bottom_right - border_bottom_width, 0.f));
    rect.SetRadiusYBottomLeft(
        std::max(border_radius_info->y_bottom_left - border_bottom_width, 0.f));
  }
  return rect;
}

RoundedRectangle LayoutInfoForDraw::GenerateBorderRectangle() const {
  RoundedRectangle rect;
  rect.SetX(GetBorderBoxX());
  rect.SetY(GetBorderBoxY());
  rect.SetWidth(GetBorderBoxWidth());
  rect.SetHeight(GetBorderBoxHeight());

  if (border_radius_info.has_value()) {
    rect.SetRadiusXTopLeft(border_radius_info->x_top_left);
    rect.SetRadiusXTopRight(border_radius_info->x_top_right);
    rect.SetRadiusXBottomRight(border_radius_info->x_bottom_right);
    rect.SetRadiusXBottomLeft(border_radius_info->x_bottom_left);
    rect.SetRadiusYTopLeft(border_radius_info->y_top_left);
    rect.SetRadiusYTopRight(border_radius_info->y_top_right);
    rect.SetRadiusYBottomRight(border_radius_info->y_bottom_right);
    rect.SetRadiusYBottomLeft(border_radius_info->y_bottom_left);
  }
  return rect;
}

}  // namespace tasm
}  // namespace lynx
