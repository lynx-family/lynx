// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_MOCK_TEMPLATE_DELEGATE_H_
#define CORE_RUNTIME_PIPER_JS_MOCK_TEMPLATE_DELEGATE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/piper/js/template_delegate.h"

namespace lynx {
namespace runtime {
namespace test {

class MockTemplateDelegate : public runtime::TemplateDelegate {
 public:
  void UpdateDataByJS(UpdateDataTask task) override {}
  void UpdateBatchedDataByJS(std::vector<UpdateDataTask> tasks,
                             uint64_t update_task_id) override {}
  std::vector<lynx::shell::CacheDataOp> FetchUpdatedCardData() override {
    return {};
  }
  piper::JsContent GetJSContentFromExternal(const std::string& entry_name,
                                            const std::string& name,
                                            long timeout) override {
    return {"", piper::JsContent::Type::SOURCE};
  }

  std::string GetLynxJSAsset(const std::string& name) override { return ""; };

  void GetComponentContextDataAsync(const std::string& component_id,
                                    const std::string& key,
                                    piper::ApiCallBack callback) override {}
  bool LoadDynamicComponentFromJS(
      const std::string& url, const piper::ApiCallBack& callback,
      const std::vector<std::string>& ids) override {
    return true;
  }
  void LoadScriptAsync(const std::string& url,
                       piper::ApiCallBack callback) override {}

  void AddFont(const lepus::Value& font,
               const piper::ApiCallBack& callback) override {}

  void OnRuntimeReady() override {}
  void OnErrorOccurred(base::LynxError error) override {}
  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override {
  }
  void OnCoreJSUpdated(std::string core_js) override {}
  // for component
  void UpdateComponentData(lynx::runtime::UpdateDataTask task) override {}
  void SelectComponent(const std::string& component_id,
                       const std::string& id_selector, const bool single,
                       piper::ApiCallBack callBack) override {}

  // for SelectorQuery
  void InvokeUIMethod(tasm::NodeSelectRoot root,
                      tasm::NodeSelectOptions options, std::string method,
                      std::unique_ptr<tasm::PropBundle> params,
                      piper::ApiCallBack call_back) override {}
  void GetPathInfo(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                   piper::ApiCallBack call_back) override {}
  void GetFields(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                 std::vector<std::string> fields,
                 piper::ApiCallBack call_back) override {}

  // for element.animate
  void ElementAnimate(const std::string& component_id,
                      const std::string& id_selector,
                      const lepus::Value& args) override {}
  void TriggerComponentEvent(const std::string& event_name,
                             const lepus::Value& msg) override {}
  void TriggerLepusGlobalEvent(const std::string& event_name,
                               const lepus::Value& msg) override {}
  void TriggerWorkletFunction(std::string component_id,
                              std::string worklet_module_name,
                              std::string method_name, lepus::Value args,
                              piper::ApiCallBack callback) override {}
  void RunOnJSThread(base::closure closure) override {}
  void RunOnJSThreadWhenIdle(base::closure closure) override { closure(); }
  void SetTiming(tasm::Timing timing) override {}
  void SetTimingWithTimingFlag(const tasm::timing::TimingFlag& timing_flag,
                               const std::string& timestamp_key,
                               tasm::timing::TimestampUs timestamp) override{};
  void FlushJSBTiming(piper::NativeModuleInfo timing) override {}

  void OnPipelineStart(
      const tasm::PipelineID& pipeline_id,
      const tasm::PipelineOrigin& pipeline_origin,
      tasm::timing::TimestampUs pipeline_start_timestamp) override{};

  void BindPipelineIDWithTimingFlag(
      const tasm::PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag) override{};

  // for lepus event
  void InvokeLepusComponentCallback(const int64_t callback_id,
                                    const std::string& entry_name,
                                    const lepus::Value& data) override {}
  // for vsync
  std::shared_ptr<runtime::IVSyncObserver> GetVSyncObserver() override {
    return nullptr;
  };

  void SetCSSVariables(const std::string& component_id,
                       const std::string& id_selector,
                       const lepus::Value& properties,
                       tasm::PipelineOptions pipeline_options) override {}

  void SetNativeProps(tasm::NodeSelectRoot root,
                      const tasm::NodeSelectOptions& options,
                      const lepus::Value& native_props,
                      tasm::PipelineOptions pipeline_options) override {}

  void ReloadFromJS(lynx::runtime::UpdateDataTask task) override {}

  // for Fiber
  void CallLepusMethod(const std::string& method_name, lepus::Value value,
                       const piper::ApiCallBack& callback,
                       uint64_t trace_flow_id) override {}

  event::DispatchEventResult DispatchMessageEvent(
      runtime::MessageEvent event) override {
    return event::DispatchEventResult::kNotCanceled;
  }

  std::unique_ptr<tasm::PropBundle> CreatePropBundle() override {
    return prop_bundle_creator_->CreatePropBundle();
  }

  void SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator>& creator) override {
    prop_bundle_creator_ = creator;
  }

  void SetWhiteBoardDelegate(const std::shared_ptr<tasm::WhiteBoardDelegate>&
                                 white_board_delegate) override {}

  void AddEventListenersToWhiteBoard(
      runtime::ContextProxy* js_context_proxy) override {}

  std::string LoadJSSource(const std::string& name) override { return ""; }

  void GetSessionStorageItem(const std::string& key,
                             const piper::ApiCallBack& callback) override{};

  void SubscribeSessionStorage(const std::string&, double listener_id,
                               const piper::ApiCallBack& callback) override{};

 protected:
  std::string sdk_version_;
  std::shared_ptr<tasm::PropBundleCreator> prop_bundle_creator_;
};
};  // namespace test

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_MOCK_TEMPLATE_DELEGATE_H_
