// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/lynx_native_module_napi.h"

#include <unordered_map>
#include <vector>

#include "core/value_wrapper/napi/value_impl_opaque_napi_primjs.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace binding {
void ReportRawException(void* raw_env, void* raw_exception);
}
}  // namespace lynx
namespace {

napi_status RunScriptFromFile(napi_env env, const char* script, size_t length,
                              const char* filename, napi_value* result);
}  // namespace

// Structure to store instance data information
struct InstanceDataInfo {
  void* data = nullptr;
  napi_finalize finalize_cb = nullptr;
  void* finalize_hint = nullptr;
};

// Thread-local storage for instance data
// Map: napi_env -> (Map: key -> InstanceDataInfo*)
using InstanceDataMap = std::unordered_map<uint64_t, InstanceDataInfo*>;
using EnvInstanceDataStore = std::unordered_map<napi_env, InstanceDataMap>;

static thread_local EnvInstanceDataStore g_instance_data_store;

// Cleanup hook when env is destroyed
static void EnvCleanupHook(void* arg) {
  napi_env env = static_cast<napi_env>(arg);
  auto env_it = g_instance_data_store.find(env);
  if (env_it != g_instance_data_store.end()) {
    InstanceDataMap& instance_map = env_it->second;
    for (auto& pair : instance_map) {
      InstanceDataInfo* info = pair.second;
      if (info != nullptr) {
        if (info->finalize_cb) {
          info->finalize_cb(env, info->data, info->finalize_hint);
        }
        delete info;
      }
    }
    g_instance_data_store.erase(env_it);
  }
}

LYNX_EXTERN_C void lynx_napi_set_instance_data(napi_env env, uint64_t key,
                                               void* data,
                                               napi_finalize finalize_cb,
                                               void* finalize_hint) {
  // Find or create the map for this env
  auto env_it = g_instance_data_store.find(env);
  if (env_it == g_instance_data_store.end()) {
    g_instance_data_store.emplace(env, InstanceDataMap());
    env_it = g_instance_data_store.find(env);
    // Register cleanup hook to clean up data when env is destroyed
    napi_add_env_cleanup_hook(env, EnvCleanupHook, env);
  }

  InstanceDataMap& instance_map = env_it->second;

  // Check if data with this key already exists
  auto it = instance_map.find(key);
  if (it != instance_map.end()) {
    // Finalize and delete the old data
    InstanceDataInfo* old_info = it->second;
    if (old_info != nullptr) {
      if (old_info->finalize_cb) {
        old_info->finalize_cb(env, old_info->data, old_info->finalize_hint);
      }
      delete old_info;
    }
    instance_map.erase(it);
  }

  // If data is null, we just removed the old one (if any)
  if (data == nullptr) {
    return;
  }

  // Create new instance data info
  InstanceDataInfo* info = new InstanceDataInfo();
  info->data = data;
  info->finalize_cb = finalize_cb;
  info->finalize_hint = finalize_hint;

  instance_map[key] = info;
}

LYNX_EXTERN_C void lynx_napi_get_instance_data(napi_env env, uint64_t key,
                                               void** data) {
  if (data == nullptr) {
    return;
  }
  *data = nullptr;

  auto env_it = g_instance_data_store.find(env);
  if (env_it == g_instance_data_store.end()) {
    return;
  }

  const InstanceDataMap& instance_map = env_it->second;
  auto it = instance_map.find(key);
  if (it != instance_map.end()) {
    *data = it->second->data;
  }
}

LYNX_EXTERN_C napi_status lynx_napi_run_script_from_file(napi_env env,
                                                         const char* script,
                                                         size_t length,
                                                         const char* filename,
                                                         napi_value* result) {
  if (env == nullptr || script == nullptr) {
    return napi_invalid_arg;
  }
  const char* safe_filename = filename != nullptr ? filename : "";
  napi_value unused_result = nullptr;
  napi_value* safe_result = result != nullptr ? result : &unused_result;
  return RunScriptFromFile(env, script, length, safe_filename, safe_result);
}

