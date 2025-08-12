// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_LYNX_RUNTIME_H_
#define CORE_RUNTIME_PIPER_JS_LYNX_RUNTIME_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/debug/lynx_assert.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/vector.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/public/page_options.h"
#include "core/public/prop_bundle.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_timing.h"
#include "core/runtime/piper/js/js_bundle_holder.h"
#include "core/runtime/piper/js/js_executor.h"
#include "core/runtime/piper/js/lynx_api_handler.h"
#include "core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"
#include "core/runtime/piper/js/runtime_lifecycle_observer_impl.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/runtime/vm/lepus/lepus_global.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace piper {
class NapiEnvironment;
}

namespace runtime {

enum LynxRuntimeFlags : uint32_t {
  INIT = 0,
  ENABLE_USER_BYTECODE = 1 << 0,
  ENABLE_JS_GROUP_THREAD = 1 << 1,
  FORCE_RELOAD_CORE_JS = 1 << 2,
  FORCE_USE_LIGHT_WEIGHT_JS_ENGINE = 1 << 3,
  PENDING_CORE_JS_LOAD = 1 << 4,
  PENDING_JS_TASK = 1 << 5,
};

void SetRuntimeFlags(uint32_t& flags, bool enable,
                     lynx::runtime::LynxRuntimeFlags flag);
uint32_t CalcRuntimeFlags(bool force_reload_js_core, bool use_quickjs_engine,
                          bool pending_js_task, bool enable_user_bytecode,
                          bool* enable_js_group_thread = nullptr,
                          bool* pending_core_js_load = nullptr);

/*
 * now only run on js thread
 */
class LynxRuntime final {
 public:
  LynxRuntime(const std::string& group_id, int32_t instance_id,
              std::unique_ptr<TemplateDelegate> delegate,
              const std::string& bytecode_source_url, uint32_t runtime_flags,
              const tasm::PageOptions& page_options);
  ~LynxRuntime();
  LynxRuntime(const LynxRuntime&) = delete;
  LynxRuntime& operator=(const LynxRuntime&) = delete;

  // now can ensure Init the first task for LynxRuntime
  void Init(
      const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
      const std::shared_ptr<piper::InspectorRuntimeObserverNG>&
          runtime_observer,
      std::vector<std::string> preload_js_paths);

  void CallJSCallback(const std::shared_ptr<piper::ModuleCallback>& callback,
                      int64_t id_to_delete);
  void CallJSApiCallback(piper::ApiCallBack callback);
  void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                  const lepus::Value& value,
                                  bool persist = false);
  void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                  piper::Value value);
  void EraseJSApiCallback(piper::ApiCallBack callback);
  int64_t RegisterJSCallbackFunction(piper::Function func);

  void CallJSFunction(const std::string& module_id,
                      const std::string& method_id,
                      const lepus::Value& arguments);

  void CallIntersectionObserver(int32_t observer_id, int32_t callback_id,
                                piper::Value data);

  void CallFunction(const std::string& module_id, const std::string& method_id,
                    const piper::Array& arguments);
  void FlushJSBTiming(piper::NativeModuleInfo timing);
  void SendSsrGlobalEvent(const std::string& name, const lepus::Value& info);
  void OnJSSourcePrepared(
      tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
      const std::string& page_name, tasm::PackageInstanceDSL dsl,
      tasm::PackageInstanceBundleModuleMode bundle_module_mode,
      const std::string& url,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options);
  void OnGlobalPropsUpdated(const lepus::Value& props);

  void call(base::closure func);
  void OnAppReload(
      tasm::TemplateData data,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options);
  void EvaluateScript(const std::string& url, std::string script,
                      piper::ApiCallBack callback);
  void EvaluateScriptStandalone(std::string url, std::string script);

  void OnScriptLoaded(const std::string& url, std::string script,
                      std::string err_msg, piper::ApiCallBack callback);

  void NotifyJSUpdatePageData(uint64_t trace_flow_id);
  void NotifyJSUpdateCardConfigData();
  void InsertCallbackForDataUpdateFinishedOnRuntime(base::closure callback);

  void OnJSIException(const piper::JSIException& exception);

  void OnErrorOccurred(base::LynxError error) {
    delegate_->OnErrorOccurred(std::move(error));
  }

  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t error_code);

  std::shared_ptr<piper::Runtime> GetJSRuntime();
  int64_t GetRuntimeId() const { return instance_id_; }

#if ENABLE_NAPI_BINDING
  piper::NapiEnvironment* GetNapiEnvironment() const {
    return napi_environment_.get();
  }
#endif

#if ENABLE_TESTBENCH_RECORDER
  void SetRecordId(int64_t record_id) { record_id_ = record_id; }
