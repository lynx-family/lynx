// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/js_task_adapter.h"

#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/make_copyable.h"
#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_event.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace piper {

namespace {

class AdapterTask {
 public:
  explicit AdapterTask(lynx::base::closure closure,
                       lynx::base::closure finish_callback = nullptr)
      : closure_(std::move(closure)),
        finish_callback_(std::move(finish_callback)) {}

  int64_t Id() { return reinterpret_cast<int64_t>(&closure_); }

  void Run() {
    DCHECK(closure_);
    closure_();
    if (finish_callback_) {
      finish_callback_();
    }
  }

 private:
  lynx::base::closure closure_;

  lynx::base::closure finish_callback_;
};

}  // namespace

JsTaskAdapter::JsTaskAdapter(const std::weak_ptr<Runtime>& rt,
                             const std::string& group_id)
    : manager_(std::make_unique<base::TimedTaskManager>()),
      runner_(fml::MessageLoop::GetCurrent().GetTaskRunner()),
      rt_(rt) {}

JsTaskAdapter::~JsTaskAdapter() { manager_->StopAllTasks(); }

piper::Value JsTaskAdapter::SetTimeout(Function func, int32_t delay) {
  auto task = MakeTask(std::move(func), TaskType::kSetTimeout);
  return piper::Value(static_cast<int>(
      manager_->SetTimeout(std::move(task), static_cast<int64_t>(delay))));
}

piper::Value JsTaskAdapter::SetInterval(Function func, int32_t delay) {
  auto task = MakeTask(std::move(func), TaskType::kSetInterval);
  return piper::Value(static_cast<int>(
      manager_->SetInterval(std::move(task), static_cast<int64_t>(delay))));
}

void JsTaskAdapter::QueueMicrotask(Function func) {
  auto task = MakeTask(std::move(func), TaskType::kQueueMicrotask);
  runner_->PostMicroTask(fml::MakeCopyable(
      [weak_this = weak_from_this(), task = std::move(task)]() mutable {
        auto lock_this = weak_this.lock();
        if (lock_this) {
          task();
        }
      }));
}

base::closure JsTaskAdapter::MakeTask(Function func, TaskType task_type) {
  return fml::MakeCopyable(
      [weak_rt = rt_, func = std::move(func), task_type]() {
        auto rt = weak_rt.lock();
        if (rt) {
          int32_t instance_id = static_cast<int32_t>(rt->getRuntimeId());
          std::string task_name;
          switch (task_type) {
            case TaskType::kSetTimeout:
              task_name = tasm::timing::kTaskNameJsTaskAdapterSetTimeout;
              break;
            case TaskType::kSetInterval:
              task_name = tasm::timing::kTaskNameJsTaskAdapterSetInterval;
              break;
            case TaskType::kQueueMicrotask:
              task_name = tasm::timing::kTaskNameJsTaskAdapterQueueMicrotask;
              break;
          }
          TRACE_EVENT("lynx", task_name, "instance_id", instance_id);

          tasm::timing::LongTaskMonitor::Scope long_task_scope(
              instance_id, tasm::timing::kTimerTask, task_name);
          piper::Scope scope(*rt);
          // Explicitly ignore the return value since the exception will be
          // handled by `func.call`.
          static_cast<void>(func.call(*rt, nullptr, 0));
        }
      });
}

void JsTaskAdapter::RemoveTask(uint32_t task) { manager_->StopTask(task); }

}  // namespace piper
}  // namespace lynx
