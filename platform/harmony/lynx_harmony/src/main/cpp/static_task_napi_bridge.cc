// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_harmony/src/main/cpp/static_task_napi_bridge.h"

#include <napi/native_api.h>

#include <cctype>
#include <string_view>

namespace lynx {
namespace harmony {

napi_env StaticTaskNapiBridge::env_ = nullptr;

void StaticTaskNapiBridge::Init(napi_env env, napi_value exports) {
  env_ = env;
}

bool StaticTaskNapiBridge::LoadAndInvokeTask(const std::string& module_path,
                                             const std::string& module_info,
                                             const std::string& class_name,
                                             std::intptr_t native_context_ptr) {
  napi_env env = env_;
  if (!env) {
    return false;
  }

  napi_value module = nullptr;
  napi_status s = napi_load_module_with_info(env, module_path.c_str(),
                                             module_info.c_str(), &module);
  if (s != napi_ok || module == nullptr) {
    return false;
  }

  napi_value export_class = nullptr;
  s = napi_get_named_property(env, module, class_name.c_str(), &export_class);
  if (s != napi_ok || export_class == nullptr) {
    return false;
  }

  napi_value export_func = nullptr;
  s = napi_get_named_property(env, export_class, "task", &export_func);
  if (s != napi_ok || export_func == nullptr) {
    return false;
  }

  napi_value args[1] = {nullptr};
  // Pointers must be passed as BigInt on HarmonyOS to avoid precision loss.
  s = napi_create_bigint_int64(env, static_cast<int64_t>(native_context_ptr),
                               &args[0]);
  if (s != napi_ok) {
    return false;
  }

  napi_value result = nullptr;
  // Use the module exports object as receiver, so `this` is stable if used.
  s = napi_call_function(env, module, export_func, 1, args, &result);
  if (s != napi_ok) {
    return false;
  }

  return true;
}

}  // namespace harmony
}  // namespace lynx
