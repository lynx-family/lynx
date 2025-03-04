// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_MESSAGE_LOOP_IMPL_H_
#define BASE_INCLUDE_FML_MESSAGE_LOOP_IMPL_H_

#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/delayed_task.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/message_loop_task_queues.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/fml/wakeable.h"

namespace lynx {
namespace fml {
// VSyncCallback from VSyncRequest. The first parameter represents
// frame_start_time, and the second one represents frame_target_time, both in
// nanoseconds
using VSyncCallback = base::MoveOnlyClosure<void, int64_t, int64_t>;

// Use for requesting a VSync from platform layer, so that we could align the
// execution of tasks with VSync to reduce jank.
using VSyncRequest = base::MoveOnlyClosure<void, VSyncCallback>;

/// An abstract class that represents the differences in implementation of a \p
/// fml::MessageLoop depending on the platform.
/// \see fml::MessageLoop
/// \see fml::MessageLoopAndroid
/// \see fml::MessageLoopDarwin
class MessageLoopImpl : public Wakeable,
                        public fml::RefCountedThreadSafe<MessageLoopImpl> {
 public:
  static fml::RefPtr<MessageLoopImpl> Create(void* platform_loop = nullptr);

  virtual ~MessageLoopImpl();

  virtual void Run() = 0;

  virtual void Terminate() = 0;

  void PostTask(base::closure task, fml::TimePoint target_time,
                fml::TaskSourceGrade task_source_grade =
                    fml::TaskSourceGrade::kUnspecified);

  void AddTaskObserver(intptr_t key, base::closure callback);

  void RemoveTaskObserver(intptr_t key);

  void DoRun();

  void DoTerminate();

  virtual TaskQueueId GetTaskQueueId() const;

  virtual std::vector<TaskQueueId> GetTaskQueueIds() const;

  void SetRestrictionDuration(fml::TimeDelta duration);

  void Bind(const TaskQueueId& queue_id);

  void UnBind(const TaskQueueId& queue_id);

  virtual bool CanRunNow();

  void SetVSyncRequest(VSyncRequest vsync_request);

 protected:
  // Exposed for the embedder shell which allows clients to poll for events
  // instead of dedicating a thread to the message loop.
  friend class MessageLoop;

  void RunExpiredTasksNow();

  void RunSingleExpiredTaskNow();

 protected:
  MessageLoopImpl();
  MessageLoopTaskQueues* task_queue_;
  TaskQueueId internal_queue_id_;
  std::vector<TaskQueueId> queue_ids_;

 private:
  std::atomic_bool terminated_;
  // Default fml::TimeDelta::Max(), means no effect.
  fml::TimeDelta restriction_duration_;

  VSyncRequest vsync_request_;

  virtual void FlushTasks(FlushType type);

  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoopImpl);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoopImpl;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_MESSAGE_LOOP_IMPL_H_
