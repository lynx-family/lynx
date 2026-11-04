// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_VSYNC_MONITOR_HARMONY_H_
#define CORE_BASE_HARMONY_VSYNC_MONITOR_HARMONY_H_

#include <utility>

#include "base/include/fml/time/time_point.h"
#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace base {

class VSyncMonitorHarmony : public base::VSyncMonitor {
 public:
  explicit VSyncMonitorHarmony(bool is_on_ui_thread = false)
      : VSyncMonitor(is_on_ui_thread) {}
  ~VSyncMonitorHarmony() override = default;

  static void OnUpdateRefreshRate(int64_t refresh_rate);

  void SetUIVSyncRequester(UIVSyncRequester requester) override {
    ui_vsync_requester_ = std::move(requester);
  }

  void RequestVSyncOnUIThread() override;

 private:
  void RequestVSync() override;

  UIVSyncRequester ui_vsync_requester_;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_HARMONY_VSYNC_MONITOR_HARMONY_H_