#endif

  void I18nResourceChanged(const std::string& msg);

  bool TryToDestroy();

  std::shared_ptr<runtime::IVSyncObserver> GetVSyncObserver() {
    return delegate_->GetVSyncObserver();
  }

  void AddLifecycleListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener);

  void SetEnableBytecode(bool enable, const std::string& bytecode_source_url);

  void SetPageOptions(const tasm::PageOptions& page_options);
  const tasm::PageOptions& GetPageOptions() { return page_options_; }

  void OnReceiveMessageEvent(runtime::MessageEvent event);

  void OnSetPresetData(lepus::Value data);
  void OnComponentDecoded(tasm::TasmRuntimeBundle bundle);
  void OnCardConfigDataChanged(const lepus::Value& data);

  // Don't store this raw ptr
  // Used in LynxShell::AttachRuntime only
  runtime::TemplateDelegate* GetDelegate() { return delegate_.get(); }

  bool IsRuntimeReady() { return state_ == State::kRuntimeReady; }

  void SetJsBundleHolder(
      const std::weak_ptr<piper::JsBundleHolder>& weak_js_bundle_holder);

  void TransitionToFullRuntime();

  void AddModuleFactory(
      std::unique_ptr<piper::NativeModuleFactory> native_factory);

  void OnRuntimeActorCreate();

 private:
  enum class State {
    kNotStarted,       // only LynxRuntime created
    kJsCoreLoaded,     // js core is loaded
    kRuntimeReady,     // js runtime is ready
    kDestroying,       // js runtime is destroying
    KPending,          // js instance changed
    kSsrRuntimeReady,  // SSR scripts is evaluated, this state will not exist
                       // when CSR.
  };

  void Destroy();
  void DestroyAppAndNapi(bool destroy);
  void ReadPreloadJSSource(
      std::vector<std::string> preload_js_paths,
      std::vector<std::pair<std::string, std::string>>& ret);
  void ReadCoreJS(std::vector<std::pair<std::string, std::string>>& ret);
  void InitPartRuntime(std::vector<std::string> preload_js_paths);
  void InitFullRuntime(std::vector<std::string> preload_js_paths);
  void InitExecutor(
      std::vector<std::pair<std::string, std::string>> preload_js_sources);
  void UpdateState(State state);
  void OnRuntimeReady();
  void OnSsrRuntimeReady();
  void TryLoadSsrScript(const std::string& ssr_script);
  void OnJsCoreLoaded();
  void ProcessGlobalEventForSsr(const std::string& name,
                                const lepus::Value& info);

  void AddEventListeners();

  // return true means abort message dispatch.
  bool OnReceiveMessageEventForSSR(const runtime::MessageEvent& event);
  void QueueOrExecTask(base::closure&& task);
  void QueueOrExecAppTask(base::closure&& task);

#if ENABLE_NAPI_BINDING
  void PrepareNapiEnvironment();
  void PrepareRestrictedNapiEnvironment();
  void RegisterNapiModules();
#endif

  static int64_t GenerateRuntimeId();

  const std::string group_id_;
  const int32_t instance_id_;
  const std::unique_ptr<runtime::TemplateDelegate> delegate_;
  std::unique_ptr<lynx::piper::JSExecutor> js_executor_;
  std::shared_ptr<piper::App> app_;
#if ENABLE_NAPI_BINDING
  std::unique_ptr<piper::NapiEnvironment> napi_environment_;
  std::unique_ptr<piper::NapiEnvironment> napi_restricted_environment_;
#else
  std::unique_ptr<bool> napi_environment_placeholder_;
#endif

#if ENABLE_TESTBENCH_RECORDER
  int64_t record_id_ = 0;
#endif

  // Once the dynamic delivery of core.js is complete, lynx_thread_local should
  // be reset.
  static thread_local std::string* js_core_source_;

  State state_ = State::kNotStarted;

  // store tasks will run after runtime ready
  std::vector<base::closure> cached_tasks_;
  std::vector<base::closure> js_core_state_tasks_;

  std::vector<base::closure> ssr_global_event_cached_tasks_;

  // store callbacks that the data has been updated for runtime.
  std::vector<base::closure> native_update_finished_callbacks_;

  std::unordered_map<int64_t, piper::ModuleCallbackFunctionHolder> callbacks_;
  int64_t callback_id_index_ = 0;
  std::string bytecode_source_url_;
  uint32_t runtime_flags_;
  std::unique_ptr<RuntimeLifecycleObserverImpl> lifecycle_observer_;
  tasm::PageOptions page_options_;
  lepus::Value init_global_props_;
  base::InlineVector<std::unique_ptr<piper::NativeModuleFactory>, 4>
      cached_native_factories_;
  bool destroy_js_app_early_{false};
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_LYNX_RUNTIME_H_
