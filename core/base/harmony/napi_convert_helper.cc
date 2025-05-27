// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/harmony/napi_convert_helper.h"

#include <memory>
#include <utility>

#include "base/include/platform/harmony/napi_util.h"
#include "base/include/value/byte_array.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/js_constants.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace base {
void create_lepus_value_from_js_object(napi_env env, napi_value obj,
                                       lepus_value& result) {
  napi_valuetype type;
  napi_typeof(env, obj, &type);
  switch (type) {
    case napi_undefined: {
      result.SetUndefined();
    } break;
    case napi_null: {
      result.SetNil();
    } break;
    case napi_bigint: {
      int64_t i = base::NapiUtil::ConvertToBigInt64(env, obj);
      result = lepus_value(i);
    } break;
    case napi_number: {
      double d = base::NapiUtil::ConvertToDouble(env, obj);
      result = lepus_value(d);
    } break;
    case napi_string: {
      std::string s = base::NapiUtil::ConvertToString(env, obj);
      result = lepus_value(s);
    } break;
    case napi_boolean: {
      bool b = base::NapiUtil::ConvertToBoolean(env, obj);
      result = lepus_value(b);
    } break;
    case napi_object: {
      if (base::NapiUtil::IsArrayBuffer(env, obj)) {
        std::unique_ptr<uint8_t[]> buffer{nullptr};
        size_t length{0};
        bool ok =
            base::NapiUtil::ConvertToArrayBuffer(env, obj, buffer, length);
        if (ok) {
          result =
              lepus_value(lepus::ByteArray::Create(std::move(buffer), length));
        } else {
          LOGE("Fail to convert array buffer");
          result.SetUndefined();
        }
      } else if (base::NapiUtil::IsArray(env, obj)) {
        auto arr = lepus::CArray::Create();
        uint32_t length;
        napi_get_array_length(env, obj, &length);
        for (uint32_t i = 0; i < length; i++) {
          napi_value item;
          napi_get_element(env, obj, i, &item);
          lepus_value value;
          create_lepus_value_from_js_object(env, item, value);
          arr->push_back(value);
        }
        result = lepus_value(arr);
      } else {
        auto dict = lepus::Dictionary::Create();
        napi_value object_keys;
        napi_get_all_property_names(env, obj, napi_key_own_only,
                                    napi_key_enumerable,
                                    napi_key_numbers_to_strings, &object_keys);
        uint32_t length;
        napi_get_array_length(env, object_keys, &length);
        for (uint32_t i = 0; i < length; i++) {
          napi_value k;
          napi_get_element(env, object_keys, i, &k);
          napi_value v;
          napi_get_property(env, obj, k, &v);
          std::string key = base::NapiUtil::ConvertToShortString(env, k);
          lepus_value value;
          create_lepus_value_from_js_object(env, v, value);
          dict->SetValue(key, value);
        }
        result = lepus_value(dict);
      }
    } break;
    default:
      result.SetUndefined();
      break;
  }
}

lepus_value NapiConvertHelper::JSONToLepusValue(napi_env env, napi_value obj) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, NAPI_CONVERT_HELPER_JSON_TO_LEPUS_VALUE);
  napi_valuetype type;
  napi_typeof(env, obj, &type);
  if (type == napi_valuetype::napi_string) {
    std::string s = base::NapiUtil::ConvertToString(env, obj);
    return lepus::jsonValueTolepusValue(s.c_str());
  } else if (type == napi_valuetype::napi_object) {
    lepus_value result;
    create_lepus_value_from_js_object(env, obj, result);
    return result;
  } else {
    LOGD("JSONToLepusValue:: type should be string or object");
  }
  return lepus_value();
}

lepus_value NapiConvertHelper::ConvertToLepusValue(napi_env env,
                                                   napi_value obj) {
  lepus_value result;
  create_lepus_value_from_js_object(env, obj, result);
  return result;
}

void NapiConvertHelper::AssembleArray(napi_env env, napi_value array,
                                      uint32_t index,
                                      const lepus::Value& value) {
  napi_value result = NapiConvertHelper::CreateNapiValue(env, value);
  napi_set_element(env, array, index, result);
}

void NapiConvertHelper::AssembleMap(napi_env env, napi_value object,
                                    const char* key,
                                    const lepus::Value& value) {
  napi_value k;
  napi_create_string_latin1(env, key, NAPI_AUTO_LENGTH, &k);
  napi_value result = NapiConvertHelper::CreateNapiValue(env, value);
  napi_set_property(env, object, k, result);
}

