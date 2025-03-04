// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/piper/js/lynx_runtime.h"

#include <memory>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/tasm/i18n/i18n.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/piper/js/js_executor.h"
#include "core/runtime/piper/js/lynx_api_handler.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/shell/lynx_runtime_actor_holder.h"
#include "core/template_bundle/template_codec/ttml_constant.h"

#if ENABLE_NAPI_BINDING
#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/napi_loader_js.h"
#endif
#include "core/runtime/bindings/jsi/modules/module_delegate.h"

#if ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/native_module_recorder.h"
#endif

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

// BINARY_KEEP_SOURCE_FILE
namespace lynx {
namespace runtime {

namespace {

class JSIExceptionHandlerImpl : public piper::JSIExceptionHandler {
 public:
  explicit JSIExceptionHandlerImpl(LynxRuntime* runtime) : runtime_(runtime) {}
  ~JSIExceptionHandlerImpl() override = default;

  void onJSIException(const piper::JSIException& exception) override {
    // JSI exception from native should be sent to JSSDK formatting.
    // If there has any JSI exception in this process, those exception will be
    // sent to here too, then sent to JSSDK, then more exception will be
    // thrown... finally you will get endless loop. So we use this flag to avoid
    // endless loop.
    if (is_handling_exception_) {
      return;
    }
    // TODO: Use scoped flag to optimize here to ensure this flag can be reset
    // even if exception thrown during this period.
    is_handling_exception_ = true;
    // avoid call by global runtime and caush dangling pointer...
    if (!destroyed_) {
      runtime_->OnJSIException(exception);
    }
    is_handling_exception_ = false;
  }

  void Destroy() override { destroyed_ = true; }

 private:
  bool destroyed_ = false;
  bool is_handling_exception_ = false;

  LynxRuntime* const runtime_;
};

}  // namespace

thread_local std::string* LynxRuntime::js_core_source_ = nullptr;

LynxRuntime::LynxRuntime(const std::string& group_id, int32_t instance_id,
                         std::unique_ptr<TemplateDelegate> delegate,
                         bool enable_user_bytecode,
                         const std::string& bytecode_source_url,
                         bool enable_js_group_thread)
    : group_id_(group_id),
      instance_id_(instance_id),
      delegate_(std::move(delegate)),
      enable_user_bytecode_(enable_user_bytecode),
      bytecode_source_url_(bytecode_source_url),
      enable_js_group_thread_(enable_js_group_thread) {
  cached_tasks_.reserve(8);
}

LynxRuntime::~LynxRuntime() { Destroy(); }

void LynxRuntime::Init(
    const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& runtime_observer,
    std::shared_ptr<runtime::IRuntimeLifecycleObserver>
        runtime_lifecycle_observer,
    std::vector<std::string> preload_js_paths, bool force_reload_js_core,
    bool force_use_light_weight_js_engine) {
  LOGI("Init LynxRuntime group_id: " << group_id_ << " runtime_id: "
                                     << instance_id_ << " this:" << this);

  tasm::TimingCollector::Scope<TemplateDelegate> scope(delegate_.get());

  if (runtime_lifecycle_observer) {
    runtime_lifecycle_observer_ = runtime_lifecycle_observer;
    runtime_lifecycle_observer_->OnRuntimeInit(instance_id_);
  }

  js_executor_ = std::make_unique<lynx::piper::JSExecutor>(
      std::make_shared<JSIExceptionHandlerImpl>(this), group_id_,
      module_manager, runtime_observer, force_use_light_weight_js_engine);

  auto js_preload_sources =
      LoadPreloadJSSource(std::move(preload_js_paths), force_reload_js_core);

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreStart);
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, "LynxJSLoadCore");
  // FIXME(wangboyong):invoke before decode...in fact in 1.4
  // here NeedGlobalConsole always return true...
  // bool need_console = delegate_->NeedGlobalConsole();
  js_executor_->loadPreJSBundle(js_preload_sources, true, GetRuntimeId(),
                                enable_user_bytecode_, bytecode_source_url_);

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);

  LOGI("js_runtime_type :" << static_cast<int32_t>(
                                  js_executor_->getJSRuntimeType())
                           << " " << this);

#if ENABLE_NAPI_BINDING
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, "Lynx::PrepareNapiEnvironment");
  PrepareNapiEnvironment();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
