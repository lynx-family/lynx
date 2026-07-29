// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/layout_info.h"

#include <utility>

namespace lynx {
namespace tasm {

void LayoutInfoForDraw::SetBorderRadiusInfo(BorderRadiusInfo radii) {
  const float width = std::max(GetBorderBoxWidth(), 0.f);
  const float height = std::max(GetBorderBoxHeight(), 0.f);

  // CSS scales all radii by the same factor when any side overflows.
  float scale = 1.f;
  const float top = radii.x_top_left + radii.x_top_right;
  if (top > width) {
    scale = std::min(scale, width / top);
  }
  const float bottom = radii.x_bottom_left + radii.x_bottom_right;
  if (bottom > width) {
    scale = std::min(scale, width / bottom);
  }
  const float left = radii.y_top_left + radii.y_bottom_left;
  if (left > height) {
    scale = std::min(scale, height / left);
  }
  const float right = radii.y_top_right + radii.y_bottom_right;
  if (right > height) {
    scale = std::min(scale, height / right);
  }

  if (scale < 1.f) {
    radii.x_top_left *= scale;
    radii.y_top_left *= scale;
    radii.x_top_right *= scale;
    radii.y_top_right *= scale;
    radii.x_bottom_right *= scale;
    radii.y_bottom_right *= scale;
    radii.x_bottom_left *= scale;
    radii.y_bottom_left *= scale;
  }
  border_radius_info = std::move(radii);
}

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
    const BorderRadiusInfo& radii = *border_radius_info;
    rect.SetRadiusXTopLeft(std::max(
        radii.x_top_left - border_left_width - padding_left_width, 0.f));
    rect.SetRadiusXTopRight(std::max(
        radii.x_top_right - border_right_width - padding_right_width, 0.f));
    rect.SetRadiusXBottomRight(std::max(
        radii.x_bottom_right - border_right_width - padding_right_width, 0.f));
    rect.SetRadiusXBottomLeft(std::max(
        radii.x_bottom_left - border_left_width - padding_left_width, 0.f));
    rect.SetRadiusYTopLeft(
        std::max(radii.y_top_left - border_top_width - padding_top_width, 0.f));
    rect.SetRadiusYTopRight(std::max(
        radii.y_top_right - border_top_width - padding_top_width, 0.f));
    rect.SetRadiusYBottomRight(std::max(
        radii.y_bottom_right - border_bottom_width - padding_bottom_width,
        0.f));
    rect.SetRadiusYBottomLeft(std::max(
        radii.y_bottom_left - border_bottom_width - padding_bottom_width, 0.f));
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
    const BorderRadiusInfo& radii = *border_radius_info;
    rect.SetRadiusXTopLeft(std::max(radii.x_top_left - border_left_width, 0.f));
    rect.SetRadiusXTopRight(
        std::max(radii.x_top_right - border_right_width, 0.f));
    rect.SetRadiusXBottomRight(
        std::max(radii.x_bottom_right - border_right_width, 0.f));
    rect.SetRadiusXBottomLeft(
        std::max(radii.x_bottom_left - border_left_width, 0.f));
    rect.SetRadiusYTopLeft(std::max(radii.y_top_left - border_top_width, 0.f));
    rect.SetRadiusYTopRight(
        std::max(radii.y_top_right - border_top_width, 0.f));
    rect.SetRadiusYBottomRight(
        std::max(radii.y_bottom_right - border_bottom_width, 0.f));
    rect.SetRadiusYBottomLeft(
        std::max(radii.y_bottom_left - border_bottom_width, 0.f));
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
    const BorderRadiusInfo& radii = *border_radius_info;
    rect.SetRadiusXTopLeft(radii.x_top_left);
    rect.SetRadiusXTopRight(radii.x_top_right);
    rect.SetRadiusXBottomRight(radii.x_bottom_right);
    rect.SetRadiusXBottomLeft(radii.x_bottom_left);
    rect.SetRadiusYTopLeft(radii.y_top_left);
    rect.SetRadiusYTopRight(radii.y_top_right);
    rect.SetRadiusYBottomRight(radii.y_bottom_right);
    rect.SetRadiusYBottomLeft(radii.y_bottom_left);
  }
  return rect;
}

}  // namespace tasm
}  // namespace lynx
