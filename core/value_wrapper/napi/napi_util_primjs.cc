// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/napi/napi_util_primjs.h"

#include <algorithm>
#include <cstdarg>
#include <string>
#include <unordered_map>
#include <variant>

#include "base/include/no_destructor.h"
#include "lynx/base/include/log/logging.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace value {

bool NapiUtil::IsArrayBuffer(napi_env env, napi_value value) {
  napi_status status;
  bool result = false;

  // Check if the value is an ArrayBuffer
  status = env->napi_is_arraybuffer(env, value, &result);
  if (status != napi_ok) {
    LOGE("napi_is_arraybuffer: failed:" << status);
    return false;
  }

  return result;
}

int32_t NapiUtil::ConvertToInt32(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  int32_t ret;
  napi_status status = env->napi_get_value_int32(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get int32:" << status);
  }
  return ret;
}

uint32_t NapiUtil::ConvertToUInt32(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  uint32_t ret;
  napi_status status = env->napi_get_value_uint32(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get uint32:" << status);
  }
  return ret;
}

int64_t NapiUtil::ConvertToInt64(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  int64_t ret;
  napi_status status = env->napi_get_value_int64(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get int64:" << status);
  }
  return ret;
}

float NapiUtil::ConvertToFloat(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  double ret;
  napi_status status = env->napi_get_value_double(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get float:" << status);
  }
  return ret;
}

double NapiUtil::ConvertToDouble(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  double ret;
  napi_status status = env->napi_get_value_double(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get double:" << status);
  }
  return ret;
}

bool NapiUtil::ConvertToBoolean(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_boolean));
  bool ret;
  napi_status status = env->napi_get_value_bool(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get bool:" << status);
  }
  return ret;
}

std::string NapiUtil::ConvertToString(napi_env env, napi_value arg) {
  DCHECK(NapiIsType(env, arg, napi_string));
  size_t str_size;
  napi_status status =
      env->napi_get_value_string_utf8(env, arg, nullptr, 0, &str_size);
  if (status != napi_ok) {
    LOGE("Fail to get size:" << status);
    return "";
  }
  auto buf = std::make_unique<char[]>(str_size + 1);
  status = env->napi_get_value_string_utf8(env, arg, buf.get(), str_size + 1,
                                           &str_size);
  if (status != napi_ok) {
    LOGE("Fail to get string:" << status);
    return "";
  }
  return std::string(buf.get(), str_size);
}

bool NapiUtil::NapiIsType(napi_env env, napi_value value, napi_valuetype type) {
  napi_status status;
  napi_valuetype arg_type;
  status = env->napi_typeof(env, value, &arg_type);
  return status == napi_ok && type == arg_type;
}

bool NapiUtil::IsArray(napi_env env, napi_value value) {
  bool ret;
  env->napi_is_array(env, value, &ret);
  return ret;
}

}  // namespace value
}  // end namespace lynx
