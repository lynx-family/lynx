// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/napi/value_impl_napi_primjs.h"

#include "base/include/log/logging.h"
#include "core/base/js_constants.h"
#include "core/value_wrapper/napi/napi_util_primjs.h"
#include "core/value_wrapper/napi/value_impl_opaque_napi_primjs.h"

#ifdef USE_PRIMJS_NAPI
#include <cstring>

#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace pub {
namespace {

// This is a workaround. Since NAPI does not support creating references for
// non-Object types.
// Therefore, we first create a wrapper of type Object, and then place the value
// inside the wrapper.
// TODO: This wrapper will affect performance and needs to be optimized
// later.
napi_status GetValueFromReference(napi_env env, napi_ref ref,
                                  napi_value* result) {
  napi_value wrapper;
  env->napi_get_reference_value(env, ref, &wrapper);
  return env->napi_get_element(env, wrapper, 0, result);
}

}  // namespace

ValueImplNapiPrimJS::ValueImplNapiPrimJS(napi_env env, napi_value value)
    : Value(ValueBackendType::ValueBackendTypeNapiPrimJS), env_(env) {
  napi_value wrapper;
  env->napi_create_array(env, &wrapper);
  env->napi_set_element(env, wrapper, 0, value);
  env->napi_create_reference(env, wrapper, 1, &backend_value_);
  env->napi_typeof(env_, value, &type_);
}

ValueImplNapiPrimJS::~ValueImplNapiPrimJS() {
  env_->napi_delete_reference(env_, backend_value_);
}

int64_t ValueImplNapiPrimJS::Type() const {
  return static_cast<int64_t>(type_);
}

bool ValueImplNapiPrimJS::IsUndefined() const {
  return type_ == napi_undefined;
}

bool ValueImplNapiPrimJS::IsBool() const { return type_ == napi_boolean; }

bool ValueImplNapiPrimJS::IsInt32() const { return false; }
bool ValueImplNapiPrimJS::IsInt64() const { return type_ == napi_bigint; }
bool ValueImplNapiPrimJS::IsUInt32() const { return false; }
bool ValueImplNapiPrimJS::IsUInt64() const { return false; }

bool ValueImplNapiPrimJS::IsDouble() const { return type_ == napi_number; }

bool ValueImplNapiPrimJS::IsNumber() const { return type_ == napi_number; }

bool ValueImplNapiPrimJS::IsNil() const { return type_ == napi_null; }

bool ValueImplNapiPrimJS::IsString() const { return type_ == napi_string; }

bool ValueImplNapiPrimJS::IsArray() const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::IsArray(env_, obj);
}

bool ValueImplNapiPrimJS::IsArrayBuffer() const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::IsArrayBuffer(env_, obj);
}

bool ValueImplNapiPrimJS::IsMap() const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return !value::NapiUtil::IsArray(env_, obj) &&
         !value::NapiUtil::IsArrayBuffer(env_, obj);
}

bool ValueImplNapiPrimJS::IsFunction() const { return false; }

bool ValueImplNapiPrimJS::Bool() const {
  DCHECK(IsBool());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::ConvertToBoolean(env_, obj);
}

double ValueImplNapiPrimJS::Double() const {
  DCHECK(IsDouble());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::ConvertToDouble(env_, obj);
}

int32_t ValueImplNapiPrimJS::Int32() const {
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::ConvertToInt32(env_, obj);
}

uint32_t ValueImplNapiPrimJS::UInt32() const {
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::ConvertToUInt32(env_, obj);
}

int64_t ValueImplNapiPrimJS::Int64() const {
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  // TODO: Support bigint, primjs napi doesn't support bigint now.
  return value::NapiUtil::ConvertToInt64(env_, obj);
}

uint64_t ValueImplNapiPrimJS::UInt64() const {
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  // TODO: Support bigint, primjs napi doesn't support bigint now.
  return value::NapiUtil::ConvertToInt64(env_, obj);
}

double ValueImplNapiPrimJS::Number() const {
  DCHECK(IsNumber());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return value::NapiUtil::ConvertToDouble(env_, obj);
}

