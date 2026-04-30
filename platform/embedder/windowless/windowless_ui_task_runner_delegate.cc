// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/windowless/windowless_ui_task_runner_delegate.h"

#include <memory>
#include <utility>

#include "base/include/fml/time/time_point.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace embedder {

namespace {

inline std::unique_ptr<WindowlessUITaskRunnerDelegate>& GetGlobalDelegate() {
  static base::NoDestructor<std::unique_ptr<WindowlessUITaskRunnerDelegate>>
      delegate;
  return *delegate;
}

}  // namespace

WindowlessUITaskRunnerDelegate::WindowlessUITaskRunnerDelegate(
    const lynx_windowless_ui_task_runner_config_t& config)
    : config_(config) {}

void WindowlessUITaskRunnerDelegate::PostTask(lynx::base::closure task) {
  // TODO: It should be noted that the target_time for PostTaskForTime is in
  // microseconds, convert to microseconds. See
  // base/src/fml/task_runner.cc::PostTaskForTime
  PostTaskForTime(std::move(task),
                  fml::TimePoint::Now().ToEpochDelta().ToMicroseconds());
}

void WindowlessUITaskRunnerDelegate::PostTaskForTime(lynx::base::closure task,
                                                     int64_t target_time) {
  if (!config_.post_task_callback) {
    LOGW(
        "[WindowlessUITaskRunnerDelegate] PostTaskForTime: post_task_callback "
        "is null");
    return;
  }

  lynx_task_t lynx_task{};
  uint64_t baton;
  {
    std::scoped_lock lock(tasks_mutex_);
    baton = ++last_baton_;
    pending_tasks_[baton] = std::move(task);
    lynx_task.task = baton;
    LOGD("[WindowlessUITaskRunnerDelegate] PostTaskForTime");
  }
  // TODO: It should be noted that the target_time from TaskRunner is in
  // microseconds, convert to nanoseconds. See
  // base/src/fml/task_runner.cc::PostTaskForTime
  config_.post_task_callback(
      lynx_task, static_cast<uint64_t>(target_time) * 1000, config_.user_data);
}

void WindowlessUITaskRunnerDelegate::PostDelayedTask(lynx::base::closure task,
                                                     int64_t delay) {
  // TODO: It should be noted that the delay from TaskRunner is in milliseconds,
  // but the target_time for PostTaskForTime is in microseconds, convert to
  // microseconds. See base/src/fml/task_runner.cc::PostDelayedTask
  int64_t target_time =
      fml::TimePoint::Now().ToEpochDelta().ToMicroseconds() + delay * 1000;
  LOGD("[WindowlessUITaskRunnerDelegate] PostDelayedTask");
  PostTaskForTime(std::move(task), target_time);
}

bool WindowlessUITaskRunnerDelegate::RunsTasksOnCurrentThread() {
  if (!config_.runs_on_current_thread_callback) {
    LOGW(
        "[WindowlessUITaskRunnerDelegate] RunsTasksOnCurrentThread: callback "
        "is null");
    return false;
  }
  bool result = config_.runs_on_current_thread_callback(config_.user_data);
  LOGD("[WindowlessUITaskRunnerDelegate] RunsTasksOnCurrentThread");
  return result;
}

bool WindowlessUITaskRunnerDelegate::RunTask(uint64_t baton) {
  lynx::base::closure task;

  {
    std::scoped_lock lock(tasks_mutex_);
    auto found = pending_tasks_.find(baton);
    if (found == pending_tasks_.end()) {
      LOGW("[WindowlessUITaskRunnerDelegate] RunTask: baton not found");
      return false;
    }
    LOGD("[WindowlessUITaskRunnerDelegate] RunTask: baton found");
    task = std::move(found->second);
    pending_tasks_.erase(found);
  }

  if (task) {
    LOGD("[WindowlessUITaskRunnerDelegate] RunTask: executing task");
    task();
    return true;
  }
  return false;
}

bool SetGlobalUITaskRunnerDelegate(
    const lynx_windowless_ui_task_runner_config_t* config) {
  LOGD("[SetGlobalUITaskRunnerDelegate] Entering");

  if (GetGlobalDelegate()) {
    LOGW(
        "[SetGlobalUITaskRunnerDelegate] Already initialized, returning false");
    return false;
  }

  if (!config) {
    LOGE("[SetGlobalUITaskRunnerDelegate] config is null");
    return false;
  }

  if (config->struct_size != sizeof(lynx_windowless_ui_task_runner_config_t)) {
    LOGE("[SetGlobalUITaskRunnerDelegate] struct_size mismatch");
    return false;
  }

  if (!config->runs_on_current_thread_callback) {
    LOGE(
        "[SetGlobalUITaskRunnerDelegate] runs_on_current_thread_callback is "
        "null");
    return false;
  }

  if (!config->post_task_callback) {
    LOGE("[SetGlobalUITaskRunnerDelegate] post_task_callback is null");
    return false;
  }

  LOGD(
      "[SetGlobalUITaskRunnerDelegate] Creating "
      "WindowlessUITaskRunnerDelegate");
  GetGlobalDelegate() =
      std::make_unique<WindowlessUITaskRunnerDelegate>(*config);

  LOGD("[SetGlobalUITaskRunnerDelegate] Calling UIThread::InitTaskRunner");
  base::UIThread::InitTaskRunner(GetGlobalDelegate().get());

  LOGD("[SetGlobalUITaskRunnerDelegate] Success, returning true");
  return true;
}

WindowlessUITaskRunnerDelegate* GetGlobalUITaskRunnerDelegate() {
  return GetGlobalDelegate().get();
}

}  // namespace embedder
}  // namespace lynx
