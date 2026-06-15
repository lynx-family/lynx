// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "base/include/fml/time/time_delta.h"
#include "core/base/threading/vsync_monitor.h"
#include "platform/embedder/vsync_monitor_fallback.h"

namespace lynx {
namespace base {

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  return std::make_shared<VSyncMonitorFallback>(fml::TimeDelta::FromSecondsF(1.0 / 60));
}

}  // namespace base
}  // namespace lynx
