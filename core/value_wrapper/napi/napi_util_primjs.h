// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_NAPI_NAPI_UTIL_PRIMJS_H_
#define CORE_VALUE_WRAPPER_NAPI_NAPI_UTIL_PRIMJS_H_

#include <string.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "third_party/binding/napi/shim/shim_napi.h"
#include "third_party/binding/napi/shim/shim_napi_env.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace value {

class NapiUtil {
 public:
  static int32_t ConvertToInt32(napi_env env, napi_value obj);
  static uint32_t ConvertToUInt32(napi_env env, napi_value obj);
  static int64_t ConvertToInt64(napi_env env, napi_value obj);
  static float ConvertToFloat(napi_env env, napi_value obj);
  static double ConvertToDouble(napi_env env, napi_value obj);
  static bool ConvertToBoolean(napi_env env, napi_value obj);
  static std::string ConvertToString(napi_env env, napi_value arg);

  static bool NapiIsType(napi_env env, napi_value value, napi_valuetype type);
  static bool IsArray(napi_env env, napi_value value);
  static bool IsArrayBuffer(napi_env env, napi_value value);
};
}  // namespace value
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // CORE_VALUE_WRAPPER_NAPI_NAPI_UTIL_PRIMJS_H_
