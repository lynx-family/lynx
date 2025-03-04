// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_UI_OPERATION_ASYNC_QUEUE_H_
#define CORE_SHELL_LYNX_UI_OPERATION_ASYNC_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "core/services/event_report/event_tracker.h"
#include "core/shell/lynx_ui_operation_queue.h"

namespace lynx {

namespace shell {

class LynxUIOperationAsyncQueue
    : public LynxUIOperationQueue,
      public std::enable_shared_from_this<LynxUIOperationAsyncQueue> {
 public:
  explicit LynxUIOperationAsyncQueue(
      fml::RefPtr<fml::TaskRunner> runner,
      int32_t instance_id = tasm::report::kUnknownInstanceId)
      : LynxUIOperationQueue(instance_id), runner_(std::move(runner)){};
  virtual void EnqueueUIOperation(UIOperation operation) override;
  virtual void EnqueueHighPriorityOperation(UIOperation operation) override;

  virtual void UpdateStatus(UIOperationStatus status) override;
  virtual void Flush() override;
  virtual void ForceFlush() override;
  virtual void MarkDirty() override;
  virtual uint32_t GetNativeUpdateDataOrder() override;
  virtual uint32_t UpdateNativeUpdateDataOrder() override;
  virtual bool IsInFlush() override { return is_in_flush_; }
  virtual bool FlushPendingOperations() override;

 private:
  void FlushOnTASMThread();
  void FlushOnUIThread();
  void FlushInterval();

  // A pending UIOperations vector for the tasm thread. All UIOperations that
  // come from the tasm thread will be added into |pending_operations_|. When
  // the tasm thread calls `Flush`, the |pending_operations_| will be moved to
  // |operations_|, then |operations_| will be eventually flush on the UI
  // thread.
  base::ConcurrentQueue<UIOperation> pending_operations_;
  base::ConcurrentQueue<UIOperation> pending_high_priority_operations_;

  // These variables below are used for syncFlush that called from the platform
  // layer by the UI thread. It will wait for the tasm and layout finish to
  // avoid screen flickering.
  UIOperationStatus status_ = UIOperationStatus::INIT;
  std::atomic_bool layout_finish_{false};
  std::atomic_bool tasm_finish_{false};
  std::mutex layout_mutex_;
  std::mutex tasm_mutex_;
  std::mutex flush_mutex_;
  std::condition_variable layout_cv_;
  std::condition_variable tasm_cv_;
  static constexpr std::chrono::milliseconds kOperationQueueTimeOut{100};

  // Actually, it will always be a UIThread runner. We add |runner_|
  // just for unit test to mock UIThread runner.
  const fml::RefPtr<fml::TaskRunner> runner_;
  std::atomic_uint32_t native_update_data_order_{0};
  bool is_in_flush_{false};
};
}  // namespace shell
}  // namespace lynx
#endif  // CORE_SHELL_LYNX_UI_OPERATION_ASYNC_QUEUE_H_
