// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/lynx_native_module_napi.h"

#include <vector>

#include "core/value_wrapper/napi/value_impl_napi_primjs.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

LYNX_EXTERN_C void lynx_napi_set_instance_data(napi_env env, uint64_t key,
                                               void* data,
                                               napi_finalize finalize_cb,
                                               void* finalize_hint) {
  env->napi_set_instance_data(env, key, data, finalize_cb, finalize_hint);
}

LYNX_EXTERN_C void lynx_napi_get_instance_data(napi_env env, uint64_t key,
                                               void** data) {
  env->napi_get_instance_data(env, key, data);
}

namespace lynx {
namespace embedder {

using LynxModuleFunctionData =
    std::pair<std::shared_ptr<runtime::LynxModuleCallback>,
              std::weak_ptr<runtime::LynxNativeModule::Delegate>>;

LynxNativeModuleNAPI::LynxNativeModuleNAPI(Napi::Env env, napi_value exports)
    : env_(env) {
  env_->napi_create_reference(env_, exports, 1, &exports_ref_);
  ExtractMethods();
  env_->napi_add_env_cleanup_hook(env_, Cleanup, this);
}

LynxNativeModuleNAPI::~LynxNativeModuleNAPI() {
  if (env_) {
    env_->napi_delete_reference(env_, exports_ref_);
    ReleaseMemberRefs();
    // Remove the cleanup hook from napi_env to avoid accessing the already
    // destroyed module.
    env_->napi_remove_env_cleanup_hook(env_, Cleanup, this);
  }
}

void LynxNativeModuleNAPI::ExtractMethods() {
  napi_value module;
  env_->napi_get_reference_value(env_, exports_ref_, &module);
  napi_value keys;
  env_->napi_get_property_names(env_, module, &keys);
  uint32_t size = 0;
  env_->napi_get_array_length(env_, keys, &size);
  for (size_t i = 0; i < size; ++i) {
    napi_value name;
    env_->napi_get_element(env_, keys, i, &name);
    size_t len = 0;
    env_->napi_get_value_string_utf8(env_, name, nullptr, 0, &len);
    std::string str(len, '\0');
    // Since std::string's *(s.begin() + s.size()) is always \0, and the string
    // returned by napi_get_value_string_utf8 always has a null terminator, it
    // can be considered that the size of the buffer is len + 1.
    env_->napi_get_value_string_utf8(env_, name, str.data(), len + 1, &len);
    napi_value member;
    env_->napi_get_property(env_, module, name, &member);
    napi_valuetype value_type;
    env_->napi_typeof(env_, member, &value_type);
    napi_ref member_ref;
    env_->napi_create_reference(env_, member, 1, &member_ref);
    if (value_type == napi_valuetype::napi_function) {
      method_refs_[str] = member_ref;
      methods_.emplace(str, runtime::NativeModuleMethod(str, 0));
    } else {
      field_refs_[str] = member_ref;
    }
  }
}

void LynxNativeModuleNAPI::ReleaseMemberRefs() {
  for (const auto& [name, ref] : field_refs_) {
    env_->napi_delete_reference(env_, ref);
  }
  for (const auto& [name, ref] : method_refs_) {
    env_->napi_delete_reference(env_, ref);
  }
}

base::expected<std::unique_ptr<pub::Value>, std::string>
LynxNativeModuleNAPI::InvokeMethod(const std::string& method_name,
                                   std::unique_ptr<pub::Value> args,
                                   size_t count,
                                   const runtime::CallbackMap& callbacks) {
  auto delegate = delegate_.lock();
  if (!delegate) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto method_iter = method_refs_.find(method_name);
  if (method_iter == method_refs_.end()) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  napi_handle_scope scope;
  env_->napi_open_handle_scope(env_, &scope);
  std::vector<napi_value> capi_args;
  for (size_t i = 0; i < count; i++) {
    auto arg = args->GetValueAtIndex(static_cast<uint32_t>(i));
    napi_value capi_arg;
    // Check if the parameter at the current index is a callback
    runtime::CallbackMap::const_iterator iter = callbacks.find(i);
    if (iter != callbacks.end()) {
      auto func_data = new LynxModuleFunctionData(iter->second, delegate_);
      env_->napi_create_function(env_, "jsb callback", 0,
                                 LynxNativeModuleNAPI::Callback, func_data,
                                 &capi_arg);
      env_->napi_add_finalizer(
          env_, capi_arg, func_data,
          [](napi_env env, void* data, void* hint) {
            delete reinterpret_cast<
                std::pair<std::shared_ptr<runtime::LynxModuleCallback>,
                          std::weak_ptr<Delegate>>*>(data);
          },
          nullptr, nullptr);
    } else {
      capi_arg =
          pub::ValueUtilsNapiPrimJS::ConvertPubValueToNapiValue(env_, *arg);
    }
    capi_args.push_back(capi_arg);
  }
  napi_value ret = nullptr;
  napi_value func;
  napi_value undefined;
  env_->napi_get_undefined(env_, &undefined);
  env_->napi_get_reference_value(env_, method_iter->second, &func);
  env_->napi_call_function(env_, undefined, func, capi_args.size(),
                           capi_args.data(), &ret);
  std::unique_ptr<lynx::pub::Value> result = nullptr;
  if (ret) {
    result = std::make_unique<pub::ValueImplNapiPrimJS>(env_, ret);
  }
  env_->napi_close_handle_scope(env_, scope);
  return result;
}

// static
napi_value LynxNativeModuleNAPI::Callback(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 0;
  env->napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
  std::vector<napi_value> argv(argc, nullptr);
  void* func_data = nullptr;
  env->napi_get_cb_info(env, info, &argc, argv.data(), nullptr, &func_data);
  if (!func_data) {
    LOGE(
        "LynxNativeModuleNAPI:: Call function fail, the function "
        "context is null.");
    napi_value ret;
    env->napi_get_undefined(env, &ret);
    return ret;
  }
  auto& callback = reinterpret_cast<LynxModuleFunctionData*>(func_data)->first;
  auto& weak_delegate =
      reinterpret_cast<LynxModuleFunctionData*>(func_data)->second;
  auto delegate = weak_delegate.lock();
  if (!delegate) {
    LOGE(
        "LynxNativeModuleNAPI:: Call function fail, the delegate has "
        "be released.");
    napi_value ret;
    env->napi_get_undefined(env, &ret);
    return ret;
  }
  napi_value args;
  env->napi_create_array(env, &args);
  for (size_t i = 0; i < argc; ++i) {
    env->napi_set_element(env, args, i, argv[i]);
  }
  callback->SetArgs(std::make_unique<pub::ValueImplNapiPrimJS>(env, args));
  delegate->InvokeCallback(callback);
  return napi_value();
}

void LynxNativeModuleNAPI::Cleanup(void* arg) {
  if (!arg) {
    return;
  }
  LynxNativeModuleNAPI* mod = static_cast<LynxNativeModuleNAPI*>(arg);
  mod->CleanupSelf();
}
void LynxNativeModuleNAPI::CleanupSelf() {
  // When CleanupSelf is triggered, it indicates that the napi_env is being
  // destroyed. At this time, there is no need to actively release the refs.
  // These refs will be released along with the napi_env. We only need to clear
  // them to avoid double-releasing when the module is destructed.
  env_ = nullptr;
  exports_ref_ = nullptr;
  method_refs_.clear();
  field_refs_.clear();
}

}  // namespace embedder
}  // namespace lynx
