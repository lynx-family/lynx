// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_HARMONY_NAPI_UTIL_H_
#define BASE_INCLUDE_PLATFORM_HARMONY_NAPI_UTIL_H_

#include <node_api.h>
#include <string.h>
#include <uv.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "base/include/base_export.h"

namespace lynx {
namespace base {

#define NAPI_THROW_IF_FAILED_STATUS(env, status, format, ...)             \
  if ((status) != napi_ok) {                                              \
    char msg[1024];                                                       \
    snprintf(msg, sizeof(msg), format, __VA_ARGS__);                      \
    napi_throw_error(env, base::NapiUtil::StatusToString(status).c_str(), \
                     msg);                                                \
    return status;                                                        \
  }

#define NAPI_THROW_IF_FAILED_NULL(env, status, message)                   \
  if ((status) != napi_ok) {                                              \
    napi_throw_error(env, base::NapiUtil::StatusToString(status).c_str(), \
                     message);                                            \
    return nullptr;                                                       \
  }

#define NAPI_CREATE_FUNCTION(env, exports, function_name, function)           \
  {                                                                           \
    napi_value native_result;                                                 \
    napi_create_function(env, function_name, strlen(function_name), function, \
                         nullptr, &native_result);                            \
    napi_set_named_property(env, exports, function_name, native_result);      \
  }

class NapiHandleScope {
 public:
  explicit NapiHandleScope(napi_env env) : env_(env) {
    napi_open_handle_scope(env_, &scope_);
  }

  ~NapiHandleScope() { napi_close_handle_scope(env_, scope_); }

 private:
  napi_env env_;
  napi_handle_scope scope_ = nullptr;
};

struct NapiAsyncContext {
  napi_env env = nullptr;
  napi_async_work async_work = nullptr;
  napi_ref ref_napi_obj = nullptr;
  std::string method_name;
  std::vector<napi_ref> args;
};

class BASE_EXPORT NapiUtil {
 public:
  static int32_t ConvertToInt32(napi_env env, napi_value obj);
  static uint32_t ConvertToUInt32(napi_env env, napi_value obj);
  static int64_t ConvertToInt64(napi_env env, napi_value obj);
  static int64_t ConvertToBigInt64(napi_env env, napi_value obj);
  static uint64_t ConvertToBigUInt64(napi_env env, napi_value obj);
  static float ConvertToFloat(napi_env env, napi_value obj);
  static double ConvertToDouble(napi_env env, napi_value obj);
  static bool ConvertToBoolean(napi_env env, napi_value obj);
  static std::string ConvertToShortString(napi_env env, napi_value arg);
  static std::string ConvertToString(napi_env env, napi_value arg);

  static bool ConvertToArrayString(napi_env env, napi_value arg,
                                   std::vector<std::string>& array_string);
  static bool ConvertToArrayBuffer(napi_env env, napi_value arg,
                                   std::vector<uint8_t>& buffer);
  static bool ConvertToArrayBuffer(napi_env env, napi_value arg,
                                   std::unique_ptr<uint8_t[]>& buffer,
                                   size_t& length);
  static bool ConvertToArray(napi_env env, napi_value arg,
                             std::vector<napi_value>& array);

  static bool ConvertToArray(napi_env env, napi_value arg, napi_value array[],
                             uint32_t size);
  /**
   * convert Record<string, string> to unordered_map<string, string>
   */
  static bool ConvertToMap(napi_env env, napi_value arg,
                           std::unordered_map<std::string, std::string>& map);

  static bool NapiIsType(napi_env env, napi_value value, napi_valuetype type);
  static bool NapiIsAnyType(napi_env env, napi_value value, ...);
  static bool IsArray(napi_env env, napi_value value);
  static bool IsArrayBuffer(napi_env env, napi_value value);

  static napi_value CreateArrayBuffer(napi_env env, void* input_data,
                                      size_t data_size);

  static napi_value CreateMap(
      napi_env env, const std::unordered_map<int32_t, std::string> map);
  static napi_value CreateMap(napi_env env,
                              const std::unordered_map<int32_t, double> map);
  static napi_value CreateMap(
      napi_env env, const std::unordered_map<std::string, std::string> map);

  static napi_value CreatePtrArray(napi_env env, uint64_t ptr);

  static napi_value CreateUint32(napi_env env, uint32_t num);

  static napi_value CreateInt32(napi_env env, int32_t num);

  static napi_status SetPropToJSMap(napi_env env, napi_value js_map,
                                    const std::string& key,
                                    const std::string& value);

  static napi_status SetPropToJSMap(napi_env env, napi_value js_map,
                                    const std::string& key, int32_t value);

  static napi_status SetPropToJSMap(napi_env env, napi_value js_map,
                                    const std::string& key, double value);

  static uint64_t ConvertToPtr(napi_env env, napi_value arr);

  static napi_status InvokeJsMethod(napi_env env, napi_ref ref_napi_obj,
                                    napi_ref ref_napi_method, size_t argc,
                                    const napi_value* argv,
                                    napi_value* result = nullptr);

  static napi_status AsyncInvokeJsMethod(napi_env env, napi_ref ref_napi_obj,
                                         const char* method_name, size_t argc,
                                         const napi_value* argv);

  static napi_status InvokeJsMethod(napi_env env, napi_ref ref_napi_obj,
                                    const char* method_name, size_t argc,
                                    const napi_value* argv,
                                    napi_value* result = nullptr);

  static napi_status InvokeJsMethod(napi_env env, napi_value napi_obj,
                                    const char* method_name, size_t argc,
                                    const napi_value* argv,
                                    napi_value* result = nullptr);

  static const std::string& StatusToString(napi_status status);

  static napi_value GetReferenceNapiValue(napi_env env, napi_ref reference);
};
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_PLATFORM_HARMONY_NAPI_UTIL_H_
