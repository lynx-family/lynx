// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <hilog/log.h>
#include <napi/native_api.h>
#include <uv.h>

#include <mutex>

#include "devtool/lynx_devtool/js_debug/lepus/manager/rts_inspector_manager_factory.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/debug_bridge_harmony.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/debug_router_wrapper.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/devtool_env_harmony.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/inspector_owner_harmony.h"

EXTERN_C_START static napi_value Init(napi_env env, napi_value exports) {
  // Anchor the RTS inspector factory when the Harmony devtool NAPI module is
  // loaded so the pure C++ registration object is not stripped by the linker.
  LynxRegisterRTSInspectorManagerFactory();
  lynx::devtool::DebugBridgeHarmony::Init(env, exports);
  lynx::devtool::InspectorOwnerHarmony::Init(env, exports);
  lynx::devtool::DebugRouterWrapper::Init(env, exports);
  lynx::devtool::DevToolEnvHarmony::Init(env, exports);

  return exports;
}

EXTERN_C_END

static napi_module lynx_devtool_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "lynxdevtool",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  napi_module_register(&lynx_devtool_module);
}
