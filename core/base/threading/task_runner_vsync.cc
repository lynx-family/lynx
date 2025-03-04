// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/task_runner_vsync.h"

#include "base/include/fml/message_loop.h"
#include "base/include/fml/message_loop_impl.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace base {
TaskRunnerVSync::TaskRunnerVSync(fml::RefPtr<fml::MessageLoopImpl> loop)
    : TaskRunner(loop) {}

bool TaskRunnerVSync::RunsTasksOnCurrentThread() {
  // The reference of vsync message loop.
  static auto kUILoopVSync = base::UIThread::GetRunner(true)->GetLoop();
  // The reference of non-vsync task runner.
  static auto kUIRunnerNonVSync = base::UIThread::GetRunner(false);

  // Since the bound loop could be change dynamically, we should check if the
  // current bound loop is vsync message loop. If it is, the non-vsync task
  // runner should be used to check RunsTasksOnCurrentThread because we didn't
  // save the vsync message loop to tls. Otherwise, we just call
  // TaskRunner::RunsTasksOnCurrentThread().
  return (loop_ == kUILoopVSync) ? kUIRunnerNonVSync->RunsTasksOnCurrentThread()
                                 : fml::TaskRunner::RunsTasksOnCurrentThread();
}
}  // namespace base
}  // namespace lynx
