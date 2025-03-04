// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_COMMON_VSYNC_MONITOR_H_
#define CORE_SHELL_COMMON_VSYNC_MONITOR_H_

#include <functional>
#include <memory>
#include <unordered_map>

#include "base/include/closure.h"
#include "base/include/fml/task_runner.h"
#include "core/renderer/dom/element_manager.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/shell/lynx_actor_specialization.h"

namespace lynx {
namespace shell {

class VSyncMonitor : public std::enable_shared_from_this<VSyncMonitor> {
 public:
  using Callback = base::MoveOnlyClosure<void, int64_t, int64_t>;

  VSyncMonitor(bool is_vsync_post_task_by_emergency = true);
  virtual ~VSyncMonitor() = default;

  static std::shared_ptr<VSyncMonitor> Create();

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

  void set_runtime_actor(
      const std::shared_ptr<LynxActor<runtime::LynxRuntime>> &actor) {
    runtime_actor_ = actor;
  }
  const std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor() const {
    return runtime_actor_;
  }

  void BindTaskRunner(const fml::RefPtr<fml::TaskRunner> &runner);

 protected:
  virtual void RequestVSync() = 0;

  Callback callback_;

  fml::RefPtr<fml::TaskRunner> runner_;

 private:
  void OnVSyncInternal(int64_t frame_start_time, int64_t frame_target_time);

  bool is_vsync_post_task_by_emergency_{false};
  bool requested_{false};
  // additional callbacks required to invoke when VSync is requested
  std::unordered_map<uintptr_t, Callback> secondary_callbacks_;
  std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_;

  // disallow copy&assign
  VSyncMonitor(const VSyncMonitor &) = delete;
  VSyncMonitor &operator==(const VSyncMonitor &) = delete;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_COMMON_VSYNC_MONITOR_H_
