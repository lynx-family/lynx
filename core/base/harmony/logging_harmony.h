// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_LOGGING_HARMONY_H_
#define CORE_BASE_HARMONY_LOGGING_HARMONY_H_

#include <node_api.h>

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace logging {

class LynxLog {
 public:
  static napi_value Init(napi_env env, napi_value exports);
  static napi_value NativeInitLynxLog(napi_env env, napi_callback_info info);
  static napi_value NativeInitLynxLogWriteFunction(napi_env env,
                                                   napi_callback_info info);
  static napi_value NativeUseSysLog(napi_env env, napi_callback_info info);
  static napi_value nativeInternalLog(napi_env env, napi_callback_info info);
  static napi_value nativeSetMinLogLevel(napi_env env, napi_callback_info info);
};

}  // namespace logging
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_HARMONY_LOGGING_HARMONY_H_