uint8_t* ValueImplNapiPrimJS::ArrayBuffer() const {
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  size_t length;
  void* data;
  napi_status status =
      env_->napi_get_arraybuffer_info(env_, obj, &data, &length);
  if (status != napi_ok || data == nullptr) {
    LOGE("Fail to get array buffer ");
    return nullptr;
  }
  return reinterpret_cast<uint8_t*>(data);
}

const std::string& ValueImplNapiPrimJS::str() const {
  DCHECK(IsString());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  if (cached_str_.empty()) {
    const_cast<ValueImplNapiPrimJS*>(this)->cached_str_ =
        value::NapiUtil::ConvertToString(env_, obj);
  }
  return cached_str_;
}

int ValueImplNapiPrimJS::Length() const {
  if (type_ != napi_object) {
    return 0;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  uint32_t length = 0;
  if (value::NapiUtil::IsArrayBuffer(env_, obj)) {
    void* data;
    size_t size;
    env_->napi_get_arraybuffer_info(env_, obj, &data, &size);
    length = static_cast<uint32_t>(size);
  } else if (value::NapiUtil::IsArray(env_, obj)) {
    env_->napi_get_array_length(env_, obj, &length);
  } else {
    napi_value object_keys;
    env_->napi_get_property_names(env_, obj, &object_keys);
    env_->napi_get_array_length(env_, object_keys, &length);
  }
  return static_cast<int>(length);
}

bool ValueImplNapiPrimJS::IsEqual(const Value& value) const {
  if (value.backend_type() !=
      pub::ValueBackendType::ValueBackendTypeNapiPrimJS) {
    return false;
  }
  napi_value raw_val;
  GetValueFromReference(env_, backend_value_, &raw_val);
  Napi::Value napi_val(env_, raw_val);
  return napi_val.Equals(Napi::Value(
      env_,
      reinterpret_cast<const ValueImplNapiPrimJS*>(&value)->backend_value()));
}

void ValueImplNapiPrimJS::ForeachArray(pub::ForeachArrayFunc func) const {
  if (type_ != napi_object) {
    return;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  if (!value::NapiUtil::IsArray(env_, obj)) {
    return;
  }
  uint32_t length;
  env_->napi_get_array_length(env_, obj, &length);
  for (uint32_t i = 0; i < length; i++) {
    napi_value item;
    env_->napi_get_element(env_, obj, i, &item);
    func(static_cast<int64_t>(i), ValueImplNapiPrimJS(env_, item));
  }
}

void ValueImplNapiPrimJS::ForeachMap(pub::ForeachMapFunc func) const {
  if (type_ != napi_object) {
    return;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  if (value::NapiUtil::IsArrayBuffer(env_, obj) ||
      value::NapiUtil::IsArray(env_, obj)) {
    return;
  }
  napi_value object_keys;
  env_->napi_get_property_names(env_, obj, &object_keys);
  uint32_t length;
  env_->napi_get_array_length(env_, object_keys, &length);
  for (uint32_t i = 0; i < length; i++) {
    napi_value k;
    env_->napi_get_element(env_, object_keys, i, &k);
    napi_value v;
    env_->napi_get_property(env_, obj, k, &v);
    func(ValueImplNapiPrimJS(env_, k), ValueImplNapiPrimJS(env_, v));
  }
}

std::unique_ptr<Value> ValueImplNapiPrimJS::GetValueAtIndex(
    uint32_t idx) const {
  if (type_ != napi_object) {
    return nullptr;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  if (!value::NapiUtil::IsArray(env_, obj)) {
    return nullptr;
  }
  uint32_t length;
  env_->napi_get_array_length(env_, obj, &length);
  if (idx >= length) {
    return nullptr;
  }
  napi_value item;
  auto status = env_->napi_get_element(env_, obj, idx, &item);
  if (status != napi_ok) {
    return nullptr;
  }
  return std::make_unique<ValueImplNapiPrimJS>(env_, item);
}

bool ValueImplNapiPrimJS::Erase(uint32_t idx) const { return false; }

std::unique_ptr<Value> ValueImplNapiPrimJS::GetValueForKey(
    const std::string& key) const {
  if (type_ != napi_object) {
    return nullptr;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  if (value::NapiUtil::IsArrayBuffer(env_, obj) ||
      value::NapiUtil::IsArray(env_, obj)) {
    return nullptr;
  }
  napi_status status;
  napi_value value;
  status = env_->napi_get_named_property(env_, obj, key.c_str(), &value);
  if (status != napi_ok) {
    return nullptr;
  }
  return std::make_unique<ValueImplNapiPrimJS>(env_, value);
}

bool ValueImplNapiPrimJS::Erase(const std::string& key) const { return false; }

bool ValueImplNapiPrimJS::Contains(const std::string& key) const {
  if (type_ != napi_object) {
    return false;
  }
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  if (value::NapiUtil::IsArrayBuffer(env_, obj) ||
      value::NapiUtil::IsArray(env_, obj)) {
    return false;
  }
  napi_status status;
  napi_value value;
  status = env_->napi_get_named_property(env_, obj, key.c_str(), &value);
  if (status != napi_ok) {
    return false;
  }
  return true;
}

bool ValueImplNapiPrimJS::PushValueToArray(const Value& value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value value_obj =
      reinterpret_cast<const ValueImplNapiPrimJS*>(&value)->backend_value();
  return PushNapiValueToArray(obj, value_obj);
}

bool ValueImplNapiPrimJS::PushValueToArray(std::unique_ptr<Value> value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value value_obj =
      reinterpret_cast<const ValueImplNapiPrimJS*>(value.get())
          ->backend_value();
  return PushNapiValueToArray(obj, value_obj);
}

bool ValueImplNapiPrimJS::PushNullToArray() {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  env_->napi_get_null(env_, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushArrayBufferToArray(
    std::unique_ptr<uint8_t[]> value, size_t length) {
  DCHECK(IsArray());
  napi_value result;
  void* data = nullptr;
  // TODO: Use napi_create_external_arraybuffer to optimize performance.
  env_->napi_create_arraybuffer(env_, length, &data, &result);
  memcpy(data, value.get(), length);
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushStringToArray(const std::string& value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  env_->napi_create_string_utf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
  return PushNapiValueToArray(obj, result);
  ;
}

bool ValueImplNapiPrimJS::PushBigIntToArray(const std::string& value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  auto int_value =
      static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
  if (int_value < piper::kMinJavaScriptNumber ||
      int_value > piper::kMaxJavaScriptNumber) {
    // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
    LOGE(
        "PushBigIntToArray error! PrimJS NAPI doesn't support bigint now. The "
        "bigint value is "
        << value);
    return false;
  }
  env_->napi_create_int64(env_, int_value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushBoolToArray(bool value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  env_->napi_get_boolean(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushDoubleToArray(double value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  env_->napi_create_double(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushInt32ToArray(int32_t value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  env_->napi_create_int32(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushUInt32ToArray(uint32_t value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  env_->napi_create_uint32(env_, value, &result);
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushInt64ToArray(int64_t value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  if (value < piper::kMinJavaScriptNumber ||
      value > piper::kMaxJavaScriptNumber) {
    // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
    LOGE(
        "PushInt64ToArray error! PrimJS NAPI doesn't support bigint now. The "
        "bigint value is "
        << value);
    return false;
  } else {
    env_->napi_create_int64(env_, value, &result);
  }
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushUInt64ToArray(uint64_t value) {
  DCHECK(IsArray());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value result;
  if (value > piper::kMaxJavaScriptNumber) {
    // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
    LOGE(
        "PushUInt64ToArray error! PrimJS NAPI doesn't support bigint now. The "
        "bigint value is "
        << value);
    return false;
  } else {
    env_->napi_create_int64(env_, value, &result);
  }
  return PushNapiValueToArray(obj, result);
}

bool ValueImplNapiPrimJS::PushValueToMap(const std::string& key,
                                         const Value& value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result =
      reinterpret_cast<const ValueImplNapiPrimJS*>(&value)->backend_value();
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushValueToMap(const std::string& key,
                                         std::unique_ptr<Value> value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result = reinterpret_cast<const ValueImplNapiPrimJS*>(value.get())
                          ->backend_value();
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushNullToMap(const std::string& key) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  env_->napi_get_null(env_, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushArrayBufferToMap(const std::string& key,
                                               std::unique_ptr<uint8_t[]> value,
                                               size_t length) {
  DCHECK(IsMap());
  napi_value result;
  void* data;
  env_->napi_create_arraybuffer(env_, length, &data, &result);
  memcpy(data, value.get(), length);
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushStringToMap(const std::string& key,
                                          const std::string& value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  env_->napi_create_string_utf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushBigIntToMap(const std::string& key,
                                          const std::string& value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  auto int_value =
      static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
  if (int_value < piper::kMinJavaScriptNumber ||
      int_value > piper::kMaxJavaScriptNumber) {
    // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
    LOGE(
        "PushBigIntToMap error! PrimJS NAPI doesn't support bigint now. The "
        "bigint value is "
        << value);
    return false;
  }
  env_->napi_create_int64(env_, int_value, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushBoolToMap(const std::string& key, bool value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  env_->napi_get_boolean(env_, value, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushDoubleToMap(const std::string& key,
                                          double value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  env_->napi_create_double(env_, value, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushInt32ToMap(const std::string& key,
                                         int32_t value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  env_->napi_create_int32(env_, value, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushUInt32ToMap(const std::string& key,
                                          uint32_t value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  env_->napi_create_uint32(env_, value, &result);
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushInt64ToMap(const std::string& key,
                                         int64_t value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  if (value < piper::kMinJavaScriptNumber ||
      value > piper::kMaxJavaScriptNumber) {
    // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
    LOGE(
        "PushInt64ToMap error! PrimJS NAPI doesn't support bigint now. The "
        "bigint value is "
        << value);
    return false;
  } else {
    env_->napi_create_int64(env_, value, &result);
  }
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

bool ValueImplNapiPrimJS::PushUInt64ToMap(const std::string& key,
                                          uint64_t value) {
  DCHECK(IsMap());
  napi_value obj;
  GetValueFromReference(env_, backend_value_, &obj);
  napi_value k;
  env_->napi_create_string_utf8(env_, key.c_str(), NAPI_AUTO_LENGTH, &k);
  napi_value result;
  if (value > piper::kMaxJavaScriptNumber) {
    // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
    LOGE(
        "PushUInt64ToMap error! PrimJS NAPI doesn't support bigint now. The "
        "bigint value is "
        << value);
    return false;
  } else {
    env_->napi_create_int64(env_, value, &result);
  }
  env_->napi_set_property(env_, obj, k, result);
  return true;
}

// private
bool ValueImplNapiPrimJS::PushNapiValueToArray(napi_value array,
                                               napi_value value) {
  napi_status status;
  uint32_t current_array_index = 0;
  env_->napi_get_array_length(env_, array, &current_array_index);
  status = env_->napi_set_element(env_, array, current_array_index, value);
  if (status != napi_ok) {
    return false;
  }
  return true;
}

// PubValueFactoryNapiPrimJS
std::unique_ptr<Value> PubValueFactoryNapiPrimJS::CreateArray() {
  napi_value result;
  env_->napi_create_array(env_, &result);
  return std::make_unique<ValueImplNapiPrimJS>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapiPrimJS::CreateMap() {
  napi_value result;
  env_->napi_create_object(env_, &result);
  return std::make_unique<ValueImplNapiPrimJS>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapiPrimJS::CreateBool(bool value) {
  napi_value result;
  env_->napi_get_boolean(env_, value, &result);
  return std::make_unique<ValueImplNapiPrimJS>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapiPrimJS::CreateNumber(double value) {
  napi_value result;
  env_->napi_create_double(env_, value, &result);
  return std::make_unique<ValueImplNapiPrimJS>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapiPrimJS::CreateString(
    const std::string& value) {
  napi_value result;
  env_->napi_create_string_utf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
  return std::make_unique<ValueImplNapiPrimJS>(env_, result);
}

std::unique_ptr<Value> PubValueFactoryNapiPrimJS::CreateArrayBuffer(
    std::unique_ptr<uint8_t[]> value, size_t length) {
  napi_value result;
  void* data;
  env_->napi_create_arraybuffer(env_, length, &data, &result);
  memcpy(data, value.get(), length);
  return std::make_unique<ValueImplNapiPrimJS>(env_, result);
}

// ValueUtilsNapiPrimJS
napi_value ValueUtilsNapiPrimJS::ConvertPubValueToNapiValue(
    napi_env env, const Value& value) {
  if (value.backend_type() ==
      lynx::pub::ValueBackendType::ValueBackendTypeNapiPrimJS) {
    return (reinterpret_cast<const ValueImplNapiPrimJS*>(&value))
        ->backend_value();
  }
  napi_value result = nullptr;
  if (value.IsNil()) {
    env->napi_get_null(env, &result);
  } else if (value.IsBool()) {
    env->napi_get_boolean(env, value.Bool(), &result);
  } else if (value.IsString()) {
    env->napi_create_string_utf8(env, value.str().c_str(), NAPI_AUTO_LENGTH,
                                 &result);
  } else if (value.IsInt32()) {
    env->napi_create_int32(env, value.Int32(), &result);
  } else if (value.IsUInt32()) {
    env->napi_create_uint32(env, value.UInt32(), &result);
  } else if (value.IsInt64()) {
    int64_t i = value.Int64();
    // When integer beyond limit, use BigInt Object to define it
    if (i < piper::kMinJavaScriptNumber || i > piper::kMaxJavaScriptNumber) {
      // TODO: Use napi_create_bigint_int64, primjs napi doesn't support it now.
      LOGE(
          "Convert int64 bigint error! PrimJS NAPI doesn't support bigint now. "
          "The bigint value is "
          << i);
      env->napi_get_null(env, &result);
    } else {
      env->napi_create_int64(env, i, &result);
    }
  } else if (value.IsUInt64()) {
    uint64_t u = value.UInt64();
    if (u > piper::kMaxJavaScriptNumber) {
      // TODO: Use napi_create_bigint_uint64, primjs napi doesn't support it
      // now.
      LOGE(
          "Convert uint64 bigint error! PrimJS NAPI doesn't support bigint "
          "now. The bigint value is "
          << u);
      env->napi_get_null(env, &result);
    } else {
      env->napi_create_int64(env, u, &result);
    }
  } else if (value.IsNumber()) {
    env->napi_create_double(env, value.Number(), &result);
  } else if (value.IsMap()) {
    result = ConvertPubValueToNapiObject(env, value);
  } else if (value.IsArray()) {
    result = ConvertPubValueToNapiArray(env, value);
  } else if (value.IsArrayBuffer()) {
    void* data = value.ArrayBuffer();
    size_t length = value.Length();
    Napi::ArrayBuffer array_buffer = Napi::ArrayBuffer::New(env, length);
    memcpy(array_buffer.Data(), data, length);
    result = array_buffer;
  } else if (value.IsUndefined()) {
    env->napi_get_undefined(env, &result);
  } else {
    LOGE("ValueUtilsNapiPrimJS, unknown type :" << value.Type());
  }
  return result;
}

napi_value ValueUtilsNapiPrimJS::ConvertPubValueToNapiArray(
    napi_env env, const Value& value) {
  napi_value result;
  env->napi_create_array(env, &result);
  value.ForeachArray(
      [env, result](int64_t index, const lynx::pub::Value& value) {
        env->napi_set_element(env, result, static_cast<uint32_t>(index),
                              ConvertPubValueToNapiValue(env, value));
      });
  return result;
}

napi_value ValueUtilsNapiPrimJS::ConvertPubValueToNapiObject(
    napi_env env, const Value& value) {
  napi_value result;
  env->napi_create_object(env, &result);
  value.ForeachMap([env, result](const lynx::pub::Value& key,
                                 const lynx::pub::Value& value) {
    napi_value k;
    env->napi_create_string_utf8(env, key.str().c_str(), NAPI_AUTO_LENGTH, &k);
    env->napi_set_property(env, result, k,
                           ConvertPubValueToNapiValue(env, value));
  });
  return result;
}

void* ValueUtilsOpaqueNapiPrimJS::ConvertPubValueToOpaqueNapiValue(
    void* opaque_env, const Value& value) {
  return static_cast<void*>(ValueUtilsNapiPrimJS::ConvertPubValueToNapiValue(
      static_cast<napi_env>(opaque_env), value));
}

std::unique_ptr<Value>
ValueUtilsOpaqueNapiPrimJS::CreateValueWithOpaqueNapiArgs(void* opaque_env,
                                                          void* opaque_value) {
  return std::make_unique<ValueImplNapiPrimJS>(
      static_cast<napi_env>(opaque_env), static_cast<napi_value>(opaque_value));
}

}  // namespace pub
}  // namespace lynx
