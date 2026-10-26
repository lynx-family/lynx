// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/vsync_monitor.h"

#include <utility>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/lynx_env.h"  // nogncheck

namespace lynx {
namespace base {

VSyncMonitor::VSyncMonitor(bool is_on_ui_thread,
                           bool is_vsync_post_task_by_emergency)
    : is_vsync_post_task_by_emergency_(is_vsync_post_task_by_emergency),
      is_on_ui_thread_(is_on_ui_thread),
      platform_impl_(nullptr) {
  LOGI("VSyncMonitor created with is_vsync_post_task_by_emergency_ "
       << is_vsync_post_task_by_emergency_);
}

VSyncMonitor::VSyncMonitor(
    const std::shared_ptr<VSyncMonitorPlatformImpl> &platform_impl,
    bool is_on_ui_thread, bool is_vsync_post_task_by_emergency)
    : is_vsync_post_task_by_emergency_(is_vsync_post_task_by_emergency),
      is_on_ui_thread_(is_on_ui_thread),
      platform_impl_(platform_impl) {
  LOGI("VSyncMonitor created with is_vsync_post_task_by_emergency_ "
       << is_vsync_post_task_by_emergency_);
}

void VSyncMonitor::AsyncRequestVSync(Callback callback) {
  // take care: do not call AsyncRequestVSync in multiple threads, or add a
  // mutex to protect
  if (!callback || callback_) {
    // request during a frame interval, just return
    return;
  }

  DCHECK(runner_->RunsTasksOnCurrentThread());

  callback_ = std::move(callback);

  if (requested_) {
    return;
  }
  requested_ = true;

  if (is_on_ui_thread_) {
    RequestVSyncOnUIThread();
  } else {
    RequestVSync();
  }
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
    requested_ = true;
    if (is_on_ui_thread_) {
      RequestVSyncOnUIThread();
    } else {
      RequestVSync();
    }
  }
}

void VSyncMonitor::StopVSync() { stop_vsync_ = true; }

void VSyncMonitor::OnVSync(int64_t frame_start_time,
                           int64_t frame_target_time) {
  if (!platform_impl_ && runner_->RunsTasksOnCurrentThread()) {
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
  if (is_vsync_post_task_by_emergency_ &&
      tasm::LynxEnv::GetInstance().IsVSyncPostTaskByEmergency()) {
    runner_->PostEmergencyTask(std::move(task));
  } else {
    runner_->PostTask(std::move(task));
  }
}

void VSyncMonitor::OnVSyncInternal(int64_t frame_start_time,
                                   int64_t frame_target_time) {
  DCHECK(runner_->RunsTasksOnCurrentThread());
  requested_ = false;
  if (stop_vsync_) {
    callback_ = nullptr;
    secondary_callbacks_.clear();
    return;
  }

  Callback primary = std::move(callback_);
  auto secondary = std::move(secondary_callbacks_);
  secondary_callbacks_.clear();
  // Freeze this signal's subscribers before invoking any of them. Requests
  // made by a callback belong to the following signal.
  if (primary) {
    primary(frame_start_time, frame_target_time);
  }
  for (auto &[id, callback] : secondary) {
    callback(frame_start_time, frame_target_time);
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
#if defined(OS_WIN) || (defined(ENABLE_HEADLESS) && defined(OS_HARMONY)) || \
    defined(OS_LINUX)
  auto ui_runner = base::UIThread::GetRunner();
  if (ui_runner->RunsTasksOnCurrentThread()) {
    runner_ = ui_runner;
    return;
  }
#endif

  runner_ = fml::MessageLoop::GetCurrent().GetTaskRunner();
}

void VSyncMonitor::RequestVSync() {
  if (!platform_impl_) {
    LOGE("VSyncMonitorPlatformImpl is null, skip RequestVSync");
    return;
  }
  platform_impl_->RequestVSync(
      [weak_self = weak_from_this()](int64_t frame_start_time,
                                     int64_t frame_target_time) {
        auto self = weak_self.lock();
        if (self != nullptr) {
          self->OnVSync(frame_start_time, frame_target_time);
        }
      });
}

}  // namespace base
}  // namespace lynx