#endif
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreEnd);

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LynxCreateAndLoadApp");
  app_ = js_executor_->createNativeAppInstance(
      GetRuntimeId(), delegate_.get(), std::make_unique<LynxApiHandler>(this));
  LOGI(" lynxRuntime:" << this << " create APP " << app_.get());
  AddEventListeners();
  UpdateState(State::kJsCoreLoaded);
}

void LynxRuntime::SetJsBundleHolder(
    const std::weak_ptr<piper::JsBundleHolder>& weak_js_bundle_holder) {
  if (app_) {
    app_->SetJsBundleHolder(weak_js_bundle_holder);
  }
}

void LynxRuntime::AdoptRuntimeLifecycleObserver(
    const std::shared_ptr<runtime::IRuntimeLifecycleObserver>&
        runtime_lifecycle_observer) {
  if (runtime_lifecycle_observer) {
    runtime_lifecycle_observer_ = runtime_lifecycle_observer;
    runtime_lifecycle_observer_->OnRuntimeInit(instance_id_);
  }
#if ENABLE_NAPI_BINDING
  RegisterNapiModules();
#endif
}

std::vector<std::pair<std::string, std::string>>
LynxRuntime::LoadPreloadJSSource(std::vector<std::string> preload_js_paths,
                                 bool force_reload_js_core) {
  std::vector<std::pair<std::string, std::string>> js_preload_sources;
  if (!js_core_source_ || js_core_source_->length() <= 0 ||
      force_reload_js_core) {
    delete js_core_source_;
    static constexpr const char* core_js_name = "assets://lynx_core.js";
    js_core_source_ = new std::string(delegate_->LoadJSSource(core_js_name));
    DCHECK(js_core_source_->length() > 0);
    delegate_->OnCoreJSUpdated(*js_core_source_);
  }

  js_preload_sources.emplace_back(kLynxCoreJSName, *js_core_source_);
  for (auto&& path : preload_js_paths) {
    std::string res = delegate_->LoadJSSource(path);
    if (res.length() > 0) {
      js_preload_sources.emplace_back(std::move(path), std::move(res));
    }
  }
  return js_preload_sources;
}

void LynxRuntime::UpdateState(State state) {
  state_ = state;
  switch (state_) {
    case State::kJsCoreLoaded: {
      OnJsCoreLoaded();
      break;
    }
    case State::kSsrRuntimeReady: {
      OnSsrRuntimeReady();
      break;
    }
    case State::kRuntimeReady: {
      TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_VITALS, "TimeToInteractive");
      OnRuntimeReady();
      break;
    }
    default: {
      // TODO
      LOGE("unkown runtime state.");
      break;
    }
  }
}

#if ENABLE_NAPI_BINDING
void LynxRuntime::PrepareNapiEnvironment() {
  napi_environment_ = std::make_unique<piper::NapiEnvironment>(
      std::make_unique<piper::NapiLoaderJS>(std::to_string(instance_id_)));
  auto proxy = piper::NapiRuntimeProxy::Create(GetJSRuntime(), delegate_.get());
  LOGI("napi attaching with proxy: " << proxy.get()
                                     << ", id: " << instance_id_);
  if (proxy) {
    napi_environment_->SetRuntimeProxy(std::move(proxy));
    napi_environment_->Attach();
  }

  RegisterNapiModules();
}

void LynxRuntime::RegisterNapiModules() {
  if (runtime_lifecycle_observer_) {
    LOGI("napi registering module");
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                "RuntimeLifecycleObserver::OnRuntimeAttach");
    runtime_lifecycle_observer_->OnRuntimeAttach(
        napi_environment_->proxy()->Env());
  }
}
#endif

void LynxRuntime::call(base::closure func) { QueueOrExecTask(std::move(func)); }

void LynxRuntime::TryLoadSsrScript(const std::string& ssr_script) {
  if (ssr_script.empty() &&
      (state_ != State::kJsCoreLoaded && state_ != State::kNotStarted &&
       state_ != State::kSsrRuntimeReady)) {
    return;
  }
  auto task = [this, ssr_script = std::move(ssr_script)]() {
    app_->SetupSsrJsEnv();
    app_->LoadSsrScript(ssr_script);
    UpdateState(State::kSsrRuntimeReady);
  };
  if (state_ == State::kSsrRuntimeReady || state_ == State::kJsCoreLoaded) {
    task();
  } else if (state_ == State::kNotStarted) {
    js_core_state_tasks_.emplace_back(std::move(task));
  }
}

