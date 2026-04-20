// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_view_controller.h"

#include <utility>

#include "clay/shell/platform/windows/overlay_windows_view.h"

namespace clay {

OverlayViewController::OverlayViewController(
    FlutterWindowsEngine* engine, OverlayWindowType type,
    const OverlayWindowSizeRequest& preferred_size, HWND parent,
    const std::wstring& title)
    : engine_(engine) {
  if (type == OverlayWindowType::kChild) {
    std::unique_ptr<WindowBindingHandler> window_wrapper =
        std::make_unique<FlutterWindow>(parent, CW_USEDEFAULT, CW_USEDEFAULT,
                                        preferred_size.preferred_view_width,
                                        preferred_size.preferred_view_height);
    child_view_ = engine_->CreateOverlayView(std::move(window_wrapper));
    overlay_view_ = child_view_.get();
    ShowWindow(child_view_->GetWindowHandle(), SW_HIDE);
  }
  type_ = type;
}

FlutterViewId OverlayViewController::view_id() {
  if (!overlay_view_) {
    return kImplicitViewId;
  }
  return overlay_view_->view_id();
}

HWND OverlayViewController::GetWindowHandle() {
  if (!overlay_view_) {
    return nullptr;
  }
  return overlay_view_->GetWindowHandle();
}

OverlayWindowType OverlayViewController::GetType() { return type_; }

FlutterWindowsView* OverlayViewController::GetView() { return overlay_view_; }

void OverlayViewController::UpdatePosition(int left, int top, int width,
                                           int height) {
  if (child_view_) {
    SetWindowPos(child_view_->GetWindowHandle(), NULL, left, top, width, height,
                 SWP_NOZORDER);
  }
}

}  // namespace clay
