// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_JS_APP_H_
#define CORE_RUNTIME_BINDINGS_JSI_JS_APP_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/log/logging.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/template_entry.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/event/context_proxy_in_js.h"
#include "core/runtime/bindings/jsi/js_task_adapter.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/common/jsi_object_wrapper.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/js_bundle_holder.h"
#include "core/runtime/piper/js/lynx_api_handler.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/services/fluency/fluency_tracer.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "third_party/rapidjson/document.h"

namespace lynx {

namespace runtime {
class LynxRuntime;
class LynxApiHandler;
class AnimationFrameTaskHandler;
}  // namespace runtime
namespace piper {
class Runtime;
class App;
class LynxProxy;

// now this do nothing!
class AppProxy : public HostObject {
 public:
  AppProxy(std::weak_ptr<Runtime> rt, std::weak_ptr<App> app)
      : rt_(rt), native_app_(app) {}
  ~AppProxy() { LOGI("LYNX ~AppProxy destroy"); }

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

 protected:
  std::weak_ptr<Runtime> rt_;
  std::weak_ptr<App> native_app_;
};

class App : public std::enable_shared_from_this<App> {
 public:
  static std::shared_ptr<App> Create(
      int64_t rt_id, std::weak_ptr<Runtime> rt,
      runtime::TemplateDelegate* delegate,
      std::shared_ptr<JSIExceptionHandler> exception_handler,
      piper::Object nativeModuleProxy,
      std::unique_ptr<lynx::runtime::LynxApiHandler> api_handler,
      const std::string& group_id) {
    auto app = std::shared_ptr<App>(new App(
        rt_id, rt, delegate, exception_handler, std::move(nativeModuleProxy),
        std::move(api_handler), group_id));
    app->Init();
    return app;
  }

  ~App() {}
  void destroy();
  void CallDestroyLifetimeFun();

  void setJsAppObj(piper::Object&& obj);
  std::string getAppGUID() { return app_guid_; }

  const std::string& GetPageUrl() const { return url_; }

  // load the app
  void loadApp(tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
               tasm::PackageInstanceDSL dsl,
               tasm::PackageInstanceBundleModuleMode bundle_module_mode,
               const std::string& url);

  void QueryComponent(const std::string& url, ApiCallBack callback,
                      const std::vector<std::string>& ids);

  void AddFont(const lepus::Value& font, ApiCallBack callback);

  // evaluate script from client
  void EvaluateScript(const std::string& url, std::string script,
                      ApiCallBack callback);

  // native call to js
  void onAppReload(tasm::TemplateData init_data);
  void NotifyUpdatePageData();
  void NotifyUpdateCardConfigData();
  void CallFunction(const std::string& module_id, const std::string& method_id,
                    const piper::Array& arguments,
                    bool force_call_despite_app_state = false);
  void SendSsrGlobalEvent(const std::string& name,
                          const lepus::Value& arguments);
  void LoadSsrScript(const std::string& script);
  void SetupSsrJsEnv();
  void InvokeApiCallBack(ApiCallBack id);
  void InvokeApiCallBackWithValue(ApiCallBack id, const lepus::Value& value,
                                  bool persist = false);
  void InvokeApiCallBackWithValue(ApiCallBack id, piper::Value value);
  ApiCallBack CreateCallBack(piper::Function func);
  void EraseApiCallBack(ApiCallBack callback);

  void OnIntersectionObserverEvent(int32_t observer_id, int32_t callback_id,
                                   piper::Value data);

  // component
  void updateComponentData(const std::string& component_id, lepus_value&& data,
                           ApiCallBack callback,
                           runtime::UpdateDataType update_data_type);
  void selectComponent(const std::string& component_id,
                       const std::string& id_selector, const bool single,
                       ApiCallBack callBack);
  void InvokeUIMethod(tasm::NodeSelectRoot root,
                      tasm::NodeSelectOptions options, std::string method,
                      const piper::Value* params, ApiCallBack callback);
  void GetPathInfo(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                   ApiCallBack callBack);
  void GetFields(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                 std::vector<std::string> fields, ApiCallBack call_back);
  void SetNativeProps(tasm::NodeSelectRoot root,
                      tasm::NodeSelectOptions options,
                      lepus::Value native_props);
  void GetSessionStorageItem(const base::String& key,
                             const ApiCallBack& callback);
  void SubscribeSessionStorage(const base::String& key, double listener_id,
                               const ApiCallBack& callback);
  void ElementAnimate(const std::string& component_id,
                      const std::string& id_selector, const lepus::Value& args);
  void triggerComponentEvent(const std::string& event_name, lepus_value&& msg);

