// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_point_converter.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "core/public/common_constants.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/transforms/transform_operations_helper.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "gfx/geometry/matrix44.h"

namespace lynx {
namespace tasm {
namespace {

Element* FindLayoutParent(Element* element, Element* root) {
  auto* layout_node = element->slnode();
  if (layout_node == nullptr) {
    return nullptr;
  }
  if (layout_node->IsNewFixed()) {
    return root;
  }
  const auto* parent_layout_node = layout_node->ParentLayoutObject();
  if (parent_layout_node == nullptr) {
    return nullptr;
  }
  if (root->slnode() == parent_layout_node) {
    return root;
  }
  for (auto* candidate = element->render_parent(); candidate != nullptr;
       candidate = candidate->render_parent()) {
    if (candidate->slnode() == parent_layout_node) {
      return candidate;
    }
  }
  return nullptr;
}

base::flex_optional<base::geometry::FloatPoint> GetPageCoordinateOffset(
    Element* element, CoordinateSpace space) {
  if (element == nullptr || element->element_manager() == nullptr) {
    return {};
  }
  const auto& snapshot =
      element->element_manager()->GetPageCoordinateSnapshot();
  switch (space) {
    case CoordinateSpace::kWindow:
      if (snapshot.has_window_offset && std::isfinite(snapshot.window_x) &&
          std::isfinite(snapshot.window_y)) {
        return base::geometry::FloatPoint(snapshot.window_x, snapshot.window_y);
      }
      break;
    case CoordinateSpace::kScreen:
      if (snapshot.has_screen_offset && std::isfinite(snapshot.screen_x) &&
          std::isfinite(snapshot.screen_y)) {
        return base::geometry::FloatPoint(snapshot.screen_x, snapshot.screen_y);
      }
      break;
    case CoordinateSpace::kElement:
      break;
  }
  return {};
}

bool BuildPathToRoot(Element* element, Element* root,
                     base::InlineVector<Element*, 8>& path) {
  while (element != nullptr) {
    if (element->will_destroy() || !element->IsAttached() ||
        element->slnode() == nullptr ||
        std::find(path.begin(), path.end(), element) != path.end()) {
      return false;
    }
    path.push_back(element);
    if (element == root) {
      return true;
    }
    if (!element->attached_to_layout_parent() &&
        !element->slnode()->IsNewFixed()) {
      return false;
    }
    element = FindLayoutParent(element, root);
  }
  return false;
}

gfx::Matrix44 LocalTransform(Element* element) {
  gfx::Matrix44 result;
  auto* style = element->computed_css_style();
  if (style == nullptr || !style->HasTransform()) {
    return result;
  }

  const auto layout_result = element->layout_result();
  const auto transform_operations = transforms::ConvertToGfxTransformOperations(
      *style->GetTransformData(), layout_result.size_.width_,
      layout_result.size_.height_);
  const auto transform = transform_operations.ApplyRemaining(
      0, layout_result.size_.width_, layout_result.size_.height_);
  float origin_x = 0.5f * layout_result.size_.width_;
  float origin_y = 0.5f * layout_result.size_.height_;
  if (style->HasTransformOrigin()) {
    const auto& origin = *style->GetTransformOriginData();
    origin_x = starlight::NLengthToLayoutUnit(
                   origin.x, starlight::LayoutUnit(layout_result.size_.width_))
                   .ToFloat();
    origin_y = starlight::NLengthToLayoutUnit(
                   origin.y, starlight::LayoutUnit(layout_result.size_.height_))
                   .ToFloat();
  }
  result.preTranslate(origin_x, origin_y, 0.f);
  result.preConcat(transform);
  result.preTranslate(-origin_x, -origin_y, 0.f);
  return result;
}

bool ParentScrolls(Element* parent, Element* child) {
  if (child->slnode()->IsNewFixed()) {
    return false;
  }
  const auto scroll = parent->slnode()->attr_map().getScroll();
  return parent->is_list() || (scroll.has_value() && *scroll);
}

base::geometry::FloatRect BoundsOfConvertedCorners(
    const base::geometry::FloatPoint (&corners)[4]) {
  float left = corners[0].X();
  float top = corners[0].Y();
  float right = corners[0].X();
  float bottom = corners[0].Y();
  for (size_t index = 1; index < std::size(corners); ++index) {
    left = std::min(left, corners[index].X());
    top = std::min(top, corners[index].Y());
    right = std::max(right, corners[index].X());
    bottom = std::max(bottom, corners[index].Y());
  }
  return {{left, top}, {right - left, bottom - top}};
}

void IntersectAxisAlignedBounds(base::geometry::FloatRect& rect,
                                const base::geometry::FloatRect& clip,
                                bool clip_x, bool clip_y) {
  float left = rect.X();
  float top = rect.Y();
  float right = rect.MaxX();
  float bottom = rect.MaxY();
  if (clip_x) {
    left = std::max(left, clip.X());
    right = std::min(right, clip.MaxX());
    if (right < left) {
      right = left;
    }
  }
  if (clip_y) {
    top = std::max(top, clip.Y());
    bottom = std::min(bottom, clip.MaxY());
    if (bottom < top) {
      bottom = top;
    }
  }
  rect = {{left, top}, {right - left, bottom - top}};
}

lepus::Value MakePointValue(float x, float y) {
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  auto result = lepus::Dictionary::Create();
  result->SetValue(kX, x);
  result->SetValue(kY, y);
  return lepus::Value(std::move(result));
}

lepus::Value MakeRectValue(const base::geometry::FloatRect& rect,
                           float layouts_unit_per_px) {
  if (!std::isfinite(layouts_unit_per_px) || layouts_unit_per_px <= 0.f) {
    return {};
  }
  const float left = rect.X() / layouts_unit_per_px;
  const float top = rect.Y() / layouts_unit_per_px;
  const float right = rect.MaxX() / layouts_unit_per_px;
  const float bottom = rect.MaxY() / layouts_unit_per_px;
  if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) ||
      !std::isfinite(bottom)) {
    return {};
  }
  BASE_STATIC_STRING_DECL(kLeft, "left");
  BASE_STATIC_STRING_DECL(kTop, "top");
  BASE_STATIC_STRING_DECL(kRight, "right");
  BASE_STATIC_STRING_DECL(kBottom, "bottom");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  auto result = lepus::Dictionary::Create();
  result->SetValue(kLeft, left);
  result->SetValue(kTop, top);
  result->SetValue(kRight, right);
  result->SetValue(kBottom, bottom);
  result->SetValue(kWidth, right - left);
  result->SetValue(kHeight, bottom - top);
  return lepus::Value(std::move(result));
}

}  // namespace

