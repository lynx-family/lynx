// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "base/include/fml/task_runner.h"

#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/message_loop_task_queues.h"
#include "base/include/fml/synchronization/waitable_event.h"

namespace lynx {
namespace fml {

TaskRunner::TaskRunner(fml::RefPtr<MessageLoopImpl> loop)
    : queue_id_(MessageLoopTaskQueues::GetInstance()->CreateTaskQueue()) {
  BindOnCreate(loop);
}

void TaskRunner::BindOnCreate(fml::RefPtr<MessageLoopImpl> loop) {
  if (!loop) {
    return;
  }
  MessageLoop* current_loop = fml::MessageLoop::IsInitializedForCurrentThread();
  loop_ = std::move(loop);
  if (current_loop != nullptr && current_loop->GetLoopImpl() == loop_) {
    loop_->Bind(queue_id_);
  } else {
    loop_->PostTask(
        [unbound = unbound_, loop = loop_, queue_id = queue_id_]() {
          if (*unbound) {
            return;
          }
          loop->Bind(queue_id);
        },
        fml::TimePoint::Now(), TaskSourceGrade::kEmergency);
  }
}

TaskRunner::~TaskRunner() {
  if (loop_) {
    loop_->PostTask(
        [loop = loop_, queue_id = queue_id_]() {
          loop->UnBind(queue_id);
          MessageLoopTaskQueues::GetInstance()->DisposeTasks(queue_id);
          MessageLoopTaskQueues::GetInstance()->Dispose(queue_id);
        },
        fml::TimePoint::Now(), fml::TaskSourceGrade::kEmergency);
  } else {
    MessageLoopTaskQueues::GetInstance()->DisposeTasks(queue_id_);
    MessageLoopTaskQueues::GetInstance()->Dispose(queue_id_);
  }
};

void TaskRunner::PostTask(base::closure task) {
  MessageLoopTaskQueues::GetInstance()->RegisterTask(
      queue_id_, std::move(task), fml::TimePoint::Now(),
      fml::TaskSourceGrade::kUnspecified);
}

void TaskRunner::PostEmergencyTask(base::closure task) {
  MessageLoopTaskQueues::GetInstance()->RegisterTask(
      queue_id_, std::move(task), fml::TimePoint::Now(),
      fml::TaskSourceGrade::kEmergency);
}

void TaskRunner::PostMicroTask(base::closure task) {
  MessageLoopTaskQueues::GetInstance()->RegisterTask(
      queue_id_, std::move(task), fml::TimePoint::Now(),
      fml::TaskSourceGrade::kMicrotask);
}

void TaskRunner::PostIdleTask(base::closure task) {
  MessageLoopTaskQueues::GetInstance()->RegisterTask(
      queue_id_, std::move(task), fml::TimePoint::Now(),
      fml::TaskSourceGrade::kIdle);
}

void TaskRunner::PostSyncTask(base::closure task) {
  if (RunsTasksOnCurrentThread()) {
    task();
  } else {
    fml::AutoResetWaitableEvent arwe;
    PostTask([task = std::move(task), &arwe]() {
      task();
      arwe.Signal();
    });
    arwe.Wait();
  }
}

void TaskRunner::PostTaskForTime(base::closure task,
                                 fml::TimePoint target_time) {
  MessageLoopTaskQueues::GetInstance()->RegisterTask(
      queue_id_, std::move(task), target_time,
      fml::TaskSourceGrade::kUnspecified);
}

void TaskRunner::PostDelayedTask(base::closure task, fml::TimeDelta delay) {
  MessageLoopTaskQueues::GetInstance()->RegisterTask(
      queue_id_, std::move(task), fml::TimePoint::Now() + delay,
      fml::TaskSourceGrade::kUnspecified);
}

TaskQueueId TaskRunner::GetTaskQueueId() {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(loop_);
  return queue_id_;
}

// TODO(heshan):this method acquires the lock of MessageLoopTaskQueues
// three times, needs to be optimized.
bool TaskRunner::RunsTasksOnCurrentThread() {
  MessageLoop* current_loop = fml::MessageLoop::IsInitializedForCurrentThread();
  if (current_loop == nullptr) {
    return false;
  }

  return MessageLoopTaskQueues::GetInstance()
      ->IsTaskQueueRunningOnGivenMessageLoop(current_loop->GetLoopImpl().get(),
                                             queue_id_);
}

void TaskRunner::RunNowOrPostTask(const fml::RefPtr<fml::TaskRunner>& runner,
                                  base::closure task) {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(runner);
  if (runner->RunsTasksOnCurrentThread()) {
    task();
  } else {
    runner->PostTask(std::move(task));
  }
}

void TaskRunner::RunNowOrPostTask(
    const std::shared_ptr<fml::TaskRunner>& runner, base::closure task) {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(runner);
  if (runner->RunsTasksOnCurrentThread()) {
    task();
  } else {
    runner->PostTask(std::move(task));
  }
}

void TaskRunner::Bind(fml::RefPtr<MessageLoopImpl> target_loop) {
  if (target_loop && target_loop != loop_) {
    LYNX_BASE_CHECK(target_loop->CanRunNow());
    UnBind();
    target_loop->Bind(queue_id_);
    loop_ = std::move(target_loop);
    // Try to wake up the loop when there are tasks in the queue.
    MessageLoopTaskQueues::GetInstance()->WakeUp(loop_->GetTaskQueueIds());
  }
}

void TaskRunner::UnBind() {
  if (!loop_) {
    return;
  }

  if (!RunsTasksOnCurrentThread()) {
    fml::AutoResetWaitableEvent arwe;
    PostEmergencyTask([this, &arwe]() {
      loop_->UnBind(queue_id_);
      *unbound_ = true;
      arwe.Signal();
    });
    arwe.Wait();
  } else {
    loop_->UnBind(queue_id_);
    *unbound_ = true;
  }
  loop_ = nullptr;
}

void TaskRunner::AddTaskObserver(intptr_t key, base::closure callback) {
  if (callback != nullptr) {
    MessageLoopTaskQueues::GetInstance()->AddTaskObserver(queue_id_, key,
                                                          std::move(callback));
  }
}

void TaskRunner::RemoveTaskObserver(intptr_t key) {
  MessageLoopTaskQueues::GetInstance()->RemoveTaskObserver(queue_id_, key);
}

const fml::RefPtr<MessageLoopImpl>& TaskRunner::GetLoop() const {
  return loop_;
}

}  // namespace fml
}  // namespace lynx
