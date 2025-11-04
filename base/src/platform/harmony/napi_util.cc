// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/harmony/napi_util.h"

#include <node_api.h>

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>
#include <variant>

#include "base/include/no_destructor.h"

// temporarily workaround to compile without logging
#undef LOGE
#undef DCHECK
#define LOGE(...)
#define DCHECK(...)
namespace lynx {
namespace base {

namespace {

[[maybe_unused]] static std::string NapiGetLastError(napi_env env,
                                                     napi_status status) {
  std::string ret("Napi Error:");
  ret += status;

  const napi_extended_error_info* error_info;
  napi_get_last_error_info(env, &error_info);
  ret += error_info->error_message;
  return ret;
}

}  // namespace

bool NapiUtil::IsArrayBuffer(napi_env env, napi_value value) {
  napi_status status;
  bool result = false;

  // Check if the value is an ArrayBuffer
  status = napi_is_arraybuffer(env, value, &result);
  if (status != napi_ok) {
    LOGE("napi_is_arraybuffer: failed:" << status);
    return false;
  }

  return result;
}

int32_t NapiUtil::ConvertToInt32(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  int32_t ret;
  napi_status status = napi_get_value_int32(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get int32:" << status);
  }
  return ret;
}

uint32_t NapiUtil::ConvertToUInt32(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  uint32_t ret;
  napi_status status = napi_get_value_uint32(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get uint32:" << status);
  }
  return ret;
}

int64_t NapiUtil::ConvertToInt64(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  int64_t ret;
  napi_status status = napi_get_value_int64(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get int64:" << status);
  }
  return ret;
}

int64_t NapiUtil::ConvertToBigInt64(napi_env env, napi_value obj) {
  int64_t ret;
  bool lossless;
  napi_status status = napi_get_value_bigint_int64(env, obj, &ret, &lossless);
  if (status != napi_ok) {
    LOGE("Fail to get bigint:" << status);
  }
  return ret;
}

uint64_t NapiUtil::ConvertToBigUInt64(napi_env env, napi_value obj) {
  uint64_t ret;
  bool lossless;
  napi_status status = napi_get_value_bigint_uint64(env, obj, &ret, &lossless);
  if (status != napi_ok) {
    LOGE("Fail to get bigint:" << status);
  }
  return ret;
}

float NapiUtil::ConvertToFloat(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  double ret;
  napi_status status = napi_get_value_double(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get float:" << status);
  }
  return ret;
}

double NapiUtil::ConvertToDouble(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_number));
  double ret;
  napi_status status = napi_get_value_double(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get double:" << status);
  }
  return ret;
}

bool NapiUtil::ConvertToBoolean(napi_env env, napi_value obj) {
  DCHECK(NapiIsType(env, obj, napi_boolean));
  bool ret;
  napi_status status = napi_get_value_bool(env, obj, &ret);
  if (status != napi_ok) {
    LOGE("Fail to get bool:" << status);
  }
  return ret;
}

std::string NapiUtil::ConvertToShortString(napi_env env, napi_value arg) {
  DCHECK(NapiIsType(env, arg, napi_string));
  constexpr int BUFFER_SIZE = 128;
  char buf[BUFFER_SIZE] = {0};
  size_t size;
  napi_status status =
      napi_get_value_string_utf8(env, arg, buf, BUFFER_SIZE, &size);
  if (status != napi_ok) {
    LOGE("Fail to get string:" << status);
    return "";
  }
  return std::string(buf);
}

std::string NapiUtil::ConvertToString(napi_env env, napi_value arg) {
  DCHECK(NapiIsType(env, arg, napi_string));
  size_t str_size;
  napi_status status =
      napi_get_value_string_utf8(env, arg, nullptr, 0, &str_size);
  if (status != napi_ok) {
    LOGE("Fail to get size:" << status);
    return "";
  }
  auto buf = std::make_unique<char[]>(str_size + 1);
  status =
      napi_get_value_string_utf8(env, arg, buf.get(), str_size + 1, &str_size);
  if (status != napi_ok) {
    LOGE("Fail to get string:" << status);
    return "";
  }
  return std::string(buf.get(), str_size);
}

bool NapiUtil::ConvertToArrayString(napi_env env, napi_value arg,
                                    std::vector<std::string>& array_string) {
  DCHECK(IsArray(env, arg));
  uint32_t array_length;
  napi_status status = napi_get_array_length(env, arg, &array_length);
  if (status != napi_ok) {
    LOGE("Fail to get array length:" << status);
    return false;
  }
  for (uint32_t i = 0; i < array_length; i++) {
    napi_value element;
    napi_get_element(env, arg, i, &element);
    std::string str = ConvertToString(env, element);
    array_string.push_back(str);
  }
  return true;
}

