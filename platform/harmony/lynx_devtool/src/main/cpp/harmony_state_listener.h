// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_STATE_LISTENER_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_STATE_LISTENER_H_

#include <node_api.h>
#include <uv.h>

#include <map>
#include <memory>
#include <string>

#include "third_party/debug_router/src/debug_router/common/debug_router.h"
#include "third_party/debug_router/src/debug_router/native/core/debug_router_core.h"
#include "third_party/napi/include/js_native_api_types.h"

namespace lynx {
namespace devtool {

class HarmonyStateListener
    : public debugrouter::common::DebugRouterStateListener,
      public std::enable_shared_from_this<HarmonyStateListener> {
 public:
  HarmonyStateListener(napi_env env, napi_value js_object);
  virtual ~HarmonyStateListener() {
    napi_delete_reference(env_, js_this_ref_);
  };

  void OnOpen(debugrouter::common::ConnectionType type) override;
  void OnClose(int32_t code, const std::string &reason) override;
  void OnMessage(const std::string &message) override;
  void OnError(const std::string &error) override;

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  napi_env env_;
  napi_ref js_this_ref_;
  uv_loop_t *loop_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_STATE_LISTENER_H_
