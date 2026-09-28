// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/js_executor.h"

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/bindings/console.h"
#include "core/runtime/js/js_realm.h"
#include "core/runtime/js/js_realm_manager.h"
#include "core/runtime/js/utils.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/event_report/event_tracker_platform_impl.h"

// BINARY_KEEP_SOURCE_FILE
namespace lynx {
namespace runtime {
namespace js {
JSExecutor::JSExecutor(
    const std::string& group_id,
    const std::shared_ptr<LynxModuleManager>& module_manager,
    const std::shared_ptr<InspectorRuntimeObserverNG>& runtime_observer,
    bool force_use_light_weight_js_engine)
    : group_id_(group_id),
      runtime_observer_ng_(runtime_observer),
      module_manager_(module_manager),
      force_use_light_weight_js_engine_(force_use_light_weight_js_engine) {
#if ENABLE_TESTBENCH_REPLAY
  module_manager_testBench_ = nullptr;
#endif
  if (module_manager_) {
    module_manager_->InitModuleInterceptor();
    if (runtime_observer_ng_) {
      module_manager_->SetNativeModuleRecordObserver(
          runtime_observer_ng_->CreateNativeModuleRecordObserver());
    }
  }
}

JSExecutor::~JSExecutor() { LOGI(GetLogContext() << " lynx ~JSExecutor"); }

void JSExecutor::Destroy() {
  LOGI(GetLogContext() << " JSExecutor::Destroy");
  // Destroy module objects before their runtime.
  module_manager_.reset();

  // The shell calls Destroy on the JS thread after destroying app and NAPI.
  if (auto* runtime = GetJSRuntime().Lock()) {
    runtime->BeforeDestroy();
  }
  auto sharing =
      realm_state_ ? realm_state_->sharing : JSRealmState::Sharing::kNone;
  realm_state_.reset();
  if (sharing != JSRealmState::Sharing::kNone) {
    runtime::JSRealmManager::Instance()->ReleaseSharedRealm(
        group_id_, sharing == JSRealmState::Sharing::kVM);
  }
}

runtime::JSRealmManager* JSExecutor::realmManagerInstance() {
  if (runtime_observer_ng_ != nullptr) {
    if (runtime::JSRealmManager::Instance()->GetRealmManagerDelegate() ==
        nullptr) {
      runtime::JSRealmManager::Instance()->SetRealmManagerDelegate(
          runtime_observer_ng_->CreateRealmManagerDelegate());
    }
  }
  return runtime::JSRealmManager::Instance();
}

runtime::JSRealmManager* JSExecutor::GetCurrentRealmManagerInstance() {
  return runtime::JSRealmManager::Instance();
}

void JSExecutor::loadPreJSBundle(
    base::MoveOnlyClosure<
        std::vector<std::pair<std::string, std::shared_ptr<Buffer>>>>
        js_pre_sources_getter,
    bool ensure_console, const JSRuntimeExternalParams& create_params,
    const tasm::PageOptions& page_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_EXECUTOR_LOAD_PRE_JS_BUNDLE);
  const int64_t runtime_id = create_params.runtime_id;
  realm_state_ =
      std::make_unique<JSRealmState>(realmManagerInstance()->CreateRealm(
          std::move(js_pre_sources_getter), force_use_light_weight_js_engine_,
          ensure_console, *this, create_params, page_options));
  auto* runtime = GetJSRuntime().Lock();
  if (runtime) {
    if (runtime_observer_ng_ != nullptr) {
      runtime_observer_ng_->OnRuntimeCreated(runtime->type());
    }

    tasm::report::EventTracker::UpdateGenericInfo(
        static_cast<int32_t>(runtime_id), "js_runtime_type",
        static_cast<int64_t>(runtime->type()));
  }
}

void JSExecutor::SetObserver(JSIObserver* observer) {
  if (auto* runtime = GetJSRuntime().Lock()) {
    runtime->SetObserver(observer);
  }
}

void JSExecutor::invokeCallback(std::shared_ptr<ModuleCallback> callback,
                                ModuleCallbackFunctionHolder* holder) {
  auto* runtime = GetJSRuntime().Lock();
  if (runtime) {
    Scope scope(*runtime);
    callback->SetLogContext(GetLogContext());
    callback->Invoke(runtime, holder);
  }
}

base::UnsafeOwningPtr<App> JSExecutor::createNativeAppInstance(
    int64_t rt_id, runtime::TemplateDelegate* delegate,
    std::shared_ptr<JSRuntimeDelegate> runtime_delegate,
    std::unique_ptr<lynx::runtime::LynxApiHandler> api_handler,
    const tasm::PageOptions& page_options) {
  auto runtime_weak = GetJSRuntime();
  auto* runtime = runtime_weak.Lock();
  if (!runtime) {
    return nullptr;
  }
  Scope scope(*runtime);
  Object nativeModuleProxy =
      Object::createFromHostObject(*runtime, module_manager_.get()->bindingPtr);
#if ENABLE_TESTBENCH_REPLAY
  Value module = module_manager_->bindingPtr->get(
      runtime, PropNameID::forAscii(*runtime, "LynxRecorderReplayDataModule"));
  if (!module.isNull()) {
    module_manager_testBench_ = std::make_shared<ModuleManagerTestBench>();
    module_manager_testBench_->SetGroupInterceptor(
        module_manager_->GetGroupInterceptor());
    module_manager_testBench_.get()->initBindingPtr(
        module_manager_testBench_, module_manager_.get()->delegate_,
        module_manager_.get()->bindingPtr);
    module_manager_testBench_.get()->initRecordModuleData(runtime);
    nativeModuleProxy = Object::createFromHostObject(
        *runtime, module_manager_testBench_.get()->bindingPtr);
  }
#endif
  auto app = App::Create(rt_id, runtime_weak, delegate, runtime_delegate,
                         std::move(nativeModuleProxy), std::move(api_handler),
                         group_id_, page_options);
  if (app && module_manager_) {
    app->SetNativeModuleRecordObserver(
        module_manager_->GetNativeModuleRecordObserver());
  }
  return app;
}

JSRuntimeCreatedType JSExecutor::getJSRuntimeType() {
  if (auto* runtime = GetJSRuntime().Lock()) {
    return runtime->getCreatedType();
  }
  return JSRuntimeCreatedType::unknown;
}

base::UnsafeWeakPtr<Runtime> JSExecutor::GetJSRuntime() {
  return realm_state_ ? realm_state_->runtime.GetWeakPtr()
                      : base::UnsafeWeakPtr<Runtime>();
}

void JSExecutor::SetUrl(const std::string& url) {
  module_manager_->SetTemplateUrl(url);
  if (auto* runtime = GetJSRuntime().Lock()) {
    runtime->SetPageUrl(url);
  }
}

void JSExecutor::TriggerVmGC() {
  if (auto* runtime = GetJSRuntime().Lock()) {
    runtime->RequestGC();
  }
}

std::shared_ptr<ConsoleMessagePostMan>
JSExecutor::CreateConsoleMessagePostMan() {
  if (runtime_observer_ng_ == nullptr) {
    return nullptr;
  }
  return runtime_observer_ng_->CreateConsoleMessagePostMan();
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
