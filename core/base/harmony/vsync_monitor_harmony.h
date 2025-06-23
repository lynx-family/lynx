// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_VSYNC_MONITOR_HARMONY_H_
#define CORE_BASE_HARMONY_VSYNC_MONITOR_HARMONY_H_

#include "base/include/fml/time/time_point.h"
#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace base {

class VSyncMonitorHarmony : public base::VSyncMonitor {
 public:
  VSyncMonitorHarmony() = default;
  ~VSyncMonitorHarmony() override = default;

  static void OnUpdateRefreshRate(int64_t refresh_rate);

 private:
  void RequestVSync() override;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_HARMONY_VSYNC_MONITOR_HARMONY_H_
