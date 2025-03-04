// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/dynamic_ui_operation_queue.h"

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/shell/lynx_ui_operation_async_queue.h"

namespace lynx {
namespace shell {

DynamicUIOperationQueue::DynamicUIOperationQueue(
    base::ThreadStrategyForRendering strategy,
    fml::RefPtr<fml::TaskRunner> ui_runner, int32_t instance_id)
    : is_engine_async_(base::IsEngineAsync(strategy)),
      ui_runner_(std::move(ui_runner)),
      instance_id_(instance_id) {
  CreateImpl();
}

void DynamicUIOperationQueue::Transfer(
    base::ThreadStrategyForRendering strategy) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DynamicUIOperationQueue::Transfer");
  // ensure on ui thread.
  DCHECK(ui_runner_->RunsTasksOnCurrentThread());

  // in Flush, just do nothing.
  if (impl_->IsInFlush()) {
    return;
  }

  if (is_engine_async_ == base::IsEngineAsync(strategy)) {
    return;
  }
  is_engine_async_ = !is_engine_async_;

  auto outdated_impl = impl_;
  // Create new impl before ForceFlush to ensure all ui operations generated
  // during Transfer will not enqueued to old impl.
  CreateImpl();

  // force flush the existing ui operations.
  // TODO(heshan):for async, here will flush with a std::lock_guard,
  // which can be optimize away.
  outdated_impl->FlushPendingOperations();
  outdated_impl->ForceFlush();
}

void DynamicUIOperationQueue::EnqueueUIOperation(UIOperation operation) {
  impl_->EnqueueUIOperation(std::move(operation));
}

void DynamicUIOperationQueue::EnqueueHighPriorityUIOperation(
    UIOperation operation) {
  impl_->EnqueueHighPriorityOperation(std::move(operation));
}

void DynamicUIOperationQueue::Destroy() { impl_->Destroy(); }

void DynamicUIOperationQueue::UpdateStatus(UIOperationStatus status) {
  impl_->UpdateStatus(status);
}

void DynamicUIOperationQueue::MarkDirty() { impl_->MarkDirty(); }

void DynamicUIOperationQueue::ForceFlush() { impl_->ForceFlush(); }

void DynamicUIOperationQueue::Flush() { impl_->Flush(); }

void DynamicUIOperationQueue::SetEnableFlush(bool enable_flush) {
  impl_->SetEnableFlush(enable_flush);
}

void DynamicUIOperationQueue::SetErrorCallback(ErrorCallback callback) {
  impl_->SetErrorCallback(std::move(callback));
}

uint32_t DynamicUIOperationQueue::GetNativeUpdateDataOrder() {
  return impl_->GetNativeUpdateDataOrder();
}

uint32_t DynamicUIOperationQueue::UpdateNativeUpdateDataOrder() {
  return impl_->UpdateNativeUpdateDataOrder();
}

void DynamicUIOperationQueue::CreateImpl() {
  impl_ = is_engine_async_
              ? std::make_shared<shell::LynxUIOperationAsyncQueue>(ui_runner_,
                                                                   instance_id_)
              : std::make_shared<shell::LynxUIOperationQueue>(instance_id_);
}

}  // namespace shell
}  // namespace lynx
