// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/common/vsync_monitor.h"

#include <utility>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace shell {

VSyncMonitor::VSyncMonitor(bool is_vsync_post_task_by_emergency)
    : is_vsync_post_task_by_emergency_(
          is_vsync_post_task_by_emergency
              ? tasm::LynxEnv::GetInstance().IsVSyncPostTaskByEmergency()
              : false) {
  LOGI("VSyncMonitor created with is_vsync_post_task_by_emergency_ "
       << is_vsync_post_task_by_emergency_);
}

void VSyncMonitor::AsyncRequestVSync(Callback callback) {
  // take care: do not call AsyncRequestVSync in multiple threads, or add a
  // mutex to protect
  if (callback_) {
    // request during a frame interval, just return
    return;
  }

  DCHECK(runner_->RunsTasksOnCurrentThread());

  callback_ = std::move(callback);

  RequestVSync();
}

void VSyncMonitor::ScheduleVSyncSecondaryCallback(uintptr_t id,
                                                  Callback callback) {
  if (!callback) {
    return;
  }

  DCHECK(runner_->RunsTasksOnCurrentThread());

  // take care: do not call AsyncRequestVSync in multiple threads
  auto &&[iter, inserted] =
      secondary_callbacks_.emplace(id, std::move(callback));
  if (!inserted) {
    // the same callback already post, ignore
    return;
  }

  if (!requested_) {
    RequestVSync();
    requested_ = true;
  }
}

void VSyncMonitor::OnVSync(int64_t frame_start_time,
                           int64_t frame_target_time) {
  if (runner_->RunsTasksOnCurrentThread()) {
    OnVSyncInternal(frame_start_time, frame_target_time);
    return;
  }

  auto task = [weak_self = weak_from_this(), frame_start_time,
               frame_target_time]() {
    auto self = weak_self.lock();
    if (self != nullptr) {
      self->OnVSyncInternal(frame_start_time, frame_target_time);
    }
  };
  if (is_vsync_post_task_by_emergency_) {
    runner_->PostEmergencyTask(std::move(task));
  } else {
    runner_->PostTask(std::move(task));
  }
}

void VSyncMonitor::OnVSyncInternal(int64_t frame_start_time,
                                   int64_t frame_target_time) {
  requested_ = false;
  // if necessary, add callback mutex
  if (callback_) {
    Callback callback = std::move(callback_);
    callback(frame_start_time, frame_target_time);
  }

  if (!secondary_callbacks_.empty()) {
    std::vector<Callback> callback_vec;
    for (auto &&[id, callback] : secondary_callbacks_) {
      callback_vec.push_back(std::move(callback));
    }
    secondary_callbacks_.clear();

    for (auto &cb : callback_vec) {
      cb(frame_start_time, frame_target_time);
    }
  }
}

void VSyncMonitor::BindTaskRunner(const fml::RefPtr<fml::TaskRunner> &runner) {
  runner_ = std::move(runner);
}

void VSyncMonitor::BindToCurrentThread() {
  if (runner_) {
    return;
  }
  // TODO(qiuxian): The macro will be removed after refactoring VSyncMonitor.
  // While creating MessageLoopAndroidVSync in UIThread::Init() on Android
  // platform, calling UIThread::GetRunner here can block current thread.
  // This piece of code fixes crash on Window platform, it's safe to delete
  // it from other platforms.
#ifdef OS_WIN
  auto ui_runner = base::UIThread::GetRunner();
  if (ui_runner->RunsTasksOnCurrentThread()) {
    runner_ = ui_runner;
    return;
  }
#endif
  runner_ = fml::MessageLoop::GetCurrent().GetTaskRunner();
}

}  // namespace shell
}  // namespace lynx
