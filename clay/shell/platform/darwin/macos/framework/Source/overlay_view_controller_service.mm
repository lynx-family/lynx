// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/overlay_view_controller_service.h"

namespace clay {
OverlayViewControllerService::OverlayViewControllerService(FlutterEngine* engine,
                                                           ClayOverlayView* overlay_view)
    : engine_(engine), overlay_view_(overlay_view) {}

FlutterEngine* OverlayViewControllerService::GetEngine() const { return engine_; }

ClayOverlayView* OverlayViewControllerService::GetOverlayView() const { return overlay_view_; }

}  // namespace clay
