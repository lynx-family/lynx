// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_THREAD_MODE_AUTO_SWITCH_H_
#define CORE_SHELL_THREAD_MODE_AUTO_SWITCH_H_

#include <optional>

#include "base/include/fml/task_runner.h"
#include "core/base/threading/thread_merger.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace lynx {
namespace shell {

struct ThreadModeManager;

class ThreadModeAutoSwitch {
 public:
  explicit ThreadModeAutoSwitch(ThreadModeManager& manager);
  ~ThreadModeAutoSwitch();

  ThreadModeAutoSwitch(const ThreadModeAutoSwitch&) = delete;
  ThreadModeAutoSwitch& operator=(const ThreadModeAutoSwitch&) = delete;
  ThreadModeAutoSwitch(ThreadModeAutoSwitch&&) = delete;
  ThreadModeAutoSwitch& operator=(ThreadModeAutoSwitch&&) = delete;

 private:
  ThreadModeManager* manager_{nullptr};  // NOT OWNED

  std::optional<base::ThreadMerger> merger_;
};

struct ThreadModeManager {
  ThreadModeManager()
      : ui_runner(nullptr), engine_runner(nullptr), queue(nullptr) {}
  ThreadModeManager(fml::TaskRunner* ui_runner, fml::TaskRunner* engine_runner,
                    DynamicUIOperationQueue* queue)
      : ui_runner(ui_runner), engine_runner(engine_runner), queue(queue) {}
  ~ThreadModeManager() = default;

  ThreadModeManager(const ThreadModeManager&) = delete;
  ThreadModeManager& operator=(const ThreadModeManager&) = delete;
  ThreadModeManager(ThreadModeManager&&) = delete;
  ThreadModeManager& operator=(ThreadModeManager&&) = delete;

  explicit operator bool() const {
    return ui_runner != nullptr && engine_runner != nullptr &&
           queue != nullptr && !is_held;
  }

  fml::TaskRunner* ui_runner;  // NOT OWNED

  fml::TaskRunner* engine_runner;  // NOT OWNED

  DynamicUIOperationQueue* queue;  // NOT OWNED

  bool is_held{false};
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_THREAD_MODE_AUTO_SWITCH_H_
