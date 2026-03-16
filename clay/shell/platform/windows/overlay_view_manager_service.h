
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_MANAGER_SERVICE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_MANAGER_SERVICE_H_

#include <Windows.h>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "clay/common/service/service.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "clay/shell/platform/windows/overlay_view_controller.h"

namespace clay {

class FlutterWindowsEngine;

class OverlayViewManager {
 public:
  explicit OverlayViewManager(FlutterWindowsEngine* engine);
  OverlayViewController* CreateView(int64_t node_id,
                                    OverlayViewController::OverlayViewType type,
                                    const RECT& rect, HWND parent = nullptr,
                                    const std::wstring& title = L"");
  void RemoveView(int64_t node_id);
  FlutterViewId FindView(int64_t node_id) const;

 private:
  std::unordered_map<int64_t, std::unique_ptr<OverlayViewController>>
      view_registry_;
  mutable std::mutex mutex_;
  FlutterWindowsEngine* engine_;
};

class OverlayViewManagerService
    : public clay::Service<OverlayViewManagerService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kManualRegister> {
 public:
  explicit OverlayViewManagerService(FlutterWindowsEngine* engine);
  FlutterWindowsEngine* GetEngine();
  OverlayViewManager* GetOverlayViewManager();

 private:
  FlutterWindowsEngine* engine_ = nullptr;
  std::unique_ptr<OverlayViewManager> overlay_view_manager_;
  BASE_DISALLOW_COPY_AND_ASSIGN(OverlayViewManagerService);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_MANAGER_SERVICE_H_
