// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/piper/js/js_executor.h"

#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/console.h"
#include "core/runtime/bindings/jsi/fetch/body_native.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/piper/js/runtime_manager.h"

// BINARY_KEEP_SOURCE_FILE
namespace lynx {
namespace piper {

JSExecutor::JSExecutor(
    const std::shared_ptr<JSIExceptionHandler>& handler,
    const std::string& group_id,
    const std::shared_ptr<LynxModuleManager>& module_manager,
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& runtime_observer,
    bool force_use_light_weight_js_engine)
    : exception_handler_(handler),
      group_id_(group_id),
      runtime_observer_ng_(runtime_observer),
      module_manager_(module_manager),
      force_use_light_weight_js_engine_(force_use_light_weight_js_engine) {
#if ENABLE_TESTBENCH_REPLAY
  module_manager_testBench_ = nullptr;
#endif
  if (module_manager_) {
    module_manager_->InitModuleInterceptor();
  }
}

JSExecutor::~JSExecutor() { LOGI("lynx ~JSExecutor"); }

void JSExecutor::Destroy() {
  // must detroy all the runtime object before Runtime is destroyed
  module_manager_.reset();

  // Destroy the runtime in the JS thread
  LOGI("JSExecutor::Destroy");

  js_runtime_.reset();
}

runtime::RuntimeManager* JSExecutor::runtimeManagerInstance() {
  if (runtime_observer_ng_ != nullptr) {
    if (runtime::RuntimeManager::Instance()->GetRuntimeManagerDelegate() ==
        nullptr) {
      runtime::RuntimeManager::Instance()->SetRuntimeManagerDelegate(
          runtime_observer_ng_->CreateRuntimeManagerDelegate());
    }
  }
  return runtime::RuntimeManager::Instance();
}

runtime::RuntimeManager* JSExecutor::GetCurrentRuntimeManagerInstance() {
  return runtime::RuntimeManager::Instance();
}

void JSExecutor::loadPreJSBundle(
    std::vector<std::pair<std::string, std::string>>& js_pre_sources,
    bool ensure_console, int64_t rt_id, bool enable_user_bytecode,
    const std::string& bytecode_source_url) {
  js_runtime_ = runtimeManagerInstance()->CreateJSRuntime(
      group_id_, exception_handler_, js_pre_sources,
      force_use_light_weight_js_engine_, *this, rt_id, ensure_console,
      enable_user_bytecode, bytecode_source_url);
}

void JSExecutor::invokeCallback(std::shared_ptr<piper::ModuleCallback> callback,
                                piper::ModuleCallbackFunctionHolder* holder) {
  Scope scope(*js_runtime_);
  callback->Invoke(js_runtime_.get(), holder);
}

std::shared_ptr<piper::App> JSExecutor::createNativeAppInstance(
    int64_t rt_id, runtime::TemplateDelegate* delegate,
    std::unique_ptr<lynx::runtime::LynxApiHandler> api_handler) {
  Scope scope(*js_runtime_);
  piper::Object nativeModuleProxy = piper::Object::createFromHostObject(
      *js_runtime_, module_manager_.get()->bindingPtr);
#if ENABLE_TESTBENCH_REPLAY
  piper::Value module = module_manager_->bindingPtr->get(
      js_runtime_.get(),
      piper::PropNameID::forAscii(*js_runtime_, "TestBenchReplayDataModule"));
  if (!module.isNull()) {
    module_manager_testBench_ = std::make_shared<ModuleManagerTestBench>();
    module_manager_testBench_->SetGroupInterceptor(
        module_manager_->GetGroupInterceptor());
    module_manager_testBench_.get()->initBindingPtr(
        module_manager_testBench_, module_manager_.get()->delegate,
        module_manager_.get()->bindingPtr);
    module_manager_testBench_.get()->initRecordModuleData(js_runtime_.get());
    nativeModuleProxy = piper::Object::createFromHostObject(
        *js_runtime_, module_manager_testBench_.get()->bindingPtr);
  }
#endif
  BodyNative::RegisterBodyNative(*js_runtime_);
  return piper::App::Create(rt_id, js_runtime_, delegate, exception_handler_,
                            std::move(nativeModuleProxy),
                            std::move(api_handler), group_id_);
}

piper::JSRuntimeCreatedType JSExecutor::getJSRuntimeType() {
  if (js_runtime_) {
    return js_runtime_->getCreatedType();
  }
  return piper::JSRuntimeCreatedType::unknown;
}

std::shared_ptr<piper::Runtime> JSExecutor::GetJSRuntime() {
  return js_runtime_;
}

void JSExecutor::SetUrl(const std::string& url) {
  module_manager_->SetTemplateUrl(url);
}

std::shared_ptr<piper::ConsoleMessagePostMan>
JSExecutor::CreateConsoleMessagePostMan() {
  if (runtime_observer_ng_ == nullptr) {
    return nullptr;
  }
  return runtime_observer_ng_->CreateConsoleMessagePostMan();
}

}  // namespace piper
}  // namespace lynx
