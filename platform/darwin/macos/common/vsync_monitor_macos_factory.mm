// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/darwin/macos/common/vsync_monitor_macos.h"

#import <AppKit/NSScreen.h>

#include <memory>

namespace lynx {
namespace base {

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  if (@available(macOS 14.0, *)) {
    return std::make_shared<lynx::base::VSyncMonitorMacOS>();
  }
  int framesPerSecond = 60;
  if (@available(macOS 12.0, *)) {
    NSScreen *screen = [[NSScreen screens] firstObject];
    framesPerSecond = screen.maximumFramesPerSecond;
  }
  return std::make_shared<lynx::base::VSyncMonitorFallback>(
      fml::TimeDelta::FromSecondsF(1.0 / framesPerSecond));
}

}  // namespace base
}  // namespace lynx