base::flex_optional<base::geometry::FloatPoint> ConvertPointBetweenElements(
    const base::geometry::FloatPoint& point, Element* from, Element* to) {
  if (from == nullptr || to == nullptr || from->element_manager() == nullptr ||
      from->element_manager() != to->element_manager() ||
      !std::isfinite(point.X()) || !std::isfinite(point.Y())) {
    return {};
  }

  auto* manager = from->element_manager();
  const auto embedded_mode = manager->GetPageOptions().GetEmbeddedMode();
  if ((static_cast<int32_t>(embedded_mode) &
       static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT)) == 0) {
    return {};
  }

  auto* root = manager->root();
  if (root == nullptr || root->slnode() == nullptr) {
    return {};
  }

  base::InlineVector<Element*, 8> source_path;
  base::InlineVector<Element*, 8> target_path;
  if (!BuildPathToRoot(from, root, source_path) ||
      !BuildPathToRoot(to, root, target_path)) {
    return {};
  }
  if (from == to) {
    return point;
  }

  size_t source_common_ancestor_index = source_path.size() - 1;
  size_t target_common_ancestor_index = target_path.size() - 1;
  while (source_common_ancestor_index > 0 && target_common_ancestor_index > 0 &&
         source_path[source_common_ancestor_index - 1] ==
             target_path[target_common_ancestor_index - 1]) {
    --source_common_ancestor_index;
    --target_common_ancestor_index;
  }

  float converted[2] = {point.X(), point.Y()};
  for (size_t index = 0; index < source_common_ancestor_index; ++index) {
    auto* child = source_path[index];
    auto* parent = source_path[index + 1];
    LocalTransform(child).mapPoint(converted, converted);
    converted[0] += child->left();
    converted[1] += child->top();
    converted[0] += child->sticky_translation_x();
    converted[1] += child->sticky_translation_y();
    if (ParentScrolls(parent, child)) {
      converted[0] -= parent->scroll_offset_x();
      converted[1] -= parent->scroll_offset_y();
    }
  }

  for (size_t index = target_common_ancestor_index; index > 0; --index) {
    auto* child = target_path[index - 1];
    auto* parent = target_path[index];
    if (ParentScrolls(parent, child)) {
      converted[0] += parent->scroll_offset_x();
      converted[1] += parent->scroll_offset_y();
    }
    converted[0] -= child->sticky_translation_x();
    converted[1] -= child->sticky_translation_y();
    converted[0] -= child->left();
    converted[1] -= child->top();
    gfx::Matrix44 inverse;
    if (!LocalTransform(child).invert(&inverse)) {
      return {};
    }
    inverse.mapPoint(converted, converted);
  }

  if (!std::isfinite(converted[0]) || !std::isfinite(converted[1])) {
    return {};
  }
  return base::geometry::FloatPoint(converted[0], converted[1]);
}