void LynxRuntime::OnSsrRuntimeReady() {
  if (state_ != State::kSsrRuntimeReady) {
    return;
  }
  LOGI("lynx ssr runtime ready");
  for (const auto& task : ssr_global_event_cached_tasks_) {
    task();
  }
  ssr_global_event_cached_tasks_.clear();
}

void LynxRuntime::CallJSFunction(const std::string& module_id,
                                 const std::string& method_id,
                                 const lepus::Value& arguments,
                                 bool force_call_despite_app_state) {
  LynxFatal(arguments.IsArrayOrJSArray(), error::E_BTS_RUNTIME_ERROR,
            "the arguments should be array when CallJSFunction!");
  QueueOrExecTask([this, module_id, method_id, arguments,
                   force_call_despite_app_state]() {
    piper::Scope scope(*GetJSRuntime());
    auto array =
        piper::arrayFromLepus(*GetJSRuntime(), *(arguments.Array().get()));
    if (!array) {
      GetJSRuntime()->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("CallJSFunction fail! Reason: Transfer "
                                     "lepus value to js value fail."));
      return;
    }
    CallFunction(module_id, method_id, *array, force_call_despite_app_state);
  });
}

void LynxRuntime::CallJSCallback(
    const std::shared_ptr<piper::ModuleCallback>& callback,
    int64_t id_to_delete) {
  uint64_t callback_thread_switch_end = base::CurrentSystemTimeMilliseconds();
  if (id_to_delete != piper::ModuleCallback::kInvalidCallbackId) {
    callbacks_.erase(id_to_delete);
  }

  if (callback == nullptr) {
    return;
  }
  if (callback->timing_collector_ != nullptr) {
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_thread_switch_end",
        [collector = callback->timing_collector_,
         callback_thread_switch_end](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("first_arg",
                                             collector->GetFirstArg());
          ctx.event()->add_debug_annotations(
              "timestamp", std::to_string(callback_thread_switch_end));
          ctx.event()->add_debug_annotations(
              "jsb_callback_thread_switch",
              std::to_string(callback_thread_switch_end -
                             collector->GetCallbackThreadSwitchStart()));
        });
  }

  auto iterator = callbacks_.find(callback->callback_id());
  if (iterator == callbacks_.end()) {
    if (callback->timing_collector_ != nullptr) {
      callback->timing_collector_->OnErrorOccurred(
          piper::NativeModuleStatusCode::FAILURE);
    }
    return;
  }
  uint64_t callback_call_start_time = base::CurrentSystemTimeMilliseconds();
  if (callback->timing_collector_ != nullptr) {
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_call_start",
        [first_arg = callback->timing_collector_->GetFirstArg(),
         callback_call_start_time](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("first_arg", first_arg);
          ctx.event()->add_debug_annotations(
              "timestamp", std::to_string(callback_call_start_time));
        });
  }
  js_executor_->invokeCallback(callback, &iterator->second);
  callback->ReportLynxErrors(delegate_.get());
  LOGV(
      "LynxModule, LynxRuntime::CallJSCallback did invoke "
      "callback, id: "
      << callback->callback_id());
  callbacks_.erase(iterator);

  if (callback->timing_collector_ != nullptr) {
    callback->timing_collector_->EndCallCallback(callback_thread_switch_end,
                                                 callback_call_start_time);
  }

  if (state_ == State::kDestroying && callbacks_.empty()) {
    shell::LynxRuntimeActorHolder::GetInstance()->Release(
        GetRuntimeId(), enable_js_group_thread_ ? group_id_ : "");
    return;
  }
}

int64_t LynxRuntime::RegisterJSCallbackFunction(piper::Function func) {
  int64_t index = ++callback_id_index_;
  callbacks_.emplace(index, std::move(func));
  return index;
}

void LynxRuntime::CallJSApiCallback(piper::ApiCallBack callback) {
  if (state_ == State::kDestroying) {
    return;
  }
  if (!callback.IsValid()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSApiCallback",
              [&](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("CallbackID");
                debug->set_string_value(std::to_string(callback.id()));
              });
  app_->InvokeApiCallBack(std::move(callback));
}