  void triggerLepusGlobalEvent(const std::string& event_name,
                               lepus_value&& msg);

  void triggerWorkletFunction(std::string component_id,
                              std::string worklet_module_name,
                              std::string method_name, lepus::Value properties,
                              ApiCallBack callback);

  // js call to native
  void appDataChange(lepus_value&& data, ApiCallBack callback,
                     runtime::UpdateDataType update_data_type);
  std::optional<JSINativeException> batchedUpdateData(const piper::Value& data);

  void OnAppJSError(const piper::JSIException& exception);

  base::expected<Value, JSINativeException> loadScript(
      const std::string entry_name, const std::string& url, long timeout);
  base::expected<Value, JSINativeException> readScript(
      const std::string entry_name, const std::string& url, long timeout);
  piper::Value setTimeout(piper::Function func, int time);
  piper::Value setInterval(piper::Function func, int time);
  void clearTimeout(double task);
  piper::Value nativeModuleProxy();

  void RunOnJSThreadWhenIdle(base::closure closure);

  std::optional<piper::Value> getInitGlobalProps();
  std::optional<piper::Value> getPresetData();
  piper::Value getI18nResource();
  void getContextDataAsync(const std::string& component_id,
                           const std::string& key, ApiCallBack callback);

  void LoadScriptAsync(const std::string& url, ApiCallBack callback);

  void SetCSSVariable(const std::string& component_id,
                      const std::string& id_selector,
                      const lepus::Value& properties);

  void onPiperInvoked(const std::string& module_name,
                      const std::string& method_name);

  void ReloadFromJS(const lepus::Value& value, ApiCallBack callback);

  void ReportException(common::JSErrorInfo error_info);
  void AddReporterCustomInfo(
      const std::unordered_map<std::string, std::string>& info);

  std::shared_ptr<Runtime> GetRuntime();
  std::optional<lepus_value> ParseJSValueToLepusValue(
      const piper::Value& data, const std::string& component_id);
  void ConsoleLogWithLevel(const std::string& level, const std::string& msg);

  void I18nResourceChanged(const std::string& msg);

  bool IsDestroying() { return state_ == State::kDestroying; }

  lepus::Value GetCustomSectionSync(const std::string& key);

  // For fiber
  void CallLepusMethod(const std::string& method_name, lepus::Value args,
                       const ApiCallBack& callback, std::string stacks);
  // TODO(kechenglong): remove this api
  void MarkTimingWithTimingFlag(const std::string& timing_flag,
                                const std::string& key);

  static std::string GenerateDynamicComponentSourceUrl(
      const std::string& entry_name, const std::string& source_url);

  std::shared_ptr<ContextProxyInJS> GetContextProxy(
      runtime::ContextProxy::Type type);

  std::shared_ptr<JSIObjectWrapperManager> jsi_object_wrapper_manager() {
    return jsi_object_wrapper_manager_;
  }

  void PauseGcSuppressionMode();
  void ResumeGcSuppressionMode();

  void OnStandaloneScriptAdded(const std::string& name, std::string source);
  void OnSetPresetData(lepus::Value data);
  // from LynxDataDispatcher
  void OnComponentDecoded(tasm::TasmRuntimeBundle bundle);
  void OnCardConfigDataChanged(const lepus::Value& data);
  JsContent GetJSContent(const std::string& bundle_name,
                         const std::string& name, long timeout);

  // For Runtime Timing API
  void MarkPipelineTiming(const tasm::PipelineID& pipeline_id,
                          const tasm::TimingKey& timing_key);
  void OnPipelineStart(const tasm::PipelineID& pipeline_id,
                       const tasm::PipelineOrigin& pipeline_origin);
  void BindPipelineIDWithTimingFlag(
      const tasm::PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag);

  void SetSourceMapRelease(common::JSErrorInfo error_info);
  std::string GetSourceMapRelease(const std::string url);

  // raf
  piper::Value RequestAnimationFrame(piper::Function func);
  void CancelAnimationFrame(int64_t id);
  void DoFrame(int64_t time_stamp);
  void PauseAnimationFrame();
  void ResumeAnimationFrame();