base::flex_optional<base::geometry::FloatRect> ConvertRectBetweenElements(
    const base::geometry::FloatRect& rect, Element* from, Element* to,
    bool clip_bounds) {
  if (!std::isfinite(rect.X()) || !std::isfinite(rect.Y()) ||
      !std::isfinite(rect.MaxX()) || !std::isfinite(rect.MaxY()) ||
      rect.Width() < 0.f || rect.Height() < 0.f) {
    return {};
  }

  const base::geometry::FloatPoint input_corners[] = {
      {rect.X(), rect.Y()},
      {rect.MaxX(), rect.Y()},
      {rect.X(), rect.MaxY()},
      {rect.MaxX(), rect.MaxY()},
  };
  base::geometry::FloatPoint converted_corners[4];
  for (size_t index = 0; index < std::size(input_corners); ++index) {
    auto converted =
        ConvertPointBetweenElements(input_corners[index], from, to);
    if (!converted.has_value()) {
      return {};
    }
    converted_corners[index] = *converted;
  }
  auto result = BoundsOfConvertedCorners(converted_corners);
  if (!clip_bounds) {
    return result;
  }

  if (from == nullptr || from->element_manager() == nullptr) {
    return {};
  }
  auto* root = from->element_manager()->root();
  base::InlineVector<Element*, 8> source_path;
  if (root == nullptr || !BuildPathToRoot(from, root, source_path)) {
    return {};
  }

  // A transformed clipping box is represented by its axis-aligned bounding
  // box here, matching the rect result contract above. Point conversion remains
  // exact for invertible transforms.
  for (auto* clipper : source_path) {
    auto* style = clipper->computed_css_style();
    if (style == nullptr) {
      continue;
    }
    const bool clip_x = !style->IsOverflowX();
    const bool clip_y = !style->IsOverflowY();
    if (!clip_x && !clip_y) {
      continue;
    }
    auto clip = ConvertRectBetweenElements(
        {{0.f, 0.f}, {clipper->width(), clipper->height()}}, clipper, to,
        false);
    if (!clip.has_value()) {
      return {};
    }
    IntersectAxisAlignedBounds(result, *clip, clip_x, clip_y);
  }
  return result;
}

