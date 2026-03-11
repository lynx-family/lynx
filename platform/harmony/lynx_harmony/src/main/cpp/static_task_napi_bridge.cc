// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_harmony/src/main/cpp/static_task_napi_bridge.h"

#include <node_api.h>

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace harmony {

napi_env StaticTaskNapiBridge::env_ = nullptr;
std::unordered_map<std::string, napi_ref> StaticTaskNapiBridge::tasks_;
std::mutex StaticTaskNapiBridge::tasks_mutex_;

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

void StaticTaskNapiBridge::Init(napi_env env, napi_value exports) {
  env_ = env;
  napi_property_descriptor descriptors[] = {
      DECLARE_NAPI_METHOD("registerStaticTask", Register),
      DECLARE_NAPI_METHOD("unregisterStaticTask", Unregister),
  };
  napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
}

napi_value StaticTaskNapiBridge::Register(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_value this_arg = nullptr;
  napi_get_cb_info(env, info, &argc, args, &this_arg, nullptr);
  if (argc < 2) {
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
  }

  std::string id = base::NapiUtil::ConvertToString(env, args[0]);

  napi_valuetype type;
  napi_typeof(env, args[1], &type);
  if (type != napi_function) {
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
  }

  napi_ref ref;
  napi_create_reference(env, args[1], 1, &ref);

  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(id);
    if (it != tasks_.end()) {
      napi_delete_reference(env, it->second);
      it->second = ref;
    } else {
      tasks_.emplace(id, ref);
    }
  }

  napi_value result;
  napi_get_boolean(env, true, &result);
  return result;
}

napi_value StaticTaskNapiBridge::Unregister(napi_env env,
                                            napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_value this_arg = nullptr;
  napi_get_cb_info(env, info, &argc, args, &this_arg, nullptr);
  if (argc < 1) {
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
  }

  std::string id = base::NapiUtil::ConvertToString(env, args[0]);

  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(id);
    if (it != tasks_.end()) {
      napi_delete_reference(env, it->second);
      tasks_.erase(it);
    }
  }

  napi_value result;
  napi_get_boolean(env, true, &result);
  return result;
}

bool StaticTaskNapiBridge::HasTask(const std::string& id) {
  std::lock_guard<std::mutex> lock(tasks_mutex_);
  return tasks_.find(id) != tasks_.end();
}

bool StaticTaskNapiBridge::InvokeTask(const std::string& id,
                                      std::intptr_t native_context_ptr) {
  napi_env env = env_;
  if (!env) {
    return false;
  }

  napi_ref ref = nullptr;
  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
      return false;
    }
    ref = it->second;
  }

  napi_value func;
  napi_status s = napi_get_reference_value(env, ref, &func);
  if (s != napi_ok || func == nullptr) {
    return false;
  }

  napi_value recv;
  napi_get_undefined(env, &recv);

  napi_value argv[1];
  napi_create_int64(env, static_cast<int64_t>(native_context_ptr), &argv[0]);

  napi_value call_result;
  s = napi_call_function(env, recv, func, 1, argv, &call_result);
  return s == napi_ok;
}

}  // namespace harmony
}  // namespace lynx
