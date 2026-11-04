// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DARWIN_VSYNC_MONITOR_DARWIN_H_
#define CORE_BASE_DARWIN_VSYNC_MONITOR_DARWIN_H_

#include <memory>

#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace base {
class LynxVSyncPulsePuppet;

class VSyncMonitorIOS : public VSyncMonitor {
  struct ConstructionKey {
    explicit ConstructionKey() = default;
  };

 public:
  static std::shared_ptr<VSyncMonitorIOS> Create(
      bool is_on_ui_thread, bool is_vsync_post_task_by_emergency = true);
  VSyncMonitorIOS(ConstructionKey, bool is_on_ui_thread,
                  bool is_vsync_post_task_by_emergency);
  ~VSyncMonitorIOS() override;

  void Init() override;

  void SetHighRefreshRate() override;

  void RequestVSync() override;

  void RequestVSyncOnUIThread(Callback callback) override;

  void RequestVSyncOnUIThread() override;

 private:
  std::unique_ptr<LynxVSyncPulsePuppet> delegate_;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_DARWIN_VSYNC_MONITOR_DARWIN_H_
