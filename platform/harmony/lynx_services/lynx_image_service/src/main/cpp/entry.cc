// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <node_api.h>

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_harmony.h"

EXTERN_C_START static napi_value Init(napi_env env, napi_value exports) {
  lynx::service::ImageServiceHarmony::Init(env, exports);
  return exports;
}

EXTERN_C_END

static napi_module lynx_image_service_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "lynx_image_service",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  napi_module_register(&lynx_image_service_module);
}