napi_value NapiConvertHelper::CreateNapiValue(napi_env env,
                                              const lepus::Value& value) {
  napi_value result = nullptr;
  if (value.IsNil()) {
    napi_get_null(env, &result);
  } else if (value.IsString()) {
    napi_create_string_utf8(env, value.CString(), NAPI_AUTO_LENGTH, &result);
  } else if (value.IsInt32()) {
    napi_create_int32(env, value.Int32(), &result);
  } else if (value.IsInt64()) {
    int64_t i = value.Int64();
    // When integer beyond limit, use BigInt Object to define it
    if (i < piper::kMinJavaScriptNumber || i > piper::kMaxJavaScriptNumber) {
      napi_create_bigint_int64(env, i, &result);
    } else {
      napi_create_int64(env, i, &result);
    }
  } else if (value.IsUInt32()) {
    napi_create_uint32(env, value.UInt32(), &result);
  } else if (value.IsUInt64()) {
    uint64_t u = value.UInt64();
    if (u > piper::kMaxJavaScriptNumber) {
      napi_create_bigint_int64(env, u, &result);
    } else {
      napi_create_int64(env, u, &result);
    }
  } else if (value.IsNumber()) {
    napi_create_double(env, value.Number(), &result);
  } else if (value.IsArrayOrJSArray()) {
    napi_create_array(env, &result);
    tasm::ForEachLepusValue(
        value, [&env, &result](const lepus::Value& i, const lepus::Value& v) {
          AssembleArray(env, result, i.Int64(), v);
        });
  } else if (value.IsObject()) {
    napi_create_object(env, &result);
    tasm::ForEachLepusValue(
        value, [&env, &result](const lepus::Value& k, const lepus::Value& v) {
          AssembleMap(env, result, k.ToString().c_str(), v);
        });
  } else if (value.IsByteArray()) {
    const auto& buffer = value.ByteArray();
    void* result_data;
    napi_create_buffer_copy(env, buffer->GetLength(), buffer->GetPtr(),
                            &result_data, &result);
  } else if (value.IsBool()) {
    napi_get_boolean(env, value.Bool(), &result);
  } else if (value.IsUndefined()) {
    napi_get_undefined(env, &result);
  } else {
    LOGE("NapiConvertHelper, unknown type :" << value.Type());
  }
  return result;
}

void copy_napi_value(napi_env env, napi_value obj, napi_value& result) {
  napi_valuetype type;
  napi_typeof(env, obj, &type);
  switch (type) {
    case napi_undefined: {
      napi_get_undefined(env, &result);
    } break;
    case napi_null: {
      napi_get_null(env, &result);
    } break;
    case napi_number: {
      double d = base::NapiUtil::ConvertToDouble(env, obj);
      napi_create_double(env, d, &result);
    } break;
    case napi_bigint: {
      int64_t i = base::NapiUtil::ConvertToBigInt64(env, obj);
      napi_create_bigint_int64(env, i, &result);
    } break;
    case napi_string: {
      std::string s = base::NapiUtil::ConvertToString(env, obj);
      napi_create_string_utf8(env, s.c_str(), s.length(), &result);
    } break;
    case napi_boolean: {
      bool b = base::NapiUtil::ConvertToBoolean(env, obj);
      napi_get_boolean(env, b, &result);
    } break;
    case napi_object: {
      if (base::NapiUtil::IsArrayBuffer(env, obj)) {
        size_t length{0};
        void* data{nullptr};
        napi_get_arraybuffer_info(env, obj, &data, &length);
        void* result_data;
        napi_create_buffer_copy(env, length, data, &result_data, &result);
      } else if (base::NapiUtil::IsArray(env, obj)) {
        uint32_t length;
        napi_get_array_length(env, obj, &length);
        napi_create_array_with_length(env, length, &result);
        for (uint32_t i = 0; i < length; i++) {
          napi_value item;
          napi_get_element(env, obj, i, &item);
          napi_value value;
          copy_napi_value(env, item, value);
          napi_set_element(env, result, i, value);
        }
      } else {
        napi_create_object(env, &result);
        napi_value object_keys;
        napi_get_all_property_names(env, obj, napi_key_own_only,
                                    napi_key_enumerable,
                                    napi_key_numbers_to_strings, &object_keys);
        uint32_t length;
        napi_get_array_length(env, object_keys, &length);
        for (uint32_t i = 0; i < length; i++) {
          napi_value k;
          napi_get_element(env, object_keys, i, &k);
          napi_value v;
          napi_get_property(env, obj, k, &v);
          napi_value key;
          copy_napi_value(env, k, key);
          napi_value value;
          copy_napi_value(env, v, value);
          napi_set_property(env, result, key, value);
        }
      }
    } break;
    default:
      napi_get_undefined(env, &result);
      break;
  }
}

napi_value NapiConvertHelper::CloneNapiValue(napi_env env, napi_value obj) {
  napi_value result;
  copy_napi_value(env, obj, result);
  return result;
}

}  // namespace base
}  // namespace lynx
