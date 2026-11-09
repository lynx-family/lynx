// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_VSYNC_VSYNC_MONITOR_ADAPTOR_H_
#define CLAY_LYNX_ADAPTOR_VSYNC_VSYNC_MONITOR_ADAPTOR_H_

#include <memory>

#include "clay/common/vsync/vsync_callback.h"
#include "core/public/vsync_monitor_platform_impl.h"

namespace clay {

// Adapts a Core VSync source to Clay. A null source returns an empty callback;
// otherwise the callback retains the source.
VsyncRequestCallback MakeVsyncRequestCallback(
    const std::shared_ptr<lynx::base::VSyncMonitorPlatformImpl>& monitor);

}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_VSYNC_VSYNC_MONITOR_ADAPTOR_H_
