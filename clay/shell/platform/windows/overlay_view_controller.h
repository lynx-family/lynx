// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_CONTROLLER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_CONTROLLER_H_

#include <memory>

#include "clay/shell/platform/windows/flutter_window.h"
#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

struct OverlayWindowSizeRequest {
  int preferred_view_width;
  int preferred_view_height;
};

enum class OverlayWindowType {
  kChild,
};

class OverlayViewController {
 public:
  OverlayViewController(FlutterWindowsEngine* engine, OverlayWindowType type,
                        const OverlayWindowSizeRequest& preferred_size,
                        HWND parent = nullptr, const std::wstring& title = L"");
  ~OverlayViewController() = default;
  FlutterViewId view_id();
  HWND GetWindowHandle();
  FlutterWindowsView* GetView();
  OverlayWindowType GetType();

  void UpdatePosition(int left, int top, int width, int height);

 private:
  std::unique_ptr<FlutterWindowsView> child_view_;
  OverlayWindowType type_;
  FlutterWindowsView* overlay_view_ = nullptr;
  FlutterWindowsEngine* engine_;
};

}  // namespace clay
#endif  // CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_CONTROLLER_H_
