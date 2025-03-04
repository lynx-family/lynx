// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SHELL_ENGINE_THREAD_SWITCH_H_
#define CORE_SHELL_ENGINE_THREAD_SWITCH_H_

#include <atomic>
#include <memory>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace lynx {
namespace shell {

class EngineThreadSwitch
    : public std::enable_shared_from_this<EngineThreadSwitch> {
 public:
  EngineThreadSwitch(
      const fml::RefPtr<fml::TaskRunner>& ui_runner,
      const fml::RefPtr<fml::TaskRunner>& engine_runner,
      const std::shared_ptr<shell::DynamicUIOperationQueue>& queue);

  ~EngineThreadSwitch() = default;

  void AttachEngineToUIThread();

  void DetachEngineFromUIThread();

  bool HasSetEngineLoop();

  void SetEngineLoop(const fml::RefPtr<fml::MessageLoopImpl>& engine_loop);

 private:
  fml::RefPtr<fml::TaskRunner> ui_runner_;
  fml::RefPtr<fml::TaskRunner> engine_runner_;
  fml::RefPtr<fml::MessageLoopImpl> ui_loop_;
  fml::RefPtr<fml::MessageLoopImpl> engine_loop_;
  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;
  std::condition_variable detaching_process_cv_;
  bool is_in_detaching_process_{false};
  std::mutex mutex_;
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ENGINE_THREAD_SWITCH_H_
