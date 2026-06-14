// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_DEBUG_ROUTER_MESSAGE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_DEBUG_ROUTER_MESSAGE_HANDLER_H_

#include <node_api.h>
#include <uv.h>

#include <memory>
#include <string>

#include "third_party/debug_router/src/debug_router/native/core/debug_router_message_handler.h"

namespace lynx {
namespace devtool {

class HarmonyDebugRouterMessageHandler
    : public debugrouter::core::DebugRouterMessageHandler,
      public std::enable_shared_from_this<HarmonyDebugRouterMessageHandler> {
 public:
  HarmonyDebugRouterMessageHandler(napi_env env, napi_value js_object,
                                   std::string handler_name);
  ~HarmonyDebugRouterMessageHandler() override {
    napi_delete_reference(env_, js_this_ref_);
  }

  std::string Handle(std::string params) override;
  std::string GetName() const override;

 private:
  std::string InvokeHandleMethod(const std::string& params);
  std::string InvokeHandleMethodOnCurrentThread(const std::string& params);

  napi_env env_;
  napi_ref js_this_ref_;
  uv_loop_t* loop_;
  std::string handler_name_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_DEBUG_ROUTER_MESSAGE_HANDLER_H_
