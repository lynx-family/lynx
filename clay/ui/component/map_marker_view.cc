// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/map_marker_view.h"

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

}  // namespace clay
