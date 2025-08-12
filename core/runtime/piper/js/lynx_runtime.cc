// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/piper/js/lynx_runtime.h"

#include <memory>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
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
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/shell/lynx_runtime_actor_holder.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "core/value_wrapper/value_impl_piper.h"

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

constexpr uint32_t LynxRuntimeFlagsMask = 0xFFFFFFFF;

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

void SetRuntimeFlags(uint32_t& flags, bool enable,
                     lynx::runtime::LynxRuntimeFlags flag) {
  if (enable) {
    flags |= flag;
  } else {
    flags &= ~flag;
  }
}

uint32_t CalcRuntimeFlags(bool force_reload_core_js, bool use_quickjs_engine,
                          bool pending_js_task, bool enable_user_bytecode,
                          bool* enable_js_group_thread,
                          bool* pending_core_js_load) {
  uint32_t flags = lynx::runtime::LynxRuntimeFlags::INIT;
  SetRuntimeFlags(flags, force_reload_core_js,
                  lynx::runtime::LynxRuntimeFlags::FORCE_RELOAD_CORE_JS);
  SetRuntimeFlags(
      flags, use_quickjs_engine,
      lynx::runtime::LynxRuntimeFlags::FORCE_USE_LIGHT_WEIGHT_JS_ENGINE);
  SetRuntimeFlags(flags, pending_js_task,
                  lynx::runtime::LynxRuntimeFlags::PENDING_JS_TASK);
  SetRuntimeFlags(flags, enable_user_bytecode,
                  lynx::runtime::LynxRuntimeFlags::ENABLE_USER_BYTECODE);
  if (enable_js_group_thread != nullptr) {
    SetRuntimeFlags(flags, *enable_js_group_thread,
                    lynx::runtime::LynxRuntimeFlags::ENABLE_JS_GROUP_THREAD);
  }
  if (pending_core_js_load != nullptr) {
    SetRuntimeFlags(flags, *pending_core_js_load,
                    lynx::runtime::LynxRuntimeFlags::PENDING_CORE_JS_LOAD);
  }
  return flags;
}

LynxRuntime::LynxRuntime(const std::string& group_id, int32_t instance_id,
                         std::unique_ptr<TemplateDelegate> delegate,
                         const std::string& bytecode_source_url,
                         uint32_t runtime_flags,
                         const tasm::PageOptions& page_options)
    : group_id_(group_id),
      instance_id_(instance_id),
      delegate_(std::move(delegate)),
      bytecode_source_url_(bytecode_source_url),
      runtime_flags_(runtime_flags & LynxRuntimeFlagsMask),
      lifecycle_observer_(std::make_unique<RuntimeLifecycleObserverImpl>()),
      page_options_(page_options) {
  cached_tasks_.reserve(8);
}

LynxRuntime::~LynxRuntime() { Destroy(); }

void LynxRuntime::Init(
    const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& runtime_observer,
    std::vector<std::string> preload_js_paths) {
  LOGI("Init LynxRuntime group_id: " << group_id_ << " runtime_id: "
                                     << GetRuntimeId() << " this:" << this);

  tasm::TimingCollector::Scope<TemplateDelegate> scope(delegate_.get());
  lifecycle_observer_->OnRuntimeInit(GetRuntimeId());

  if (module_manager && cached_native_factories_.size() > 0) {
    for (auto&& module : cached_native_factories_) {
      module_manager->SetModuleFactory(std::move(module));
    }
    cached_native_factories_.clear();
  }
  js_executor_ = std::make_unique<lynx::piper::JSExecutor>(
      std::make_shared<JSIExceptionHandlerImpl>(this), group_id_,
      module_manager, runtime_observer,
      runtime_flags_ & LynxRuntimeFlags::FORCE_USE_LIGHT_WEIGHT_JS_ENGINE);

  if (runtime_flags_ & LynxRuntimeFlags::PENDING_CORE_JS_LOAD) {
    InitPartRuntime(std::move(preload_js_paths));
  } else {
    InitFullRuntime(std::move(preload_js_paths));
  }
  LOGI("js_runtime_type :" << static_cast<int32_t>(
                                  js_executor_->getJSRuntimeType())
                           << " " << this);

#if ENABLE_NAPI_BINDING
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, PREPARE_NAPI_ENV);
  PrepareNapiEnvironment();
  PrepareRestrictedNapiEnvironment();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
