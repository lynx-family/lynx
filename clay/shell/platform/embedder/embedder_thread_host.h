// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_THREAD_HOST_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_THREAD_HOST_H_

#include <map>
#include <memory>
#include <set>

#include "base/include/fml/macros.h"
#include "clay/common/task_runners.h"
#include "clay/common/thread_host.h"
#include "clay/public/clay.h"
#include "clay/shell/platform/embedder/embedder_task_runner.h"

namespace clay {

typedef struct {
  /// The size of this struct. Must be sizeof(ClayCustomTaskRunners).
  size_t struct_size;
  /// Specify the task runner for the thread on which the
  /// `EmbedderEngine::RunTask` call is made. The same task runner description
  /// can be specified for both the render and platform task runners. This makes
  /// the Flutter engine use the same thread for both task runners.
  const ClayTaskRunnerDescription* platform_task_runner;
  /// Specify the task runner for the thread on which the render tasks will be
  /// run. The same task runner description can be specified for both the render
  /// and platform task runners. This makes the Flutter engine use the same
  /// thread for both task runners.
  const ClayTaskRunnerDescription* render_task_runner;
  /// Specify a callback that is used to set the thread priority for embedder
  /// task runners.
  void (*thread_priority_setter)(fml::Thread::ThreadPriority);
} ClayCustomTaskRunners;

class EmbedderThreadHost {
 public:
  static std::unique_ptr<EmbedderThreadHost>
  CreateEmbedderOrEngineManagedThreadHost(
      const ClayCustomTaskRunners* custom_task_runners,
      const clay::ThreadConfigSetter& config_setter =
          fml::Thread::SetCurrentThreadName);

  EmbedderThreadHost(ThreadHost thread_host, const clay::TaskRunners& runners,
                     fml::RefPtr<EmbedderTaskRunner> platform_task_runner,
                     fml::RefPtr<EmbedderTaskRunner> render_task_runner);

  ~EmbedderThreadHost();

  bool IsValid() const;

  const clay::TaskRunners& GetTaskRunners() const;

  bool PostTask(int64_t runner, uint64_t task) const;

 private:
  ThreadHost thread_host_;
  clay::TaskRunners runners_;
  std::map<int64_t, fml::RefPtr<EmbedderTaskRunner>> runners_map_;
  fml::RefPtr<EmbedderTaskRunner> platform_task_runner_;
  fml::RefPtr<EmbedderTaskRunner> render_task_runner_;

  static std::unique_ptr<EmbedderThreadHost> CreateEmbedderManagedThreadHost(
      const ClayCustomTaskRunners* custom_task_runners,
      const clay::ThreadConfigSetter& config_setter =
          fml::Thread::SetCurrentThreadName);

  static std::unique_ptr<EmbedderThreadHost> CreateEngineManagedThreadHost(
      const clay::ThreadConfigSetter& config_setter =
          fml::Thread::SetCurrentThreadName);

  BASE_DISALLOW_COPY_AND_ASSIGN(EmbedderThreadHost);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_THREAD_HOST_H_