void LynxRuntime::CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                             const lepus::Value& value,
                                             bool persist) {
  if (state_ == State::kDestroying) {
    return;
  }
  if (!callback.IsValid()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSApiCallbackWithValue",
              [&](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("CallbackID");
                debug->set_string_value(std::to_string(callback.id()));
              });
  app_->InvokeApiCallBackWithValue(std::move(callback), value, persist);
}

void LynxRuntime::CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                             piper::Value value) {
  if (state_ == State::kDestroying) {
    return;
  }
  if (!callback.IsValid()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSApiCallbackWithValue",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations(
                    "callback_id", std::to_string(callback.id()));
              });
  app_->InvokeApiCallBackWithValue(std::move(callback), std::move(value));
}

void LynxRuntime::EraseJSApiCallback(piper::ApiCallBack callback) {
  if (state_ == State::kDestroying) {
    return;
  }

  if (!callback.IsValid()) {
    return;
  }

  app_->EraseApiCallBack(std::move(callback));
}

void LynxRuntime::CallIntersectionObserver(int32_t observer_id,
                                           int32_t callback_id,
                                           piper::Value data) {
  QueueOrExecTask(
      [this, observer_id, callback_id, data = std::move(data)]() mutable {
        app_->OnIntersectionObserverEvent(observer_id, callback_id,
                                          std::move(data));
      });
}

void LynxRuntime::CallFunction(const std::string& module_id,
                               const std::string& method_id,
                               const piper::Array& arguments,
                               bool force_call_despite_app_state) {
  if (state_ == State::kDestroying) {
    return;
  }
#if ENABLE_TESTBENCH_RECORDER
  if (module_id == "GlobalEventEmitter") {
    auto js_runtime = GetJSRuntime();
    auto size = arguments.length(*js_runtime);
    if (size) {
      piper::Value values[*size];
      for (size_t index = 0; index < *size; index++) {
        auto item_opt = arguments.getValueAtIndex(*js_runtime, index);
        if (item_opt) {
          values[index] = std::move(*item_opt);
        }
      }
      tasm::recorder::NativeModuleRecorder::GetInstance().RecordGlobalEvent(
          module_id, method_id, values, *size, js_runtime.get(), record_id_);
    }
  }
#endif
  app_->CallFunction(module_id, method_id, std::move(arguments),
                     force_call_despite_app_state);
}

void LynxRuntime::FlushJSBTiming(piper::NativeModuleInfo timing) {
  delegate_->FlushJSBTiming(std::move(timing));
}

void LynxRuntime::ProcessGlobalEventForSsr(const std::string& name,
                                           const lepus::Value& info) {
  auto infoArray = lepus::CArray::Create();
  infoArray->emplace_back(lepus::Value::ShallowCopy(info));
  SendSsrGlobalEvent(name, lepus::Value(std::move(infoArray)));

  if (info.IsTable()) {
    BASE_STATIC_STRING_DECL(kFromSSRCache, "from_ssr_cache");
    info.Table()->SetValue(kFromSSRCache, true);
  }
}

void LynxRuntime::SendSsrGlobalEvent(const std::string& name,
                                     const lepus::Value& info) {
  if (name.length() <= 0 || state_ == State::kDestroying ||
      state_ == State::kRuntimeReady) {
    return;
  }

  if (state_ == State::kSsrRuntimeReady) {
    app_->SendSsrGlobalEvent(name, info);
  } else {
    ssr_global_event_cached_tasks_.emplace_back(
        [this, name, info] { app_->SendSsrGlobalEvent(name, info); });
  }
}

