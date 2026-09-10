
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_windows_view.h"

#include <Windows.h>

#include <memory>
#include <utility>

#include "clay/shell/platform/windows/flutter_window.h"
#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

OverlayWindowsView::OverlayWindowsView(
    std::unique_ptr<WindowBindingHandler> window_binding)
    : FlutterWindowsView(std::move(window_binding)) {}

OverlayWindowsView::OverlayWindowsView(
    FlutterViewId view_id, std::unique_ptr<WindowBindingHandler> window_binding)
    : FlutterWindowsView(view_id, std::move(window_binding)) {}

OverlayWindowsView::~OverlayWindowsView() { Destroy(); }

void OverlayWindowsView::SendWindowMetrics(size_t width, size_t height,
                                           double dpi_scale) const {
  // Intentionally empty.
  // Overlays act as secondary views; forwarding this would incorrectly
  // override the engine's primary layout dimensions.
}

void OverlayWindowsView::ConvertPointerPosition(double* x, double* y) {
  FlutterWindowsEngine* engine = GetEngine();
  FlutterWindowsView* implicit_view = engine ? engine->view() : nullptr;
  HWND source_window = GetWindowHandle();
  HWND target_window =
      implicit_view ? implicit_view->GetWindowHandle() : nullptr;
  if (!source_window || !target_window || source_window == target_window) {
    return;
  }

  POINT point = {static_cast<LONG>(*x), static_cast<LONG>(*y)};
  MapWindowPoints(source_window, target_window, &point, 1);
  *x = point.x;
  *y = point.y;
}

void OverlayWindowsView::Destroy() {
  if (auto* engine = GetEngine()) {
    engine->RemoveOverlayView(view_id());
  }
}

}  // namespace clay
