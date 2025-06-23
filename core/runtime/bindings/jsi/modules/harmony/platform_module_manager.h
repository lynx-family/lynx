// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_PLATFORM_MODULE_MANAGER_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_PLATFORM_MODULE_MANAGER_H_

#include <napi/native_api.h>
#include <node_api.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lynx {
namespace harmony {

/*
 * The NativeModuleFactory implementation for harmony platform rendering
 * Maybe this file should be put in another directory
 */
class PlatformModuleManager {
 public:
  PlatformModuleManager(napi_env env, napi_value module_args[4],
                        napi_value sendable_module_args[4]);
  ~PlatformModuleManager();

  napi_value JSModuleManager(napi_env env, bool sendable);
  napi_value JSGetModuleFunc(napi_env env, bool sendable);

  const auto& JSModuleMap() { return js_module_map_; }

  napi_env Env() { return env_; }

 private:
  void AddPlatformModules(napi_value module_key, napi_value module_value,
                          bool sendable);
  static napi_value EnsureSendable(napi_env env, void* buffer, napi_ref& ref);

  napi_env env_;
  napi_ref js_module_manager_;
  napi_ref js_get_module_;
  napi_ref sendable_js_module_manager_ = nullptr;
  napi_ref sendable_js_get_module_ = nullptr;

  void* sendable_js_module_manager_buffer_ = nullptr;
  void* sendable_js_module_buffer_ = nullptr;

  std::unordered_map<std::string, std::pair<bool, std::vector<std::string>>>
      js_module_map_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_PLATFORM_MODULE_MANAGER_H_
