// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_VSYNC_MONITOR_FALLBACK_H_
#define PLATFORM_EMBEDDER_VSYNC_MONITOR_FALLBACK_H_

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace base {

class VSyncMonitorFallback : public VSyncMonitor {
 public:
  VSyncMonitorFallback(fml::TimeDelta interval);
  ~VSyncMonitorFallback() override;

  void Init() override;

  void RequestVSync() override;

 private:
  fml::TimePoint phase_;
  fml::TimeDelta interval_;
};

}  // namespace base
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_VSYNC_MONITOR_FALLBACK_H_
