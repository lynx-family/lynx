// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_PRESENTER_SERVICE_WIN_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_PRESENTER_SERVICE_WIN_H_

#include <memory>
#include <vector>

#include "clay/shell/common/services/compositor/presenter_service.h"

namespace clay {

class PresenterServiceWin final : public PresenterService {
 public:
  PresenterServiceWin() = default;
  void UpdateOverlay(const OverlayData& overlay_data) override;
  // Organize the layers by their z indexes.
  void BringOverlayToFront(const PlatformOverlay& overlay) override;
  void DisposeOverlay(PlatformOverlay& overlay) override;

  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;
  void OnDestroy() override;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_PRESENTER_SERVICE_WIN_H_
