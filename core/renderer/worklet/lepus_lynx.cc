// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/worklet/lepus_lynx.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/worklet/base/worklet_utils.h"
#include "core/runtime/bindings/napi/worklet/napi_frame_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_func_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_element.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace worklet {

LepusLynx::LepusLynx(Napi::Env env, const std::string& entry_name,
                     tasm::TemplateAssembler* assembler)
    : env_(env),
      entry_name_(entry_name),
      tasm_(assembler),
      task_handler_(std::make_unique<LepusApiHandler>()) {}

uint32_t LepusLynx::SetTimeout(std::unique_ptr<NapiFuncCallback> callback,
                               int64_t delay) {
  int32_t instance_id = tasm_->GetInstanceId();
  TRACE_EVENT("lynx", "MainThread::SetTimeout", "delay", delay, "instance_id",
              instance_id);
  EnsureTimeTaskInvoker();
  auto callback_id = task_handler_->StoreTimedTask(std::move(callback));
  auto task_id = timer_->SetTimeout(
      fml::MakeCopyable([env = NapiEnv(), callback_id, this, instance_id]() {
        TRACE_EVENT("lynx", "MainThread::InvokeSetTimeoutTask", "instance_id",
                    instance_id);
        tasm::timing::LongTaskMonitor::Scope long_task_scope(
            instance_id, tasm::timing::kTimerTask,
            tasm::timing::kTaskNameLepusLynxSetTimeout);
        task_handler_->InvokeWithTimedTaskID(callback_id,
                                             Napi::Object::New(env), tasm_);
        task_handler_->RemoveTimeTask(callback_id);
        tasm::PipelineOptions options;
        // TODO(kechenglong): SetNeedsLayout if and only if needed.
        tasm_->page_proxy()->element_manager()->SetNeedsLayout();
        tasm_->page_proxy()->element_manager()->OnPatchFinish(options);
      }),
      delay);
  task_to_callback_map_[task_id] = callback_id;
  return task_id;
}

uint32_t LepusLynx::SetInterval(std::unique_ptr<NapiFuncCallback> callback,
                                int64_t delay) {
  int32_t instance_id = tasm_->GetInstanceId();
  TRACE_EVENT("lynx", "MainThread::SetInterval", "delay", delay, "instance_id",
              instance_id);
  EnsureTimeTaskInvoker();
  auto callback_id = task_handler_->StoreTimedTask(std::move(callback));
  auto task_id = timer_->SetInterval(
      [callback_id, this, instance_id]() {
        TRACE_EVENT("lynx", "MainThread::InvokeSetIntervalTask", "instance_id",
                    instance_id);
        tasm::timing::LongTaskMonitor::Scope long_task_scope(
            instance_id, tasm::timing::kTimerTask,
            tasm::timing::kTaskNameLepusLynxSetInterval);
        task_handler_->InvokeWithTimedTaskID(
            callback_id, Napi::Object::New(NapiEnv()), tasm_);
        tasm::PipelineOptions options;
        // TODO(kechenglong): SetNeedsLayout if and only if needed.
        tasm_->page_proxy()->element_manager()->SetNeedsLayout();
        tasm_->page_proxy()->element_manager()->OnPatchFinish(options);
      },
      delay);
  task_to_callback_map_[task_id] = callback_id;
  return task_id;
}

void LepusLynx::ClearTimeout(uint32_t task_id) { RemoveTimedTask(task_id); }

void LepusLynx::ClearInterval(uint32_t task_id) { RemoveTimedTask(task_id); }

void LepusLynx::RemoveTimedTask(uint32_t task_id) {
  EnsureTimeTaskInvoker();
  timer_->StopTask(task_id);
  // TODO(songshourui.null): The NapiFunction should be removed to avoid memory
  // leak. However, the developers may currently remove the callback itself in
  // the setTimeout or setInterval callback, which can lead to crashes.
  // Therefore, this part of the code has been commented out for the time being
  // to prevent crashes. We will fix the memory leak issue while also avoiding
  // crashes in the future.
  // task_handler_->RemoveTimeTask(task_to_callback_map_[task_id]);
  task_to_callback_map_.erase(task_id);
}

void LepusLynx::EnsureTimeTaskInvoker() {
  if (timer_ == nullptr) {
    timer_ = std::make_unique<base::TimedTaskManager>();
  }
}

void LepusLynx::TriggerLepusBridge(const std::string& method_name,
                                   Napi::Object method_detail,
                                   std::unique_ptr<NapiFuncCallback> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusLynx:::TriggerLepusBridge",
              "method_name", method_name);

  constexpr const static char* kEventDetail = "methodDetail";
  constexpr const static char* kEventCallbackId = "callbackId";
  constexpr const static char* kEventEntryName = "tasmEntryName";

  tasm::report::FeatureCounter::Instance()->Count(
      tasm::report::LynxFeature::CPP_USE_LEGACY_LEPUS_BRIDGE_ASYNC);

  int64_t callback_id = task_handler_->StoreTask(std::move(callback));
  // Native Method triggered from lepus, toLepus default value is ture, toJS
  // default value is false.
  // Construct event para.
  Napi::Object para = Napi::Object::New(NapiEnv());
  para.Set(kEventDetail, method_detail);
  para.Set(kEventCallbackId, callback_id);
  para.Set(kEventEntryName, entry_name_);
  const auto& lepus_para = ValueConverter::ConvertNapiValueToLepusValue(para);
  tasm_->TriggerLepusBridgeAsync(method_name, lepus_para);
}

Napi::Value LepusLynx::TriggerLepusBridgeSync(const std::string& method_name,
                                              Napi::Object method_detail) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusLynx:::TriggerLepusBridgeSync",
              "method_name", method_name);
  LOGI("LepusLynx TriggerLepusBridgeSync triggered");
  if (tasm_ == nullptr) {
    LOGE("LepusLynx TriggerLepusBridge failed since tasm is nullptr");
    return Napi::Object::New(NapiEnv());
  }

  constexpr const static char* kEventDetail = "methodDetail";
  constexpr const static char* kEventComponentId = "componentId";
  constexpr const static char* kEventEntryName = "tasmEntryName";

  tasm::report::FeatureCounter::Instance()->Count(
      tasm::report::LynxFeature::CPP_USE_LEGACY_LEPUS_BRIDGE_SYNC);

  Napi::Object para = Napi::Object::New(NapiEnv());
  para.Set(kEventDetail, method_detail);
  // TODO(fulei.bill): remove this componentId later
  para.Set(kEventComponentId, Napi::String::New(NapiEnv(), std::to_string(-1)));
  para.Set(kEventEntryName, entry_name_);
  const auto& lepus_para = ValueConverter::ConvertNapiValueToLepusValue(para);

  Napi::Value callback_param = ValueConverter::ConvertLepusValueToNapiValue(
      NapiEnv(), tasm_->TriggerLepusBridge(method_name, lepus_para));
  return callback_param;
}

void LepusLynx::InvokeLepusBridge(const int32_t callback_id,
                                  const lepus::Value& data) {
  constexpr const static char* kEventCallbackParams = "callbackParams";
  Napi::Object callback_param = Napi::Object::New(NapiEnv());
  callback_param.Set(
      kEventCallbackParams,
      ValueConverter::ConvertLepusValueToNapiValue(NapiEnv(), data));
  task_handler_->InvokeWithTaskID(callback_id, callback_param, tasm_);
}

}  // namespace worklet
}  // namespace lynx