#endif

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, CREATE_AND_LOAD_APP);
  app_ = js_executor_->createNativeAppInstance(
      GetRuntimeId(), delegate_.get(), std::make_unique<LynxApiHandler>(this),
      page_options_);
  LOGI(" lynxRuntime:" << this << " create APP " << app_.get());
  AddEventListeners();

  if ((runtime_flags_ & LynxRuntimeFlags::PENDING_CORE_JS_LOAD) == 0) {
    UpdateState(State::kJsCoreLoaded);
  }
}

void LynxRuntime::InitFullRuntime(std::vector<std::string> preload_js_paths) {
  std::vector<std::pair<std::string, std::string>> preload_js_sources;
  // read lynx_core.js
  ReadCoreJS(preload_js_sources);
  // read preload js
  ReadPreloadJSSource(std::move(preload_js_paths), preload_js_sources);
  // init jsvm runtime
  InitExecutor(std::move(preload_js_sources));
}

void LynxRuntime::InitPartRuntime(std::vector<std::string> preload_js_paths) {
  std::vector<std::pair<std::string, std::string>> preload_js_sources;
  ReadPreloadJSSource(std::move(preload_js_paths), preload_js_sources);
  InitExecutor(std::move(preload_js_sources));
}

void LynxRuntime::InitExecutor(
    std::vector<std::pair<std::string, std::string>> preload_js_sources) {
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreStart);
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, LYNX_JS_LOAD_CORE);
  // FIXME(wangboyong):invoke before decode...in fact in 1.4
  // here NeedGlobalConsole always return true...
  // bool need_console = delegate_->NeedGlobalConsole();
  js_executor_->loadPreJSBundle(
      preload_js_sources, true, GetRuntimeId(),
      runtime_flags_ & LynxRuntimeFlags::ENABLE_USER_BYTECODE,
      bytecode_source_url_,
      [delegate_ptr = delegate_.get()](const std::string& url) {
        return delegate_ptr->LoadBytecode(url);
      });
  js_executor_->SetObserver(delegate_.get());

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreEnd);
}

void LynxRuntime::TransitionToFullRuntime() {
  if ((runtime_flags_ & LynxRuntimeFlags::PENDING_CORE_JS_LOAD) == 0) {
    return;
  }
  runtime_flags_ &= ~LynxRuntimeFlags::PENDING_CORE_JS_LOAD;
  auto rt = GetJSRuntime();
  if (!rt) {
    return;
  }
  std::vector<std::pair<std::string, std::string>> preload_js_sources;
  ReadCoreJS(preload_js_sources);
  // load the lynx_core.js
  piper::Scope scope(*rt);
  for (auto& [url, source] : preload_js_sources) {
    auto buffer = std::make_shared<piper::StringBuffer>(source);
    auto prep = rt->prepareJavaScript(buffer, url);
    auto ret = rt->evaluatePreparedJavaScript(prep);
    if (!ret.has_value()) {
      rt->reportJSIException(ret.error());
    }
  }
  UpdateState(State::kJsCoreLoaded);
}

void LynxRuntime::SetJsBundleHolder(
    const std::weak_ptr<piper::JsBundleHolder>& weak_js_bundle_holder) {
  if (app_) {
    app_->SetJsBundleHolder(weak_js_bundle_holder);
  }
}

void LynxRuntime::ReadPreloadJSSource(
    std::vector<std::string> preload_js_paths,
    std::vector<std::pair<std::string, std::string>>& ret) {
  for (auto&& path : preload_js_paths) {
    std::string res = delegate_->LoadJSSource(path);
    if (res.length() > 0) {
      ret.emplace_back(std::move(path), std::move(res));
    }
  }
}

