// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/restricted_value.h"

#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/table.h"

namespace lynx {
namespace lepus {

RestrictedValue& RestrictedValue::operator=(const RestrictedValue& value) {
  Assign(*this, value);
  return *this;
}

RestrictedValue& RestrictedValue::operator=(RestrictedValue&& value) noexcept {
  if (likely(this != &value)) {
    this->~RestrictedValue();
    new (this) RestrictedValue(std::move(value));
  }
  return *this;
}

RestrictedValue& RestrictedValue::operator=(const Value& value) {
  Assign(*this, value);
  return *this;
}

RestrictedValue& RestrictedValue::operator=(Value&& value) noexcept {
  FreeValue();
  value_ = value.value();
  std::move(value).value().type = lynx_value_null;
  return *this;
}

RestrictedValue::RestrictedValue(const base::String& data) {
  auto* str = base::String::Unsafe::GetUntaggedStringRawRef(data);
  value_.type = lynx_value_string;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(str);
  str->AddRef();
}

RestrictedValue::RestrictedValue(base::String&& data) {
  auto* str = base::String::Unsafe::GetUntaggedStringRawRef(data);
  value_.type = lynx_value_string;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(str);
  if (str != base::String::Unsafe::GetStringRawRef(data)) {
    str->AddRef();
  }
  base::String::Unsafe::SetStringToEmpty(data);
}

RestrictedValue::RestrictedValue(const char* data) {
  auto* str = base::RefCountedStringImpl::Unsafe::RawCreate(data);
  value_.type = lynx_value_string;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(str);
}

RestrictedValue::RestrictedValue(const std::string& data) {
  auto* str = base::RefCountedStringImpl::Unsafe::RawCreate(data);
  value_.type = lynx_value_string;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(str);
}

RestrictedValue::RestrictedValue(std::string&& data) {
  auto* str = base::RefCountedStringImpl::Unsafe::RawCreate(std::move(data));
  value_.type = lynx_value_string;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(str);
}

RestrictedValue::RestrictedValue(const fml::RefPtr<Dictionary>& data) {
  value_.type = lynx_value_map;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  data.get()->AddRef();
}

RestrictedValue::RestrictedValue(fml::RefPtr<Dictionary>&& data) {
  value_.type = lynx_value_map;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef());
}

RestrictedValue::RestrictedValue(const fml::WeakRefPtr<Dictionary>& data) {
  value_.type = lynx_value_map;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  data.get()->AddRef();
}

RestrictedValue::RestrictedValue(const fml::RefPtr<CArray>& data) {
  value_.type = lynx_value_array;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  data.get()->AddRef();
}

RestrictedValue::RestrictedValue(fml::RefPtr<CArray>&& data) {
  value_.type = lynx_value_array;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef());
}

RestrictedValue::RestrictedValue(const fml::WeakRefPtr<CArray>& data) {
  value_.type = lynx_value_array;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  data.get()->AddRef();
}

RestrictedValue::RestrictedValue(const fml::RefPtr<lepus::ByteArray>& data) {
  value_.type = lynx_value_arraybuffer;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  data.get()->AddRef();
}

RestrictedValue::RestrictedValue(fml::RefPtr<lepus::ByteArray>&& data) {
  value_.type = lynx_value_arraybuffer;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef());
}

RestrictedValue::RestrictedValue(const fml::RefPtr<lepus::RefCounted>& data) {
  value_.type = lynx_value_object;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  value_.tag = static_cast<int32_t>(data->GetRefType());
  data.get()->AddRef();
}

RestrictedValue::RestrictedValue(fml::RefPtr<lepus::RefCounted>&& data) {
  value_.type = lynx_value_object;
  value_.tag = static_cast<int32_t>(data->GetRefType());
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef());
}

RestrictedValue::RestrictedValue(CFunction val) {
  value_.type = lynx_value_function;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(val);
  value_.tag = static_cast<int32_t>(CFunctionType_Default);
}

RestrictedValue::RestrictedValue(CFunctionBuiltin val) {
  value_.type = lynx_value_function;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(val);
  value_.tag = static_cast<int32_t>(CFunctionType_Builtin);
}

bool RestrictedValue::IsFalse() const {
  switch (value_.type) {
    case lynx_value_null:
    case lynx_value_nan:
    case lynx_value_undefined:
      return true;
    case lynx_value_bool:
      return !value_.val_bool;
    case lynx_value_double:
      return value_.val_double == 0;
    case lynx_value_int32:
      return value_.val_int32 == 0;
    case lynx_value_uint32:
      return value_.val_uint32 == 0;
    case lynx_value_int64:
      return value_.val_int64 == 0;
    case lynx_value_uint64:
      return value_.val_uint64 == 0;
    case lynx_value_string:
      return Unsafe::TypeSure::GetStdString(*this).empty();
    default:
      return false;
  }
}

double RestrictedValue::Number() const {
  switch (value_.type) {
    case lynx_value_double:
      return value_.val_double;
    case lynx_value_int32:
      return value_.val_int32;
    case lynx_value_uint32:
      return value_.val_uint32;
    case lynx_value_int64:
      return value_.val_int64;
    case lynx_value_uint64:
      return value_.val_uint64;
    default:
      return 0;
  }
}

