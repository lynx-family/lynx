// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_runtime_wrapper.h"

#include <js_native_api.h>

#include <memory>
#include <utility>

#include "base/harmony/napi_convert_helper.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/renderer/data/harmony/template_data_harmony.h"
#include "core/renderer/dom/harmony/lynx_template_bundle_harmony.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "core/shell/runtime/common/module_delegate_impl.h"

namespace lynx {
namespace harmony {

namespace {

// Check napi_unwrap object, print error message if failed.
// Do not throw if failed because we have not defined the exceptions for js api.
bool CheckNapiUnwrapObject(napi_status status, void* obj, const char* message) {
  if (status != napi_ok) {
    LOGE("napi unwrap status error: " << base::NapiUtil::StatusToString(status)
                                      << ", " << message);
    return false;
  }
  if (!obj) {
    LOGE("napi unwrap object is null: " << message);
    return false;
  }
  return true;
}

}  // namespace

// NativeRuntimeFacadeHarmony
void NativeRuntimeFacadeHarmony::ReportError(const base::LynxError& error) {
  if (!runtime_wrapper_) {
    return;
  }
  base::NapiHandleScope scope(runtime_wrapper_->env_);
  std::string level_str =
      base::LynxError::GetLevelString(static_cast<int32_t>(error.error_level_));
  napi_value param[6];
  napi_create_string_utf8(runtime_wrapper_->env_, level_str.c_str(),
                          level_str.length(), &param[0]);
  napi_create_int32(runtime_wrapper_->env_, error.error_code_, &param[1]);
  napi_create_string_utf8(runtime_wrapper_->env_, error.error_message_.c_str(),
                          error.error_message_.length(), &param[2]);
  napi_create_string_utf8(runtime_wrapper_->env_, error.fix_suggestion_.c_str(),
                          error.fix_suggestion_.length(), &param[3]);
  param[4] =
      base::NapiUtil::CreateMap(runtime_wrapper_->env_, error.custom_info_);
  napi_get_boolean(runtime_wrapper_->env_, error.is_logbox_only_, &param[5]);
  base::NapiUtil::AsyncInvokeJsMethod(runtime_wrapper_->env_,
                                      runtime_wrapper_->runtime_wrapper_ref_,
                                      "onErrorOccurred", 6, param);
}

void NativeRuntimeFacadeHarmony::OnModuleMethodInvoked(
    const std::string& module, const std::string& method, int32_t code) {
  if (!runtime_wrapper_) {
    return;
  }
  base::NapiHandleScope scope(runtime_wrapper_->env_);
  int argc = 3;
  napi_value param[argc];
  napi_create_string_utf8(runtime_wrapper_->env_, module.c_str(),
                          module.length(), &param[0]);
  napi_create_string_utf8(runtime_wrapper_->env_, method.c_str(),
                          method.length(), &param[1]);
  napi_create_int32(runtime_wrapper_->env_, code, &param[2]);
  base::NapiUtil::AsyncInvokeJsMethod(runtime_wrapper_->env_,
                                      runtime_wrapper_->runtime_wrapper_ref_,
                                      "onModuleMethodInvoked", argc, param);
}

void NativeRuntimeFacadeHarmony::OnEvaluateJavaScriptEnd(
    const std::string& url) {
  if (!runtime_wrapper_) {
    return;
  }
  base::NapiHandleScope scope(runtime_wrapper_->env_);
  int argc = 1;
  napi_value param[argc];
  napi_create_string_utf8(runtime_wrapper_->env_, url.c_str(), url.length(),
                          &param[0]);
  base::NapiUtil::AsyncInvokeJsMethod(runtime_wrapper_->env_,
                                      runtime_wrapper_->runtime_wrapper_ref_,
                                      "onEvaluateJavaScriptEnd", argc, param);
}

// RuntimeLifecycleListenerDelegateHarmony
RuntimeLifecycleListenerDelegateHarmony::
    RuntimeLifecycleListenerDelegateHarmony(napi_env env, napi_ref listener_ref)
    : RuntimeLifecycleListenerDelegate(
          RuntimeLifecycleListenerDelegate::DelegateType::PART),
      env_(env),
      listener_ref_(listener_ref) {}

void RuntimeLifecycleListenerDelegateHarmony::OnRuntimeAttach(Napi::Env env) {
  base::NapiHandleScope scope(env_);
  napi_value param[1];
  void* env_ptr = static_cast<void*>(env);
  param[0] = base::NapiUtil::CreatePtrArray(
      env_, reinterpret_cast<uintptr_t>(static_cast<napi_env>(env_ptr)));
  base::NapiUtil::AsyncInvokeJsMethod(env_, listener_ref_, "onRuntimeAttach", 1,
                                      param);
}

void RuntimeLifecycleListenerDelegateHarmony::OnRuntimeDetach() {
  base::NapiHandleScope scope(env_);
  base::NapiUtil::AsyncInvokeJsMethod(env_, listener_ref_, "onRuntimeDetach", 0,
                                      nullptr);
}

// LynxRuntimeWrapper
LynxRuntimeWrapper::LynxRuntimeWrapper(
    napi_env env, napi_value js_this,
    const std::shared_ptr<LynxResourceLoaderHarmony>& resource_loader,
    std::string group_id, bool use_quickjs, bool enable_js_group_thread,
    std::vector<std::string> preload_js_paths, bool enable_bytecode,
    std::string bytecode_source_url,
    std::unique_ptr<ModuleFactoryHarmony> module_factory,
    std::shared_ptr<tasm::TemplateData> template_data,
    lepus::Value global_props)
    : env_(env) {
  napi_create_reference(env, js_this, 0, &runtime_wrapper_ref_);

  const std::string group_name = enable_js_group_thread ? group_id : "";
  auto native_facade_runtime =
      std::make_unique<NativeRuntimeFacadeHarmony>(this);
  auto bundle_creator = std::make_shared<tasm::PropBundleCreatorHarmony>();

  napi_value param[1];
  param[0] =
      base::NapiUtil::CreatePtrArray(env, reinterpret_cast<uintptr_t>(this));
  base::NapiUtil::InvokeJsMethod(env_, runtime_wrapper_ref_, "createDevTool", 1,
                                 param, nullptr);
  std::shared_ptr<lynx::runtime::js::InspectorRuntimeObserverNG>
      inspector_observer;
  if (inspector_owner_ != nullptr) {
    inspector_observer =
        inspector_owner_->OnBackgroundRuntimeCreated(group_name);
  }

  // Init module manager
  module_manager_ = std::make_shared<runtime::js::LynxModuleManager>();
  module_manager_->SetModuleFactory(std::move(module_factory));

  auto on_runtime_actor_created = [this](auto& actor, auto& facade_actor) {
    module_manager_->initBindingPtr(
        module_manager_,
        std::make_shared<shell::ModuleDelegateImpl>(actor, facade_actor));
    runtime_proxy_ = std::make_shared<shell::LynxBTSRuntimeProxyImpl>(actor);
    module_manager_->runtime_proxy = runtime_proxy_;
  };
  std::shared_ptr<lynx::tasm::WhiteBoard> white_board = nullptr;

  auto runtime_flags =
      shell::CalcRuntimeFlags(false, use_quickjs, false, enable_bytecode);

  runtime_standalone_ =
      lynx::shell::BTSRuntimeStandalone::InitRuntimeStandalone(
          group_name, group_id, std::move(native_facade_runtime),
          std::move(inspector_observer), resource_loader, module_manager_,
          bundle_creator, white_board, on_runtime_actor_created,
          std::move(preload_js_paths), bytecode_source_url, runtime_flags,
          &global_props);
}

LynxRuntimeWrapper::~LynxRuntimeWrapper() { DestroyRuntime(); }

void LynxRuntimeWrapper::DestroyRuntime() {
  if (is_attached_) {
    return;
  }
  runtime_standalone_->DestroyRuntime();
}

void LynxRuntimeWrapper::SetAttached(bool is_attached) {
  is_attached_ = is_attached;
}

void LynxRuntimeWrapper::AddRuntimeLifecycleListener(napi_env env,
                                                     napi_ref ref) {
  runtime_proxy_->AddLifecycleListener(
      std::make_unique<RuntimeLifecycleListenerDelegateHarmony>(env, ref));
}

napi_value LynxRuntimeWrapper::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("nativeCreate", NativeCreate),
      DECLARE_NAPI_FUNCTION("nativeEvaluateScript", NativeEvaluateScript),
      DECLARE_NAPI_FUNCTION("nativeEvaluateBundleScript",
                            NativeEvaluateTemplateBundle),
      DECLARE_NAPI_FUNCTION("nativeTransitionToFullRuntime",
                            NativeTransitionToFullRuntime),
      DECLARE_NAPI_FUNCTION("nativeCallJSFunction", NativeCallJSFunction),
      DECLARE_NAPI_FUNCTION("nativeAddRuntimeLifecycleListener",
                            NativeAddRuntimeLifecycleListener),
  };
#undef DECLARE_NAPI_FUNCTION
  napi_value cons;
  std::string class_name = "LynxRuntimeWrapper";
  napi_define_class(env, class_name.c_str(), NAPI_AUTO_LENGTH, New, nullptr,
                    sizeof(properties) / sizeof(properties[0]), properties,
                    &cons);

