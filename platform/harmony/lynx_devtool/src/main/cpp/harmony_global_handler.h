// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_GLOBAL_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_GLOBAL_HANDLER_H_

#include <node_api.h>
#include <uv.h>

#include <map>
#include <memory>
#include <string>

#include "third_party/debug_router/src/debug_router/common/debug_router_global_handler.h"
#include "third_party/napi/include/js_native_api_types.h"

namespace lynx {
namespace devtool {

class HarmonyGlobalHandler
    : public debugrouter::common::DebugRouterGlobalHandler,
      public std::enable_shared_from_this<HarmonyGlobalHandler> {
 public:
  HarmonyGlobalHandler(napi_env env, napi_value js_object);
  virtual ~HarmonyGlobalHandler() {
    napi_delete_reference(env_, js_this_ref_);
  };

  void OpenCard(const std::string& url) override;
  void OnMessage(const std::string& message, const std::string& type) override;

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  napi_env env_;
  napi_ref js_this_ref_;
  uv_loop_t* loop_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_GLOBAL_HANDLER_H_
