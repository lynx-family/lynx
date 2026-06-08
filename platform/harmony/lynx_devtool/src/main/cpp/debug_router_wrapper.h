// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_DEBUG_ROUTER_WRAPPER_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_DEBUG_ROUTER_WRAPPER_H_

#include <node_api.h>
#include <uv.h>

#include <map>
#include <memory>
#include <string>

#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_debug_router_message_handler.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_global_handler.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_session_handler.h"
#include "third_party/debug_router/src/debug_router/common/debug_router.h"

namespace lynx {
namespace devtool {
struct NapiValueCompare {
  bool operator()(const napi_value &lhs, const napi_value &rhs) const {
    return lhs < rhs;
  }
};

class DebugRouterWrapper {
 public:
  DebugRouterWrapper() = default;
  static napi_value Init(napi_env env, napi_value exports);

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value AddGlobalHandler(napi_env env, napi_callback_info info);
  static napi_value RemoveGlobalHandler(napi_env env, napi_callback_info info);
  static napi_value AddMessageHandler(napi_env env, napi_callback_info info);
  static napi_value RemoveMessageHandler(napi_env env, napi_callback_info info);
  static napi_value SendDataAsync(napi_env env, napi_callback_info info);
  static napi_value AddSessionHandler(napi_env env, napi_callback_info info);
  static napi_value HandleSchema(napi_env env, napi_callback_info info);
  static napi_value GetAppInfoByKey(napi_env env, napi_callback_info info);
  static napi_value AddStateListener(napi_env env, napi_callback_info info);

  static std::map<napi_value, std::shared_ptr<HarmonyGlobalHandler>,
                  NapiValueCompare>
      global_handlers_;
  static std::map<napi_value, std::shared_ptr<HarmonySessionHandler>,
                  NapiValueCompare>
      session_handlers_;
  static std::map<std::string,
                  std::shared_ptr<HarmonyDebugRouterMessageHandler>>
      message_handlers_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_DEBUG_ROUTER_WRAPPER_H_