bool NapiUtil::ConvertToArrayBuffer(napi_env env, napi_value arg,
                                    std::vector<uint8_t>& buffer) {
  DCHECK(IsArrayBuffer(env, arg));
  size_t length;
  void* data;
  napi_status status = napi_get_arraybuffer_info(env, arg, &data, &length);
  if (status != napi_ok || data == nullptr) {
    LOGE("Fail to get array buffer " << NapiGetLastError(env, status));
    return false;
  }
  buffer.resize(length);
  memcpy(buffer.data(), data, length);
  return true;
}

bool NapiUtil::ConvertToArrayBuffer(napi_env env, napi_value arg,
                                    std::unique_ptr<uint8_t[]>& buffer,
                                    size_t& length) {
  DCHECK(IsArrayBuffer(env, arg));
  void* data;
  napi_status status = napi_get_arraybuffer_info(env, arg, &data, &length);
  if (status != napi_ok || data == nullptr) {
    LOGE("Fail to get array buffer " << NapiGetLastError(env, status));
    return false;
  }

  buffer = std::make_unique<uint8_t[]>(length);
  if (buffer == nullptr) {
    LOGE("Fail to alloc memory");
    return false;
  }

  memcpy(buffer.get(), data, length);
  return true;
}

bool NapiUtil::ConvertToArray(napi_env env, napi_value arg,
                              std::vector<napi_value>& array) {
  DCHECK(IsArray(env, arg));
  uint32_t len;
  napi_status status = napi_get_array_length(env, arg, &len);
  if (status != napi_ok) {
    return false;
  }
  for (uint32_t i = 0; i < len; i++) {
    napi_value item;
    napi_get_element(env, arg, i, &item);
    array.push_back(item);
  }
  return true;
}

bool NapiUtil::ConvertToArray(napi_env env, napi_value arg, napi_value array[],
                              uint32_t size) {
  DCHECK(IsArray(env, arg));
  uint32_t len;
  napi_status status = napi_get_array_length(env, arg, &len);
  if (status != napi_ok || len != size) {
    return false;
  }
  for (uint32_t i = 0; i < len; i++) {
    napi_value item;
    napi_get_element(env, arg, i, &item);
    array[i] = item;
  }
  return true;
}

bool NapiUtil::ConvertToMap(napi_env env, napi_value arg,
                            std::unordered_map<std::string, std::string>& map) {
  DCHECK(NapiIsType(env, arg, napi_object));
  napi_value property_names;
  napi_status status = napi_get_property_names(env, arg, &property_names);
  if (status != napi_ok) {
    return false;
  }

  uint32_t length;
  status = napi_get_array_length(env, property_names, &length);
  if (status != napi_ok) {
    return false;
  }

  for (uint32_t i = 0; i < length; i++) {
    // get key
    napi_value property_name;
    status = napi_get_element(env, property_names, i, &property_name);
    if (status != napi_ok) {
      continue;
    }
    std::string key = NapiUtil::ConvertToString(env, property_name);
    if (key.empty()) {
      continue;
    }

    // get value
    napi_value property;
    status = napi_get_named_property(env, arg, key.c_str(), &property);
    if (status != napi_ok) {
      continue;
    }
    std::string value = NapiUtil::ConvertToString(env, property);

    map.emplace(std::move(key), std::move(value));
  }
  return true;
}

bool NapiUtil::NapiIsType(napi_env env, napi_value value, napi_valuetype type) {
  napi_status status;
  napi_valuetype arg_type;
  status = napi_typeof(env, value, &arg_type);
  return status == napi_ok && type == arg_type;
}

bool NapiUtil::NapiIsAnyType(napi_env env, napi_value value, ...) {
  napi_status status;
  napi_valuetype arg_type;
  status = napi_typeof(env, value, &arg_type);

  if (status != napi_ok) {
    return false;
  }

  bool matched = false;
  {
    va_list types;
    va_start(types, value);
    napi_valuetype cur;
    while (!matched) {
      cur = va_arg(types, napi_valuetype);
      matched = (cur == arg_type);
    }
    va_end(types);
  }
  return matched;
}

bool NapiUtil::IsArray(napi_env env, napi_value value) {
  bool ret;
  napi_is_array(env, value, &ret);
  return ret;
}

napi_value NapiUtil::CreateArrayBuffer(napi_env env, void* input_data,
                                       size_t data_size) {
  void* data = nullptr;
  napi_value array_buffer = nullptr;
  if (input_data == nullptr) {
    return nullptr;
  }
  napi_status status =
      napi_create_arraybuffer(env, data_size, &data, &array_buffer);
  NAPI_THROW_IF_FAILED_NULL(env, status, "napi_create_arraybuffer failed");

  memcpy(data, input_data, data_size);
  return array_buffer;
}

