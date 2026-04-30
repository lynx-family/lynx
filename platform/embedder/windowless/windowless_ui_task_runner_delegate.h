// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_WINDOWLESS_WINDOWLESS_UI_TASK_RUNNER_DELEGATE_H_
#define PLATFORM_EMBEDDER_WINDOWLESS_WINDOWLESS_UI_TASK_RUNNER_DELEGATE_H_

#include <mutex>
#include <unordered_map>
#include <utility>

#include "base/include/fml/task_runner_delegate.h"
#include "platform/embedder/public/capi/lynx_windowless_renderer_capi.h"

namespace lynx {
namespace embedder {

class WindowlessUITaskRunnerDelegate final
    : public lynx::fml::TaskRunnerDelegate {
 public:
  explicit WindowlessUITaskRunnerDelegate(
      const lynx_windowless_ui_task_runner_config_t& config);

  WindowlessUITaskRunnerDelegate(const WindowlessUITaskRunnerDelegate&) =
      delete;
  WindowlessUITaskRunnerDelegate& operator=(
      const WindowlessUITaskRunnerDelegate&) = delete;
  WindowlessUITaskRunnerDelegate(WindowlessUITaskRunnerDelegate&&) = delete;
  WindowlessUITaskRunnerDelegate& operator=(WindowlessUITaskRunnerDelegate&&) =
      delete;

  void PostTask(lynx::base::closure task) override;

  void PostTaskForTime(lynx::base::closure task, int64_t target_time) override;

  void PostDelayedTask(lynx::base::closure task, int64_t delay) override;

  bool RunsTasksOnCurrentThread() override;

  bool RunTask(uint64_t baton);

 private:
  lynx_windowless_ui_task_runner_config_t config_;
  std::mutex tasks_mutex_;
  uint64_t last_baton_ = 0;
  std::unordered_map<uint64_t, lynx::base::closure> pending_tasks_;
};

bool SetGlobalUITaskRunnerDelegate(
    const lynx_windowless_ui_task_runner_config_t* config);

WindowlessUITaskRunnerDelegate* GetGlobalUITaskRunnerDelegate();

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_WINDOWLESS_WINDOWLESS_UI_TASK_RUNNER_DELEGATE_H_
