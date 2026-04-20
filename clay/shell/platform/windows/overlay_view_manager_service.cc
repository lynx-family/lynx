// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_view_manager_service.h"

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

OverlayViewManager::OverlayViewManager(FlutterWindowsEngine* engine)
    : engine_(engine) {}

std::shared_ptr<OverlayViewController> OverlayViewManager::CreateView(
    int64_t overlay_id, OverlayWindowType type,
    const OverlayWindowCreationRequest& request, HWND parent_handle) {
  auto view = std::make_shared<OverlayViewController>(
      engine_, type, request.preferred_size, parent_handle, L"ClayOverlayView");
  {
    std::unique_lock write_lock(active_views_mutex_);
    active_views_[overlay_id] = view;
  }
  return view;
}

void OverlayViewManager::RemoveView(int64_t overlay_id) {
  std::unique_lock write_lock(active_views_mutex_);
  active_views_.erase(overlay_id);
}

std::shared_ptr<OverlayViewController> OverlayViewManager::GetView(
    int64_t overlay_id) {
  std::shared_lock read_lock(active_views_mutex_);
  auto it = active_views_.find(overlay_id);
  if (it != active_views_.end()) {
    return it->second;
  }
  return nullptr;
}

OverlayViewManagerService::OverlayViewManagerService(
    FlutterWindowsEngine* engine)
    : engine_(engine),
      overlay_window_manager_(std::make_unique<OverlayViewManager>(engine)) {}

FlutterWindowsEngine* OverlayViewManagerService::GetEngine() { return engine_; }

OverlayViewManager* OverlayViewManagerService::GetOverlayWindowManager() {
  return overlay_window_manager_.get();
}

}  // namespace clay
