// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_STATIC_TASK_NAPI_BRIDGE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_STATIC_TASK_NAPI_BRIDGE_H_

#include <node_api.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace lynx {
namespace harmony {

class StaticTaskNapiBridge {
 public:
  static void Init(napi_env env, napi_value exports);
  static bool HasTask(const std::string& id);
  static bool InvokeTask(const std::string& id,
                         std::intptr_t native_context_ptr);

 private:
  static napi_value Register(napi_env env, napi_callback_info info);
  static napi_value Unregister(napi_env env, napi_callback_info info);

  static napi_env env_;
  static std::unordered_map<std::string, napi_ref> tasks_;
  static std::mutex tasks_mutex_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_STATIC_TASK_NAPI_BRIDGE_H_
