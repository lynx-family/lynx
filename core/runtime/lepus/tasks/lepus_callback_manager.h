// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_TASKS_LEPUS_CALLBACK_MANAGER_H_
#define CORE_RUNTIME_LEPUS_TASKS_LEPUS_CALLBACK_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/include/thread/timed_task.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {

class LepusCallbackManager {
 public:
  LepusCallbackManager() = default;
  ~LepusCallbackManager();
  // common method : cache , invoke and clear
  int64_t CacheTask(runtime::MTSRuntime* context,
                    std::unique_ptr<lepus::Value> callback_closure);
  void InvokeTask(int64_t id, const lepus::Value& data);
  void InvokeTask(int64_t id, const std::vector<lepus::Value>& data);
  // timed task methods:setTime , setInterval , clear and invoke
  // The return type depends on TimedTaskManager。
  uint32_t SetTimeOut(runtime::MTSRuntime* context,
                      std::unique_ptr<lepus::Value> closure,
                      int64_t delay_time);
  // The return type depends on TimedTaskManager。
  uint32_t SetInterval(runtime::MTSRuntime* context,
                       std::unique_ptr<lepus::Value> closure,
                       int64_t interval_time);
  void RemoveTimeTask(uint32_t task_id);

 private:
  // FuncTask cached in TaskMap , it has execute method
  class FuncTask {
   public:
    FuncTask(runtime::MTSRuntime* context,
             std::unique_ptr<lepus::Value> closure);
    void Execute(const lepus::Value& args);
    void Execute(const std::vector<lepus::Value>& args);

   private:
    // real closure callBack , operate by Execute method
    std::unique_ptr<lepus::Value> closure_;
    runtime::MTSRuntime* context_;
  };

  using TaskMap = std::unordered_map<int64_t, std::unique_ptr<FuncTask>>;

  // task form _TriggerLepusBridge
  TaskMap task_map_;
  int64_t current_task_id_{0};

  // time task for _SetTimeout , _SetTimeInterval
  std::unique_ptr<base::TimedTaskManager> timer_task_manager_{nullptr};
  // The return type depends on TimedTaskManager。
  uint32_t SetTimeTask(runtime::MTSRuntime* context,
                       std::unique_ptr<lepus::Value> closure,
                       int64_t delay_time, bool is_interval);

  void EnsureTimerTaskInvokerInited(runtime::MTSRuntime* context);
  void Destroy();
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_TASKS_LEPUS_CALLBACK_MANAGER_H_
