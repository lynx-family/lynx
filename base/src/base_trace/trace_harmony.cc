// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/src/base_trace/trace_harmony.h"

#include <dlfcn.h>

#include "base/include/base_trace/trace_event_utils.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace base {
namespace trace {

napi_value LynxBaseTrace::Init(napi_env env, napi_value exports) {
  NAPI_CREATE_FUNCTION(env, exports, "nativeInitLynxBaseTrace",
                       NativeInitLynxBaseTrace);
  return exports;
}

napi_value LynxBaseTrace::NativeInitLynxBaseTrace(napi_env env,
                                                  napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  trace_backend_ptr trace_backend_address = reinterpret_cast<trace_backend_ptr>(
      NapiUtil::ConvertToInt64(env, args[0]));
  if (trace_backend_address == nullptr) {
    LOGV("base trace init failed.");
    return nullptr;
  }
  SetTraceBackend(trace_backend_address);
  return nullptr;
}
}  // namespace trace
}  // namespace base
}  // namespace lynx
