// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/js_task_adapter.h"

#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/make_copyable.h"
#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
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
                             const std::string& group_id,
                             const tasm::PageOptions& page_options)
    : manager_(std::make_unique<base::TimedTaskManager>()),
      micro_tasks_(
          std::make_shared<std::unordered_map<uint64_t, base::closure>>()),
      current_micro_task_id_(0),
      runner_(fml::MessageLoop::GetCurrent().GetTaskRunner()),
      rt_(rt),
      page_options_(page_options) {}

JsTaskAdapter::~JsTaskAdapter() { manager_->StopAllTasks(); }

piper::Value JsTaskAdapter::SetTimeout(Function func, int32_t delay,
                                       uint64_t trace_flow_id) {
  auto task = MakeTask(std::move(func), TaskType::kSetTimeout, trace_flow_id);
  return piper::Value(static_cast<int>(
      manager_->SetTimeout(std::move(task), static_cast<int64_t>(delay))));
}

piper::Value JsTaskAdapter::SetInterval(Function func, int32_t delay,
                                        uint64_t trace_flow_id) {
  auto task = MakeTask(std::move(func), TaskType::kSetInterval, trace_flow_id);
  return piper::Value(static_cast<int>(
      manager_->SetInterval(std::move(task), static_cast<int64_t>(delay))));
}

void JsTaskAdapter::QueueMicrotask(Function func, uint64_t trace_flow_id) {
  auto task =
      MakeTask(std::move(func), TaskType::kQueueMicrotask, trace_flow_id);
  auto current_id = current_micro_task_id_++;
  micro_tasks_->emplace(current_id, std::move(task));
  runner_->PostMicroTask(fml::MakeCopyable(
      [current_id,
       weak_tasks = std::weak_ptr<std::unordered_map<uint64_t, base::closure>>(
           micro_tasks_)]() mutable {
        auto tasks = weak_tasks.lock();
        if (tasks) {
          auto it = tasks->find(current_id);
          if (it != tasks->end()) {
            it->second();
            tasks->erase(it);
          }
        }
      }));
}

base::closure JsTaskAdapter::MakeTask(Function func, TaskType task_type,
                                      uint64_t trace_flow_id) {
  return fml::MakeCopyable([weak_rt = rt_, func = std::move(func), task_type,
                            trace_flow_id, page_options = page_options_]() {
    auto rt = weak_rt.lock();
    if (rt) {
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
      // Explicitly reference `trace_flow_id` to suppress the compiler warning.
      (void)trace_flow_id;
      TRACE_EVENT("lynx", task_name,
                  [trace_flow_id, instance_id = rt->getRuntimeId()](
                      lynx::perfetto::EventContext ctx) {
                    ctx.event()->add_flow_ids(trace_flow_id);
                    ctx.event()->add_debug_annotations(
                        INSTANCE_ID, std::to_string(instance_id));
                  });

      tasm::timing::LongTaskMonitor::Scope long_task_scope(
          page_options, tasm::timing::kTimerTask, task_name);
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
