// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_RUNTIME_BTS_BTS_RUNTIME_MEDIATOR_H_
#define CORE_SHELL_RUNTIME_BTS_BTS_RUNTIME_MEDIATOR_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/resource/lazy_bundle/bundle_resource_info.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/js/template_delegate.h"
#include "core/runtime/js/update_data_type.h"
#include "core/services/performance/performance_controller.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_card_cache_data_manager.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"
#include "core/shell/vsync_observer_impl.h"

namespace lynx {

namespace runtime {
namespace js {
class JsBundleHolder;
}  // namespace js
}  // namespace runtime

namespace tasm {
class LazyBundleLoader;
}  // namespace tasm

namespace shell {

class BTSRuntime;

// ensure run on js thread, lifecycle manage by LynxRuntime
class BTSRuntimeMediator : public runtime::TemplateDelegate {
 public:
  BTSRuntimeMediator(
      const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor,
      const std::shared_ptr<LynxActor<LynxEngine>>& engine_actor,
      const std::shared_ptr<
          LynxActor<tasm::performance::PerformanceController>>&
          performance_controller_actor,
      const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
      const fml::RefPtr<fml::TaskRunner>& js_runner,
      std::unique_ptr<ExternalResourceLoader> external_resource_loader)
      : facade_actor_(facade_actor),
        engine_actor_(engine_actor),
        perf_controller_actor_(performance_controller_actor),
        card_cached_data_mgr_(card_cached_data_mgr),
        js_runner_(js_runner),
        external_resource_loader_(std::shared_ptr<ExternalResourceLoader>(
            std::move(external_resource_loader))) {
    runtime_standalone_mode_ =
        !facade_actor || !engine_actor || !card_cached_data_mgr;
    // TODO(chenyouhui): Use LynxResourceLoader directly.
    external_resource_loader_->SetEngineActor(engine_actor);
  }
  ~BTSRuntimeMediator() override = default;
  void AttachToLynxShell(
      const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor,
      const std::shared_ptr<LynxActor<LynxEngine>>& engine_actor,
      const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr);

  void OnRuntimeGC(
      std::unordered_map<std::string, std::string> mem_info) override;
  // inherit from TemplateDelegate
  void UpdateDataByJS(runtime::UpdateDataTask task) override;
  void UpdateBatchedDataByJS(std::vector<runtime::UpdateDataTask> tasks,
                             uint64_t update_task_id) override;
  std::vector<lynx::shell::CacheDataOp> FetchUpdatedCardData() override;
  piper::JsContent GetJSContentFromExternal(const std::string& entry_name,
                                            const std::string& name,
                                            long timeout) override;
  std::string GetLynxJSAsset(const std::string& name) override;

  void GetComponentContextDataAsync(const std::string& component_id,
                                    const std::string& key,
                                    piper::ApiCallBack callback) override;
  bool LoadDynamicComponentFromJS(const std::string& url,
                                  const piper::ApiCallBack& callback,
                                  const std::vector<std::string>& ids) override;
  void LoadScriptAsync(const std::string& url,
                       piper::ApiCallBack callback) override;
  void AddFont(const lepus::Value& font,
               const piper::ApiCallBack& callback) override;

  void InvokeResponsePromiseCallback(base::closure closure) override;

  void FetchBundle(
      const std::string& bundle_url,
      const std::shared_ptr<runtime::ResponsePromise<tasm::BundleResourceInfo>>&
          response_promise) override;

  void OnRuntimeReady() override;
  void OnErrorOccurred(base::LynxError error) override;

  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;
  void OnEvaluateJavaScriptEnd(const std::string& url) override;

  void UpdateComponentData(runtime::UpdateDataTask task) override;
  void SelectComponent(const std::string& component_id,
                       const std::string& id_selector, const bool single,
                       piper::ApiCallBack callBack) override;
  void InvokeUIMethod(tasm::NodeSelectRoot root,
                      tasm::NodeSelectOptions options, std::string method,
                      fml::RefPtr<tasm::PropBundle> params,
                      piper::ApiCallBack callback) override;
  void InvokeUIMethod(tasm::NodeSelectRoot root,
                      tasm::NodeSelectOptions options, std::string method,
                      const pub::ValueImplLepus& params,
                      piper::ApiCallBack callback) override;
  void GetPathInfo(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                   piper::ApiCallBack call_back) override;
  void GetFields(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                 std::vector<std::string> fields,
                 piper::ApiCallBack call_back) override;
  void ElementAnimate(const std::string& component_id,
                      const std::string& id_selector,
                      const lepus::Value& args) override;
  void ElementAnimateV2(const std::string& component_id,
                        const std::string& id_selector,
                        const lepus::Value& args) override;
  void TriggerComponentEvent(const std::string& event_name,
                             const lepus::Value& msg) override;
  void TriggerLepusGlobalEvent(const std::string& event_name,
                               const lepus::Value& msg) override;
  void InvokeLepusComponentCallback(const int64_t callback_id,
                                    const std::string& entry_name,
                                    const lepus::Value& data) override;

  void TriggerWorkletFunction(std::string component_id,
                              std::string worklet_module_name,
                              std::string method_name, lepus::Value args,
                              piper::ApiCallBack callback) override;

  void OnCoreJSUpdated(std::string core_js) override;