  void SetJsBundleHolder(const std::weak_ptr<piper::JsBundleHolder>& holder);

  void QueueMicrotask(piper::Function func);

 private:
  App(int64_t rt_id, std::weak_ptr<Runtime> rt,
      runtime::TemplateDelegate* delegate,
      std::shared_ptr<JSIExceptionHandler> exception_handler,
      piper::Object nativeModuleProxy,
      std::unique_ptr<lynx::runtime::LynxApiHandler> api_handler,
      const std::string& group_id)
      : app_guid_(std::to_string(rt_id)),
        rt_(rt),
        js_app_(),
        delegate_(delegate),
        exception_handler_(exception_handler),
        js_task_adapter_(std::make_shared<JsTaskAdapter>(rt, group_id)),
        nativeModuleProxy_(std::move(nativeModuleProxy)),
        api_handler_(std::move(api_handler)),
        jsi_object_wrapper_manager_(
            std::make_shared<JSIObjectWrapperManager>()),
        app_dsl_(tasm::PackageInstanceDSL::TT),
        bundle_module_mode_(
            tasm::PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE),
        animation_frame_handler_(
            std::make_unique<runtime::AnimationFrameTaskHandler>()) {}

  void Init();
  std::optional<Value> SendPageEvent(const std::string& page_name,
                                     const std::string& handler,
                                     const lepus::Value& info);
  void CallJSFunctionInLepusEvent(const std::string& component_id,
                                  const std::string& name,
                                  const lepus::Value& params);
  void SendGlobalEvent(const std::string& name, const lepus::Value& arguments);
  std::optional<Value> PublishComponentEvent(const std::string& component_id,
                                             const std::string& handler,
                                             const lepus::Value& info);

  enum class State {
    kNotStarted,     // only app created
    kStarted,        // app started loadApp
    kAppLoaded,      // app has been loaded successfully
    kAppLoadFailed,  // app load failed
    kDestroying,     // app is destroying
  };
  State state_ = State::kNotStarted;

  std::string app_guid_;
  std::weak_ptr<Runtime> rt_;
  std::string i18_resource_;
  piper::Value js_app_;
  runtime::TemplateDelegate* const delegate_;
  std::shared_ptr<JSIExceptionHandler> exception_handler_;
  // Ownered js_task_adapter_
  std::shared_ptr<piper::JsTaskAdapter> js_task_adapter_;
  piper::Object nativeModuleProxy_;
  ApiCallBackManager api_callback_manager_;
  std::unique_ptr<lynx::runtime::LynxApiHandler> api_handler_;
  std::shared_ptr<JSIObjectWrapperManager> jsi_object_wrapper_manager_;
  tasm::PackageInstanceDSL app_dsl_;
  tasm::PackageInstanceBundleModuleMode bundle_module_mode_;
  std::shared_ptr<LynxProxy> lynx_proxy_;
  std::string url_;
  piper::Value ssr_global_event_emitter_;
  std::unique_ptr<GCPauseSuppressionMode> gc_pause_suppression_mode_{nullptr};

  std::shared_ptr<ContextProxyInJS> context_proxy_vector_[static_cast<int32_t>(
      runtime::ContextProxy::Type::kUnknown)] = {nullptr};

  tasm::TasmRuntimeBundle card_bundle_;
  std::unordered_map<std::string, tasm::TasmRuntimeBundle> component_bundles_;
  std::unordered_map<std::string, JsBundle> js_bundles_;
  lepus::Value card_config_;  // cache the init card config data
  lepus::Value init_global_props_;
  JsBundle standalone_js_bundle_;
  std::weak_ptr<piper::JsBundleHolder> weak_js_bundle_holder_;

  // This is set by LynxRuntimeStandalone, once set, it cannot be modified.
  lepus::Value preset_data_;

  common::JSErrorReporter js_error_reporter_;

  bool IsJsAppStateValid() {
    return (js_app_.isObject() && state_ != State::kAppLoadFailed);
  }

  void handleLoadAppFailed(std::string error_msg);

  std::unique_ptr<runtime::AnimationFrameTaskHandler> animation_frame_handler_;
  bool has_paused_animation_frame_{false};
  lynx::tasm::FluencyTracer fluency_tracer_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_JS_APP_H_
