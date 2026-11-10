// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <napi/native_api.h>

#include <memory>
#include <mutex>

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"
#include "devtool/lynx_devtool/js_debug/js/quickjs/proxy/js_debug_proxy_quickjs.h"
#include "devtool/lynx_devtool/js_debug/lepus/proxy/js_debug_proxy_lepus.h"

namespace {

napi_value Init(napi_env env, napi_value exports) {
  static std::once_flag once;
  std::call_once(once, [] {
    auto* helper = lynx::devtool::JSDebugHelper::GetInstance();
    helper->SetQuickJSProxy(
        std::make_unique<lynx::devtool::JSDebugProxyQuickJS>());
    helper->SetLepusProxy(std::make_unique<lynx::devtool::JSDebugProxyLepus>());
  });
  return exports;
}

napi_module qjs_bridge_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "lynxdevtool_qjs_bridge",
    .nm_priv = nullptr,
    .reserved = {0},
};

}  // namespace

extern "C" __attribute__((constructor)) void RegisterQJSBridgeModule() {
  napi_module_register(&qjs_bridge_module);
}
