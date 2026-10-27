// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_SCREENSHOT_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_SCREENSHOT_SERVICE_H_

#include <memory>

#include "clay/common/graphics/screenshot.h"
#include "clay/common/service/service.h"
#include "clay/common/service/service_manager.h"

namespace clay {

class ScreenshotService
    : public clay::Service<ScreenshotService, clay::Owner::kPlatform> {
 public:
  static std::shared_ptr<ScreenshotService> Create();

  void SetExternalScreenshotCallback(clay::ExternalScreenshotCallback callback);

  GrDataPtr TakeScreenshotHardware(
      const clay::ScreenshotRequest& screenshot_request);

 private:
  void OnInit(clay::ServiceManager&,
              const clay::PlatformServiceContext& ctx) override;

  GrDataPtr TakeExternalScreenshot(
      const clay::ScreenshotRequest& screenshot_request);

  clay::ExternalScreenshotCallback external_screenshot_callback_ = nullptr;
  Shell* shell_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_SCREENSHOT_SERVICE_H_