base::flex_optional<base::geometry::FloatPoint> ConvertPointToPageCoordinate(
    const base::geometry::FloatPoint& point, Element* from,
    CoordinateSpace space) {
  if (from == nullptr || from->element_manager() == nullptr) {
    return {};
  }
  auto* root = from->element_manager()->root();
  auto offset = GetPageCoordinateOffset(from, space);
  if (root == nullptr || !offset.has_value()) {
    return {};
  }
  auto converted = ConvertPointBetweenElements(point, from, root);
  if (!converted.has_value()) {
    return {};
  }
  converted->SetX(converted->X() + offset->X());
  converted->SetY(converted->Y() + offset->Y());
  return converted;
}

base::flex_optional<base::geometry::FloatRect> ConvertRectToPageCoordinate(
    const base::geometry::FloatRect& rect, Element* from, bool clip_bounds,
    CoordinateSpace space) {
  if (from == nullptr || from->element_manager() == nullptr) {
    return {};
  }
  auto* root = from->element_manager()->root();
  auto offset = GetPageCoordinateOffset(from, space);
  if (root == nullptr || !offset.has_value()) {
    return {};
  }
  auto converted = ConvertRectBetweenElements(rect, from, root, clip_bounds);
  if (!converted.has_value()) {
    return {};
  }
  converted->Move(offset->X(), offset->Y());
  return converted;
}

lepus::Value ConvertPointForElementAPI(double x, double y, Element* from,
                                       Element* to, CoordinateSpace space) {
  if (from == nullptr || !std::isfinite(x) || !std::isfinite(y)) {
    return {};
  }
  const float layouts_unit_per_px = from->GetLayoutsUnitPerPx();
  if (!std::isfinite(layouts_unit_per_px) || layouts_unit_per_px <= 0.f) {
    return {};
  }

  const base::geometry::FloatPoint input{
      static_cast<float>(x) * layouts_unit_per_px,
      static_cast<float>(y) * layouts_unit_per_px};
  base::flex_optional<base::geometry::FloatPoint> converted;
  switch (space) {
    case CoordinateSpace::kElement:
      converted = ConvertPointBetweenElements(input, from, to);
      break;
    case CoordinateSpace::kWindow:
    case CoordinateSpace::kScreen:
      converted = ConvertPointToPageCoordinate(input, from, space);
      break;
  }
  if (!converted.has_value()) {
    return {};
  }
  return MakePointValue(converted->X() / layouts_unit_per_px,
                        converted->Y() / layouts_unit_per_px);
}

lepus::Value ConvertRectForElementAPI(double left, double top, double right,
                                      double bottom, Element* from, Element* to,
                                      bool clip_bounds, CoordinateSpace space) {
  if (from == nullptr || !std::isfinite(left) || !std::isfinite(top) ||
      !std::isfinite(right) || !std::isfinite(bottom) || right < left ||
      bottom < top) {
    return {};
  }
  const float layouts_unit_per_px = from->GetLayoutsUnitPerPx();
  if (!std::isfinite(layouts_unit_per_px) || layouts_unit_per_px <= 0.f) {
    return {};
  }

  const base::geometry::FloatRect input{
      {static_cast<float>(left) * layouts_unit_per_px,
       static_cast<float>(top) * layouts_unit_per_px},
      {static_cast<float>(right - left) * layouts_unit_per_px,
       static_cast<float>(bottom - top) * layouts_unit_per_px}};
  base::flex_optional<base::geometry::FloatRect> converted;
  switch (space) {
    case CoordinateSpace::kElement:
      converted = ConvertRectBetweenElements(input, from, to, clip_bounds);
      break;
    case CoordinateSpace::kWindow:
    case CoordinateSpace::kScreen:
      converted = ConvertRectToPageCoordinate(input, from, clip_bounds, space);
      break;
  }
  if (!converted.has_value()) {
    return {};
  }
  return MakeRectValue(*converted, layouts_unit_per_px);
}

}  // namespace tasm
}  // namespace lynx
