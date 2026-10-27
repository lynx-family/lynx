// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_EXAMPLE_GLFW_THREAD_HOST_HOLDER_H_
#define CLAY_EXAMPLE_GLFW_THREAD_HOST_HOLDER_H_

#include <memory>

#include "clay/common/task_runners.h"
#include "clay/common/thread_host.h"
#include "clay/example/glfw/custom_task_runner.h"
#include "clay/public/clay.h"

namespace clay {
namespace example {

class ThreadHostHolder {
 public:
  static std::unique_ptr<ThreadHostHolder> CreateThreadHostHolder(
      ClayTaskRunnerDescription* platform_task_runner_desc);
  ThreadHostHolder(clay::ThreadHost thread_host,
                   const clay::TaskRunners& runners,
                   fml::RefPtr<CustomTaskRunner> platform_task_runner);
  ~ThreadHostHolder();

  // Disallow copy.
  ThreadHostHolder(const ThreadHostHolder&) = delete;
  ThreadHostHolder& operator=(const ThreadHostHolder&) = delete;

  const clay::TaskRunners& GetTaskRunners() const { return runners_; }

  bool PostTask(uint64_t task) const;

 private:
  clay::ThreadHost thread_host_;
  clay::TaskRunners runners_;
  fml::RefPtr<CustomTaskRunner> platform_task_runner_;
};

}  // namespace example
}  // namespace clay

#endif  // CLAY_EXAMPLE_GLFW_THREAD_HOST_HOLDER_H_
