// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_STATIC_TASK_NAPI_BRIDGE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_STATIC_TASK_NAPI_BRIDGE_H_

#include <node_api.h>

#include <mutex>
#include <string>

namespace lynx {
namespace harmony {

class StaticTaskNapiBridge {
 public:
  static void Init(napi_env env, napi_value exports);
  static bool LoadAndInvokeTask(const std::string& module_path,
                                const std::string& module_info,
                                const std::string& class_name,
                                std::intptr_t native_context_ptr);

 private:
  static napi_env env_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_STATIC_TASK_NAPI_BRIDGE_H_
