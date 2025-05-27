// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/harmony/value_impl_napi.h"

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/base/js_constants.h"

namespace lynx {
namespace pub {

ValueImplNapi::ValueImplNapi(napi_env env, napi_value value)
    : Value(ValueBackendType::ValueBackendTypeNapi), env_(env) {
  napi_create_reference(env, value, 1, &backend_value_);
  napi_typeof(env_, value, &type_);
}

ValueImplNapi::~ValueImplNapi() { napi_delete_reference(env_, backend_value_); }

int64_t ValueImplNapi::Type() const { return static_cast<int64_t>(type_); }

bool ValueImplNapi::IsUndefined() const { return type_ == napi_undefined; }

bool ValueImplNapi::IsBool() const { return type_ == napi_boolean; }

bool ValueImplNapi::IsInt32() const { return false; }
bool ValueImplNapi::IsInt64() const { return type_ == napi_bigint; }
bool ValueImplNapi::IsUInt32() const { return false; }
bool ValueImplNapi::IsUInt64() const { return false; }

bool ValueImplNapi::IsDouble() const { return type_ == napi_number; }

bool ValueImplNapi::IsNumber() const { return type_ == napi_number; }

bool ValueImplNapi::IsNil() const { return type_ == napi_null; }

bool ValueImplNapi::IsString() const { return type_ == napi_string; }

bool ValueImplNapi::IsArray() const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::IsArray(env_, obj);
}

bool ValueImplNapi::IsArrayBuffer() const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::IsArrayBuffer(env_, obj);
}

bool ValueImplNapi::IsMap() const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return !base::NapiUtil::IsArray(env_, obj) &&
         !base::NapiUtil::IsArrayBuffer(env_, obj);
}

bool ValueImplNapi::IsFunction() const { return false; }

bool ValueImplNapi::Bool() const {
  DCHECK(IsBool());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToBoolean(env_, obj);
}

double ValueImplNapi::Double() const {
  DCHECK(IsDouble());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToDouble(env_, obj);
}

int32_t ValueImplNapi::Int32() const {
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToInt32(env_, obj);
}

uint32_t ValueImplNapi::UInt32() const {
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToUInt32(env_, obj);
}

int64_t ValueImplNapi::Int64() const {
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToBigInt64(env_, obj);
}

uint64_t ValueImplNapi::UInt64() const {
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToBigUInt64(env_, obj);
}

double ValueImplNapi::Number() const {
  DCHECK(IsNumber());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return base::NapiUtil::ConvertToDouble(env_, obj);
}

uint8_t* ValueImplNapi::ArrayBuffer() const {
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  size_t length;
  void* data;
  napi_status status = napi_get_arraybuffer_info(env_, obj, &data, &length);
  if (status != napi_ok || data == nullptr) {
    LOGE("Fail to get array buffer ");
    return nullptr;
  }
  return reinterpret_cast<uint8_t*>(data);
}

const std::string& ValueImplNapi::str() const {
  DCHECK(IsString());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  if (cached_str_.empty()) {
    const_cast<ValueImplNapi*>(this)->cached_str_ =
        base::NapiUtil::ConvertToString(env_, obj);
  }
  return cached_str_;
}