void LynxRuntime::OnJSSourcePrepared(
    tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
    const std::string& page_name, tasm::PackageInstanceDSL dsl,
    tasm::PackageInstanceBundleModuleMode bundle_module_mode,
    const std::string& url, const tasm::PipelineOptions& pipeline_options) {
  init_global_props_ = global_props;
  if (state_ != State::kJsCoreLoaded && state_ != State::kNotStarted &&
      state_ != State::kSsrRuntimeReady) {
    return;
  }
  auto task = [this, bundle = std::move(bundle), dsl, bundle_module_mode, url,
               pipeline_options]() mutable {
    tasm::timing::LongTaskMonitor::Scope long_task_scope(
        instance_id_, tasm::timing::kLoadJSTask, url);
    tasm::TimingCollector::Scope<TemplateDelegate> scope(delegate_.get(),
                                                         pipeline_options);
    LOGI("lynx runtime loadApp, napi id:" << instance_id_);
    // TODO(huzhanbo): This is needed by Lynx Network now, will be removed
    // after we fully switch to it.
    js_executor_->SetUrl(url);

    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBackgroundStart);
    // We should set enable_circular_data_check flag to js runtime ahead of load
    // app_service.js, so we can check all js data updated if necessary.
    auto js_runtime = js_executor_->GetJSRuntime();
    if (js_runtime) {
      // If devtool is enabled, enable circular data check always.
      bool enable_circular_data_check =
          (bundle.enable_circular_data_check ||
           tasm::LynxEnv::GetInstance().IsDevToolEnabled());
      js_runtime->SetCircularDataCheckFlag(enable_circular_data_check);
      LOGI("[LynxRuntime] circular data check flag: "
           << enable_circular_data_check);
      // set enable_js_binding_api_throw_exception
      js_runtime->SetEnableJsBindingApiThrowException(
          bundle.enable_js_binding_api_throw_exception);
    }
    // bind icu for js env
    if (bundle.enable_bind_icu) {
#if ENABLE_NAPI_BINDING
      Napi::Env env = napi_environment_->proxy()->Env();
      tasm::I18n::Bind(reinterpret_cast<intptr_t>(static_cast<napi_env>(env)));
#endif
    }
    app_->loadApp(std::move(bundle), init_global_props_, dsl,
                  bundle_module_mode, url);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBackgroundEnd);

    UpdateState(State::kRuntimeReady);
  };
  if (state_ == State::kSsrRuntimeReady || state_ == State::kJsCoreLoaded) {
    task();
  } else if (state_ == State::kNotStarted) {
    js_core_state_tasks_.emplace_back(std::move(task));
  }
}

bool LynxRuntime::TryToDestroy() {
  if (state_ == State::kNotStarted) {
    return true;
  }
  state_ = State::kDestroying;

  // Firstly, clear all JSB callbacks that registered before destroy.
  callbacks_.clear();
  cached_tasks_.clear();
  ssr_global_event_cached_tasks_.clear();

  // Destroy app when js_executor_ exists and its runtime is valid, as well as
  // the app_ object exists. These procedures remains the same for Lynx stand
  // alone mode, as the js_executor_ and its runtime must be valid to destroy
  // the app_ object. But in shared context mode, we must check the validity of
  // the JSRuntime in case it is release by its shell owner or other Lynx
  // instance.
  if (js_executor_->GetJSRuntime() && js_executor_->GetJSRuntime()->Valid()) {
    app_->CallDestroyLifetimeFun();
    // After reloading, the old LynxRuntime may be destroyed later than the new
    // LynxRuntime is created, and the inspector-related object
    // InspectorClientNG is a thread-local singleton, in this case, the members
    // it maintaines will be damaged, so that we need to call DestroyInspector()
    // now.
    js_executor_->GetJSRuntime()->DestroyInspector();
  }

  if (callbacks_.empty()) {
    return true;
  } else {
    return false;
  }
}

void LynxRuntime::Destroy() {
  LOGI("LynxRuntime::Destroy, runtime_id: " << instance_id_
                                            << " this: " << this);
  if (state_ == State::kNotStarted) {
    return;
  }
  cached_tasks_.clear();
  ssr_global_event_cached_tasks_.clear();
  callbacks_.clear();
#if ENABLE_NAPI_BINDING
  if (napi_environment_) {
    LOGI("napi detaching runtime, id: " << instance_id_);
    napi_environment_->Detach();
  }
#endif
  if (runtime_lifecycle_observer_) {
    runtime_lifecycle_observer_->OnRuntimeDetach();
    runtime_lifecycle_observer_->OnRuntimeDestroy();
  }
  app_->destroy();
  app_ = nullptr;
  js_executor_->Destroy();
  js_executor_ = nullptr;
}

void LynxRuntime::OnAppReload(tasm::TemplateData data,
                              const tasm::PipelineOptions& pipeline_options) {
  QueueOrExecTask([this, data = std::move(data), pipeline_options]() mutable {
    tasm::TimingCollector::Scope<TemplateDelegate> scope(delegate_.get(),
                                                         pipeline_options);
    // when reloadTemplate, we will use OnAppReload to mock
    // SETUP_LOAD_CORE_START & SETUP_LOAD_CORE_END timing.
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreStart);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreEnd);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBackgroundStart);
    app_->onAppReload(std::move(data));
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBackgroundEnd);
  });
}

