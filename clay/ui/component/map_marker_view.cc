// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/map_marker_view.h"

#include <algorithm>
#include <cmath>

#include "clay/ui/rendering/render_object.h"

namespace clay {
namespace {

constexpr char kMapMarkerTag[] = "x-map-marker-ng";

}  // namespace

MapMarkerView::MapMarkerView(int id, PageView* page_view)
    : WithTypeInfo(id, kMapMarkerTag, page_view) {
  SetRepaintBoundary(true);
  render_object()->SetShouldBuildIntoLayerTree(false);
}

FloatSize MapMarkerView::RasterSnapshotSize() {
  FloatSize snapshot_size(Width(), Height());
  BaseView* parent = Parent();
  if (parent == nullptr || GetChildren().empty()) {
    return snapshot_size;
  }

  float content_width = 0.f;
  float content_height = 0.f;
  bool has_valid_content = false;
  bool content_starts_inside_root_x = true;
  bool content_starts_inside_root_y = true;
  for (BaseView* child : GetChildren()) {
    if (child == nullptr || child->Width() <= 0.f || child->Height() <= 0.f) {
      continue;
    }
    has_valid_content = true;
    content_starts_inside_root_x =
        content_starts_inside_root_x && child->Left() >= 0.f;
    content_starts_inside_root_y =
        content_starts_inside_root_y && child->Top() >= 0.f;
    content_width =
        std::max(content_width, std::max(0.f, child->Left()) + child->Width() +
                                    std::max(0.f, child->MarginRight()));
    content_height =
        std::max(content_height, std::max(0.f, child->Top()) + child->Height() +
                                     std::max(0.f, child->MarginBottom()));
  }
  if (has_valid_content) {
    content_width += PaddingRight() + BorderRight();
    content_height += PaddingBottom() + BorderBottom();
  }

  // A marker root may fill one parent axis even though its child content is
  // smaller. Tighten only that single filled axis. Two filled axes are
  // ambiguous with an intentional full-size marker, so keep the root viewport.
  constexpr float kLayoutEpsilon = 0.5f;
  const bool fills_width = std::fabs(snapshot_size.width() -
                                     parent->ContentWidth()) <= kLayoutEpsilon;
  const bool fills_height =
      std::fabs(snapshot_size.height() - parent->ContentHeight()) <=
      kLayoutEpsilon;
  if (fills_width == fills_height) {
    return snapshot_size;
  }
  const bool tighten_width =
      fills_width && has_valid_content && content_starts_inside_root_x &&
      content_width > 0.f && content_width < snapshot_size.width();
  const bool tighten_height =
      fills_height && has_valid_content && content_starts_inside_root_y &&
      content_height > 0.f && content_height < snapshot_size.height();
  if (tighten_width) {
    snapshot_size.SetWidth(content_width);
  }
  if (tighten_height) {
    snapshot_size.SetHeight(content_height);
  }
  return snapshot_size;
}

}  // namespace clay