  void RunOnJSThread(base::closure closure) override;
  void RunOnJSThreadWhenIdle(base::closure closure) override;

  void set_vsync_monitor(
      const std::shared_ptr<base::VSyncMonitor>& vsync_monitor,
      const std::shared_ptr<LynxActor<BTSRuntime>>& runtime_actor) {
    vsync_observer_ = std::make_shared<VSyncObserverImpl>(
        vsync_monitor, js_runner_, runtime_actor);
  }

  std::shared_ptr<runtime::IVSyncObserver> GetVSyncObserver() override {
    return vsync_observer_;
  }

  void SetJsBundleHolder(const std::weak_ptr<piper::JsBundleHolder>& holder) {
    weak_js_bundle_holder_ = holder;
  }

  void SetLazyBundleLoader(
      const std::shared_ptr<tasm::LazyBundleLoader>& lazy_bundle_loader) {
    lazy_bundle_loader_ = lazy_bundle_loader;
  }

  void SetCSSVariables(
      const std::string& component_id, const std::string& id_selector,
      const lepus::Value& properties,
      std::shared_ptr<tasm::PipelineOptions> pipeline_options) override;

  void SetNativeProps(
      tasm::NodeSelectRoot root, const tasm::NodeSelectOptions& options,
      const lepus::Value& native_props,
      std::shared_ptr<tasm::PipelineOptions> pipeline_options) override;

  void ReloadFromJS(runtime::UpdateDataTask task) override;

  void SetTiming(tasm::Timing timing) override;
  void SetTimingWithTimingFlag(const tasm::timing::TimingFlag& timing_flag,
                               const std::string& timestamp_key,
                               tasm::timing::TimestampUs timestamp) override;
  void SetFrameworkExtraTimingInfo(const std::string& pipeline_id,
                                   const std::string& key,
                                   const std::string& value) override;
  void FlushJSBTiming(piper::NativeModuleInfo timing) override;
  void ResetTimingBeforeReload() override;

  void OnPipelineStart(
      const tasm::PipelineID& pipeline_id,
      const tasm::PipelineOrigin& pipeline_origin,
      tasm::timing::TimestampUs pipeline_start_timestamp) override;

  virtual void BindPipelineIDWithTimingFlag(
      const tasm::PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag) override;

  void AddJSBlockingTime(uint64_t enqueue_time) override;

  // For fiber
  void CallLepusMethod(const std::string& method_name, lepus::Value args,
                       const piper::ApiCallBack& callback) override;

  event::DispatchEventResult DispatchMessageEvent(
      fml::RefPtr<runtime::MessageEvent> event) override;

  std::string LoadJSSource(const std::string& name) override;
  std::shared_ptr<piper::Buffer> LoadBytecode(const std::string& url) override;

  BTSRuntimeMediator(const BTSRuntimeMediator&) = delete;
  BTSRuntimeMediator& operator=(const BTSRuntimeMediator&) = delete;
  BTSRuntimeMediator(BTSRuntimeMediator&&) = delete;
  BTSRuntimeMediator& operator=(BTSRuntimeMediator&&) = delete;

  fml::RefPtr<tasm::PropBundle> CreatePropBundle() override {
    return prop_bundle_creator_->CreatePropBundle();
  }

  void SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator>& creator) override {
    prop_bundle_creator_ = creator;
  }

  void SetWhiteBoardDelegate(const std::shared_ptr<tasm::WhiteBoardDelegate>&
                                 white_board_delegate) override {
    white_board_delegate_ = white_board_delegate;
  }

  void AddEventListenersToWhiteBoard(
      runtime::ContextProxy* js_context_proxy) override;

  void GetSessionStorageItem(const std::string& key,
                             const piper::ApiCallBack& callback) override;

  void SubscribeSessionStorage(const std::string& key, double listener_id,
                               const piper::ApiCallBack& callback) override;
#if ENABLE_TESTBENCH_RECORDER
  void SetRecordID(int64_t record_id) { record_id_ = record_id; }
  int64_t record_id_ = 0;
#endif

  fml::RefPtr<fml::TaskRunner> GetJSRunner() override { return js_runner_; }

 private:
  std::shared_ptr<LynxActor<NativeFacade>> facade_actor_;

  std::shared_ptr<LynxActor<LynxEngine>> engine_actor_;

  std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>
      perf_controller_actor_;

  std::shared_ptr<LynxCardCacheDataManager> card_cached_data_mgr_;

  fml::RefPtr<fml::TaskRunner> js_runner_;

  // for vsync
  std::shared_ptr<runtime::IVSyncObserver> vsync_observer_{nullptr};

  // ExternalResourceLoader will use weak_from_this() internally
  std::shared_ptr<ExternalResourceLoader> external_resource_loader_;
  std::weak_ptr<piper::JsBundleHolder> weak_js_bundle_holder_;
  std::shared_ptr<tasm::LazyBundleLoader> lazy_bundle_loader_;
  std::shared_ptr<tasm::PropBundleCreator> prop_bundle_creator_;
  bool runtime_standalone_mode_;
  std::shared_ptr<tasm::WhiteBoardDelegate> white_board_delegate_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_RUNTIME_BTS_BTS_RUNTIME_MEDIATOR_H_
