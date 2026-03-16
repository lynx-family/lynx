// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_CONTROLLER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_CONTROLLER_H_

#include <memory>
#include <string>

#include "clay/shell//platform//windows/flutter_windows_engine.h"
#include "clay/shell//platform//windows/host_overlay_window.h"

namespace clay {

class OverlayViewController {
 public:
  enum class OverlayViewType {
    kChild,
  };
  explicit OverlayViewController(FlutterWindowsEngine* engine,
                                 OverlayViewType type, const RECT& rect,
                                 HWND parent = nullptr,
                                 const std::wstring& title = L"");
  ~OverlayViewController() = default;
  FlutterViewId view_id();
  HWND GetWindowHandle();

 private:
  std::unique_ptr<HostOverlayWindow> window_;
  std::unique_ptr<FlutterWindowsView> child_view_;

  FlutterWindowsView* flutter_view_ = nullptr;
  FlutterWindowsEngine* engine_;
};

}  // namespace clay
#endif  // CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_CONTROLLER_H_
