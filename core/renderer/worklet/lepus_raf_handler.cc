// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/worklet/lepus_raf_handler.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/worklet/base/worklet_utils.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_lynx.h"
#include "core/runtime/bindings/napi/worklet/napi_frame_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_func_callback.h"

namespace lynx {
namespace worklet {

LepusAnimationFrameTaskHandler::FrameTask::FrameTask(
    std::unique_ptr<NapiFrameCallback> callback)
    : callback_(std::move(callback)), cancelled_(false) {}

void LepusAnimationFrameTaskHandler::FrameTask::Execute(
    int64_t time_stamp, std::shared_ptr<tasm::TemplateAssembler> tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LepusAnimationFrameTaskHandler::FrameTask::Execute");
  if (cancelled_ || tasm == nullptr) {
    return;
  }

  auto exception_handler = [tasm](Napi::Env env) {
    Napi::Object error = env.GetAndClearPendingException().As<Napi::Object>();

    std::ostringstream ss;
    constexpr const static char* sKeyMessage = "message";
    constexpr const static char* sKeyStack = "stack";

    if (error.Has(sKeyMessage).FromMaybe(false)) {
      ss << "Exception has happened when exec requestAnimationFrame, the error "
            "message is: "
         << std::endl;
      ss << error.Get(sKeyMessage).As<Napi::String>().Utf8Value() << std::endl;
    }
    if (error.Has(sKeyStack).FromMaybe(false)) {
      ss << "Exception has happened when exec requestAnimationFrame, the error "
            "stack is: "
         << std::endl;
      ss << error.Get(sKeyStack).As<Napi::String>().Utf8Value() << std::endl;
    }

    tasm->ReportError(error::E_WORKLET_RAF_CALL_EXCEPTION, ss.str());
  };

  callback_->SetExceptionHandler(exception_handler);
  callback_->Invoke(time_stamp);
}

void LepusAnimationFrameTaskHandler::FrameTask::Cancel() { cancelled_ = true; }

LepusAnimationFrameTaskHandler::LepusAnimationFrameTaskHandler()
    : current_index_(0), first_map_is_the_current_(true), doing_frame_(false) {}

LepusAnimationFrameTaskHandler::~LepusAnimationFrameTaskHandler() { Destroy(); }

int64_t LepusAnimationFrameTaskHandler::RequestAnimationFrame(
    std::unique_ptr<NapiFrameCallback> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LepusAnimationFrameTaskHandler::RequestAnimationFrame");
  const int64_t task_id = current_index_++;
  std::unique_ptr<FrameTask> task =
      std::make_unique<FrameTask>(std::move(callback));

  if (doing_frame_) {
    // avoid recursive function call of "RequestAnimationFrame"
    NextFrameTaskMap().insert(std::make_pair(task_id, std::move(task)));
  } else {
    CurrentFrameTaskMap().insert(std::make_pair(task_id, std::move(task)));
  }
  return task_id;
}

void LepusAnimationFrameTaskHandler::CancelAnimationFrame(int64_t id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LepusAnimationFrameTaskHandler::CancelAnimationFrame");
  auto itr = task_map_first_.find(id);
  if (itr != task_map_first_.end()) {
    itr->second->Cancel();
    return;
  }

  itr = task_map_second_.find(id);
  if (itr != task_map_second_.end()) {
    itr->second->Cancel();
  }
}

void LepusAnimationFrameTaskHandler::DoFrame(
    int64_t time_stamp, std::shared_ptr<tasm::TemplateAssembler> tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusAnimationFrameTaskHandler::DoFrame");
  doing_frame_ = true;
  TaskMap& task_map = CurrentFrameTaskMap();
  for (auto& itr : task_map) {
    itr.second->Execute(time_stamp, tasm);
  }
  task_map.clear();

  // swap current task map and pending task map.
  first_map_is_the_current_ = !(first_map_is_the_current_);
  doing_frame_ = false;
  // trigger patch finish when a worklet operation is completed
  tasm::PipelineOptions options;
  // TODO(kechenglong): SetNeedsLayout if and only if needed.
  tasm->page_proxy()->element_manager()->SetNeedsLayout();
  tasm->page_proxy()->element_manager()->OnPatchFinish(options);
}

void LepusAnimationFrameTaskHandler::Destroy() {
  task_map_first_.clear();
  task_map_second_.clear();
}

bool LepusAnimationFrameTaskHandler::HasPendingRequest() {
  return !task_map_first_.empty() || !task_map_second_.empty();
}

LepusAnimationFrameTaskHandler::TaskMap&
LepusAnimationFrameTaskHandler::CurrentFrameTaskMap() {
  if (first_map_is_the_current_) {
    return task_map_first_;
  }
  return task_map_second_;
}

LepusAnimationFrameTaskHandler::TaskMap&
LepusAnimationFrameTaskHandler::NextFrameTaskMap() {
  if (!first_map_is_the_current_) {
    return task_map_first_;
  }
  return task_map_second_;
}

LepusApiHandler::~LepusApiHandler() { Destroy(); }

LepusApiHandler::FuncTask::FuncTask(std::unique_ptr<NapiFuncCallback> callback)
    : callback_(std::move(callback)), cancelled_(false) {}

void LepusApiHandler::FuncTask::Execute(const lepus::Value& value,
                                        tasm::TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::FuncTask::Execute");
  if (cancelled_ || tasm == nullptr) {
    return;
  }

  bool valid = true;
  auto env = callback_->Env(&valid);
  if (!valid) {
    tasm->ReportError(
        error::E_WORKLET_RAF_CALL_EXCEPTION,
        "LepusApiHandler::FuncTask::Execute failed since Napi Env not valid.");
  }

  Execute(ValueConverter::ConvertLepusValueToNapiValue(env, value), tasm);
}

void LepusApiHandler::FuncTask::Execute(Napi::Value value,
                                        tasm::TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::FuncTask::Execute");
  if (cancelled_ || tasm == nullptr) {
    return;
  }

  auto exception_handler = [tasm](Napi::Env env) {
    Napi::Object error = env.GetAndClearPendingException().As<Napi::Object>();
    tasm->ReportError(error::E_WORKLET_RAF_CALL_EXCEPTION, error.ToString());
  };

  callback_->SetExceptionHandler(exception_handler);
  callback_->Invoke(value);
}

void LepusApiHandler::FuncTask::Cancel() { cancelled_ = true; }

int64_t LepusApiHandler::StoreTask(std::unique_ptr<NapiFuncCallback> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::StoreTask");
  const int64_t task_id = current_task_id_++;
  std::unique_ptr<FuncTask> task =
      std::make_unique<FuncTask>(std::move(callback));

  lepus_task_map_.insert(std::make_pair(task_id, std::move(task)));
  return task_id;
}

// void LepusApiHandler::Destroy() { lepus_task_map_.clear(); }
int64_t LepusApiHandler::StoreTimedTask(
    std::unique_ptr<NapiFuncCallback> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::StoreTimedTask");
  const int64_t task_id = current_task_id_++;
  std::unique_ptr<FuncTask> task =
      std::make_unique<FuncTask>(std::move(callback));
  lepus_timed_task_map_.insert(std::make_pair(task_id, std::move(task)));
  return task_id;
}

void LepusApiHandler::Destroy() {
  lepus_task_map_.clear();
  lepus_timed_task_map_.clear();
}

bool LepusApiHandler::HasPendingCalling() { return !lepus_task_map_.empty(); }

void LepusApiHandler::InvokeWithTaskID(int64_t task_id,
                                       const lepus::Value& value,
                                       tasm::TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::InvokeWithTaskID");
  LepusTaskMap& task_map = lepus_task_map_;
  auto itr = task_map.find(task_id);
  if (itr != task_map.end()) {
    itr->second->Execute(value, tasm);
    task_map.erase(itr);
  }
}

void LepusApiHandler::InvokeWithTaskID(int64_t task_id, Napi::Value value,
                                       tasm::TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::InvokeWithTaskID");
  LepusTaskMap& task_map = lepus_task_map_;
  auto itr = task_map.find(task_id);
  if (itr != task_map.end()) {
    itr->second->Execute(value, tasm);
    task_map.erase(itr);
  }
}

void LepusApiHandler::InvokeWithTimedTaskID(int64_t task_id, Napi::Value value,
                                            tasm::TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusApiHandler::InvokeWithTimedTaskID");
  auto itr = lepus_timed_task_map_.find(task_id);
  if (itr != lepus_timed_task_map_.end()) {
    itr->second->Execute(value, tasm);
  }
}

void LepusApiHandler::RemoveTimeTask(int64_t task_id) {
  LepusTaskMap& task_map = lepus_timed_task_map_;
  auto itr = task_map.find(task_id);
  if (itr != task_map.end()) {
    task_map.erase(itr);
  }
}

void LepusApiHandler::RemoveTimeTask() { lepus_timed_task_map_.clear(); }

}  // namespace worklet
}  // namespace lynx
