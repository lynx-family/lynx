// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/harmony/platform_module_manager.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace harmony {

PlatformModuleManager::PlatformModuleManager(napi_env env,
                                             napi_value module_args[4],
                                             napi_value sendable_module_args[4])
    : env_(env) {
  napi_create_reference(env, module_args[0], 0, &js_module_manager_);
  napi_create_reference(env, module_args[1], 0, &js_get_module_);
  AddPlatformModules(module_args[2], module_args[3], false);
  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_serialize(env_, sendable_module_args[0], undefined, undefined,
                 &sendable_js_module_manager_buffer_);
  napi_serialize(env_, sendable_module_args[1], undefined, undefined,
                 &sendable_js_module_buffer_);
  AddPlatformModules(sendable_module_args[2], sendable_module_args[3], true);
}

PlatformModuleManager::~PlatformModuleManager() {
  LOGI("~PlatformModuleManager");
  // The memory of PlatformModuleManager is managed by main thread now, so
  // we can delete reference directly here.
  napi_delete_reference(env_, js_module_manager_);
  napi_delete_reference(env_, js_get_module_);
}

napi_value PlatformModuleManager::JSModuleManager(napi_env env, bool sendable) {
  napi_value result = nullptr;
  if (sendable) {
    result = EnsureSendable(env, sendable_js_module_manager_buffer_,
                            sendable_js_module_manager_);
  } else {
    napi_get_reference_value(env, js_module_manager_, &result);
  }
  return result;
}

napi_value PlatformModuleManager::JSGetModuleFunc(napi_env env, bool sendable) {
  napi_value result = nullptr;
  if (sendable) {
    result = EnsureSendable(env, sendable_js_module_buffer_,
                            sendable_js_get_module_);
  } else {
    napi_get_reference_value(env, js_get_module_, &result);
  }
  return result;
}

void PlatformModuleManager::AddPlatformModules(napi_value module_key,
                                               napi_value module_value,
                                               bool sendable) {
  DCHECK(base::NapiUtil::IsArray(env_, module_key));
  DCHECK(base::NapiUtil::IsArray(env_, module_value));
  uint32_t length;
  napi_get_array_length(env_, module_key, &length);
  for (uint32_t i = 0; i < length; i++) {
    napi_value module;
    napi_get_element(env_, module_key, i, &module);
    napi_value methods;
    napi_get_element(env_, module_value, i, &methods);
    DCHECK(base::NapiUtil::IsArray(env_, methods));
    std::string key = base::NapiUtil::ConvertToShortString(env_, module);
    std::vector<std::string> value;
    base::NapiUtil::ConvertToArrayString(env_, methods, value);
    js_module_map_[key] = std::make_pair(sendable, value);
  }
}

napi_value PlatformModuleManager::EnsureSendable(napi_env env, void* buffer,
                                                 napi_ref& ref) {
  napi_value result = nullptr;
  if (!ref) {
    napi_value value;
    napi_deserialize(env, buffer, &value);
    napi_create_reference(env, value, 0, &ref);
    napi_delete_serialization_data(env, buffer);
  }
  napi_get_reference_value(env, ref, &result);
  return result;
}

}  // namespace harmony
}  // namespace lynx