int ValueImplNapi::Length() const {
  if (type_ != napi_object) {
    return 0;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  uint32_t length = 0;
  if (base::NapiUtil::IsArrayBuffer(env_, obj)) {
    void* data;
    size_t size;
    napi_get_arraybuffer_info(env_, obj, &data, &size);
    length = static_cast<uint32_t>(size);
  } else if (base::NapiUtil::IsArray(env_, obj)) {
    napi_get_array_length(env_, obj, &length);
  } else {
    napi_value object_keys;
    napi_get_all_property_names(env_, obj, napi_key_own_only,
                                napi_key_enumerable,
                                napi_key_numbers_to_strings, &object_keys);
    napi_get_array_length(env_, object_keys, &length);
  }
  return static_cast<int>(length);
}

bool ValueImplNapi::IsEqual(const Value& value) const { return false; }

void ValueImplNapi::ForeachArray(pub::ForeachArrayFunc func) const {
  if (type_ != napi_object) {
    return;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  if (!base::NapiUtil::IsArray(env_, obj)) {
    return;
  }
  uint32_t length;
  napi_get_array_length(env_, obj, &length);
  for (uint32_t i = 0; i < length; i++) {
    napi_value item;
    napi_get_element(env_, obj, i, &item);
    func(static_cast<int64_t>(i), ValueImplNapi(env_, item));
  }
}

void ValueImplNapi::ForeachMap(pub::ForeachMapFunc func) const {
  if (type_ != napi_object) {
    return;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  if (base::NapiUtil::IsArrayBuffer(env_, obj) ||
      base::NapiUtil::IsArray(env_, obj)) {
    return;
  }
  napi_value object_keys;
  napi_get_all_property_names(env_, obj, napi_key_own_only, napi_key_enumerable,
                              napi_key_numbers_to_strings, &object_keys);
  uint32_t length;
  napi_get_array_length(env_, object_keys, &length);
  for (uint32_t i = 0; i < length; i++) {
    napi_value k;
    napi_get_element(env_, object_keys, i, &k);
    napi_value v;
    napi_get_property(env_, obj, k, &v);
    func(ValueImplNapi(env_, k), ValueImplNapi(env_, v));
  }
}

std::unique_ptr<Value> ValueImplNapi::GetValueAtIndex(uint32_t idx) const {
  if (type_ != napi_object) {
    return nullptr;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  if (!base::NapiUtil::IsArray(env_, obj)) {
    return nullptr;
  }
  uint32_t length;
  napi_get_array_length(env_, obj, &length);
  if (idx >= length) {
    return nullptr;
  }
  napi_value item;
  napi_get_element(env_, obj, idx, &item);
  return std::make_unique<ValueImplNapi>(env_, item);
}

bool ValueImplNapi::Erase(uint32_t idx) const { return false; }

std::unique_ptr<Value> ValueImplNapi::GetValueForKey(
    const std::string& key) const {
  if (type_ != napi_object) {
    return nullptr;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  if (base::NapiUtil::IsArrayBuffer(env_, obj) ||
      base::NapiUtil::IsArray(env_, obj)) {
    return nullptr;
  }
  napi_status status;
  napi_value value;
  status = napi_get_named_property(env_, obj, key.c_str(), &value);
  if (status != napi_ok) {
    return nullptr;
  }
  return std::make_unique<ValueImplNapi>(env_, value);
}

bool ValueImplNapi::Erase(const std::string& key) const { return false; }

bool ValueImplNapi::Contains(const std::string& key) const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  if (base::NapiUtil::IsArrayBuffer(env_, obj) ||
      base::NapiUtil::IsArray(env_, obj)) {
    return false;
  }
  napi_status status;
  napi_value value;
  status = napi_get_named_property(env_, obj, key.c_str(), &value);
  if (status != napi_ok) {
    return false;
  }
  return true;
}

bool ValueImplNapi::PushValueToArray(const Value& value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_ref value_ref =
      reinterpret_cast<const ValueImplNapi*>(&value)->backend_value_;
  napi_value value_obj;
  napi_get_reference_value(env_, value_ref, &value_obj);
  return PushNapiValueToArray(obj, value_obj);
}

bool ValueImplNapi::PushValueToArray(std::unique_ptr<Value> value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_ref value_ref =
      reinterpret_cast<const ValueImplNapi*>(value.get())->backend_value_;
  napi_value value_obj;
  napi_get_reference_value(env_, value_ref, &value_obj);
  return PushNapiValueToArray(obj, value_obj);
}

bool ValueImplNapi::PushNullToArray() {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  napi_get_null(env_, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value,
                                           size_t length) {
  DCHECK(IsArray());
  napi_value result;
  void* data;
  napi_create_arraybuffer(env_, length, &data, &result);
  memcpy(data, value.get(), length);
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushStringToArray(const std::string& value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  napi_create_string_utf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
  return PushNapiValueToArray(obj, result);
  ;
}

bool ValueImplNapi::PushBigIntToArray(const std::string& value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  auto int_value =
      static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
  napi_create_bigint_int64(env_, int_value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushBoolToArray(bool value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  napi_get_boolean(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushDoubleToArray(double value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  napi_create_double(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushInt32ToArray(int32_t value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  napi_create_int32(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushUInt32ToArray(uint32_t value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  napi_create_uint32(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushInt64ToArray(int64_t value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  if (value < piper::kMinJavaScriptNumber ||
      value > piper::kMaxJavaScriptNumber) {
    napi_create_bigint_int64(env_, value, &result);
  } else {
    napi_create_int64(env_, value, &result);
  }
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushUInt64ToArray(uint64_t value) {
  DCHECK(IsArray());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value result;
  if (value > piper::kMaxJavaScriptNumber) {
    napi_create_bigint_int64(env_, value, &result);
  } else {
    napi_create_int64(env_, value, &result);
  }
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapi::PushValueToMap(const std::string& key, const Value& value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_ref value_ref =
      reinterpret_cast<const ValueImplNapi*>(&value)->backend_value_;
  napi_value result;
  napi_get_reference_value(env_, value_ref, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushValueToMap(const std::string& key,
                                   std::unique_ptr<Value> value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_ref value_ref =
      reinterpret_cast<const ValueImplNapi*>(value.get())->backend_value_;
  napi_value result;
  napi_get_reference_value(env_, value_ref, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushNullToMap(const std::string& key) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  napi_get_null(env_, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushArrayBufferToMap(const std::string& key,
                                         std::unique_ptr<uint8_t[]> value,
                                         size_t length) {
  DCHECK(IsMap());
  napi_value result;
  void* data;
  napi_create_arraybuffer(env_, length, &data, &result);
  memcpy(data, value.get(), length);
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushStringToMap(const std::string& key,
                                    const std::string& value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  napi_create_string_utf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushBigIntToMap(const std::string& key,
                                    const std::string& value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  auto int_value =
      static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
  napi_create_bigint_int64(env_, int_value, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushBoolToMap(const std::string& key, bool value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  napi_get_boolean(env_, value, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushDoubleToMap(const std::string& key, double value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  napi_create_double(env_, value, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushInt32ToMap(const std::string& key, int32_t value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  napi_create_int32(env_, value, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushUInt32ToMap(const std::string& key, uint32_t value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  napi_create_uint32(env_, value, &result);
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushInt64ToMap(const std::string& key, int64_t value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  if (value < piper::kMinJavaScriptNumber ||
      value > piper::kMaxJavaScriptNumber) {
    napi_create_bigint_int64(env_, value, &result);
  } else {
    napi_create_int64(env_, value, &result);
  }
  napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapi::PushUInt64ToMap(const std::string& key, uint64_t value) {
  DCHECK(IsMap());
  napi_value obj;
  napi_get_reference_value(env_, backend_value_, &obj);
  napi_value k;
  napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  if (value > piper::kMaxJavaScriptNumber) {
    napi_create_bigint_int64(env_, value, &result);
  } else {
    napi_create_int64(env_, value, &result);
  }
  napi_set_property(env_, obj, k, result);
  return true;
}

// private
bool ValueImplNapi::PushNapiValueToArray(napi_value array, napi_value value) {
  napi_status status;
  status = napi_set_element(env_, array, current_array_index_, value);
  if (status != napi_ok) {
    return false;
  }
  current_array_index_++;
  return true;
}

// PubValueFactoryNapi
std::unique_ptr<Value> PubValueFactoryNapi::CreateArray() {
  napi_value result;
  napi_create_array(env_, &result);
  return std::make_unique<ValueImplNapi>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapi::CreateMap() {
  napi_value result;
  napi_create_object(env_, &result);
  return std::make_unique<ValueImplNapi>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapi::CreateBool(bool value) {
  napi_value result;
  napi_get_boolean(env_, value, &result);
  return std::make_unique<ValueImplNapi>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapi::CreateNumber(double value) {
  napi_value result;
  napi_create_double(env_, value, &result);
  return std::make_unique<ValueImplNapi>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapi::CreateString(
    const std::string& value) {
  napi_value result;
  napi_create_string_utf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
  return std::make_unique<ValueImplNapi>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapi::CreateArrayBuffer(
    std::unique_ptr<uint8_t[]> value, size_t length) {
  napi_value result;
  void* data;
  napi_create_arraybuffer(env_, length, &data, &result);
  memcpy(data, value.get(), length);
  return std::make_unique<ValueImplNapi>(env_, result);
}

// ValueUtilsNapi
napi_value ValueUtilsNapi::ConvertPubValueToNapiValue(napi_env env,
                                                      const Value& value) {
  if (value.backend_type() ==
      lynx::pub::ValueBackendType::ValueBackendTypeNapi) {
    napi_ref ref =
        (reinterpret_cast<const ValueImplNapi*>(&value))->backend_value();
    napi_value obj;
    napi_get_reference_value(env, ref, &obj);
    return obj;
  }
  napi_value result = nullptr;
  if (value.IsNil()) {
    napi_get_null(env, &result);
  } else if (value.IsBool()) {
    napi_get_boolean(env, value.Bool(), &result);
  } else if (value.IsString()) {
    napi_create_string_utf8(env, value.str().c_str(), NAPI_AUTO_LENGTH,
                            &result);
  } else if (value.IsInt32()) {
    napi_create_int32(env, value.Int32(), &result);
  } else if (value.IsUInt32()) {
    napi_create_uint32(env, value.UInt32(), &result);
  } else if (value.IsInt64()) {
    int64_t i = value.Int64();
    // When integer beyond limit, use BigInt Object to define it
    if (i < piper::kMinJavaScriptNumber || i > piper::kMaxJavaScriptNumber) {
      napi_create_bigint_int64(env, i, &result);
    } else {
      napi_create_int64(env, i, &result);
    }
  } else if (value.IsUInt64()) {
    uint64_t u = value.UInt64();
    if (u > piper::kMaxJavaScriptNumber) {
      napi_create_bigint_int64(env, u, &result);
    } else {
      napi_create_int64(env, u, &result);
    }
  } else if (value.IsNumber()) {
    napi_create_double(env, value.Number(), &result);
  } else if (value.IsMap()) {
    result = ConvertPubValueToNapiObject(env, value);
  } else if (value.IsArray()) {
    result = ConvertPubValueToNapiArray(env, value);
  } else if (value.IsUndefined()) {
    napi_get_undefined(env, &result);
  } else {
    LOGE("ValueUtilsNapi, unknown type :" << value.Type());
  }
  return result;
}

napi_value ValueUtilsNapi::ConvertPubValueToNapiArray(napi_env env,
                                                      const Value& value) {
  napi_value result;
  napi_create_array(env, &result);
  value.ForeachArray(
      [env, result](int64_t index, const lynx::pub::Value& value) {
        napi_set_element(env, result, index,
                         ConvertPubValueToNapiValue(env, value));
      });
  return result;
}

napi_value ValueUtilsNapi::ConvertPubValueToNapiObject(napi_env env,
                                                       const Value& value) {
  napi_value result;
  napi_create_object(env, &result);
  value.ForeachMap([env, result](const lynx::pub::Value& key,
                                 const lynx::pub::Value& value) {
    napi_value k;
    napi_create_string_utf8(env, key.str().c_str(), NAPI_AUTO_LENGTH, &k);
    napi_set_property(env, result, k, ConvertPubValueToNapiValue(env, value));
  });
  return result;
}

}  // namespace pub
}  // namespace lynx