napi_value NapiUtil::CreateMap(
    napi_env env, const std::unordered_map<std::string, std::string> map) {
  napi_value result;
  napi_create_object(env, &result);
  for (const auto& pair : map) {
    napi_value key, value;
    napi_create_string_utf8(env, pair.first.c_str(), pair.first.length(), &key);
    napi_create_string_utf8(env, pair.second.c_str(), pair.second.length(),
                            &value);
    napi_set_property(env, result, key, value);
  }
  return result;
}

napi_value NapiUtil::CreateMap(napi_env env,
                               const std::unordered_map<int32_t, double> map) {
  napi_value result;
  napi_create_object(env, &result);
  for (const auto& pair : map) {
    napi_value key, value;
    napi_create_int32(env, pair.first, &key);
    napi_create_double(env, pair.second, &value);
    napi_set_property(env, result, key, value);
  }
  return result;
}

napi_value NapiUtil::CreateMap(
    napi_env env, const std::unordered_map<int32_t, std::string> map) {
  napi_value result;
  napi_create_object(env, &result);
  for (const auto& pair : map) {
    napi_value key, value;
    napi_create_int32(env, pair.first, &key);
    napi_create_string_utf8(env, pair.second.c_str(), pair.second.length(),
                            &value);
    napi_set_property(env, result, key, value);
  }
  return result;
}

napi_value NapiUtil::CreatePtrArray(napi_env env, uint64_t ptr) {
  napi_value result;
  napi_create_array_with_length(env, 2, &result);
  uint32_t ptr_high = static_cast<uint32_t>(ptr >> 32);
  uint32_t ptr_low = static_cast<uint32_t>(ptr & 0xffffffff);
  napi_set_element(env, result, 0, CreateUint32(env, ptr_high));
  napi_set_element(env, result, 1, CreateUint32(env, ptr_low));
  return result;
}

napi_value NapiUtil::CreateUint32(napi_env env, uint32_t num) {
  napi_value result;
  napi_create_uint32(env, num, &result);
  return result;
}

napi_value NapiUtil::CreateInt32(napi_env env, int32_t num) {
  napi_value result;
  napi_create_int32(env, num, &result);
  return result;
}

napi_status NapiUtil::SetPropToJSMap(napi_env env, napi_value js_map,
                                     const std::string& key,
                                     const std::string& value) {
  napi_value js_value;
  napi_create_string_utf8(env, value.c_str(), value.length(), &js_value);
  return napi_set_named_property(env, js_map, key.c_str(), js_value);
}

napi_status NapiUtil::SetPropToJSMap(napi_env env, napi_value js_map,
                                     const std::string& key, int32_t value) {
  return napi_set_named_property(env, js_map, key.c_str(),
                                 CreateInt32(env, value));
}

napi_status NapiUtil::SetPropToJSMap(napi_env env, napi_value js_map,
                                     const std::string& key, double value) {
  napi_value js_value;
  napi_create_double(env, value, &js_value);
  return napi_set_named_property(env, js_map, key.c_str(), js_value);
}

uint64_t NapiUtil::ConvertToPtr(napi_env env, napi_value arr) {
  napi_value item;
  napi_get_element(env, arr, 0, &item);
  uint32_t ptr_high = ConvertToUInt32(env, item);
  napi_get_element(env, arr, 1, &item);
  uint32_t ptr_low = ConvertToUInt32(env, item);
  return (static_cast<uint64_t>(ptr_high) << 32) + ptr_low;
}

napi_status NapiUtil::InvokeJsMethod(napi_env env, napi_ref ref_napi_obj,
                                     napi_ref ref_napi_method, size_t argc,
                                     const napi_value* argv,
                                     napi_value* result) {
  napi_value napi_obj;
  napi_get_reference_value(env, ref_napi_obj, &napi_obj);
  napi_value napi_method;
  napi_get_reference_value(env, ref_napi_method, &napi_method);
  if (!napi_obj || !napi_method) {
    return napi_invalid_arg;
  }
  // call js func
  return napi_call_function(env, napi_obj, napi_method, argc, argv, result);
}

