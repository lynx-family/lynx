// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_DARWIN_MACOS_PRESENTER_SERVICE_MAC_H_
#define CLAY_SHELL_PLATFORM_DARWIN_MACOS_PRESENTER_SERVICE_MAC_H_

#import <QuartzCore/QuartzCore.h>

#include <memory>
#include <vector>

#include "clay/shell/common/services/compositor/presenter_service.h"
#include "clay/shell/platform/darwin/macos/framework/Source/overlay_view_controller_service.h"
#include "clay/shell/platform/darwin/macos/framework/Source/platform_overlay_service_mac.h"

namespace clay {

// macOS implementation of PresenterService.
// - Runs overlay surface submits and CALayer updates on the platform thread.
// - Groups overlay changes from one Present call in a single CATransaction.
// - Repositions overlay CALayers when UpdateOverlay is called.
// - No-ops for native view z-ordering (native views are positioned by
//   the Lynx layout system, not by the compositor).
class PresenterServiceMac final : public PresenterService {
 public:
  PresenterServiceMac() = default;

 private:
  // PresenterService hooks
  void OnBeforePresent() override;
  void OnAfterPresent() override;

  // Overlay management — position the CALayer and set its contents scale.
  void UpdateOverlay(const OverlayData& overlay_data) override;
  void BringOverlayToFront(const PlatformOverlay& overlay) override;
  void DisposeOverlay(PlatformOverlay& overlay) override;

  // Native view management — no-ops on macOS.
  void CompositePlatformView(int64_t id, const EmbeddedViewParams& params) override {}
  void BringPlatformViewToFront(int64_t id) override {}
  void DisposePlatformView(int64_t id) override {}
  void HidePlatformView(int64_t id) override {}

  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;
  void OnDestroy() override;

  clay::Puppet<clay::Owner::kPlatform, OverlayViewControllerService>
      overlay_view_controller_service_;
  std::vector<CGRect> overlay_hit_rects_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_DARWIN_MACOS_PRESENTER_SERVICE_MAC_H_
