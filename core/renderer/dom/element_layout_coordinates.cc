// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_layout_coordinates.h"

#include <algorithm>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_point_converter.h"

namespace lynx::tasm {

LayoutCoordinatesResult QueryLayoutCoordinates(ElementManager* manager,
                                               int32_t uid,
                                               LayoutCoordinatesQuery query,
                                               double x, double y) {
  LayoutCoordinatesResult result;
  auto* element = manager->node_manager()->Get(uid);
  result.attached =
      element && !element->will_destroy() && element->IsAttached();
  if (query == LayoutCoordinatesQuery::kAttached) {
    result.valid = true;
    return result;
  }
  if (!result.attached) return result;
  const double pixels =
      manager->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit();
  if (query == LayoutCoordinatesQuery::kSize) {
    result.width = element->width() * pixels;
    result.height = element->height() * pixels;
    result.valid = true;
    return result;
  }
  if (!manager->IsLayoutInElementModeOn()) return result;
  auto* page = manager->root();
  float origin[2] = {0.f, 0.f};
  auto* target = page;
  switch (query) {
    case LayoutCoordinatesQuery::kLocalToRoot:
      target = element;
      while (target != nullptr && target != page && !target->IsOverlay()) {
        target = target->render_parent();
      }
      break;
    case LayoutCoordinatesQuery::kLocalToWindow:
    case LayoutCoordinatesQuery::kBoundsInWindow:
      if (!manager->painting_context()->GetRootViewLocationInWindow(origin)) {
        return result;
      }
      break;
    default:
      return result;
  }

  if (query != LayoutCoordinatesQuery::kBoundsInWindow) {
    auto point = ConvertPointBetweenElements(
        {static_cast<float>(x / pixels), static_cast<float>(y / pixels)},
        element, target);
    if (!point.has_value()) return result;
    result.x = (point->X() + origin[0]) * pixels;
    result.y = (point->Y() + origin[1]) * pixels;
  } else {
    auto rect = ConvertRectBetweenElements(
        {{0.f, 0.f}, {element->width(), element->height()}}, element, page,
        true);
    if (!rect.has_value()) return result;
    // The host viewport clips even when the Page has overflow: visible.
    const float left = std::max(0.f, rect->X());
    const float top = std::max(0.f, rect->Y());
    const float right = std::min(page->width(), rect->MaxX());
    const float bottom = std::min(page->height(), rect->MaxY());
    if (right > left && bottom > top) {
      result.left = (left + origin[0]) * pixels;
      result.top = (top + origin[1]) * pixels;
      result.right = (right + origin[0]) * pixels;
      result.bottom = (bottom + origin[1]) * pixels;
    }
  }
  result.valid = true;
  return result;
}

}  // namespace lynx::tasm
