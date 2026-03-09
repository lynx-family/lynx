// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/presenter_service_mac.h"

#import <Cocoa/Cocoa.h>

#include <cmath>

#include "clay/common/service/service_manager.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayOverlayView.h"

namespace clay {

std::shared_ptr<PresenterService> PresenterService::Create() {
  return std::make_shared<PresenterServiceMac>();
}

void PresenterServiceMac::OnBeforePresent() {
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  overlay_hit_rects_.clear();
}

void PresenterServiceMac::OnAfterPresent() {
  NSMutableArray<NSValue*>* rects = [NSMutableArray arrayWithCapacity:overlay_hit_rects_.size()];
  for (const CGRect& rect : overlay_hit_rects_) {
    [rects addObject:[NSValue valueWithRect:rect]];
  }
  [overlay_view_controller_service_->GetOverlayView() updateOpaqueRects:rects];
  [CATransaction commit];
}

void PresenterServiceMac::UpdateOverlay(const OverlayData& overlay_data) {
  auto* mac_overlay = static_cast<PlatformOverlayMac*>(overlay_data.overlay.get());
  if (!mac_overlay) {
    return;
  }
  const skity::Rect& rect = overlay_data.rect;
  CGRect hit_rect = mac_overlay->DisplayOverlaySurface(
      static_cast<int>(std::floor(rect.X())), static_cast<int>(std::floor(rect.Y())),
      static_cast<int>(std::ceil(rect.Width())), static_cast<int>(std::ceil(rect.Height())));
  overlay_hit_rects_.push_back(hit_rect);
}

void PresenterServiceMac::BringOverlayToFront(const PlatformOverlay& overlay) {
  static_cast<const PlatformOverlayMac&>(overlay).BringToFront();
}

void PresenterServiceMac::DisposeOverlay(PlatformOverlay& overlay) {
  static_cast<PlatformOverlayMac&>(overlay).RemoveFromParent();
}

void PresenterServiceMac::OnInit(clay::ServiceManager& service_manager,
                                 const clay::PlatformServiceContext& ctx) {
  overlay_view_controller_service_ = service_manager.GetService<OverlayViewControllerService>();
}

void PresenterServiceMac::OnDestroy() {
  overlay_view_controller_service_ = nullptr;
  overlay_hit_rects_.clear();
}

}  // namespace clay
