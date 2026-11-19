// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "base/include/geometry/point.h"
#include "base/include/geometry/rect.h"
#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_point_converter.h"
#include "core/renderer/dom/fiber/text_element.h"

namespace lynx {
namespace tasm {
namespace {

struct InlineTextBounds {
  TextElement* paragraph;
  base::geometry::FloatRect rect;
};

std::optional<InlineTextBounds> ResolveInlineTextBounds(Element* element) {
  if (element->will_destroy() || !element->IsAttached() ||
      !element->HasEventListener("tap")) {
    return std::nullopt;
  }
  // Inline text has no independent layout box. Its bounds are measured in the
  // content coordinates of the nearest laid-out paragraph.
  auto* ancestor = element->render_parent();
  while (ancestor && ancestor->is_text() && ancestor->is_inline_element()) {
    ancestor = ancestor->render_parent();
  }
  if (!ancestor || !ancestor->is_text()) {
    return std::nullopt;
  }
  auto* paragraph = static_cast<TextElement*>(ancestor);
  const auto* geometry = paragraph->GetInlineTextGeometry(element->impl_id());
  if (!geometry) {
    return std::nullopt;
  }
  const auto& padding = paragraph->paddings();
  const auto& border = paragraph->borders();
  const auto& bounds = geometry->bounds;
  return InlineTextBounds{
      paragraph,
      {{bounds.X() + padding[starlight::kLeft] + border[starlight::kLeft],
        bounds.Y() + padding[starlight::kTop] + border[starlight::kTop]},
       {bounds.Width(), bounds.Height()}}};
}

fml::RefPtr<lepus::CArray> ProjectGeometryPoints(
    const lepus::CArray& points, Element* page, Element* geometry_element,
    const base::geometry::FloatRect& local_rect, float scale) {
  BASE_STATIC_STRING_DECL(kInputXPx, "xPx");
  BASE_STATIC_STRING_DECL(kInputYPx, "yPx");
  base::InlineVector<base::geometry::FloatPoint, 8> converted;
  for (size_t index = 0; index < points.size(); ++index) {
    const auto& point = points.get(index);
    const auto x = point.GetProperty(kInputXPx);
    const auto y = point.GetProperty(kInputYPx);
    if (!x.IsNumber() || !y.IsNumber()) {
      return nullptr;
    }
    converted.emplace_back(static_cast<float>(x.Number()) / scale,
                           static_cast<float>(y.Number()) / scale);
  }
  if (!ConvertPointsBetweenElements(converted.data(), converted.size(), page,
                                    geometry_element)) {
    return nullptr;
  }
  auto projected = lepus::CArray::Create();
  for (const auto& point : converted) {
    auto value = lepus::Value(lepus::Dictionary::Create());
    value.SetProperty(kInputXPx,
                      lepus::Value((point.X() - local_rect.X()) * scale));
    value.SetProperty(kInputYPx,
                      lepus::Value((point.Y() - local_rect.Y()) * scale));
    projected->emplace_back(std::move(value));
  }
  return projected;
}

bool WriteGeometryBounds(lepus::Value& result,
                         const base::geometry::FloatRect& local_rect,
                         Element* geometry_element, Element* destination,
                         float scale, bool clip) {
  BASE_STATIC_STRING_DECL(kInputLeftPx, "leftPx");
  BASE_STATIC_STRING_DECL(kInputTopPx, "topPx");
  BASE_STATIC_STRING_DECL(kInputRightPx, "rightPx");
  BASE_STATIC_STRING_DECL(kInputBottomPx, "bottomPx");
  auto rect = ConvertRectBetweenElements(local_rect, geometry_element,
                                         destination, clip);
  if (!rect) {
    return false;
  }
  result.SetProperty(kInputLeftPx, lepus::Value(rect->X() * scale));
  result.SetProperty(kInputTopPx, lepus::Value(rect->Y() * scale));
  result.SetProperty(kInputRightPx, lepus::Value(rect->MaxX() * scale));
  result.SetProperty(kInputBottomPx, lepus::Value(rect->MaxY() * scale));
  return true;
}

lepus::Value BuildGeometryTreePath(Element* element) {
  auto tree_path = lepus::CArray::Create();
  std::vector<int32_t> indices;
  // Preserve the queried element's logical path, including virtual inline
  // nodes.
  for (auto* node = element; node->parent(); node = node->parent()) {
    indices.push_back(node->parent()->IndexOf(node));
  }
  for (auto index = indices.rbegin(); index != indices.rend(); ++index) {
    tree_path->emplace_back(*index);
  }
  return lepus::Value(std::move(tree_path));
}

}  // namespace

lepus::Value Element::QueryGeometry(const lepus::Value& request) {
  BASE_STATIC_STRING_DECL(kInputValid, "valid");
  BASE_STATIC_STRING_DECL(kInputPoints, "points");
  BASE_STATIC_STRING_DECL(kInputTreePath, "treePath");
  BASE_STATIC_STRING_DECL(kInputWidthPx, "widthPx");
  BASE_STATIC_STRING_DECL(kInputHeightPx, "heightPx");
  BASE_STATIC_STRING_DECL(kInputRelativeTo, "relativeTo");
  BASE_STATIC_STRING_DECL(kInputRect, "rect");
  BASE_STATIC_STRING_DECL(kInputClip, "clip");
  BASE_STATIC_STRING_DECL(kInputLeftPx, "leftPx");
  BASE_STATIC_STRING_DECL(kInputTopPx, "topPx");
  BASE_STATIC_STRING_DECL(kInputRightPx, "rightPx");
  BASE_STATIC_STRING_DECL(kInputBottomPx, "bottomPx");
  auto result = lepus::Value(lepus::Dictionary::Create());
  result.SetProperty(kInputValid, lepus::Value(false));
  auto* page = element_manager_->root();
  const float scale =
      element_manager_->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit();
  Element* geometry_element = this;
  base::geometry::FloatRect local_rect({0, 0}, {width(), height()});
  // The inline flag also covers views, images and wrappers. Only text spans
  // use the paragraph's measured character-range bounds here.
  if (is_text() && is_inline_element()) {
    const auto geometry = ResolveInlineTextBounds(this);
    if (!geometry) {
      return result;
    }
    geometry_element = geometry->paragraph;
    local_rect = geometry->rect;
  }
  const auto points = request.GetProperty(kInputPoints);
  fml::RefPtr<lepus::CArray> projected;
  if (points.IsArray()) {
    projected = ProjectGeometryPoints(*points.Array(), page, geometry_element,
                                      local_rect, scale);
    if (!projected) {
      return result;
    }
  } else {
    auto* destination = page;
    const auto relative_to = request.GetProperty(kInputRelativeTo);
    if (relative_to.IsNumber() && relative_to.Number() > 0) {
      destination = element_manager_->node_manager()->Get(relative_to.Number());
      if (!destination) {
        return result;
      }
    }
    auto bounds_rect = local_rect;
    const auto requested_rect = request.GetProperty(kInputRect);
    if (requested_rect.IsObject()) {
      const auto left = requested_rect.GetProperty(kInputLeftPx);
      const auto top = requested_rect.GetProperty(kInputTopPx);
      const auto right = requested_rect.GetProperty(kInputRightPx);
      const auto bottom = requested_rect.GetProperty(kInputBottomPx);
      if (!left.IsNumber() || !top.IsNumber() || !right.IsNumber() ||
          !bottom.IsNumber() || !std::isfinite(left.Number()) ||
          !std::isfinite(top.Number()) || !std::isfinite(right.Number()) ||
          !std::isfinite(bottom.Number()) || right.Number() < left.Number() ||
          bottom.Number() < top.Number()) {
        return result;
      }
      // Requested rectangles are local to the queried element, including
      // inline text whose layout geometry belongs to its paragraph.
      bounds_rect = {
          {local_rect.X() + static_cast<float>(left.Number()) / scale,
           local_rect.Y() + static_cast<float>(top.Number()) / scale},
          {static_cast<float>(right.Number() - left.Number()) / scale,
           static_cast<float>(bottom.Number() - top.Number()) / scale}};
    }
    const auto clip = request.GetProperty(kInputClip);
    if (!WriteGeometryBounds(result, bounds_rect, geometry_element, destination,
                             scale, !clip.IsBool() || clip.Bool())) {
      return result;
    }
    result.SetProperty(kInputTreePath, BuildGeometryTreePath(this));
    projected = lepus::CArray::Create();
  }
  result.SetProperty(kInputValid, lepus::Value(true));
  result.SetProperty(kInputWidthPx, lepus::Value(local_rect.Width() * scale));
  result.SetProperty(kInputHeightPx, lepus::Value(local_rect.Height() * scale));
  result.SetProperty(kInputPoints, lepus::Value(std::move(projected)));
  return result;
}

}  // namespace tasm
}  // namespace lynx
