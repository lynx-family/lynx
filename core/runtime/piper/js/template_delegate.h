// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PIPER_JS_TEMPLATE_DELEGATE_H_
#define CORE_RUNTIME_PIPER_JS_TEMPLATE_DELEGATE_H_
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "core/public/pipeline_option.h"
#include "core/public/prop_bundle.h"
#include "core/public/vsync_observer_interface.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/js_bundle.h"
#include "core/runtime/piper/js/update_data_type.h"
#include "core/runtime/vm/lepus/lepus_global.h"
#include "core/services/timing_handler/timing.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/shell/lynx_card_cache_data_op.h"
#include "third_party/rapidjson/document.h"

namespace lynx {

namespace base {
struct LynxError;
}
namespace tasm {
class WhiteBoardDelegate;
}

namespace runtime {

// just constructor and move
struct UpdateDataTask {
  UpdateDataTask(bool card, const std::string& component_id,
                 const lepus::Value& data, piper::ApiCallBack callback,
                 UpdateDataType type, tasm::PipelineOptions pipeline_options,
                 std::string stacks = "")
      : is_card_(card),
        component_id_(component_id),
        data_(data),
        callback_(callback),
        type_(std::move(type)),
        pipeline_options_(std::move(pipeline_options)),
        stacks_(std::move(stacks)) {}

  UpdateDataTask(const UpdateDataTask&) = delete;
  UpdateDataTask& operator=(const UpdateDataTask&) = delete;
  UpdateDataTask(UpdateDataTask&&) = default;
  UpdateDataTask& operator=(UpdateDataTask&&) = default;

  bool is_card_;
  std::string component_id_;
  lepus::Value data_;
  piper::ApiCallBack callback_;
  UpdateDataType type_;
  tasm::PipelineOptions pipeline_options_;
  // stacks of setState/setData tasks, only use for debug mode
  std::string stacks_;
};

class TemplateDelegate : public ContextProxy::Delegate {
 public:
  TemplateDelegate() {}
  virtual ~TemplateDelegate() override = default;

  virtual void UpdateDataByJS(UpdateDataTask task) = 0;
  virtual void UpdateBatchedDataByJS(std::vector<UpdateDataTask> tasks,
                                     uint64_t update_task_id) = 0;
  virtual std::vector<lynx::shell::CacheDataOp> FetchUpdatedCardData() = 0;
  virtual piper::JsContent GetJSContentFromExternal(
      const std::string& entry_name, const std::string& name, long timeout) = 0;
  virtual std::string GetLynxJSAsset(const std::string& name) = 0;

  virtual void GetComponentContextDataAsync(const std::string& component_id,
                                            const std::string& key,
                                            piper::ApiCallBack callback) = 0;
  virtual bool LoadDynamicComponentFromJS(
      const std::string& url, const piper::ApiCallBack& callback,
      const std::vector<std::string>& ids) = 0;
  virtual void LoadScriptAsync(const std::string& url,
                               piper::ApiCallBack callback) = 0;

  virtual void AddFont(const lepus::Value& font,
                       const piper::ApiCallBack& callback) = 0;

  virtual void OnRuntimeReady() = 0;

  virtual void OnErrorOccurred(base::LynxError error) = 0;

  virtual void OnModuleMethodInvoked(const std::string& module,
                                     const std::string& method,
                                     int32_t code) = 0;
  virtual void OnCoreJSUpdated(std::string core_js) = 0;

  // for component
  virtual void UpdateComponentData(UpdateDataTask task) = 0;
  virtual void SelectComponent(const std::string& component_id,
                               const std::string& id_selector,
                               const bool single,
                               piper::ApiCallBack callBack) = 0;

  // for SelectorQuery
  virtual void InvokeUIMethod(tasm::NodeSelectRoot root,
                              tasm::NodeSelectOptions options,
                              std::string method,
                              std::unique_ptr<tasm::PropBundle> params,
                              piper::ApiCallBack call_back) = 0;
  virtual void GetPathInfo(tasm::NodeSelectRoot root,
                           tasm::NodeSelectOptions options,
                           piper::ApiCallBack call_back) = 0;
  virtual void GetFields(tasm::NodeSelectRoot root,
                         tasm::NodeSelectOptions options,
                         std::vector<std::string> fields,
                         piper::ApiCallBack call_back) = 0;

  // for element.animate
  virtual void ElementAnimate(const std::string& component_id,
                              const std::string& id_selector,
                              const lepus::Value& args) = 0;
  virtual void TriggerComponentEvent(const std::string& event_name,
                                     const lepus::Value& msg) = 0;
  virtual void TriggerLepusGlobalEvent(const std::string& event_name,
                                       const lepus::Value& msg) = 0;
  virtual void TriggerWorkletFunction(std::string component_id,
                                      std::string worklet_module_name,
                                      std::string method_name,
                                      lepus::Value args,
                                      piper::ApiCallBack callback) = 0;
  virtual void RunOnJSThread(base::closure closure) = 0;
  virtual void RunOnJSThreadWhenIdle(base::closure closure) = 0;
  virtual void SetTiming(tasm::Timing timing) = 0;
  virtual void SetTimingWithTimingFlag(
      const tasm::timing::TimingFlag& timing_flag,
      const std::string& timestamp_key,
      tasm::timing::TimestampUs timestamp) = 0;
  virtual void FlushJSBTiming(piper::NativeModuleInfo timing) = 0;

  virtual void OnPipelineStart(
      const tasm::PipelineID& pipeline_id,
      const tasm::PipelineOrigin& pipeline_origin,
      tasm::timing::TimestampUs pipeline_start_timestamp) = 0;

  virtual void BindPipelineIDWithTimingFlag(
      const tasm::PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag) = 0;

  // for lepus event
  virtual void InvokeLepusComponentCallback(const int64_t callback_id,
                                            const std::string& entry_name,
                                            const lepus::Value& data) = 0;
  // for vsync
  virtual std::shared_ptr<runtime::IVSyncObserver> GetVSyncObserver() = 0;

  virtual void SetCSSVariables(const std::string& component_id,
                               const std::string& id_selector,
                               const lepus::Value& properties,
                               tasm::PipelineOptions pipeline_options) = 0;

  virtual void SetNativeProps(tasm::NodeSelectRoot root,
                              const tasm::NodeSelectOptions& options,
                              const lepus::Value& native_props,
                              tasm::PipelineOptions pipeline_options) = 0;

  virtual void ReloadFromJS(UpdateDataTask task) = 0;

  // for Fiber
  virtual void CallLepusMethod(const std::string& method_name,
                               lepus::Value value,
                               const piper::ApiCallBack& callback,
                               uint64_t trace_flow_id) = 0;

  virtual std::unique_ptr<tasm::PropBundle> CreatePropBundle() = 0;

  virtual void SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator>& creator) = 0;

  virtual void SetWhiteBoardDelegate(
      const std::shared_ptr<tasm::WhiteBoardDelegate>&
          white_board_delegate) = 0;

  virtual void AddEventListenersToWhiteBoard(
      runtime::ContextProxy* js_context_proxy) = 0;

  virtual std::string LoadJSSource(const std::string& name) = 0;

  virtual void GetSessionStorageItem(const std::string& key,
                                     const piper::ApiCallBack& callback) = 0;

  virtual void SubscribeSessionStorage(const std::string&, double listener_id,
                                       const piper::ApiCallBack& callback) = 0;
};
}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_PIPER_JS_TEMPLATE_DELEGATE_H_
