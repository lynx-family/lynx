// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LAYOUT_RESULT_MANAGER_H_
#define CORE_SHELL_LAYOUT_RESULT_MANAGER_H_

#include <mutex>
#include <vector>

#include "base/include/closure.h"
#include "core/shell/tasm_operation_queue.h"

namespace lynx {
namespace shell {

// TODO(klaxxi): The TASMOperationQueue will be removed in the future,
// as only LayoutResultManager is needed.
class LayoutResultManager final : public TASMOperationQueue {
 public:
  LayoutResultManager() = default;
  ~LayoutResultManager() override = default;

  LayoutResultManager(const LayoutResultManager&) = delete;
  LayoutResultManager& operator=(const LayoutResultManager&) = delete;
  LayoutResultManager(LayoutResultManager&&) = delete;
  LayoutResultManager& operator=(LayoutResultManager&&) = delete;

  static bool ExecuteTASMOperations(
      const std::vector<TASMOperationQueue::TASMOperationWrapper>& operations);

  // begin override
  bool Flush() override;
  void AppendPendingTask() override;

  void SetAppendPendingTaskNeededDuringFlush(bool needed) override;
  // end

  std::vector<TASMOperationQueue::TASMOperationWrapper> FetchTASMOperations();

  void EnqueueOnLayoutAfterTask(base::closure on_layout_after_task);

  void RunOnLayoutAfterTasks();

 private:
  std::mutex on_layout_after_tasks_mutex_;

  std::vector<base::closure> on_layout_after_tasks_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LAYOUT_RESULT_MANAGER_H_
