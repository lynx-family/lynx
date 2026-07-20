// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_MAP_MARKER_VIEW_H_
#define CLAY_UI_COMPONENT_MAP_MARKER_VIEW_H_

#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/component/native_view.h"

namespace clay {

class MapMarkerView : public WithTypeInfo<MapMarkerView, NativeView> {
 public:
  MapMarkerView(int id, PageView* page_view);
  ~MapMarkerView() override = default;

  FloatSize SnapshotSize();
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_MAP_MARKER_VIEW_H_