  napi_set_named_property(env, exports, class_name.c_str(), cons);

  return exports;
}

napi_value LynxRuntimeWrapper::New(napi_env env, napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

napi_value LynxRuntimeWrapper::NativeCreate(napi_env env,
                                            napi_callback_info info) {
  napi_value js_this;
  size_t argc = 13;
  napi_value args[13] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  // providers
  auto resource_loader =
      std::make_shared<LynxResourceLoaderHarmony>(env, args[0]);

  // runtime options
  // js group
  std::string group_id = base::NapiUtil::ConvertToString(env, args[1]);
  bool use_quickjs;
  napi_get_value_bool(env, args[2], &use_quickjs);
  bool enable_js_group_thread;
  napi_get_value_bool(env, args[3], &enable_js_group_thread);
  std::vector<std::string> preload_js_paths;
  base::NapiUtil::ConvertToArrayString(env, args[4], preload_js_paths);

  // bytecode
  bool enable_bytecode = false;
  napi_get_value_bool(env, args[5], &enable_bytecode);
  std::string bytecode_source_url =
      base::NapiUtil::ConvertToString(env, args[6]);

  // module
  static constexpr uint32_t kArgsSize = 4;
  napi_value module_args[kArgsSize];
  base::NapiUtil::ConvertToArray(env, args[7], module_args, kArgsSize);
  napi_value sendable_module_args[kArgsSize];
  base::NapiUtil::ConvertToArray(env, args[8], sendable_module_args, kArgsSize);
  auto module_factory = std::make_unique<ModuleFactoryHarmony>(
      env, module_args, sendable_module_args);

  auto template_data = tasm::TemplateDataHarmony::GenerateTemplateData(
      env, args[10], args[11], args[9]);

  lepus_value global_props =
      base::NapiConvertHelper::JSONToLepusValue(env, args[12]);

  // LynxTemplateRenderer
  LynxRuntimeWrapper* wrapper = new LynxRuntimeWrapper(
      env, js_this, resource_loader, std::move(group_id), use_quickjs,
      enable_js_group_thread, std::move(preload_js_paths), enable_bytecode,
      std::move(bytecode_source_url), std::move(module_factory),
      std::move(template_data), std::move(global_props));

  static auto finalizer = [](napi_env env, void* data, void* hint) {
    if (data != nullptr) {
      auto* wrapper = static_cast<LynxRuntimeWrapper*>(data);
      delete wrapper;
    }
  };

  napi_status status =
      napi_wrap(env, js_this, wrapper, finalizer, nullptr, nullptr);

  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "NativeCreate failed due to napi_wrap failed!");
  return nullptr;
}