void LynxRuntime::ReadCoreJS(
    std::vector<std::pair<std::string, std::string>>& ret) {
  if (!js_core_source_ || js_core_source_->length() <= 0 ||
      (runtime_flags_ & LynxRuntimeFlags::FORCE_RELOAD_CORE_JS)) {
    delete js_core_source_;
    static constexpr const char* core_js_name = "assets://lynx_core.js";
    js_core_source_ = new std::string(delegate_->LoadJSSource(core_js_name));
    DCHECK(js_core_source_->length() > 0);
    delegate_->OnCoreJSUpdated(*js_core_source_);
  }
  ret.emplace_back(kLynxCoreJSName, *js_core_source_);
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
      TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_VITALS, TIME_TO_INTERACTIVE);
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
      std::make_unique<piper::NapiLoaderJS>(std::to_string(GetRuntimeId())));
  auto proxy = piper::NapiRuntimeProxy::Create(GetJSRuntime(), delegate_.get());
  LOGI("napi attaching with proxy: " << proxy.get()
                                     << ", id: " << GetRuntimeId());
  if (proxy) {
    napi_environment_->SetRuntimeProxy(std::move(proxy));
    napi_environment_->Attach();
  }

  RegisterNapiModules();
}

// What is a restricted napi environment?
// A restricted napi environment refers to a NAPI environment where some
// interfaces are not allowed to be called (e.g., napi_run_script,
// napi_get_global). The purpose of this is to prevent users from
// inappropriately using these interfaces in their custom modules, which could
// lead to instability in the Lynx script runtime (especially in the case of
// multiple pages sharing a js context). These disabled APIs are not essential
// for users to implement custom modules. Therefore, users can still provide
// complete native capabilities to JS in their custom modules.
void LynxRuntime::PrepareRestrictedNapiEnvironment() {
  // Create a restricted environment with an dummy delegate.
  napi_restricted_environment_ = std::make_unique<piper::NapiEnvironment>(
      std::make_unique<piper::NapiEnvironment::Delegate>());
  auto proxy = std::make_unique<piper::RestrictedNapiRuntimeProxyDecorator>(
      piper::NapiRuntimeProxy::Create(GetJSRuntime(), delegate_.get()));
  LOGI("napi attaching with restricted proxy: " << proxy.get()
                                                << ", id: " << GetRuntimeId());
  if (proxy) {
    napi_restricted_environment_->SetRuntimeProxy(std::move(proxy));
    napi_restricted_environment_->Attach();
  }
}

