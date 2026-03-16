// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_view_manager_service.h"

#include <memory>

#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

OverlayViewManagerService::OverlayViewManagerService(
    FlutterWindowsEngine* engine)
    : engine_(engine),
      overlay_view_manager_(std::make_unique<OverlayViewManager>(engine)) {}

FlutterWindowsEngine* OverlayViewManagerService::GetEngine() { return engine_; }

OverlayViewManager::OverlayViewManager(FlutterWindowsEngine* engine)
    : engine_(engine) {}

OverlayViewManager* OverlayViewManagerService::GetOverlayViewManager() {
  return overlay_view_manager_.get();
}

OverlayViewController* OverlayViewManager::CreateView(
    int64_t node_id, OverlayViewController::OverlayViewType type,
    const RECT& rect, HWND parent, const std::wstring& title) {
  std::lock_guard<std::mutex> locker(mutex_);
  if (view_registry_.find(node_id) == view_registry_.end()) {
    view_registry_[node_id] = std::make_unique<OverlayViewController>(
        engine_, type, rect, parent, title);
  }
  return view_registry_[node_id].get();
}

void OverlayViewManager::RemoveView(int64_t node_id) {
  std::lock_guard<std::mutex> locker(mutex_);
  if (view_registry_.find(node_id) != view_registry_.end()) {
    view_registry_.erase(node_id);
  }
}

FlutterViewId OverlayViewManager::FindView(int64_t node_id) const {
  std::lock_guard<std::mutex> locker(mutex_);
  auto iterator = view_registry_.find(node_id);
  if (iterator == view_registry_.end()) {
    return kImplicitViewId;
  }

  return iterator->second->view_id();
}

}  // namespace clay
