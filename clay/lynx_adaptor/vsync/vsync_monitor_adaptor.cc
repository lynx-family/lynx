// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/vsync/vsync_monitor_adaptor.h"

#include <utility>

namespace clay {

VsyncRequestCallback MakeVsyncRequestCallback(
    const std::shared_ptr<lynx::base::VSyncMonitorPlatformImpl>& monitor) {
  if (!monitor) {
    return {};
  }
  return [monitor](VsyncSignalCallback callback) {
    monitor->RequestVSync(std::move(callback));
  };
}

}  // namespace clay