void LynxRuntime::EvaluateScript(const std::string& url, std::string script,
                                 piper::ApiCallBack callback) {
  QueueOrExecTask([this, url, script = std::move(script), callback] {
    app_->EvaluateScript(url, std::move(script), callback);
  });
}

void LynxRuntime::EvaluateScriptStandalone(std::string url,
                                           std::string script) {
  LOGI("EvaluateScriptStandalone, url: " << url);
  if (state_ != State::kJsCoreLoaded) {
    delegate_->OnErrorOccurred(base::LynxError(
        error::E_BTS_RUNTIME_ERROR,
        "call evaluateJavaScript on invalid state, will be ignored"));
    return;
  }

  tasm::report::EventTracker::OnEvent(
      [url](tasm::report::MoveOnlyEvent& event) {
        event.SetName("lynxsdk_background_runtime_evaluate_script");
        event.SetProps("script_url", url);
      });

  // We can safely access app_ here. `EvaluateScriptStandalone`
  // can only be used in LynxBackgroundRuntime which will
  // never use pending JS so the app_ is always created.
  app_->OnStandaloneScriptAdded(url, std::move(script));
  app_->loadApp(tasm::TasmRuntimeBundle(), lepus::Value(),
                tasm::PackageInstanceDSL::STANDALONE,
                tasm::PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE,
                std::move(url));
}

void LynxRuntime::ConsoleLogWithLevel(const std::string& level,
                                      const std::string& msg) {
  QueueOrExecTask(
      [this, level, msg] { app_->ConsoleLogWithLevel(level, msg); });
}

void LynxRuntime::I18nResourceChanged(const std::string& msg) {
  QueueOrExecTask([this, msg] { app_->I18nResourceChanged(msg); });
}

void LynxRuntime::NotifyJSUpdatePageData() {
  QueueOrExecTask([this]() mutable { app_->NotifyUpdatePageData(); });
  return;
}

void LynxRuntime::InsertCallbackForDataUpdateFinishedOnRuntime(
    base::closure callback) {
  if (state_ == State::kDestroying) {
    return;
  }
  native_update_finished_callbacks_.emplace_back(std::move(callback));
}

void LynxRuntime::NotifyJSUpdateCardConfigData() {
  if (state_ != State::kRuntimeReady) {
    return;
  }

  app_->NotifyUpdateCardConfigData();
}

void LynxRuntime::OnJsCoreLoaded() {
  if (state_ == State::kDestroying) {
    return;
  }
  for (const auto& task : js_core_state_tasks_) {
    task();
  }
  js_core_state_tasks_.clear();
}

void LynxRuntime::OnRuntimeReady() {
  if (state_ == State::kDestroying) {
    return;
  }

  LOGI("lynx runtime ready");

  delegate_->OnRuntimeReady();

  for (const auto& task : cached_tasks_) {
    task();
  }
  cached_tasks_.clear();
}

void LynxRuntime::AddEventListeners() {
  auto core_context_proxy =
      app_->GetContextProxy(runtime::ContextProxy::Type::kCoreContext);
  core_context_proxy->AddEventListener(
      kMessageEventTypeOnAppEnterForeground,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        if (runtime_lifecycle_observer_) {
          runtime_lifecycle_observer_->OnAppEnterForeground();
        }
      }));
  core_context_proxy->AddEventListener(
      kMessageEventTypeOnAppEnterBackground,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        if (runtime_lifecycle_observer_) {
          runtime_lifecycle_observer_->OnAppEnterBackground();
        }
      }));

  auto js_context_proxy =
      app_->GetContextProxy(runtime::ContextProxy::Type::kJSContext);

  delegate_->AddEventListenersToWhiteBoard(js_context_proxy.get());
}

