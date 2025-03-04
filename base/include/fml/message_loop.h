// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_MESSAGE_LOOP_H_
#define BASE_INCLUDE_FML_MESSAGE_LOOP_H_
#include <iostream>

#include "base/include/fml/macros.h"
#include "base/include/fml/task_queue_id.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"

namespace lynx {
namespace fml {

class TaskRunner;
class MessageLoopImpl;

/// An event loop associated with a thread.
///
/// This class is the generic front-end to the MessageLoop, differences in
/// implementation based on the running platform are in the subclasses of
/// flutter::MessageLoopImpl (ex flutter::MessageLoopAndroid).
///
/// For scheduling events on the message loop see flutter::TaskRunner.
///
/// \see fml::TaskRunner
/// \see fml::MessageLoopImpl
/// \see fml::MessageLoopTaskQueues
/// \see fml::Wakeable
class MessageLoop {
 public:
  static MessageLoop& GetCurrent();

  void Run();

  void Terminate();

  void AddTaskObserver(intptr_t key, base::closure callback);

  void RemoveTaskObserver(intptr_t key);

  const fml::RefPtr<fml::TaskRunner>& GetTaskRunner() const;

  // Exposed for the embedder shell which allows clients to poll for events
  // instead of dedicating a thread to the message loop.
  void RunExpiredTasksNow();

  /// Set the max restriction duration to restrict maximum flush duration when
  /// FlushTasks
  void SetMessageLoopRestrictionDuration(fml::TimeDelta restriction_duration);

  static MessageLoop& EnsureInitializedForCurrentThread(
      void* platform_loop = nullptr);
  const fml::RefPtr<MessageLoopImpl>& GetLoopImpl() const;

  void Bind(const TaskQueueId& queue_id);

  void UnBind(const TaskQueueId& queue_id);

  /// Returns nonnull MessageLoop pointer if \p
  /// EnsureInitializedForCurrentThread has been called on this thread already,
  /// or else returns nullptr.
  static MessageLoop* IsInitializedForCurrentThread();

  ~MessageLoop();

  /// Gets the unique identifier for the TaskQueue associated with the current
  /// thread.
  /// \see fml::MessageLoopTaskQueues
  static TaskQueueId GetCurrentTaskQueueId();

 private:
  friend class TaskRunner;
  friend class MessageLoopImpl;

  fml::RefPtr<MessageLoopImpl> loop_;
  fml::RefPtr<fml::TaskRunner> task_runner_;

  explicit MessageLoop(void* platform_loop = nullptr);

  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoop);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoop;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_MESSAGE_LOOP_H_
