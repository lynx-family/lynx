// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_HARMONY_NAPI_ENV_HOLDER_H_
#define CORE_BASE_HARMONY_HARMONY_NAPI_ENV_HOLDER_H_

#include <napi/native_api.h>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
namespace harmony {
/**
 * Get the NapiEnv of the js thread, only can be called in js thread.
 * then you can use the NapiEnv to call napi functions from system lib or other
 * sdk
 * @return NapiEnv of the js thread
 * @warning non-sendable object is always different instance & type in different
 * thread / ark runtime
 * @example
 * the code below shows an example that use the http api from arkts to load a
 * resource in js thread
 * @code
 * namespace {
 * std::string NapiValueToString(napi_env env, napi_value value) {
 *  napi_value str_value = nullptr;
 *  napi_coerce_to_string(env, value, &str_value);
 *
 * size_t str_length = 0;
 * napi_get_value_string_utf8(env, str_value, nullptr, 0, &str_length);
 * char* value_str_value = new char[str_length + 1];
 * napi_get_value_string_utf8(env, str_value, value_str_value, str_length + 1,
 *                           &str_length);
 * std::string result(value_str_value);
 * delete[] value_str_value;
 * return result;
 * }
 *
 * void PrintProperties(napi_env env, napi_value object, const std::string
 * &object_name) {
 *  // list properties
 *  LOGI("start to print properties of the object: " << object_name);
 *  napi_value properties_list = nullptr;
 *  napi_status ret = napi_get_property_names(env , object, &properties_list);
 *  DCHECK(ret == napi_ok);
 *
 *  bool is_array = false;
 *  napi_is_array(env, properties_list, &is_array);
 *
 *  if (is_array) {
 *    uint32_t property_count = 0;
 *    napi_get_array_length(env, properties_list, &property_count);
 *    for (size_t i = 0; i < property_count; i++) {
 *      napi_value property_name = nullptr;
 *      napi_get_element(env, properties_list, i, &property_name);
 *
 *      auto name_str_value = NapiValueToString(env, property_name);
 *
 *      // now get property value
 *      napi_value property_value = nullptr;
 *      napi_get_property(env, object, property_name, &property_value);
 *
 *      auto value_str_value = NapiValueToString(env, property_value);
 *
 *      LOGI("property_name: " << name_str_value << " property_value: " <<
 * value_str_value);
 *    }
 *  }
 * }
 * }
 *
 * void LoadBitmap(const std::string& path, LoadBitmapCallback callback)
 * { LOGI("LoadBitmap start")
 *
 * auto env = lynx::base::harmony::GetJSThreadNapiEnv();
 * napi_value http_module = nullptr;
 * auto ret = napi_load_module(env, "@ohos.net.http", &http_module);
 * DCHECK(ret == napi_ok);
 *
 * PrintProperties(env, http_module, "http_module");
 *
 * napi_value create_http_func = nullptr;
 * napi_value create_http_str = nullptr;
 * napi_create_string_latin1(env, "createHttp", NAPI_AUTO_LENGTH,
 *                          &create_http_str);
 * napi_get_property(env, http_module, create_http_str, &create_http_func);
 *
 * napi_value http_request = nullptr;
 * napi_value napi_undefined_instance = nullptr;
 * napi_get_undefined(env, &napi_undefined_instance);
 * ret = napi_call_function(env, napi_undefined_instance, create_http_func, 0,
 *                         nullptr, &http_request);
 * DCHECK(ret == napi_ok);
 *
 * // setup the httpRequest.request params
 * // url
 * napi_value napi_value_url = nullptr;
 * napi_create_string_latin1(env, path.c_str(), NAPI_AUTO_LENGTH,
 *                          &napi_value_url);
 *
 * // options
 * napi_value request_option = nullptr;
 * // get option constructor is too slow, so just use a raw object.
 * napi_create_object(env, &request_option);
 *
 * // get HttpDataType
 * napi_value http_data_type = nullptr;
 * napi_get_named_property(env, http_module, "HttpDataType", &http_data_type);
 * napi_value http_data_type_array_buffer = nullptr;
 * napi_get_named_property(env, http_data_type, "ARRAY_BUFFER",
 *                        &http_data_type_array_buffer);
 * napi_set_named_property(env, request_option, "expectDataType",
 *                        http_data_type_array_buffer);
 *
 * // create the callback function
 * auto napi_callback_func = [](napi_env env, napi_callback_info info) {
 *  size_t argc = 2;
 *  napi_value args[2] = {nullptr};
 *  void* data = nullptr;
 *  napi_get_cb_info(env, info, &argc, args, nullptr, &data);
 *
 *  // handle error
 *  napi_valuetype type;
 *  napi_typeof(env, args[0], &type);
 *  if (type == napi_object) {
 *    PrintProperties(env, args[0], "args[0]");
 *    napi_value error_code = nullptr;
 *    napi_get_named_property(env, args[0], "code", &error_code);
 *    int32_t error_code_value = 0;
 *    napi_get_value_int32(env, error_code, &error_code_value);
 *  }
 *
 *  // handle array buffer
 *  PrintProperties(env, args[1], "args[1]");
 *  napi_value array_buffer = nullptr;
 *  napi_get_named_property(env, args[1], "result", &array_buffer);
 *  napi_typeof(env, array_buffer, &type);
 *
 *  bool is_array_buffer = false;
 *  napi_is_arraybuffer(env, array_buffer, &is_array_buffer);
 *
 *  if (is_array_buffer) {
 *    size_t buffer_size;
 *    void* buffer_data;
 *    napi_get_arraybuffer_info(env, array_buffer, &buffer_data, &buffer_size);
 *
 *    auto data = DataHolder::MakeWithCopy(buffer_data, buffer_size);
 *  }
 *  return napi_value();
 * };
 * napi_value callback_func = nullptr;
 * napi_create_function(env, nullptr, NAPI_AUTO_LENGTH, napi_callback_func,
 * this, &callback_func);
 *
 * size_t argc = 3;
 * napi_value argv[3] = {napi_value_url, request_option, callback_func};
 *
 * napi_value request_func = nullptr;
 * ret = napi_get_named_property(env, http_request, "request", &request_func);
 * DCHECK(ret == napi_ok);
 *
 * napi_valuetype type;
 * napi_typeof(env, request_func, &type);
 *
 * ret =
 *    napi_call_function(env, http_request, request_func, argc, argv, nullptr);
 * DCHECK(ret == napi_ok);
 * }
 */
BASE_EXPORT napi_env GetJSThreadNapiEnv();

void InitializationNapiEnvForCurrentThread(napi_env env);

}  // namespace harmony
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_HARMONY_HARMONY_NAPI_ENV_HOLDER_H_