LYNX_EXTERN_C void lynx_napi_report_and_clear_exception(napi_env env) {
  if (env == nullptr) {
    return;
  }
  bool has_exception = false;
  napi_status status = napi_is_exception_pending(env, &has_exception);
  if (status != napi_ok || !has_exception) {
    return;
  }

  napi_value exception = nullptr;
  status = napi_get_and_clear_last_exception(env, &exception);
  if (status != napi_ok || exception == nullptr) {
    return;
  }

  lynx::binding::ReportRawException(env, exception);
}

namespace lynx {
namespace embedder {

using LynxModuleFunctionData =
    std::pair<std::shared_ptr<runtime::LynxModuleCallback>,
              std::weak_ptr<runtime::LynxNativeModule::Delegate>>;

LynxNativeModuleNAPI::LynxNativeModuleNAPI(Napi::Env env, napi_value exports)
    : env_(env) {
  napi_create_reference(env_, exports, 1, &exports_ref_);
  ExtractMethods();
  napi_add_env_cleanup_hook(env_, Cleanup, this);
}

LynxNativeModuleNAPI::~LynxNativeModuleNAPI() {
  if (env_) {
    napi_delete_reference(env_, exports_ref_);
    ReleaseMemberRefs();
    // Remove the cleanup hook from napi_env to avoid accessing the already
    // destroyed module.
    napi_remove_env_cleanup_hook(env_, Cleanup, this);
  }
}

void LynxNativeModuleNAPI::ExtractMethods() {
  napi_value module;
  napi_get_reference_value(env_, exports_ref_, &module);
  napi_value keys;
  napi_get_property_names(env_, module, &keys);
  uint32_t size = 0;
  napi_get_array_length(env_, keys, &size);
  for (size_t i = 0; i < size; ++i) {
    napi_value name;
    napi_get_element(env_, keys, i, &name);
    size_t len = 0;
    napi_get_value_string_utf8(env_, name, nullptr, 0, &len);
    std::string str(len, '\0');
    // Since std::string's *(s.begin() + s.size()) is always \0, and the string
    // returned by napi_get_value_string_utf8 always has a null terminator, it
    // can be considered that the size of the buffer is len + 1.
    napi_get_value_string_utf8(env_, name, str.data(), len + 1, &len);
    napi_value member;
    napi_get_property(env_, module, name, &member);
    napi_valuetype value_type;
    napi_typeof(env_, member, &value_type);
    napi_ref member_ref;
    napi_create_reference(env_, member, 1, &member_ref);
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
    napi_delete_reference(env_, ref);
  }
  for (const auto& [name, ref] : method_refs_) {
    napi_delete_reference(env_, ref);
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
  napi_open_handle_scope(env_, &scope);
  std::vector<napi_value> capi_args;
  for (size_t i = 0; i < count; i++) {
    auto arg = args->GetValueAtIndex(static_cast<uint32_t>(i));
    napi_value capi_arg;
    // Check if the parameter at the current index is a callback
    runtime::CallbackMap::const_iterator iter = callbacks.find(i);
    if (iter != callbacks.end()) {
      auto func_data = new LynxModuleFunctionData(iter->second, delegate_);
      napi_create_function(env_, "jsb callback", 0,
                           LynxNativeModuleNAPI::Callback, func_data,
                           &capi_arg);
      napi_add_finalizer(
          env_, capi_arg, func_data,
          [](napi_env env, void* data, void* hint) {
            delete reinterpret_cast<
                std::pair<std::shared_ptr<runtime::LynxModuleCallback>,
                          std::weak_ptr<Delegate>>*>(data);
          },
          nullptr, nullptr);
    } else {
      capi_arg = static_cast<napi_value>(
          pub::ValueUtilsOpaqueNapiPrimJS::ConvertPubValueToOpaqueNapiValue(
              static_cast<void*>(env_), *arg));
    }
    capi_args.push_back(capi_arg);
  }
  napi_value ret = nullptr;
  napi_value func;
  napi_value undefined;
  napi_get_undefined(env_, &undefined);
  napi_get_reference_value(env_, method_iter->second, &func);
  napi_call_function(env_, undefined, func, capi_args.size(), capi_args.data(),
                     &ret);
  std::unique_ptr<lynx::pub::Value> result = nullptr;
  if (ret) {
    result = pub::ValueUtilsOpaqueNapiPrimJS::CreateValueWithOpaqueNapiArgs(
        static_cast<void*>(env_), static_cast<void*>(ret));
  }
  napi_close_handle_scope(env_, scope);
  return result;
}

// static
napi_value LynxNativeModuleNAPI::Callback(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
  std::vector<napi_value> argv(argc, nullptr);
  void* func_data = nullptr;
  napi_get_cb_info(env, info, &argc, argv.data(), nullptr, &func_data);
  if (!func_data) {
    LOGE(
        "LynxNativeModuleNAPI:: Call function fail, the function "
        "context is null.");
    napi_value ret;
    napi_get_undefined(env, &ret);
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
    napi_get_undefined(env, &ret);
    return ret;
  }
  napi_value args;
  napi_create_array(env, &args);
  for (size_t i = 0; i < argc; ++i) {
    napi_set_element(env, args, i, argv[i]);
  }
  callback->SetArgs(
      pub::ValueUtilsOpaqueNapiPrimJS::CreateValueWithOpaqueNapiArgs(
          static_cast<void*>(env), static_cast<void*>(args)));
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

std::unique_ptr<pub::Value> LynxNativeModuleNAPI::GetAttributeValue(
    const std::string& attribute_name) {
  napi_handle_scope scope;
  napi_open_handle_scope(env_, &scope);
  auto iter = field_refs_.find(attribute_name);
  if (iter == field_refs_.end()) {
    return nullptr;
  }
  napi_value member;
  napi_get_reference_value(env_, iter->second, &member);
  auto ret = pub::ValueUtilsOpaqueNapiPrimJS::CreateValueWithOpaqueNapiArgs(
      static_cast<void*>(env_), static_cast<void*>(member));
  napi_close_handle_scope(env_, scope);
  return ret;
}

}  // namespace embedder
}  // namespace lynx

// Since the standard napi run_script interface does not support the filename
// parameter, we need to use the lower-level primjs napi_run_script interface.
// Since both weak-node-api and primjs napi symbols appear here, and their
// symbols may be renamed, macros are needed to handle various cases.
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif
#include "third_party/napi/include/js_native_api.h"

namespace {
#ifdef USE_WEAK_SUFFIX_NAPI
napi_status_weak RunScriptFromFile(napi_env_weak env, const char* script,
                                   size_t length, const char* filename,
                                   napi_value_weak* result) {
#else
napi_status RunScriptFromFile(napi_env env, const char* script, size_t length,
                              const char* filename, napi_value* result) {
#endif
#ifdef USE_PRIMJS_NAPI
  napi_env_primjs env_primjs = reinterpret_cast<napi_env_primjs>(env);
  napi_value_primjs* result_primjs =
      reinterpret_cast<napi_value_primjs*>(result);
#else
  napi_env env_primjs = reinterpret_cast<napi_env>(env);
  napi_value* result_primjs = reinterpret_cast<napi_value*>(result);
#endif

#ifdef USE_WEAK_SUFFIX_NAPI
  return static_cast<napi_status_weak>(env_primjs->napi_run_script(
      env_primjs, script, length, filename, result_primjs));
#else
  return static_cast<napi_status>(env_primjs->napi_run_script(
      env_primjs, script, length, filename, result_primjs));
#endif
}
}  // namespace