void LynxRuntime::RegisterNapiModules() {
  LOGI("napi registering module");
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
              RUNTIME_LIFECYCLE_OBSERVER_RUNTIME_ATTACH);
  lifecycle_observer_->OnRuntimeAttach(napi_environment_->proxy()->Env());
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
                                 const lepus::Value& arguments) {
  LynxFatal(arguments.IsArrayOrJSArray(), error::E_BTS_RUNTIME_ERROR,
            "the arguments should be array when CallJSFunction!");
  QueueOrExecTask([this, module_id, method_id, arguments]() {
    auto js_runtime = GetJSRuntime();
    if (!js_runtime) {
      LOGE("js_runtime is nullptr!");
      return;
    }
    piper::Scope scope(*js_runtime);
    auto array = piper::arrayFromLepus(*js_runtime, *(arguments.Array().get()));
    if (!array) {
      js_runtime->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("CallJSFunction fail! Reason: Transfer "
                                     "lepus value to js value fail."));
      return;
    }
    CallFunction(module_id, method_id, *array);
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
        LYNX_TRACE_CATEGORY_JSB, JSB_TIMING_CALLBACK_THREAD_SWITCH_END,
        [collector = callback->timing_collector_,
         callback_thread_switch_end](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations(
              "timestamp", std::to_string(callback_thread_switch_end));
          ctx.event()->add_debug_annotations(
              "jsb_callback_thread_switch",
              std::to_string(callback_thread_switch_end -
                             collector->GetCallbackThreadSwitchStart()));
          ctx.event()->add_flow_ids(collector->FlowId());
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
        LYNX_TRACE_CATEGORY_JSB, JSB_TIMING_CALLBACK_CALL_START,
        [callback_call_start_time,
         timing_collector =
             callback->timing_collector_](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations(
              "timestamp", std::to_string(callback_call_start_time));
          if (timing_collector != nullptr) {
            ctx.event()->add_flow_ids(timing_collector->FlowId());
          }
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
        GetRuntimeId(),
        (runtime_flags_ & LynxRuntimeFlags::ENABLE_JS_GROUP_THREAD) ? group_id_
                                                                    : "");
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

  TRACE_EVENT(LYNX_TRACE_CATEGORY, RUNTIME_CALL_JS_API_CALLBACK,
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

  TRACE_EVENT(LYNX_TRACE_CATEGORY, RUNTIME_CALL_JS_API_CALLBACK_WITH_VALUE,
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

  TRACE_EVENT(LYNX_TRACE_CATEGORY, RUNTIME_CALL_JS_API_CALLBACK_WITH_VALUE,
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
                               const piper::Array& arguments) {
  if (state_ == State::kDestroying) {
    return;
  }
  auto js_runtime = GetJSRuntime();
  if (!js_runtime) {
    LOGW("js_runtime is nullptr!");
    return;
  }
#if ENABLE_TESTBENCH_RECORDER
  if (module_id == "GlobalEventEmitter") {
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
  auto native_context_proxy =
      app_->GetContextProxy(runtime::ContextProxy::Type::kNative);
  if (native_context_proxy != nullptr &&
      native_context_proxy->HasEventListener(kMessageEventTypeGlobalEvent) &&
      module_id == "GlobalEventEmitter" && method_id == "emit") {
    MessageEvent jsContextEvent(
        kMessageEventTypeGlobalEvent, ContextProxy::Type::kNative,
        ContextProxy::Type::kJSContext,
        std::make_unique<pub::ValueImplPiper>(
            *js_runtime, piper::Value(*js_runtime, arguments)));
    native_context_proxy->DispatchEvent(jsContextEvent);
    MessageEvent coreContextEvent(
        kMessageEventTypeGlobalEvent, ContextProxy::Type::kNative,
        ContextProxy::Type::kCoreContext,
        std::make_unique<pub::ValueImplLepus>(*app_->ParseJSValueToLepusValue(
            piper::Value(*js_runtime, std::move(arguments)), PAGE_GROUP_ID)));
    delegate_->DispatchMessageEvent(std::move(coreContextEvent));
    return;
  }
  app_->CallFunction(module_id, method_id, std::move(arguments));
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
    const std::string& url,
    const std::shared_ptr<tasm::PipelineOptions>& pipeline_options) {
  init_global_props_ = global_props;
  if (state_ != State::kJsCoreLoaded && state_ != State::kNotStarted &&
      state_ != State::kSsrRuntimeReady) {
    return;
  }
  auto task = [this, bundle = std::move(bundle), dsl, bundle_module_mode, url,
               pipeline_options]() mutable {
    tasm::timing::LongTaskMonitor::Scope long_task_scope(
        page_options_, tasm::timing::kLoadJSTask, url);
    tasm::TimingCollector::Scope<TemplateDelegate> scope(delegate_.get(),
                                                         pipeline_options);
    LOGI("lynx runtime loadApp, napi id:" << GetRuntimeId());
    // TODO(huzhanbo): This is needed by Lynx Network now, will be removed
    // after we fully switch to it.
    js_executor_->SetUrl(url);

    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBackgroundStart);
    // We should set enable_circular_data_check flag to js runtime ahead of load
    // app_service.js, so we can check all js data updated if necessary.
    auto js_runtime = GetJSRuntime();
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
      if (group_id_ != PAGE_GROUP_ID) {
        delegate_->OnErrorOccurred(base::LynxError(
            error::E_BTS_RUNTIME_ERROR,
            "enable_bind_icu is not supported in shared context env"));
      } else {
        Napi::Env env = napi_environment_->proxy()->Env();
        tasm::I18n::Bind(
            reinterpret_cast<intptr_t>(static_cast<napi_env>(env)));
      }
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
  auto js_runtime = GetJSRuntime();
  if (js_runtime && js_runtime->Valid()) {
    auto native_context_proxy =
        app_->GetContextProxy(runtime::ContextProxy::Type::kNative);
    if (native_context_proxy != nullptr &&
        native_context_proxy->HasEventListener(
            kMessageEventTypeDestroyLifetime)) {
      MessageEvent jsContextEvent(
          kMessageEventTypeDestroyLifetime, ContextProxy::Type::kNative,
          ContextProxy::Type::kJSContext,
          std::make_unique<pub::ValueImplPiper>(
              *js_runtime,
              piper::Value(*js_runtime, piper::String::createFromUtf8(
                                            *js_runtime, app_->getAppGUID()))));
      native_context_proxy->DispatchEvent(jsContextEvent);
    } else {
      app_->CallDestroyLifetimeFun();
    }
    // clear bytecode getter
    js_runtime->SetBytecodeGetter(nullptr);
    // After reloading, the old LynxRuntime may be destroyed later than the new
    // LynxRuntime is created, and the inspector-related object
    // InspectorClientNG is a thread-local singleton, in this case, the members
    // it maintaines will be damaged, so that we need to call DestroyInspector()
    // now.
    js_runtime->DestroyInspector();
  }

  destroy_js_app_early_ = tasm::LynxEnv::GetInstance().GetBoolEnv(
      tasm::LynxEnv::Key::OPT_DESTROY_JS_APP_EARLY, true);
  DestroyAppAndNapi(destroy_js_app_early_);

  // if opt_avoid_destroy_runtime_wait is true, destroy LynxRuntime directly.
  if (tasm::LynxEnv::GetInstance().GetBoolEnv(
          tasm::LynxEnv::Key::OPT_AVOID_DESTROY_RUNTIME_WAIT, true)) {
    callbacks_.clear();
    return true;
  }

  if (callbacks_.empty()) {
    return true;
  } else {
    return false;
  }
}

void LynxRuntime::Destroy() {
  LOGI("LynxRuntime::Destroy, runtime_id: " << GetRuntimeId()
                                            << " this: " << this);
  if (state_ == State::kNotStarted) {
    return;
  }
  cached_tasks_.clear();
  ssr_global_event_cached_tasks_.clear();
  callbacks_.clear();
  DestroyAppAndNapi(!destroy_js_app_early_);

  js_executor_->SetObserver(nullptr);
  js_executor_->Destroy();
  js_executor_ = nullptr;
}

void LynxRuntime::DestroyAppAndNapi(bool destroy) {
  if (destroy) {
    LOGI("LynxRuntime::DestroyAppAndNapi, runtime_id: " << GetRuntimeId()
                                                        << " this: " << this);
    // App destroy might invoke front-page's destroy, which could call a NAPI
    // API, so it's important to call destroy first, and then call NAPI destroy.
    app_->destroy();
    app_ = nullptr;
#if ENABLE_NAPI_BINDING
    if (napi_environment_) {
      LOGI("napi detaching runtime, id: " << GetRuntimeId());
      napi_environment_->Detach();
      napi_environment_.reset();
    }
    if (napi_restricted_environment_) {
      LOGI("restricted napi detaching runtime, id: " << GetRuntimeId());
      napi_restricted_environment_->Detach();
      napi_restricted_environment_.reset();
    }
#endif
    lifecycle_observer_->OnRuntimeDetach();
  }
}

void LynxRuntime::OnAppReload(
    tasm::TemplateData data,
    const std::shared_ptr<tasm::PipelineOptions>& pipeline_options) {
  QueueOrExecTask([this, data = std::move(data), pipeline_options]() mutable {
    tasm::TimingCollector::Scope<TemplateDelegate> scope(delegate_.get(),
                                                         pipeline_options);
    // when reloadTemplate, we will use OnAppReload to mock
    // SETUP_LOAD_CORE_START & SETUP_LOAD_CORE_END timing.
    // we already have ReloadBundleEntry now, so we mark ReloadBackground here.
    // and we will remove the LoadCore mock and the LoadBackground polyfill
    // after onTimingSetup goes offline.
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreStart);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadCoreEnd);
    tasm::TimingCollector::Instance()->Mark(
        tasm::timing::kReloadBackgroundStart);
    app_->onAppReload(std::move(data));
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kReloadBackgroundEnd);
  });
}

void LynxRuntime::EvaluateScript(const std::string& url, std::string script,
                                 piper::ApiCallBack callback) {
  QueueOrExecTask([this, url, script = std::move(script), callback]() mutable {
    app_->EvaluateScript(url, std::move(script), callback);
  });
}

void LynxRuntime::OnScriptLoaded(const std::string& url, std::string script,
                                 std::string error,
                                 piper::ApiCallBack callback) {
  QueueOrExecTask([this, url, script = std::move(script),
                   error = std::move(error), callback]() mutable {
    app_->OnScriptLoaded(url, std::move(script), std::move(error), callback);
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
  app_->loadApp(tasm::TasmRuntimeBundle(), init_global_props_,
                tasm::PackageInstanceDSL::STANDALONE,
                tasm::PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE,
                url);
  delegate_->OnEvaluateJavaScriptEnd(url);
}

void LynxRuntime::I18nResourceChanged(const std::string& msg) {
  QueueOrExecTask([this, msg] { app_->I18nResourceChanged(msg); });
}

void LynxRuntime::NotifyJSUpdatePageData(uint64_t trace_flow_id) {
  QueueOrExecTask([this, trace_flow_id]() mutable {
    app_->NotifyUpdatePageData(trace_flow_id);
  });
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
  if (core_context_proxy != nullptr) {
    core_context_proxy->AddEventListener(
        kMessageEventTypeOnAppEnterForeground,
        std::make_unique<event::ClosureEventListener>(
            [this](lepus::Value args) {
              lifecycle_observer_->OnAppEnterForeground();
            }));
    core_context_proxy->AddEventListener(
        kMessageEventTypeOnAppEnterBackground,
        std::make_unique<event::ClosureEventListener>(
            [this](lepus::Value args) {
              lifecycle_observer_->OnAppEnterBackground();
            }));
  }

  auto js_context_proxy =
      app_->GetContextProxy(runtime::ContextProxy::Type::kJSContext);

  if (js_context_proxy != nullptr) {
    delegate_->AddEventListenersToWhiteBoard(js_context_proxy.get());
  }
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
  return js_executor_ ? js_executor_->GetJSRuntime() : nullptr;
}

int64_t LynxRuntime::GenerateRuntimeId() {
  static std::atomic<int64_t> current_id_;
  return ++current_id_;
}

void LynxRuntime::SetEnableBytecode(bool enable,
                                    const std::string& bytecode_source_url) {
  QueueOrExecAppTask([this, enable, bytecode_source_url]() {
    LOGI("LynxRuntime::SetEnableBytecode, enable: "
         << enable << " bytecode_source_url: " << bytecode_source_url);
    if (auto rt = GetJSRuntime()) {
      rt->SetEnableUserBytecode(enable);
      rt->SetBytecodeSourceUrl(bytecode_source_url);
    }
  });
}

void LynxRuntime::SetPageOptions(const tasm::PageOptions& page_options) {
  page_options_ = page_options;
  auto app = app_;
  if (app) {
    app->SetPageOptions(page_options);
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
    auto proxy = app_->GetContextProxy(event.GetOriginType());
    if (proxy != nullptr) {
      proxy->DispatchEvent(event);
    }
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
        runtime::ContextProxy::Type::kJSContext,
        std::make_unique<pub::ValueImplLepus>(props));
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
  auto args = pub::ValueUtils::ConvertValueToLepusValue(*event.message());
  if (event.type() == kMessageEventTypeOnSsrScriptReady) {
    TryLoadSsrScript(args.StdString());
    return true;
  }
  if (state_ == State::kSsrRuntimeReady &&
      event.type() == kMessageEventTypeSendGlobalEvent) {
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

void LynxRuntime::AddLifecycleListener(
    std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) {
  lifecycle_observer_->AddEventListener(std::move(listener));
}

void LynxRuntime::AddModuleFactory(
    std::unique_ptr<piper::NativeModuleFactory> native_factory) {
  if (js_executor_) {
    auto& module_manager = js_executor_->GetModuleManager();
    if (module_manager) {
      module_manager->SetModuleFactory(std::move(native_factory));
      return;
    }
  }
  cached_native_factories_.push_back(std::move(native_factory));
}

void LynxRuntime::OnRuntimeActorCreate() {
  lifecycle_observer_->OnRuntimeCreate(GetVSyncObserver());
}

}  // namespace runtime
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif
