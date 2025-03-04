// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_MEMORY_TASK_RUNNER_CHECKER_H_
#define BASE_INCLUDE_FML_MEMORY_TASK_RUNNER_CHECKER_H_

#include <set>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner.h"

namespace lynx {
namespace fml {

class TaskRunnerChecker final {
 public:
  TaskRunnerChecker();

  ~TaskRunnerChecker();

  bool RunsOnCreationTaskRunner() const;

  static bool RunsOnTheSameThread(TaskQueueId queue_a, TaskQueueId queue_b);

 private:
  TaskQueueId initialized_queue_id_;
  std::set<TaskQueueId> subsumed_queue_ids_;

  TaskQueueId InitTaskQueueId();
};

#if !defined(NDEBUG)
#define FML_DECLARE_TASK_RUNNER_CHECKER(c) fml::TaskRunnerChecker c
// TODO(zhengsenyao): Add DCHECK when DCHECK available.
#define FML_DCHECK_TASK_RUNNER_IS_CURRENT(c) (c).RunsOnCreationTaskRunner()
#else
#define FML_DECLARE_TASK_RUNNER_CHECKER(c)
#define FML_DCHECK_TASK_RUNNER_IS_CURRENT(c) ((void)0)
#endif

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::TaskRunnerChecker;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_MEMORY_TASK_RUNNER_CHECKER_H_
