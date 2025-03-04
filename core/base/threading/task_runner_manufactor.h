// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREADING_TASK_RUNNER_MANUFACTOR_H_
#define CORE_BASE_THREADING_TASK_RUNNER_MANUFACTOR_H_

#include <memory>
#include <mutex>
#include <string>

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
#include "base/include/no_destructor.h"

namespace lynx {
namespace base {

enum ThreadStrategyForRendering {
  ALL_ON_UI = 0,
  MOST_ON_TASM = 1,
  PART_ON_LAYOUT = 2,
  MULTI_THREADS = 3,
};

enum class ConcurrentTaskType : int32_t {
  HIGH_PRIORITY = 0,
  NORMAL_PRIORITY,
};

inline bool IsEngineAsync(base::ThreadStrategyForRendering strategy) {
  return strategy == base::ThreadStrategyForRendering::MULTI_THREADS ||
         strategy == base::ThreadStrategyForRendering::MOST_ON_TASM;
}

inline ThreadStrategyForRendering ToAsyncEngineStrategy(
    base::ThreadStrategyForRendering strategy) {
  if (strategy == ALL_ON_UI) {
    return MOST_ON_TASM;
  } else if (strategy == PART_ON_LAYOUT) {
    return MULTI_THREADS;
  }
  return strategy;
}

class UIThread {
 public:
  BASE_EXPORT_FOR_DEVTOOL static fml::RefPtr<fml::TaskRunner>& GetRunner(
      bool enable_vsync_aligned_msg_loop = false);

  // ensure call on ui thread.
  static void Init(void* platform_loop = nullptr);

 private:
  UIThread() = delete;
  ~UIThread() = delete;
};

class TaskRunnerManufactor {
 public:
  // Should be created on UI thread
  TaskRunnerManufactor(ThreadStrategyForRendering strategy,
                       bool enable_multi_tasm_thread,
                       bool enable_multi_layout_thread,
                       bool enable_vsync_aligned_msg_loop = false,
                       bool enable_async_thread_cache = false,
                       std::string js_group_thread_name = "");

  virtual ~TaskRunnerManufactor() = default;

  TaskRunnerManufactor(const TaskRunnerManufactor&) = delete;
  TaskRunnerManufactor& operator=(const TaskRunnerManufactor&) = delete;
  TaskRunnerManufactor(TaskRunnerManufactor&&) = default;
  TaskRunnerManufactor& operator=(TaskRunnerManufactor&&) = default;

  BASE_EXPORT static fml::RefPtr<fml::TaskRunner> GetJSRunner(
      const std::string& js_group_thread_name);

  BASE_EXPORT_FOR_DEVTOOL fml::RefPtr<fml::TaskRunner> GetTASMTaskRunner();

  fml::RefPtr<fml::TaskRunner> GetLayoutTaskRunner();

  BASE_EXPORT_FOR_DEVTOOL fml::RefPtr<fml::TaskRunner> GetUITaskRunner();

  BASE_EXPORT_FOR_DEVTOOL fml::RefPtr<fml::TaskRunner> GetJSTaskRunner();

  fml::RefPtr<fml::MessageLoopImpl> GetTASMLoop();

  ThreadStrategyForRendering GetManufactorStrategy();

  void OnThreadStrategyUpdated(ThreadStrategyForRendering new_strategy);

  static fml::Thread CreateJSWorkerThread(const std::string& worker_name);

  static void PostTaskToConcurrentLoop(base::closure, ConcurrentTaskType type);

 private:
  void StartUIThread(bool enable_vsync_aligned_msg_loop);

  fml::RefPtr<fml::MessageLoopImpl> StartTASMThread();

  void StartLayoutThread(bool enable_multi_layout_thread);

  void StartJSThread();

  void CreateTASMRunner(fml::RefPtr<fml::MessageLoopImpl> loop,
                        bool enable_vsync_aligned_msg_loop);

  static fml::ConcurrentMessageLoop& GetNormalPriorityLoop();

  fml::RefPtr<fml::TaskRunner> tasm_task_runner_;
  fml::RefPtr<fml::TaskRunner> layout_task_runner_;
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;

  fml::RefPtr<fml::TaskRunner> js_task_runner_;

  fml::RefPtr<fml::MessageLoopImpl> tasm_loop_;

  // Can only be used when multiple TASM thread is enabled
  std::unique_ptr<fml::Thread> tasm_thread_;
  // Can only be used when multiple Layout thread is enabled
  std::unique_ptr<fml::Thread> layout_thread_;

  ThreadStrategyForRendering thread_strategy_;
  bool enable_multi_tasm_thread_;

  std::string js_group_thread_name_;

  size_t label_;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREADING_TASK_RUNNER_MANUFACTOR_H_
