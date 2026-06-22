// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/presenter_service_win.h"

#include <cmath>

#include "clay/shell/platform/windows/platform_overlay_service_win.h"

namespace clay {

void PresenterServiceWin::UpdateOverlay(const OverlayData& overlay_data) {
  std::shared_ptr<PlatformOverlayWin> overlay =
      std::static_pointer_cast<PlatformOverlayWin>(overlay_data.overlay);
  const int x = static_cast<int>(std::floor(overlay_data.rect.X()));
  const int y = static_cast<int>(std::floor(overlay_data.rect.Y()));
  const int w = static_cast<int>(std::ceil(overlay_data.rect.Width()));
  const int h = static_cast<int>(std::ceil(overlay_data.rect.Height()));
  overlay->DisplayOverlaySurface(x, y, w, h);
}

void PresenterServiceWin::BringOverlayToFront(const PlatformOverlay& overlay) {
  static_cast<const PlatformOverlayWin&>(overlay).BringToFront();
}

void PresenterServiceWin::DisposeOverlay(PlatformOverlay& overlay) {
  static_cast<PlatformOverlayWin&>(overlay).RemoveFromParent();
}

void PresenterServiceWin::OnInit(clay::ServiceManager& service_manager,
                                 const clay::PlatformServiceContext& ctx) {}
void PresenterServiceWin::OnDestroy() {}

std::shared_ptr<PresenterService> PresenterService::Create() {
  return std::make_shared<PresenterServiceWin>();
}

}  // namespace clay
