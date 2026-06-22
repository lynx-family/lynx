// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_MANAGER_SERVICE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_MANAGER_SERVICE_H_

#include <Windows.h>

#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

#include "clay/common/service/service.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "clay/shell/platform/windows/overlay_view_controller.h"

namespace clay {

class FlutterWindowsEngine;

struct OverlayWindowCreationRequest {
  OverlayWindowSizeRequest preferred_size;
  LPCWSTR title;
};

class OverlayViewManager {
 public:
  explicit OverlayViewManager(FlutterWindowsEngine* engine);
  virtual ~OverlayViewManager() = default;

  std::shared_ptr<OverlayViewController> CreateView(
      int64_t overlay_id, OverlayWindowType type,
      const OverlayWindowCreationRequest& request, HWND parent_handle);
  void RemoveView(int64_t overlay_id);
  std::shared_ptr<OverlayViewController> GetView(int64_t overlay_id);

 private:
  // The Flutter engine that owns this manager.
  FlutterWindowsEngine* const engine_;

  // A map of active windows. Used to destroy remaining windows on engine
  // shutdown.
  mutable std::shared_mutex active_views_mutex_;
  std::unordered_map<int64_t, std::shared_ptr<OverlayViewController>>
      active_views_;

  BASE_DISALLOW_COPY_AND_ASSIGN(OverlayViewManager);
};

class OverlayViewManagerService
    : public clay::Service<OverlayViewManagerService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kManualRegister> {
 public:
  explicit OverlayViewManagerService(FlutterWindowsEngine* engine);
  FlutterWindowsEngine* GetEngine();
  OverlayViewManager* GetOverlayWindowManager();

 private:
  FlutterWindowsEngine* engine_ = nullptr;
  std::unique_ptr<OverlayViewManager> overlay_window_manager_;
  BASE_DISALLOW_COPY_AND_ASSIGN(OverlayViewManagerService);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_VIEW_MANAGER_SERVICE_H_