void LynxRuntime::OnJSIException(const piper::JSIException& exception) {
  if (state_ == State::kDestroying || !app_) {
    if (delegate_) {
      auto error = base::LynxError(
          exception.errorCode(),
          std::string("report js exception directly: ") + exception.message());
      error.AddCallStack(exception.stack());
      delegate_->OnErrorOccurred(std::move(error));
    }
    return;
  }
  // JSI Exception is from native, we should send it to JSSDK. JSSDK will format
  // the error and send it to native for reporting error.
  if (app_) {
    app_->OnAppJSError(exception);
  }
}

// issue: #1510
void LynxRuntime::OnModuleMethodInvoked(const std::string& module,
                                        const std::string& method,
                                        int32_t code) {
  delegate_->OnModuleMethodInvoked(module, method, code);
}

std::shared_ptr<piper::Runtime> LynxRuntime::GetJSRuntime() {
  return js_executor_->GetJSRuntime();
}

int64_t LynxRuntime::GenerateRuntimeId() {
  static std::atomic<int64_t> current_id_;
  return ++current_id_;
}

void LynxRuntime::SetEnableBytecode(bool enable,
                                    const std::string& bytecode_source_url) {
  if (auto rt = GetJSRuntime()) {
    rt->SetEnableUserBytecode(enable);
    rt->SetBytecodeSourceUrl(bytecode_source_url);
  }
}

void LynxRuntime::OnReceiveMessageEvent(runtime::MessageEvent event) {
  if (state_ == State::kDestroying) {
    return;
  }

  if (OnReceiveMessageEventForSSR(event)) {
    return;
  }

  QueueOrExecTask([this, event = std::move(event)]() mutable {
    app_->GetContextProxy(event.GetOriginType())->DispatchEvent(event);
  });
}

void LynxRuntime::OnSetPresetData(lepus::Value data) {
  // We can safely access app_ here. `EvaluateScriptStandalone`
  // can only be used in LynxBackgroundRuntime which will
  // never use pending JS so the app_ is always created.
  app_->OnSetPresetData(std::move(data));
}

void LynxRuntime::OnGlobalPropsUpdated(const lepus::Value& props) {
  // If app is not started, set updated globalProps as init props to reduce
  // updating times
  if (state_ == State::kNotStarted) {
    init_global_props_ = props;
  } else {
    runtime::MessageEvent event(
        runtime::kMessageEventTypeNotifyGlobalPropsUpdated,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, props);
    OnReceiveMessageEvent(std::move(event));
  }
}

void LynxRuntime::OnComponentDecoded(tasm::TasmRuntimeBundle bundle) {
  QueueOrExecTask([this, bundle = std::move(bundle)]() mutable {
    app_->OnComponentDecoded(std::move(bundle));
  });
}

void LynxRuntime::OnCardConfigDataChanged(const lepus::Value& data) {
  QueueOrExecAppTask(
      [this, data]() mutable { app_->OnCardConfigDataChanged(data); });
}

bool LynxRuntime::OnReceiveMessageEventForSSR(
    const runtime::MessageEvent& event) {
  // TODO(liyanbo.monster): refactor state and this.
  // SSR state is different.
  if (event.type() == kMessageEventTypeOnSsrScriptReady) {
    TryLoadSsrScript(event.message().StdString());
    return true;
  }
  if (state_ == State::kSsrRuntimeReady &&
      event.type() == kMessageEventTypeSendGlobalEvent) {
    auto args = event.message();
    if (args.IsArray() && args.Array()->size() == 2) {
      auto args_array = args.Array();
      const auto& name = args_array->get(0).StdString();
      auto params = args_array->get(1);
      // There are two ways to trigger global events, the first one is triggered
      // by native, and the other is triggered by LynxContext. Here we process
      // SSR global events for the first way. Global events from LynxContext are
      // processed in LynxTemplateRender.
      ProcessGlobalEventForSsr(name, params);
    } else {
      // args format is error, abort message dispatch.
      return true;
    }
  }
  return false;
}

void LynxRuntime::QueueOrExecTask(base::closure&& task) {
  if (state_ == State::kDestroying) {
    return;
  }
  if (state_ == State::kRuntimeReady) {
    task();
  } else {
    cached_tasks_.emplace_back(std::move(task));
  }
}

void LynxRuntime::QueueOrExecAppTask(base::closure&& task) {
  if (state_ == State::kDestroying) {
    return;
  }
  if (state_ == State::kNotStarted) {
    js_core_state_tasks_.emplace_back(std::move(task));
  } else {
    task();
  }
}

}  // namespace runtime
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif
