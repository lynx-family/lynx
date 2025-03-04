// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_DYNAMIC_UI_OPERATION_QUEUE_H_
#define CORE_SHELL_DYNAMIC_UI_OPERATION_QUEUE_H_

#include <memory>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/services/event_report/event_tracker.h"
#include "core/shell/lynx_ui_operation_queue.h"

namespace lynx {

namespace shell {

class DynamicUIOperationQueue {
 public:
  explicit DynamicUIOperationQueue(
      base::ThreadStrategyForRendering strategy,
      fml::RefPtr<fml::TaskRunner> ui_runner,
      int32_t instance_id = tasm::report::kUnknownInstanceId);
  ~DynamicUIOperationQueue() = default;

  void Transfer(base::ThreadStrategyForRendering strategy);
  void EnqueueUIOperation(UIOperation operation);
  void EnqueueHighPriorityUIOperation(UIOperation operation);
  void Destroy();
  void UpdateStatus(UIOperationStatus status);
  void MarkDirty();
  void ForceFlush();
  void Flush();
  void SetEnableFlush(bool enable_flush);
  void SetErrorCallback(ErrorCallback callback);
  uint32_t GetNativeUpdateDataOrder();
  uint32_t UpdateNativeUpdateDataOrder();

 protected:
  void CreateImpl();

  bool is_engine_async_;

  std::shared_ptr<LynxUIOperationQueue> impl_;

  const fml::RefPtr<fml::TaskRunner> ui_runner_;

  const int32_t instance_id_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_DYNAMIC_UI_OPERATION_QUEUE_H_
