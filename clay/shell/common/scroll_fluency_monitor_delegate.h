// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SCROLL_FLUENCY_MONITOR_DELEGATE_H_
#define CLAY_SHELL_COMMON_SCROLL_FLUENCY_MONITOR_DELEGATE_H_

#include <string>

#include "clay/shell/common/frame_timing_listener.h"

namespace clay {

class ScrollFluencyMonitorDelegate : public FrameTimingListener {
 public:
  virtual ~ScrollFluencyMonitorDelegate() = default;

  virtual void StartFluencyMonitor(int id, const std::string& scene,
                                   const std::string& scroll_monitor_tag,
                                   int max_refresh_rate) = 0;
  virtual void EndFluencyMonitor(int id) = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SCROLL_FLUENCY_MONITOR_DELEGATE_H_
