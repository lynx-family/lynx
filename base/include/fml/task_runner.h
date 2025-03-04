// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TASK_RUNNER_H_
#define BASE_INCLUDE_FML_TASK_RUNNER_H_

#include <memory>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/message_loop_task_queues.h"
#include "base/include/fml/time/time_point.h"
namespace lynx {
namespace fml {

class MessageLoopImpl;

/// An interface over the ability to schedule tasks on a \p TaskRunner.
class BasicTaskRunner {
 public:
  /// Schedules \p task to be executed on the TaskRunner's associated event
  /// loop.
  virtual void PostTask(base::closure task) = 0;
};

/// The object for scheduling tasks on a \p fml::MessageLoop.
///
/// Typically there is one \p TaskRunner associated with each thread.  When one
/// wants to execute an operation on that thread they post a task to the
/// TaskRunner.
///
/// \see fml::MessageLoop
class TaskRunner : public fml::RefCountedThreadSafe<TaskRunner>,
                   public BasicTaskRunner {
 public:
  virtual ~TaskRunner();

  virtual void PostTask(base::closure task) override;

  virtual void PostTaskForTime(base::closure task, fml::TimePoint target_time);

  /// Schedules a task to be run on the MessageLoop after the time \p delay has
  /// passed.
  /// \note There is latency between when the task is schedule and actually
  /// executed so that the actual execution time is: now + delay +
  /// message_loop_latency, where message_loop_latency is undefined and could be
  /// tens of milliseconds.
  virtual void PostDelayedTask(base::closure task, fml::TimeDelta delay);

  /// Returns \p true when the current executing thread's TaskRunner matches
  /// this instance.
  virtual bool RunsTasksOnCurrentThread();

  /// Returns the unique identifier associated with the TaskRunner.
  /// \see fml::MessageLoopTaskQueues
  virtual TaskQueueId GetTaskQueueId();

  void PostEmergencyTask(base::closure task);

  void PostMicroTask(base::closure task);

  // Schedules a task in the idle period.
  // TODO(heshan):now this method just schedules a lowest priority task,
  // and will implement as web standard in the future.
  // https://w3c.github.io/requestidlecallback/#the-requestidlecallback-method
  void PostIdleTask(base::closure task);

  void PostSyncTask(base::closure task);

  /// Executes the \p task directly if the TaskRunner \p runner is the
  /// TaskRunner associated with the current executing thread.
  static void RunNowOrPostTask(const fml::RefPtr<fml::TaskRunner>& runner,
                               base::closure task);
  static void RunNowOrPostTask(const std::shared_ptr<fml::TaskRunner>& runner,
                               base::closure task);

  // WARN: This can only be safely called on Android. On other platforms,
  // sometimes `loop_` is null (e.g. in case of using `EmbedderTaskRunner`).
  void AddTaskObserver(intptr_t key, base::closure callback);

  void RemoveTaskObserver(intptr_t key);

  void Bind(fml::RefPtr<MessageLoopImpl> target_loop);

  void UnBind();

  const fml::RefPtr<MessageLoopImpl>& GetLoop() const;

 protected:
  explicit TaskRunner(fml::RefPtr<MessageLoopImpl> loop);
  fml::RefPtr<MessageLoopImpl> loop_;

 private:
  TaskQueueId queue_id_;
  std::shared_ptr<bool> unbound_ = std::make_shared<bool>(false);

  void BindOnCreate(fml::RefPtr<MessageLoopImpl> loop);

  FML_FRIEND_MAKE_REF_COUNTED(TaskRunner);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(TaskRunner);
  BASE_DISALLOW_COPY_AND_ASSIGN(TaskRunner);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::BasicTaskRunner;
using lynx::fml::TaskRunner;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_TASK_RUNNER_H_
