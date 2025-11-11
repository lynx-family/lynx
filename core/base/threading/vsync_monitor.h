// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREADING_VSYNC_MONITOR_H_
#define CORE_BASE_THREADING_VSYNC_MONITOR_H_

#include <functional>
#include <memory>
#include <unordered_map>

#include "base/include/closure.h"
#include "base/include/fml/task_runner.h"
#include "core/public/vsync_monitor_platform_impl.h"

namespace lynx {
namespace base {

class VSyncMonitor : public std::enable_shared_from_this<VSyncMonitor> {
 public:
  using Callback = base::MoveOnlyClosure<void, int64_t, int64_t>;

  VSyncMonitor(bool is_on_ui_thread = false,
               bool is_vsync_post_task_by_emergency = true);
  explicit VSyncMonitor(
      const std::shared_ptr<VSyncMonitorPlatformImpl> &platform_impl,
      bool is_on_ui_thread = false,
      bool is_vsync_post_task_by_emergency = true);
  virtual ~VSyncMonitor() = default;

  static std::shared_ptr<VSyncMonitor> Create(bool is_on_ui_thread = false);

  virtual void Init() {}

  virtual void SetHighRefreshRate() {}

  // TODO(heshan):invoke this method in Init.
  // after initialization, VSyncMonitor needs to bind
  // to MessageLoop of current thread.
  void BindToCurrentThread();

  // the callback only be set once on one frame
  void AsyncRequestVSync(Callback callback);

  // the callback is unique per id
  void ScheduleVSyncSecondaryCallback(uintptr_t id, Callback callback);

  // frame_start_time/frame_target_time is in nanoseconds
  void OnVSync(int64_t frame_start_time, int64_t frame_target_time);

  void BindTaskRunner(const fml::RefPtr<fml::TaskRunner> &runner);

  void StopVSync();

  virtual void RequestVSyncOnUIThread(Callback callback){};

  virtual void RequestVSyncOnUIThread(){};

 protected:
  virtual void RequestVSync();

  Callback callback_;

  fml::RefPtr<fml::TaskRunner> runner_;

 private:
  void OnVSyncInternal(int64_t frame_start_time, int64_t frame_target_time);

  bool is_vsync_post_task_by_emergency_{false};
  bool is_on_ui_thread_{false};
  bool requested_{false};
  bool stop_vsync_{false};
  // additional callbacks required to invoke when VSync is requested
  std::unordered_map<uintptr_t, Callback> secondary_callbacks_;
  std::shared_ptr<VSyncMonitorPlatformImpl> platform_impl_;

  // disallow copy&assign
  VSyncMonitor(const VSyncMonitor &) = delete;
  VSyncMonitor &operator==(const VSyncMonitor &) = delete;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREADING_VSYNC_MONITOR_H_
