
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_WINDOWS_VIEW_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_WINDOWS_VIEW_H_

#include <memory>

#include "clay/shell/platform/windows/flutter_windows_view.h"

namespace clay {

class OverlayWindowsView : public FlutterWindowsView {
 public:
  explicit OverlayWindowsView(
      std::unique_ptr<WindowBindingHandler> window_binding);
  OverlayWindowsView(FlutterViewId view_id,
                     std::unique_ptr<WindowBindingHandler> window_binding);
  ~OverlayWindowsView() override;
  void SendWindowMetrics(size_t width, size_t height,
                         double dpi_scale) const override;

 private:
  void Destroy();
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_WINDOWS_VIEW_H_
