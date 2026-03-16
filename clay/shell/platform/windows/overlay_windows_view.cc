
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_windows_view.h"

#include <memory>
#include <utility>

namespace clay {

OverlayWindowsView::OverlayWindowsView(
    std::unique_ptr<WindowBindingHandler> window_binding)
    : FlutterWindowsView(std::move(window_binding)) {}

OverlayWindowsView::OverlayWindowsView(
    FlutterViewId view_id, std::unique_ptr<WindowBindingHandler> window_binding)
    : FlutterWindowsView(view_id, std::move(window_binding)) {}

OverlayWindowsView::~OverlayWindowsView() { Destroy(); }

void OverlayWindowsView::SendWindowMetrics(size_t width, size_t height,
                                           double dpi_scale) const {}

void OverlayWindowsView::Destroy() {
  if (GetEngine()->running()) {
    GetEngine()->RemoveOverlayView(view_id());
  }
}

}  // namespace clay
