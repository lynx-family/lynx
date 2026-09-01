// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/cover_view_platform_plugin_mac.h"

namespace clay {

CoverViewPlatformPluginMac::CoverViewPlatformPluginMac(ClayOverlayView* overlay_view)
    : overlay_view_(overlay_view) {}

void CoverViewPlatformPluginMac::SetEventsPassThrough(bool events_pass_through) {
  events_pass_through_ = events_pass_through;
  if (node_id_ != -1) {
    [overlay_view_ setEventsPassThrough:events_pass_through forViewId:node_id_];
  }
}

void CoverViewPlatformPluginMac::OnDetachFromTree() {
  if (node_id_ != -1) {
    [overlay_view_ removeOpaqueRectForViewId:node_id_];
  }
}

void CoverViewPlatformPluginMac::OnViewDestroy() {
  if (node_id_ != -1) {
    [overlay_view_ removeHitTestStateForViewId:node_id_];
    node_id_ = -1;
  }
}

void CoverViewPlatformPluginMac::Initialize(int id) {
  node_id_ = id;
  [overlay_view_ setEventsPassThrough:events_pass_through_ forViewId:node_id_];
}

std::unique_ptr<CoverViewPlatformPlugin>
CoverViewPlatformServiceMac::CreateCoverViewPlatformPlugin() {
  if (!overlay_view_controller_service_) {
    return nullptr;
  }
  return std::make_unique<CoverViewPlatformPluginMac>(
      overlay_view_controller_service_->GetOverlayView());
}

void CoverViewPlatformServiceMac::OnInit(ServiceManager& service_manager,
                                         const PlatformServiceContext& ctx) {
  overlay_view_controller_service_ = service_manager.GetService<OverlayViewControllerService>();
}

void CoverViewPlatformServiceMac::OnDestroy() { overlay_view_controller_service_ = nullptr; }

}  // namespace clay