double RestrictedValue::Double() const {
  if (value_.type != lynx_value_double) {
    return 0;
  }
  return value_.val_double;
}

int32_t RestrictedValue::Int32() const {
  if (value_.type != lynx_value_int32) {
    return 0;
  }
  return value_.val_int32;
}

uint32_t RestrictedValue::UInt32() const {
  if (value_.type != lynx_value_uint32) {
    return 0;
  }
  return value_.val_uint32;
}

uint64_t RestrictedValue::UInt64() const {
  if (value_.type != lynx_value_uint64) {
    return 0;
  }
  return value_.val_uint64;
}

int64_t RestrictedValue::Int64() const {
  if (value_.type == lynx_value_int64) {
    return value_.val_int64;
  }
  return 0;
}

const std::string& RestrictedValue::StdString() const {
  if (value_.type == lynx_value_string) {
    return reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr)->str();
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::RefCountedStringImpl::Unsafe::kTrueString().str()
               : base::RefCountedStringImpl::Unsafe::kFalseString().str();
  }
  return base::RefCountedStringImpl::Unsafe::kEmptyString.str();
}

base::String RestrictedValue::String() const& {
  if (value_.type == lynx_value_string) {
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr));
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::String::Unsafe::ConstructWeakRefStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructWeakRefStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  }
  return base::String();
}

base::String RestrictedValue::String() && {
  if (value_.type == lynx_value_string) {
    return base::String::Unsafe::ConstructStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr));
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  }
  return base::String();
}

fml::WeakRefPtr<lepus::ByteArray> RestrictedValue::ByteArray() const& {
  return fml::WeakRefPtr<lepus::ByteArray>(
      value_.type == lynx_value_arraybuffer
          ? reinterpret_cast<lepus::ByteArray*>(value_.val_ptr)
          : Value::DummyByteArray());
}

fml::RefPtr<lepus::ByteArray> RestrictedValue::ByteArray() && {
  if (value_.type == lynx_value_arraybuffer) {
    return fml::RefPtr<lepus::ByteArray>(
        reinterpret_cast<lepus::ByteArray*>(value_.val_ptr));
  }
  return lepus::ByteArray::Create();
}

fml::WeakRefPtr<Dictionary> RestrictedValue::Table() const& {
  return fml::WeakRefPtr<Dictionary>(
      value_.type == lynx_value_map
          ? reinterpret_cast<Dictionary*>(value_.val_ptr)
          : Value::DummyTable());
}

fml::RefPtr<Dictionary> RestrictedValue::Table() && {
  if (value_.type == lynx_value_map) {
    return fml::RefPtr<Dictionary>(
        reinterpret_cast<Dictionary*>(value_.val_ptr));
  }
  return Dictionary::Create();
}

fml::WeakRefPtr<CArray> RestrictedValue::Array() const& {
  return fml::WeakRefPtr<CArray>(value_.type == lynx_value_array
                                     ? reinterpret_cast<CArray*>(value_.val_ptr)
                                     : Value::DummyArray());
}

fml::RefPtr<CArray> RestrictedValue::Array() && {
  if (value_.type == lynx_value_array) {
    return fml::RefPtr<CArray>(reinterpret_cast<CArray*>(value_.val_ptr));
  }
  return CArray::Create();
}

fml::WeakRefPtr<lepus::RefCounted> RestrictedValue::RefCounted() const& {
  return fml::WeakRefPtr<lepus::RefCounted>(
      value_.type == lynx_value_object
          ? reinterpret_cast<lepus::RefCounted*>(value_.val_ptr)
          : nullptr);
}

fml::RefPtr<lepus::RefCounted> RestrictedValue::RefCounted() && {
  return fml::RefPtr<lepus::RefCounted>(
      value_.type == lynx_value_object
          ? reinterpret_cast<lepus::RefCounted*>(value_.val_ptr)
          : nullptr);
}

void* RestrictedValue::CPoint() const {
  if (value_.type == lynx_value_external) {
    return Ptr();
  }
  return nullptr;
}

void RestrictedValue::SetString(const base::String& str) {
  FreeValue();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(str);
  ptr->AddRef();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  value_.type = lynx_value_string;
}

void RestrictedValue::SetString(base::String&& str) {
  FreeValue();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(str);
  if (ptr != base::String::Unsafe::GetStringRawRef(str)) {
    ptr->AddRef();
  }
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  value_.type = lynx_value_string;
  base::String::Unsafe::SetStringToEmpty(str);
}

void RestrictedValue::SetTable(const fml::RefPtr<Dictionary>& dictionary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(dictionary.get());
  value_.type = lynx_value_map;
  dictionary->AddRef();
}

void RestrictedValue::SetTable(fml::RefPtr<Dictionary>&& dictionary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(dictionary.AbandonRef());
  value_.type = lynx_value_map;
}

void RestrictedValue::SetArray(const fml::RefPtr<CArray>& ary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ary.get());
  value_.type = lynx_value_array;
  ary->AddRef();
}

