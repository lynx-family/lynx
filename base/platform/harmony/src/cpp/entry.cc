// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <napi/native_api.h>

#include "base/src/log/logging_harmony.h"

EXTERN_C_START static napi_value InitLynxBase(napi_env env,
                                              napi_value exports) {
  lynx::base::logging::LynxLog::Init(env, exports);
  return exports;
}

EXTERN_C_END

static napi_module lynx_base_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = InitLynxBase,
    .nm_modname = "lynx_base",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  napi_module_register(&lynx_base_module);
}
