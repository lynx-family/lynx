// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_DARWIN_MACOS_OVERLAY_VIEW_CONTROLLER_SERVICE_H_
#define CLAY_SHELL_PLATFORM_DARWIN_MACOS_OVERLAY_VIEW_CONTROLLER_SERVICE_H_

#include "clay/common/service/service.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayOverlayView.h"

namespace clay {

class OverlayViewControllerService
    : public clay::Service<OverlayViewControllerService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kManualRegister> {
 public:
  OverlayViewControllerService(FlutterEngine* engine,
                               ClayOverlayView* overlay_view);
  FlutterEngine* GetEngine() const;
  ClayOverlayView* GetOverlayView() const;

 private:
  FlutterEngine* engine_ = nullptr;
  ClayOverlayView* overlay_view_ = nullptr;
};

}  // namespace clay

#endif