void RestrictedValue::SetArray(fml::RefPtr<CArray>&& ary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ary.AbandonRef());
  value_.type = lynx_value_array;
}

void RestrictedValue::SetByteArray(const fml::RefPtr<lepus::ByteArray>& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.get());
  value_.type = lynx_value_arraybuffer;
  src->AddRef();
}

void RestrictedValue::SetByteArray(fml::RefPtr<lepus::ByteArray>&& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.AbandonRef());
  value_.type = lynx_value_arraybuffer;
}

void RestrictedValue::SetRefCounted(const fml::RefPtr<lepus::RefCounted>& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int32_t>(src->GetRefType());
  src->AddRef();
}

void RestrictedValue::SetRefCounted(fml::RefPtr<lepus::RefCounted>&& src) {
  FreeValue();
  value_.tag = static_cast<int32_t>(src->GetRefType());
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.AbandonRef());
  value_.type = lynx_value_object;
}

bool RestrictedValue::SetProperty(uint32_t idx, const RestrictedValue& val) {
  if (likely(IsArray())) {
    reinterpret_cast<CArray*>(value_.val_ptr)->set(idx, val);
  }
  return false;
}

bool RestrictedValue::SetProperty(uint32_t idx, RestrictedValue&& val) {
  if (likely(IsArray())) {
    reinterpret_cast<CArray*>(value_.val_ptr)->set(idx, std::move(val));
  }
  return false;
}

bool RestrictedValue::SetProperty(const base::String& key,
                                  const RestrictedValue& val) {
  if (likely(IsTable())) {
    reinterpret_cast<Dictionary*>(value_.val_ptr)->SetValue(key, val);
  }
  return false;
}

bool RestrictedValue::SetProperty(base::String&& key,
                                  const RestrictedValue& val) {
  if (likely(IsTable())) {
    return reinterpret_cast<Dictionary*>(value_.val_ptr)
        ->SetValue(std::move(key), val)
        .has_value();
  }
  return false;
}

bool RestrictedValue::SetProperty(base::String&& key, RestrictedValue&& val) {
  if (likely(IsTable())) {
    return reinterpret_cast<Dictionary*>(value_.val_ptr)
        ->SetValue(std::move(key), std::move(val))
        .has_value();
  }
  return false;
}

int RestrictedValue::GetLength() const {
  if (value_.val_ptr == nullptr) {
    return 0;
  }
  switch (value_.type) {
    case lynx_value_array:
      return static_cast<int>(
          reinterpret_cast<lepus::CArray*>(value_.val_ptr)->size());
    case lynx_value_map:
      return static_cast<int>(
          reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->size());
    case lynx_value_string:
      return static_cast<int>(
          reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr)
              ->length_utf8());
    default:
      break;
  }
  return 0;
}

RestrictedValue RestrictedValue::GetProperty(uint32_t idx) const {
  if (likely(IsArray())) {
    return RestrictedValue(reinterpret_cast<CArray*>(value_.val_ptr)->get(idx));
  } else if (value_.type == lynx_value_string) {
    auto* ptr = reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr);
    if (ptr->length() > idx) {
      char ss[2] = {ptr->str()[idx], 0};
      return RestrictedValue(base::String(ss, 1));
    }
  }
  return RestrictedValue();
}

RestrictedValue RestrictedValue::GetProperty(const base::String& key) const {
  if (likely(IsTable())) {
    return RestrictedValue(
        reinterpret_cast<Dictionary*>(value_.val_ptr)->GetValue(key));
  }
  return RestrictedValue();
}

bool operator==(const RestrictedValue& left, const RestrictedValue& right) {
  if (&left == &right) {
    return true;
  }
  if (left.IsNumber()) {
    if (right.IsNumber()) {
      return fabs(RestrictedValue::Unsafe::TypeSure::GetNumberFast(left) -
                  RestrictedValue::Unsafe::TypeSure::GetNumberFast(right)) <
             0.000001;
    } else {
      return false;
    }
  }
  if (left.value_.type != right.value_.type) {
    return false;
  }
  switch (left.value_.type) {
    case lynx_value_null:
    case lynx_value_undefined:
      return true;
    case lynx_value_bool:
      return left.value_.val_bool == right.value_.val_bool;
    case lynx_value_string:
      return RestrictedValue::Unsafe::TypeSure::GetStdString(left) ==
             RestrictedValue::Unsafe::TypeSure::GetStdString(right);
    case lynx_value_function:
    case lynx_value_external:
      return left.Ptr() == right.Ptr();
    case lynx_value_map:
      return *(RestrictedValue::Unsafe::TypeSure::GetTable(left)) ==
             *(RestrictedValue::Unsafe::TypeSure::GetTable(right));
    case lynx_value_array:
      return *(RestrictedValue::Unsafe::TypeSure::GetArray(left)) ==
             *(RestrictedValue::Unsafe::TypeSure::GetArray(right));
    case lynx_value_object: {
      auto left_value = left.RefCounted();
      auto right_value = right.RefCounted();
      if (!left_value && !right_value) {
        return true;
      }
      return left_value && left_value->Equals(right_value);
    }
    default:
      break;
  }
  return false;
}

}  // namespace lepus
}  // namespace lynx