napi_value LynxRuntimeWrapper::NativeEvaluateScript(napi_env env,
                                                    napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxRuntimeWrapper* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "NativeEvaluateScript failed")) {
    return nullptr;
  }
  std::string url = base::NapiUtil::ConvertToString(env, args[0]);
  std::string sources = base::NapiUtil::ConvertToString(env, args[1]);
  obj->RuntimeStandalone().EvaluateScript(std::move(url), std::move(sources));
  return nullptr;
}

napi_value LynxRuntimeWrapper::NativeEvaluateTemplateBundle(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxRuntimeWrapper* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj,
                             "NativeEvaluateTemplateBundle failed")) {
    return nullptr;
  }
  std::string url = base::NapiUtil::ConvertToString(env, args[0]);
  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, args[1], reinterpret_cast<void**>(&bundle));
  if (bundle == nullptr) {
    LOGE("get template bundle nullptr");
    return nullptr;
  }
  std::string js_file = base::NapiUtil::ConvertToString(env, args[2]);
  obj->RuntimeStandalone().EvaluateScript(
      std::move(url), &(bundle->GetBundle()), std::move(js_file));
  return nullptr;
}

napi_value LynxRuntimeWrapper::NativeTransitionToFullRuntime(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  LynxRuntimeWrapper* wrapper = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&wrapper));
  if (!CheckNapiUnwrapObject(status, wrapper,
                             "NativeTransitionToFullRuntime failed")) {
    return nullptr;
  }
  wrapper->RuntimeStandalone().TransitionToFullRuntime();
  return nullptr;
}

napi_value LynxRuntimeWrapper::NativeCallJSFunction(napi_env env,
                                                    napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string module_id = base::NapiUtil::ConvertToShortString(env, args[0]);
  std::string method = base::NapiUtil::ConvertToShortString(env, args[1]);

  auto lepus_value = base::NapiConvertHelper::ConvertToLepusValue(env, args[2]);
  auto params = std::make_unique<pub::ValueImplLepus>(std::move(lepus_value));

  LynxRuntimeWrapper* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "CallJSFunction failed")) {
    return nullptr;
  }
  obj->runtime_proxy_->CallJSFunction(module_id, method, std::move(params));
  return nullptr;
}

napi_value LynxRuntimeWrapper::NativeAddRuntimeLifecycleListener(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  napi_ref listener_ref;
  napi_create_reference(env, args[0], 0, &listener_ref);

  LynxRuntimeWrapper* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "CallJSFunction failed")) {
    return nullptr;
  }
  obj->AddRuntimeLifecycleListener(env, listener_ref);
  return nullptr;
}

}  // namespace harmony
}  // namespace lynx
