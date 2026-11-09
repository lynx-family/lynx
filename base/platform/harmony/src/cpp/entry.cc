// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <napi/native_api.h>

#include <utility>

#include "base/include/application_directory.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/src/base_trace/trace_harmony.h"
#include "base/src/log/logging_harmony.h"

static napi_value SetApplicationDataDirectory(napi_env env,
                                              napi_callback_info info) {
  size_t argc = 1;
  napi_value argument = nullptr;
  std::string directory;
  if (napi_get_cb_info(env, info, &argc, &argument, nullptr, nullptr) ==
          napi_ok &&
      argc == 1 &&
      lynx::base::NapiUtil::ConvertToStringChecked(env, argument, &directory) ==
          lynx::base::NapiValueConversionStatus::kSuccess) {
    lynx::base::SetApplicationDataDirectory(std::move(directory));
  }
  return nullptr;
}

EXTERN_C_START static napi_value InitLynxBase(napi_env env,
                                              napi_value exports) {
  NAPI_CREATE_FUNCTION(env, exports, "nativeSetApplicationDataDirectory",
                       SetApplicationDataDirectory);
  lynx::base::logging::LynxLog::Init(env, exports);
  lynx::base::trace::LynxBaseTrace::Init(env, exports);
  return exports;
}

EXTERN_C_END

static napi_module lynx_base_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = InitLynxBase,
    .nm_modname = "lynxbase",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  napi_module_register(&lynx_base_module);
}
