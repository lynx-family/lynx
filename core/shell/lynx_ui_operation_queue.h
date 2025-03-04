// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_UI_OPERATION_QUEUE_H_
#define CORE_SHELL_LYNX_UI_OPERATION_QUEUE_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/concurrent_queue.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {

namespace base {
struct LynxError;
}

namespace shell {

using UIOperation = base::closure;
using ErrorCallback = base::MoveOnlyClosure<void, base::LynxError>;

enum class UIOperationStatus : uint32_t {
  INIT = 0,
  TASM_FINISH,
  LAYOUT_FINISH,
  ALL_FINISH
};

class LynxUIOperationQueue {
 public:
  explicit LynxUIOperationQueue(
      int32_t instance_id = tasm::report::kUnknownInstanceId)
      : instance_id_(instance_id) {}
  virtual ~LynxUIOperationQueue() = default;

  virtual void EnqueueUIOperation(UIOperation operation);
  virtual void EnqueueHighPriorityOperation(UIOperation operation);

  void Destroy();
  virtual void UpdateStatus(UIOperationStatus status) {}
  virtual void MarkDirty() {}
  virtual void ForceFlush();
  virtual void Flush();
  virtual void SetEnableFlush(bool enable_flush);
  void SetErrorCallback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
  };
  virtual uint32_t GetNativeUpdateDataOrder() { return 0; }
  virtual uint32_t UpdateNativeUpdateDataOrder() { return 0; }
  virtual bool IsInFlush() { return false; }
  virtual bool FlushPendingOperations() { return false; }

 protected:
  void ConsumeOperations(
      const base::ConcurrentQueue<UIOperation>::IterableContainer&
          high_priority_operations,
      const base::ConcurrentQueue<UIOperation>::IterableContainer& operations);

  base::ConcurrentQueue<UIOperation> operations_;
  base::ConcurrentQueue<UIOperation> high_priority_operations_;
  std::atomic_bool destroyed_{false};
  bool enable_flush_{true};
  ErrorCallback error_callback_;
  int32_t instance_id_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_UI_OPERATION_QUEUE_H_
