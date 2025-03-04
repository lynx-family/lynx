// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/tasks/lepus_callback_manager.h"

#include <utility>

#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {
LepusCallbackManager::FuncTask::FuncTask(lepus::Context* context,
                                         std::unique_ptr<lepus::Value> closure)
    : closure_(std::move(closure)), context_(context) {}

void LepusCallbackManager::FuncTask::Execute(const lepus::Value& args) {
  context_->CallClosure(*closure_, args);
}

int64_t LepusCallbackManager::CacheTask(
    lepus::Context* context, std::unique_ptr<lepus::Value> callback_closure) {
  task_map_.emplace(std::make_pair(
      ++current_task_id_,
      std::make_unique<FuncTask>(context, std::move(callback_closure))));
  return current_task_id_;
}

void LepusCallbackManager::InvokeTask(int64_t id, const lepus::Value& data) {
  auto iter = task_map_.find(id);
  if (iter != task_map_.end()) {
    auto task = std::move(iter->second);
    task_map_.erase(iter);
    task->Execute(data);
  }
}

uint32_t LepusCallbackManager::SetTimeOut(lepus::Context* context,
                                          std::unique_ptr<lepus::Value> closure,
                                          int64_t delay_time) {
  return SetTimeTask(context, std::move(closure), delay_time, false);
}

uint32_t LepusCallbackManager::SetInterval(
    lepus::Context* context, std::unique_ptr<lepus::Value> closure,
    int64_t interval_time) {
  return SetTimeTask(context, std::move(closure), interval_time, true);
}

void LepusCallbackManager::RemoveTimeTask(uint32_t task_id) {
  if (timer_task_manager_) {
    timer_task_manager_->StopTask(task_id);
  }
}

uint32_t LepusCallbackManager::SetTimeTask(
    lepus::Context* context, std::unique_ptr<lepus::Value> closure,
    int64_t delay_time, bool is_interval) {
  EnsureTimerTaskInvokerInited(context);
  auto task = [func =
                   std::make_unique<FuncTask>(context, std::move(closure))]() {
    func->Execute({lepus::Value::CreateObject()});
  };
  if (is_interval) {
    return timer_task_manager_->SetInterval(std::move(task), delay_time);
  } else {
    return timer_task_manager_->SetTimeout(std::move(task), delay_time);
  }
}

LepusCallbackManager::~LepusCallbackManager() { Destroy(); }

void LepusCallbackManager::EnsureTimerTaskInvokerInited(
    lepus::Context* context) {
  if (!timer_task_manager_) {
    timer_task_manager_ = std::make_unique<base::TimedTaskManager>(
        true, context->GetDelegate()
                  ? context->GetDelegate()->GetLepusTimedTaskRunner()
                  : nullptr);
  }
}

void LepusCallbackManager::Destroy() { task_map_.clear(); }

}  // namespace tasm
}  // namespace lynx
