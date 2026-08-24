// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/overlay_platform_plugin_win.h"

#include <Windows.h>

#include <utility>

#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

OverlayPlatformPluginWin::OverlayPlatformPluginWin(FlutterWindowsEngine* engine,
                                                   OverlayViewManager* manager)
    : engine_(engine), manager_(manager) {}

void OverlayPlatformPluginWin::ChangeVisibility(bool visible) {
  if (!view_) {
    return;
  }
  HWND window = view_->GetWindowHandle();
  if (visible) {
    ShowWindow(window, SW_SHOW);
  } else {
    ShowWindow(window, SW_HIDE);
  }
}

void OverlayPlatformPluginWin::SetEventThrough(bool event_through) {
  event_through_ = event_through;
  if (view_) {
    view_->SetEventThrough(event_through);
  }
}

void OverlayPlatformPluginWin::SetSize(int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  preferred_width_ = width;
  preferred_height_ = height;
  EnsureView();
}

void OverlayPlatformPluginWin::OnDetachFromTree() { ChangeVisibility(false); }

void OverlayPlatformPluginWin::OnViewDestroy() {
  if (manager_ && node_id_ != -1) {
    manager_->RemoveView(node_id_);
  }
  view_.reset();
  node_id_ = -1;
}

void OverlayPlatformPluginWin::InitPlatformOverlay(
    std::shared_ptr<Actor<fml::WeakPtr<OverlayListener>>> overlay_listener,
    int id, std::string tag, ExternalViewPlugin* recording_plugin) {
  OverlayPlatformPlugin::InitPlatformOverlay(std::move(overlay_listener), id,
                                             std::move(tag), recording_plugin);
  if (!engine_ || !engine_->view() || !manager_) {
    return;
  }
  node_id_ = id;
  EnsureView();
}

void OverlayPlatformPluginWin::EnsureView() {
  if (view_ || node_id_ == -1 || preferred_width_ <= 0 ||
      preferred_height_ <= 0 || !engine_ || !engine_->view() || !manager_) {
    return;
  }
  OverlayWindowCreationRequest request = {
      .preferred_size = {preferred_width_, preferred_height_},
      .title = L"cover-view"};
  view_ = manager_->CreateView(node_id_, OverlayWindowType::kChild, request,
                               engine_->view()->GetWindowHandle());
  if (view_) {
    view_->SetEventThrough(event_through_);
  }
}

std::unique_ptr<OverlayPlatformPlugin>
OverlayPlatformServiceWin::CreateOverlayPlatformPlugin() {
  if (!overlay_view_manager_service_) {
    return nullptr;
  }
  return std::make_unique<OverlayPlatformPluginWin>(
      overlay_view_manager_service_->GetEngine(),
      overlay_view_manager_service_->GetOverlayWindowManager());
}

void OverlayPlatformServiceWin::OnInit(ServiceManager& service_manager,
                                       const PlatformServiceContext& ctx) {
  overlay_view_manager_service_ =
      service_manager.GetService<OverlayViewManagerService>();
}

void OverlayPlatformServiceWin::OnDestroy() {
  overlay_view_manager_service_ = nullptr;
}

}  // namespace clay
