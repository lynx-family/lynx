// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TASK_RUNNER_DELEGATE_H_
#define BASE_INCLUDE_FML_TASK_RUNNER_DELEGATE_H_

#include "base/include/base_export.h"
#include "base/include/closure.h"

namespace lynx {
namespace fml {
class BASE_EXPORT TaskRunnerDelegate {
 public:
  TaskRunnerDelegate() = default;
  ~TaskRunnerDelegate() = default;
  virtual void PostTask(lynx::base::closure task) {}

  virtual void PostTaskForTime(lynx::base::closure task, int64_t target_time) {}

  virtual void PostDelayedTask(lynx::base::closure task, int64_t delay) {}

  virtual bool RunsTasksOnCurrentThread() { return false; }
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_TASK_RUNNER_DELEGATE_H_
