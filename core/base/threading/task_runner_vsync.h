// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREADING_TASK_RUNNER_VSYNC_H_
#define CORE_BASE_THREADING_TASK_RUNNER_VSYNC_H_
#include "base/include/fml/task_runner.h"

namespace lynx {
namespace base {
class TaskRunnerVSync : public fml::TaskRunner {
 public:
  ~TaskRunnerVSync() override = default;

  bool RunsTasksOnCurrentThread() override;

  explicit TaskRunnerVSync(fml::RefPtr<fml::MessageLoopImpl> loop);

 private:
  FML_FRIEND_MAKE_REF_COUNTED(TaskRunnerVSync);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(TaskRunnerVSync);
  BASE_DISALLOW_COPY_AND_ASSIGN(TaskRunnerVSync);
};
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREADING_TASK_RUNNER_VSYNC_H_