napi_status NapiUtil::AsyncInvokeJsMethod(napi_env env, napi_ref ref_napi_obj,
                                          const char* method_name, size_t argc,
                                          const napi_value* argv) {
  napi_value work_name;
  napi_create_string_utf8(env, "NapiUtil::AsyncInvokeJsMethod",
                          NAPI_AUTO_LENGTH, &work_name);

  auto* context = new NapiAsyncContext();
  context->env = env;
  napi_value value;
  napi_status status = napi_get_reference_value(env, ref_napi_obj, &value);
  if (status != napi_ok || value == nullptr) {
    LOGE("napi_get_reference_value fail: " << method_name);
    return status;
  }
  // Create new weak ref for async work
  napi_create_reference(env, value, 0, &context->ref_napi_obj);
  context->method_name = method_name;
  context->args.resize(argc);
  for (size_t i = 0; i < argc; ++i) {
    napi_ref ref;
    napi_create_reference(env, argv[i], 1, &ref);
    context->args[i] = ref;
  }
  napi_create_async_work(
      env, nullptr, work_name, [](napi_env env, void* data) {},
      [](napi_env env, napi_status status, void* data) {
        NapiHandleScope scope(env);
        auto* context = reinterpret_cast<NapiAsyncContext*>(data);
        std::vector<napi_value> argv(context->args.size());
        std::transform(context->args.begin(), context->args.end(), argv.begin(),
                       [context](napi_ref ref) {
                         napi_value value;
                         napi_get_reference_value(context->env, ref, &value);
                         napi_delete_reference(context->env, ref);
                         return value;
                       });
        InvokeJsMethod(env, context->ref_napi_obj, context->method_name.c_str(),
                       context->args.size(), argv.data(), nullptr);
        napi_delete_reference(env, context->ref_napi_obj);
        napi_delete_async_work(env, context->async_work);
        delete context;
      },
      reinterpret_cast<void*>(context), &context->async_work);
  return napi_queue_async_work(env, context->async_work);
}

napi_status NapiUtil::InvokeJsMethod(napi_env env, napi_ref ref_napi_obj,
                                     const char* method_name, size_t argc,
                                     const napi_value* argv,
                                     napi_value* result) {
  napi_value napi_obj = nullptr;
  napi_status status = napi_get_reference_value(env, ref_napi_obj, &napi_obj);
  if (status != napi_ok || napi_obj == nullptr) {
    LOGE("napi_get_reference_value fail: " << method_name);
    return status;
  }
  return InvokeJsMethod(env, napi_obj, method_name, argc, argv, result);
}

napi_status NapiUtil::InvokeJsMethod(napi_env env, napi_value napi_obj,
                                     const char* method_name, size_t argc,
                                     const napi_value* argv,
                                     napi_value* result) {
  static const char* const kGetPropertyFailed =
      "napi_get_named_property fail: %s";
  static const char* const kCallFunctionFailed = "napi_call_function fail: %s";
  napi_value fn;
  napi_status status = napi_get_named_property(env, napi_obj, method_name, &fn);
  NAPI_THROW_IF_FAILED_STATUS(env, status, kGetPropertyFailed, method_name);
  status = napi_call_function(env, napi_obj, fn, argc, argv, result);
  NAPI_THROW_IF_FAILED_STATUS(env, status, kCallFunctionFailed, method_name);
  return napi_ok;
}

const std::string& NapiUtil::StatusToString(napi_status status) {
  const static base::NoDestructor<std::unordered_map<napi_status, std::string>>
      status_map{{
          {napi_ok, "napi_ok"},
          {napi_invalid_arg, "napi_invalid_arg"},
          {napi_object_expected, "napi_object_expected"},
          {napi_string_expected, "napi_string_expected"},
          {napi_name_expected, "napi_name_expected"},
          {napi_function_expected, "napi_function_expected"},
          {napi_number_expected, "napi_number_expected"},
          {napi_boolean_expected, "napi_boolean_expected"},
          {napi_array_expected, "napi_array_expected"},
          {napi_generic_failure, "napi_generic_failure"},
          {napi_pending_exception, "napi_pending_exception"},
          {napi_cancelled, "napi_cancelled"},
          {napi_escape_called_twice, "napi_escape_called_twice"},
          {napi_handle_scope_mismatch, "napi_handle_scope_mismatch"},
          {napi_callback_scope_mismatch, "napi_callback_scope_mismatch"},
          {napi_queue_full, "napi_queue_full"},
          {napi_closing, "napi_closing"},
          {napi_bigint_expected, "napi_bigint_expected"},
          {napi_date_expected, "napi_date_expected"},
          {napi_arraybuffer_expected, "napi_arraybuffer_expected"},
          {napi_detachable_arraybuffer_expected,
           "napi_detachable_arraybuffer_expected"},
          {napi_would_deadlock, "napi_would_deadlock"},
      }};
  const static base::NoDestructor<std::string> kEmpty;
  auto it = status_map->find(status);
  return it != status_map->end() ? it->second : *kEmpty;
}

napi_value NapiUtil::GetReferenceNapiValue(const napi_env env,
                                           const napi_ref reference) {
  napi_value ret = nullptr;
  if (const napi_status status = napi_get_reference_value(env, reference, &ret);
      status != napi_ok) {
    return nullptr;
  }
  return ret;
}

}  // end namespace base
}  // end namespace lynx
