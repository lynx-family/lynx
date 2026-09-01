// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/cover_view_platform_plugin_win.h"

#include <Windows.h>

#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

CoverViewPlatformPluginWin::CoverViewPlatformPluginWin(
    FlutterWindowsEngine* engine, OverlayViewManager* manager)
    : engine_(engine), manager_(manager) {}

void CoverViewPlatformPluginWin::ChangeVisibility(bool visible) {
  if (!view_) {
    return;
  }
  HWND window = view_->GetWindowHandle();
  if (!window) {
    return;
  }
  if (visible) {
    ShowWindow(window, SW_SHOW);
  } else {
    ShowWindow(window, SW_HIDE);
  }
}

void CoverViewPlatformPluginWin::SetPreferredSize(int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  preferred_width_ = width;
  preferred_height_ = height;
  EnsureView();
}

void CoverViewPlatformPluginWin::OnDetachFromTree() { ChangeVisibility(false); }

void CoverViewPlatformPluginWin::OnViewDestroy() {
  if (manager_ && node_id_ != -1) {
    manager_->RemoveView(node_id_);
  }
  view_.reset();
  node_id_ = -1;
}

void CoverViewPlatformPluginWin::Initialize(int id) {
  if (!engine_ || !engine_->view() || !manager_) {
    return;
  }
  node_id_ = id;
  EnsureView();
}

void CoverViewPlatformPluginWin::EnsureView() {
  if (view_ || node_id_ == -1 || preferred_width_ <= 0 ||
      preferred_height_ <= 0 || !engine_ || !engine_->view() || !manager_) {
    return;
  }
  OverlayWindowCreationRequest request = {
      .preferred_size = {preferred_width_, preferred_height_},
      .title = L"ClayOverlayView"};
  view_ = manager_->GetView(node_id_);
  if (!view_) {
    view_ = manager_->CreateView(node_id_, OverlayWindowType::kChild, request,
                                 engine_->view()->GetWindowHandle());
  }
}

std::unique_ptr<CoverViewPlatformPlugin>
CoverViewPlatformServiceWin::CreateCoverViewPlatformPlugin() {
  if (!overlay_view_manager_service_) {
    return nullptr;
  }
  return std::make_unique<CoverViewPlatformPluginWin>(
      overlay_view_manager_service_->GetEngine(),
      overlay_view_manager_service_->GetOverlayWindowManager());
}

void CoverViewPlatformServiceWin::OnInit(ServiceManager& service_manager,
                                         const PlatformServiceContext& ctx) {
  overlay_view_manager_service_ =
      service_manager.GetService<OverlayViewManagerService>();
}

void CoverViewPlatformServiceWin::OnDestroy() {
  overlay_view_manager_service_ = nullptr;
}

}  // namespace clay
