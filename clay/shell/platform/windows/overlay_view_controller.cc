// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_view_controller.h"

#include <utility>

#include "clay/shell/platform/windows/overlay_windows_view.h"

namespace clay {

OverlayViewController::OverlayViewController(FlutterWindowsEngine* engine,
                                             OverlayViewType type,
                                             const RECT& rect, HWND parent,
                                             const std::wstring& title)
    : engine_(engine) {
  if (type == OverlayViewType::kChild) {
    std::unique_ptr<WindowBindingHandler> window_wrapper =
        std::make_unique<FlutterWindow>(parent, rect.left, rect.top,
                                        rect.right - rect.left,
                                        rect.bottom - rect.top);
    child_view_ = engine_->CreateOverlayView(std::move(window_wrapper));
    flutter_view_ = child_view_.get();
  }
}

FlutterViewId OverlayViewController::view_id() {
  return flutter_view_->view_id();
}

HWND OverlayViewController::GetWindowHandle() {
  return window_ ? window_->GetWindowHandle()
                 : flutter_view_->GetWindowHandle();
}

}  // namespace clay
