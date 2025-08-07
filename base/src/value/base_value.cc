// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/base_value.h"

#include <math.h>

#include <memory>
#include <utility>

#include "base/include/base_defines.h"
#include "base/include/sorted_for_each.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/lynx_value_extended.h"
#include "base/include/value/path_parser.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_defines.h"
#include "base/trace/native/trace_event.h"

inline constexpr const char* const VALUE_TO_LEPUS_VALUE = "Value::ToLepusValue";
inline constexpr const char* const VALUE_SHADOW_COPY = "Value::ShallowCopy";

namespace lynx {
namespace lepus {

Value::Value(CreateAsUndefinedTag) { value_.type = lynx_value_undefined; }

Value::Value(const Value& value) {
  value_.type = lynx_value_null;
  Copy(value);
}

Value::Value(Value&& value) noexcept {
  if (value.IsJSValue()) {
    env_ = value.env_;
    value_ref_ = nullptr;
    lynx_value_move_reference(env_, value.value_, value.value_ref_,
                              &value_ref_);
    value_ = value.value_;
    value.value_ref_ = nullptr;
    value.env_ = nullptr;
    value.value_.type = lynx_value_null;
  } else {
    value_ = value.value_;
    value.value_.type = lynx_value_null;
  }
}

Value& Value::operator=(Value&& value) noexcept {
  if (this != &value) {
    this->~Value();
    new (this) Value(std::move(value));
  }
  return *this;
}

Value::Value(const base::String& data) {
  auto* str = base::String::Unsafe::GetUntaggedStringRawRef(data);
  value_ = {.val_ptr = reinterpret_cast<lynx_value_ptr>(str),
            .type = lynx_value_string};
  str->AddRef();
}

Value::Value(base::String&& data) {
  auto* str = base::String::Unsafe::GetUntaggedStringRawRef(data);
  value_ = {.val_ptr = reinterpret_cast<lynx_value_ptr>(str),
            .type = lynx_value_string};
  if (str != base::String::Unsafe::GetStringRawRef(data)) {
    str->AddRef();
  }
  base::String::Unsafe::SetStringToEmpty(data);
}

Value::Value(const fml::RefPtr<lepus::ByteArray>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_arraybuffer}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::ByteArray>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_arraybuffer}) {}

Value::Value(const fml::RefPtr<lepus::RefCounted>& data) {
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int32_t>(data->GetRefType());
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::RefCounted>&& data) {
  value_.tag = static_cast<int32_t>(data->GetRefType());
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef());
  value_.type = lynx_value_object;
}

Value::Value(bool val) : value_({.val_bool = val, .type = lynx_value_bool}) {}

Value::Value(const char* val) {
  auto* str = base::RefCountedStringImpl::Unsafe::RawCreate(val);
  value_ = {.val_ptr = reinterpret_cast<lynx_value_ptr>(str),
            .type = lynx_value_string};
}

Value::Value(const std::string& str) {
  auto* ptr = base::RefCountedStringImpl::Unsafe::RawCreate(str);
  value_ = {.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr),
            .type = lynx_value_string};
}

Value::Value(std::string&& str) {
  auto* ptr = base::RefCountedStringImpl::Unsafe::RawCreate(std::move(str));
  value_ = {.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr),
            .type = lynx_value_string};
}

Value::Value(void* data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data),
              .type = lynx_value_external}) {}

Value::Value(CFunction val)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(val),
              .type = lynx_value_function}) {}

Value::Value(BuiltinFunctionTable* data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data),
              .type = lynx_value_function_table}) {}

Value::Value(bool for_nan, bool val) {
  if (for_nan) {
    value_.val_bool = val;
    value_.type = lynx_value_nan;
  }
}

Value::Value(double val)
    : value_({.val_double = val, .type = lynx_value_double}) {}
Value::Value(int32_t val)
    : value_({.val_int32 = val, .type = lynx_value_int32}) {}
Value::Value(uint32_t val)
    : value_({.val_uint32 = val, .type = lynx_value_uint32}) {}
Value::Value(int64_t val)
    : value_({.val_int64 = val, .type = lynx_value_int64}) {}
Value::Value(uint64_t val)
    : value_({.val_uint64 = val, .type = lynx_value_uint64}) {}
Value::Value(uint8_t data)
    : value_({.val_uint32 = data, .type = lynx_value_uint32}) {}

Value::Value(const fml::RefPtr<Dictionary>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_map}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<Dictionary>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_map}) {}

Value::Value(const fml::WeakRefPtr<Dictionary>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_map}) {
  data.get()->AddRef();
}

Value::Value(const fml::RefPtr<CArray>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_array}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<CArray>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_array}) {}

Value::Value(const fml::WeakRefPtr<CArray>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_array}) {
  data.get()->AddRef();
}

Value::Value(lynx_value&& value) : value_(std::move(value)) {}

Value::Value(lynx_api_env env, int64_t val, int32_t tag) : env_(env) {
  value_.val_int64 = val;
  value_.type = lynx_value_extended;
  value_.tag = tag;
  value_ref_ = nullptr;
  lynx_value_add_reference(env_, value_, &value_ref_);
}

Value::Value(lynx_api_env env, const lynx_value& value)
    : value_(value), env_(env) {
  if (value.type == lynx_value_extended && env) {
    value_ref_ = nullptr;
    lynx_value_add_reference(env_, value_, &value_ref_);
  } else if (!env) {
    DupValue();
  }
}

Value::Value(lynx_api_env env, lynx_value&& value)
    : value_(std::move(value)), env_(env) {
  if (value.type == lynx_value_extended && env) {
    value_ref_ = nullptr;
    lynx_value_move_reference(env_, value_, nullptr, &value_ref_);
  }
}

// nested use of recursive implementation to prevent excessive trace
// instrumentation
Value Value::ToLepusValue(bool deep_convert) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, VALUE_TO_LEPUS_VALUE);
  ToLepusValueRecursively(const_cast<lepus::Value&>(*this), deep_convert);
  return *this;
}

// recursively convert all internal values to lepus values
void Value::ToLepusValueRecursively(Value& value, bool deep_convert) {
  if (!value.IsJSValue()) {
    if (value.IsTable()) {
      const auto tbl =
          reinterpret_cast<lepus::Dictionary*>(value.value_.val_ptr);
      if (tbl != nullptr) {
        for (auto& itr : *tbl) {
          ToLepusValueRecursively(itr.second, deep_convert);
        }
      }
    } else if (value.IsArray()) {
      const auto arr = reinterpret_cast<lepus::CArray*>(value.value_.val_ptr);
      if (arr != nullptr) {
        for (std::size_t i = 0; i < arr->size(); ++i) {
          ToLepusValueRecursively(const_cast<lepus::Value&>(arr->get(i)),
                                  deep_convert);
        }
      }
    }
    return;
  }
  int32_t flag = deep_convert ? 1 : 0;
  value = ToLepusValue(value.env_, value.value_, flag);
}

Value::~Value() { FreeValue(); }

double Value::Number() const {
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
      if (IsJSNumber()) return LEPUSNumber();
  }
  return 0;
}

const std::string& Value::StdString() const {
  if (value_.type == lynx_value_string) {
    return reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr)->str();
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::RefCountedStringImpl::Unsafe::kTrueString().str()
               : base::RefCountedStringImpl::Unsafe::kFalseString().str();
  } else if (IsJSString()) {
    void* str_ref;
    lynx_value_get_string_ref(env_, value_, &str_ref);
    return reinterpret_cast<base::RefCountedStringImpl*>(str_ref)->str();
  } else if (IsJSBool()) {
    return LEPUSBool()
               ? base::RefCountedStringImpl::Unsafe::kTrueString().str()
               : base::RefCountedStringImpl::Unsafe::kFalseString().str();
  }
  return base::RefCountedStringImpl::Unsafe::kEmptyString.str();
}

base::String Value::String() const& {
  if (value_.type == lynx_value_string) {
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr));
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::String::Unsafe::ConstructWeakRefStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructWeakRefStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  } else if (IsJSString()) {
    void* str_ref;
    lynx_value_get_string_ref(env_, value_, &str_ref);
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(str_ref));
  } else if (IsJSBool()) {
    return LEPUSBool()
               ? base::String::Unsafe::ConstructWeakRefStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructWeakRefStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  }
  return base::String();
}

base::String Value::String() && {
  if (value_.type == lynx_value_string) {
    return base::String::Unsafe::ConstructStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr));
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  } else if (IsJSString()) {
    void* str_ref;
    lynx_value_get_string_ref(env_, value_, &str_ref);
    return base::String::Unsafe::ConstructStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(str_ref));
  } else if (IsJSBool()) {
    return LEPUSBool()
               ? base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  }
  return base::String();
}

fml::WeakRefPtr<lepus::ByteArray> Value::ByteArray() const& {
  return fml::WeakRefPtr<lepus::ByteArray>(
      value_.val_ptr != nullptr && value_.type == lynx_value_arraybuffer
          ? reinterpret_cast<lepus::ByteArray*>(value_.val_ptr)
          : DummyByteArray());
}

fml::RefPtr<lepus::ByteArray> Value::ByteArray() && {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_arraybuffer) {
    return fml::RefPtr<lepus::ByteArray>(
        reinterpret_cast<lepus::ByteArray*>(value_.val_ptr));
  }
  return lepus::ByteArray::Create();
}

fml::WeakRefPtr<Dictionary> Value::Table() const& {
  return fml::WeakRefPtr<Dictionary>(
      value_.val_ptr != nullptr && value_.type == lynx_value_map
          ? reinterpret_cast<Dictionary*>(value_.val_ptr)
          : DummyTable());
}

fml::RefPtr<Dictionary> Value::Table() && {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_map) {
    return fml::RefPtr<Dictionary>(
        reinterpret_cast<Dictionary*>(value_.val_ptr));
  }
  return Dictionary::Create();
}

fml::WeakRefPtr<CArray> Value::Array() const& {
  return fml::WeakRefPtr<CArray>(value_.val_ptr != nullptr &&
                                         value_.type == lynx_value_array
                                     ? reinterpret_cast<CArray*>(value_.val_ptr)
                                     : DummyArray());
}

fml::RefPtr<CArray> Value::Array() && {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_array) {
    return fml::RefPtr<CArray>(reinterpret_cast<CArray*>(value_.val_ptr));
  }
  return CArray::Create();
}

fml::WeakRefPtr<lepus::RefCounted> Value::RefCounted() const& {
  return fml::WeakRefPtr<lepus::RefCounted>(
      value_.type == lynx_value_object
          ? reinterpret_cast<lepus::RefCounted*>(value_.val_ptr)
          : nullptr);
}

fml::RefPtr<lepus::RefCounted> Value::RefCounted() && {
  if (value_.type == lynx_value_object) {
    return fml::RefPtr<lepus::RefCounted>(
        reinterpret_cast<lepus::RefCounted*>(value_.val_ptr));
  }
  return nullptr;
}

CFunction Value::Function() const {
  if (likely(value_.type == lynx_value_function)) {
    return reinterpret_cast<CFunction>(Ptr());
  }
  return nullptr;
}

BuiltinFunctionTable* Value::FunctionTable() const {
  if (likely(value_.type == lynx_value_function_table)) {
    return reinterpret_cast<BuiltinFunctionTable*>(Ptr());
  }
  return nullptr;
}

void* Value::CPoint() const {
  if (value_.type == lynx_value_external) {
    return Ptr();
  }
  if (IsJSCPointer()) {
    return LEPUSCPointer();
  }
  return nullptr;
}

void Value::SetNan(bool value) {
  FreeValue();
  value_.type = lynx_value_nan;
  value_.val_bool = value;
}

void Value::SetCPoint(void* point) {
  FreeValue();
  value_.type = lynx_value_external;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(point);
}

void Value::SetCFunction(CFunction func) {
  FreeValue();
  value_.type = lynx_value_function;
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(func);
}

void Value::SetBool(bool value) {
  FreeValue();
  value_.type = lynx_value_bool;
  value_.val_bool = value;
}

void Value::SetString(const base::String& str) {
  FreeValue();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(str);
  ptr->AddRef();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  value_.type = lynx_value_string;
}

void Value::SetString(base::String&& str) {
  FreeValue();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(str);
  if (ptr != base::String::Unsafe::GetStringRawRef(str)) {
    ptr->AddRef();
  }
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  value_.type = lynx_value_string;
  base::String::Unsafe::SetStringToEmpty(str);
}

void Value::SetTable(const fml::RefPtr<lepus::Dictionary>& dictionary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(dictionary.get());
  value_.type = lynx_value_map;
  dictionary->AddRef();
}

void Value::SetTable(fml::RefPtr<lepus::Dictionary>&& dictionary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(dictionary.AbandonRef());
  value_.type = lynx_value_map;
}

void Value::SetArray(const fml::RefPtr<lepus::CArray>& ary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ary.get());
  value_.type = lynx_value_array;
  ary->AddRef();
}

void Value::SetArray(fml::RefPtr<lepus::CArray>&& ary) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(ary.AbandonRef());
  value_.type = lynx_value_array;
}

void Value::SetByteArray(const fml::RefPtr<lepus::ByteArray>& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.get());
  value_.type = lynx_value_arraybuffer;
  src->AddRef();
}

void Value::SetByteArray(fml::RefPtr<lepus::ByteArray>&& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.AbandonRef());
  value_.type = lynx_value_arraybuffer;
}

void Value::SetRefCounted(const fml::RefPtr<lepus::RefCounted>& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int32_t>(src->GetRefType());
  src->AddRef();
}

void Value::SetRefCounted(fml::RefPtr<lepus::RefCounted>&& src) {
  FreeValue();
  value_.tag = static_cast<int32_t>(src->GetRefType());
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.AbandonRef());
  value_.type = lynx_value_object;
}

int Value::GetLength() const {
  if (value_.val_ptr == nullptr) {
    return 0;
  }
  if (IsJSValue()) {
    uint32_t len;
    lynx_value_get_length(env_, value_, &len);
    return len;
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

bool Value::IsEqual(const Value& value) const { return (*this == value); }

bool Value::SetProperty(uint32_t idx, const Value& val) {
  if (IsJSArray()) {
    return lynx_value_set_element(env_, value_, idx, val.value_) == lynx_api_ok;
  }

  if (IsArray() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::CArray*>(value_.val_ptr)->set(idx, val);
  }
  return false;
}

bool Value::SetProperty(uint32_t idx, Value&& val) {
  if (IsJSArray()) {
    return lynx_value_set_element(env_, value_, idx, val.value_) == lynx_api_ok;
  }

  if (IsArray() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::CArray*>(value_.val_ptr)
        ->set(idx, std::move(val));
  }
  return false;
}

bool Value::SetProperty(const base::String& key, const Value& val) {
  if (IsJSTable()) {
    return lynx_value_set_named_property(env_, value_, key.c_str(),
                                         val.value_) == lynx_api_ok;
  }

  if (IsTable() && value_.val_ptr != nullptr) {
    reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->SetValue(key, val);
  }
  return false;
}

bool Value::SetProperty(base::String&& key, const Value& val) {
  if (IsJSTable()) {
    return lynx_value_set_named_property(env_, value_, key.c_str(),
                                         val.value_) == lynx_api_ok;
  }

  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)
        ->SetValue(std::move(key), val);
  }
  return false;
}

bool Value::SetProperty(base::String&& key, Value&& val) {
  if (IsJSTable()) {
    return lynx_value_set_named_property(env_, value_, key.c_str(),
                                         val.value_) == lynx_api_ok;
  }

  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)
        ->SetValue(std::move(key), std::move(val));
  }
  return false;
}

Value Value::GetProperty(uint32_t idx) const {
  if (IsJSArray()) {
    lynx_value result;
    lynx_value_get_element(env_, value_, idx, &result);
    return Value(env_, std::move(result));
  }

  if (IsArray()) {
    if (value_.val_ptr != nullptr) {
      return reinterpret_cast<lepus::CArray*>(value_.val_ptr)->get(idx);
    }
  } else if (value_.type == lynx_value_string) {
    auto* ptr = reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr);
    if (ptr->length() > idx) {
      char ss[2] = {ptr->str()[idx], 0};
      return Value(base::String(ss, 1));
    }
  } else if (IsJSString()) {
    const auto& s = StdString();
    if (s.length() > idx) {
      char ss[2] = {s.c_str()[idx], 0};
      return lepus::Value(base::String(ss, 1));
    }
  }

  return Value();
}

Value Value::GetProperty(const base::String& key) const {
  if (IsJSTable()) {
    lynx_value result;
    lynx_value_get_named_property(env_, value_, key.c_str(), &result);
    return Value(env_, std::move(result));
  }
  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->GetValue(key);
  }
  return Value();
}

bool Value::Contains(const base::String& key) const {
  if (IsJSTable()) {
    bool ret;
    lynx_value_has_named_property(env_, value_, key.c_str(), &ret);
    return ret;
  }
  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->Contains(key);
  }
  return false;
}

void Value::MergeValue(lepus::Value& target, const lepus::Value& update) {
  if (update.IsJSTable()) {
    ForEachLepusValue(update, [&target](const Value& key, const Value& val) {
      // the update key may be a path
      auto path = lepus::ParseValuePath(key.StdString());
      if (!path.empty()) {
        UpdateValueByPath(target, val.ToLepusValue(), path);
      }
    });
    return;
  }
  // check target's first level variable.
  // 1. if update key is not path, simply add new k-v pair for the first
  // level
  // 2. if update key is value path, clone the first level k-v pair and
  // update
  //     the exact value.
  auto update_table =
      update.IsTable()
          ? reinterpret_cast<lepus::Dictionary*>(update.value_.val_ptr)
          : nullptr;
  if (update_table == nullptr) {
    return;
  }
  auto target_table =
      target.IsTable()
          ? reinterpret_cast<lepus::Dictionary*>(target.value_.val_ptr)
          : nullptr;
  for (auto& it : *update_table) {
    auto result = lepus::ParseValuePath(it.first.str());
    if (result.size() == 1) {
      target.SetProperty(it.first, it.second);
    } else if (result.size() > 1) {
      if (target_table != nullptr) {
        auto front_value = result.begin();
        lepus_value old_value = target_table->GetValue(*front_value);
        if ((old_value.IsTable() && old_value.Table()->IsConst()) ||
            (old_value.IsArray() && old_value.Array()->IsConst())) {
          old_value = lepus_value::Clone(old_value);
        }
        result.erase(front_value);
        UpdateValueByPath(old_value, it.second, result);
        target_table->SetValue(*front_value, old_value);
      }
    }
  }
}

bool Value::UpdateValueByPath(lepus::Value& target, const lepus::Value& update,
                              const base::Vector<std::string>& path) {
  // Feature: if path is empty, update target directly
  // Many uses rely on this feature, please do not touch it.
  if (path.empty()) {
    target = update;
    return true;
  }

  /**
   * example:
   * path: ["a", "b", "c", "d"]
   *         |    |    |    |
   *        get  get  get  set
   */
  auto current = target;
  std::for_each(path.begin(), path.end() - 1, [&current](const auto& key) {
    auto next = current.GetPropertyFromTableOrArray(key);
    std::swap(current, next);
  });
  return current.SetPropertyToTableOrArray(path.back(), update);
}

Value Value::GetPropertyFromTableOrArray(const std::string& key) const {
  if (IsTable() || IsJSTable()) {
    return GetProperty(key);
  }

  if (IsArray() || IsJSArray()) {
    int index;
    if (lynx::base::StringToInt(key, &index, 10)) {
      return GetProperty(index);
    }
  }

  return Value();
}

bool Value::SetPropertyToTableOrArray(const std::string& key,
                                      const Value& update) {
  if (IsTable() || IsJSTable()) {
    return SetProperty(key, update);
  }

  if (IsArray() || IsJSArray()) {
    int index;
    if (lynx::base::StringToInt(key, &index, 10)) {
      return SetProperty(index, update);
    }
  }

  return false;
}

// don't support Closure, CFunction, Cpoint
// nested use of recursive implementation to prevent excessive trace
// instrumentation
Value Value::Clone(const Value& src, bool clone_as_jsvalue) {
  return CloneRecursively(src, clone_as_jsvalue);
}

Value Value::CloneRecursively(const Value& src, bool clone_as_jsvalue) {
  if (src.IsJSValue()) {
    if (clone_as_jsvalue) {
      return Value(src.env_, src.DeepCopyExtendedValue());
    } else {
      return ToLepusValue(src.env_, src.value_, 1);
    }
  }
  switch (src.value_.type) {
    case lynx_value_null:
      return Value();
    case lynx_value_undefined: {
      Value v;
      v.SetUndefined();
      return v;
    }
    case lynx_value_double: {
      double data = src.Number();
      return Value(data);
    }
    case lynx_value_int32:
      return Value(src.Int32());
    case lynx_value_int64:
      return Value(src.Int64());
    case lynx_value_uint32:
      return Value(src.UInt32());
    case lynx_value_uint64:
      return Value(src.UInt64());
    case lynx_value_bool:
      return Value(src.Bool());
    case lynx_value_nan:
      return Value(true, src.NaN());
    case lynx_value_string: {
      return Value(src.String());
    }
    case lynx_value_map: {
      auto lepus_map = lepus::Dictionary::Create();
      const auto src_tbl =
          reinterpret_cast<lepus::Dictionary*>(src.value_.val_ptr);
      if (src_tbl != nullptr) {
        auto it = src_tbl->begin();
        for (; it != src_tbl->end(); it++) {
          lepus_map->SetValue(it->first, Value::Clone(it->second));
        }
      }
      return Value(std::move(lepus_map));
    }
    case lynx_value_array: {
      auto ary = CArray::Create();
      const auto src_ary = reinterpret_cast<lepus::CArray*>(src.value_.val_ptr);
      if (src_ary != nullptr) {
        ary->reserve(src_ary->size());
        for (size_t i = 0; i < src_ary->size(); ++i) {
          ary->emplace_back(Value::Clone(src_ary->get(i)));
        }
      }
      return Value(std::move(ary));
    }
    case lynx_value_object: {
      RefType ref_type = static_cast<RefType>(src.value_.tag);
      switch (ref_type) {
        case RefType::kJSIObject:
          return Value(src.RefCounted()->Clone());
#if !ENABLE_JUST_LEPUSNG
        case RefType::kCDate: {
          auto date = src.RefCounted()->Clone();
          return Value(std::move(date));
        }
#endif
        default:
          break;
      }
    } break;
    case lynx_value_function:
    case lynx_value_external:
      break;
    default:
      LOGE("!! Value::Clone unknow type: " << src.value_.type);
      break;
  }
  return Value();
}

// copy the first level, and mark last as const.
Value Value::ShallowCopy(const Value& src, bool clone_as_jsvalue) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, VALUE_SHADOW_COPY);
  if (src.IsJSValue()) {
    if (clone_as_jsvalue) {
      return Value(src.env_, src.DeepCopyExtendedValue());
    } else {
      return ToLepusValue(src.env_, src.value_, 2);
    }
  }
  switch (src.value_.type) {
    case lynx_value_map: {
      auto lepus_map = lepus::Dictionary::Create();
      const auto src_tbl =
          reinterpret_cast<lepus::Dictionary*>(src.value_.val_ptr);
      if (src_tbl != nullptr) {
        auto it = src_tbl->begin();
        for (; it != src_tbl->end(); it++) {
          if (it->second.MarkConst()) {
            lepus_map->SetValue(it->first, it->second);
          } else {
            lepus_map->SetValue(it->first, Value::Clone(it->second));
          }
        }
      }
      return Value(std::move(lepus_map));
    }
    case lynx_value_array: {
      auto ary = CArray::Create();
      const auto src_ary = reinterpret_cast<lepus::CArray*>(src.value_.val_ptr);
      if (src_ary != nullptr) {
        ary->reserve(src_ary->size());
        for (size_t i = 0; i < src_ary->size(); ++i) {
          if (src_ary->get(i).MarkConst()) {
            ary->push_back(src_ary->get(i));
          } else {
            ary->emplace_back(Value::Clone(src_ary->get(i)));
          }
        }
      }
      return Value(std::move(ary));
    }
    default:
      break;
  }
  return Value::Clone(src);
}

bool operator==(const Value& left, const Value& right) {
  if (&left == &right) {
    return true;
  }
  // process JSValue type
  if (left.IsJSValue() && right.IsJSValue()) {
    bool ret;
    lynx_value_equals(left.env_, left.value_, right.value_, &ret);
    return ret;
  } else if (right.IsJSValue()) {
    return Value::IsLepusValueEqualToExtendedValue(right.env_, left,
                                                   right.value_);
  } else if (left.IsJSValue()) {
    return Value::IsLepusValueEqualToExtendedValue(left.env_, right,
                                                   left.value_);
  }
  if (left.IsNumber() && right.IsNumber()) {
    return fabs(left.Number() - right.Number()) < 0.000001;
  }
  if (left.value_.type != right.value_.type) return false;
  switch (left.value_.type) {
    case lynx_value_null:
      return true;
    case lynx_value_undefined:
      return true;
    case lynx_value_double:
      return fabs(left.Number() - right.Number()) < 0.000001;
    case lynx_value_bool:
      return left.Bool() == right.Bool();
    case lynx_value_nan:
      return false;
    case lynx_value_string:
      return left.StdString() == right.StdString();
    case lynx_value_function:
      return left.Ptr() == right.Ptr();
    case lynx_value_external:
      return left.Ptr() == right.Ptr();
    case lynx_value_map:
      return *(left.Table().get()) == *(right.Table().get());
    case lynx_value_array:
      return *(left.Array().get()) == *(right.Array().get());
    case lynx_value_arraybuffer:
      // TODO(frendy): add impl
      break;
    case lynx_value_object: {
      if (!left.RefCounted() && !right.RefCounted()) {
        return true;
      }
      return left.RefCounted() && left.RefCounted()->Equals(right.RefCounted());
    }
    case lynx_value_int32:
    case lynx_value_int64:
    case lynx_value_uint32:
    case lynx_value_uint64:
    case lynx_value_extended:
      // handled, ignore
      break;
    default:
      break;
  }
  return false;
}

void Value::Print() const {
  std::ostringstream s;
  PrintValue(s);
  LOGE(s.str() << std::endl);
}

void Value::PrintValue(std::ostream& output, bool ignore_other, bool pretty,
                       bool sort_map_key) const {
  if (IsJSValue()) {
    lynx_value_print(env_, value_, &output, nullptr);
    return;
  }
  switch (value_.type) {
    case lynx_value_null:
      if (ignore_other) {
        output << "";
      } else {
        output << "null";
      }
      break;
    case lynx_value_undefined:
      if (ignore_other) {
        output << "";
      } else {
        output << "undefined";
      }
      break;
    case lynx_value_double:
      output << base::StringConvertHelper::DoubleToString(Number());
      break;
    case lynx_value_int32:
      output << Int32();
      break;
    case lynx_value_int64:
      output << Int64();
      break;
    case lynx_value_uint32:
      output << UInt32();
      break;
    case lynx_value_uint64:
      output << UInt64();
      break;
    case lynx_value_bool:
      output << (Bool() ? "true" : "false");
      break;
    case lynx_value_string:
      if (pretty) {
        output << "\"" << CString() << "\"";
      } else {
        output << CString();
      }
      break;
    case lynx_value_map: {
      auto table = Table();
      output << "{";
      if (sort_map_key) {
        bool first = true;
        base::sorted_for_each(
            table->begin(), table->end(),
            [&](const auto& it) {
              if (!first) {
                output << ",";
              }
              if (pretty) {
                output << "\"" << it.first.str() << "\""
                       << ":";
              } else {
                output << it.first.str() << ":";
              }
              it.second.PrintValue(output, ignore_other, pretty, sort_map_key);
              first = false;
            },
            [](const auto& left, const auto& right) {
              return left.first.str() < right.first.str();
            });
      } else {
        for (auto it = table->begin(); it != table->end(); it++) {
          if (it != table->begin()) {
            output << ",";
          }
          if (pretty) {
            output << "\"" << it->first.str() << "\""
                   << ":";
          } else {
            output << it->first.str() << ":";
          }
          it->second.PrintValue(output, ignore_other, pretty, sort_map_key);
        }
      }
      output << "}";
      break;
    }
    case lynx_value_array:
      output << "[";
      for (size_t i = 0; i < Array()->size(); i++) {
        Array()->get(i).PrintValue(output, ignore_other, pretty, sort_map_key);
        if (i != (Array()->size() - 1)) {
          output << ",";
        }
      }
      output << "]";
      break;
    case lynx_value_function:
    case lynx_value_external:
      if (ignore_other) {
        output << "";
      } else {
        output << "closure/cfunction/cpointer/refcounted" << std::endl;
      }
      break;
    case lynx_value_object: {
      RefType ref_type = static_cast<RefType>(value_.tag);
      switch (ref_type) {
        case RefType::kJSIObject:
          if (ignore_other) {
            output << "";
          } else {
            RefCounted()->Print(output);
          }
          break;
#if !ENABLE_JUST_LEPUSNG
        case RefType::kClosure:
          if (ignore_other) {
            output << "";
          } else {
            output << "closure/cfunction/cpointer/refcounted" << std::endl;
          }
          break;
        case RefType::kCDate:
          if (ignore_other) {
            output << "";
          } else {
            RefCounted()->Print(output);
          }
          break;
        case RefType::kRegExp: {
          RefCounted()->Print(output);
        } break;
#endif

        default:
          // RefCounted
          if (ignore_other) {
            output << "";
          } else {
            output << "closure/cfunction/cpointer/refcounted" << std::endl;
          }
          break;
      }
    } break;
    case lynx_value_nan:
      if (ignore_other) {
        output << "";
      } else {
        output << "NaN";
      }
      break;
    case lynx_value_arraybuffer:
      if (ignore_other) {
        output << "";
      } else {
        output << "ByteArray";
      }
      break;
    default:
      if (ignore_other) {
        output << "";
      } else {
        output << "unknow type";
      }
      break;
  }
}

bool Value::MarkConst() const {
  switch (value_.type) {
    case lynx_value_null ... lynx_value_string:
    case lynx_value_arraybuffer:
    case lynx_value_function:
    case lynx_value_function_table:
    case lynx_value_external:
      // ByteArray and Element objects don't cross thread, and don't need to
      // markConst.
      return true;
    case lynx_value_object: {
      RefType ref_type = static_cast<RefType>(value_.tag);
      if (ref_type < RefType::kJSIObject) {
        reinterpret_cast<lepus::RefCounted*>(value_.val_ptr)
            ->js_object_cache.reset();
      }
      return true;
    }
    case lynx_value_map:
      return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->MarkConst();
    case lynx_value_array:
      return reinterpret_cast<lepus::CArray*>(value_.val_ptr)->MarkConst();
    case lynx_value_extended:
      // JSValue
      bool ret;
      lynx_value_has_ref_count(env_, value_, &ret);
      if (ret) {
        return false;
      }
      // Primitive type value can be lightly converted to lepus::Value.
      ToLepusValue();
      return true;
  }
}
void Value::Copy(const Value& value) {
  // avoid self-assignment
  if (this == &value) {
    return;
  }
  value.DupValue();
  FreeValue();
  if (value.IsJSValue()) {
    env_ = value.env_;
    if (value_.type != lynx_value_extended) {
      value_ref_ = nullptr;
    }
    lynx_value_add_reference(value.env_, value.value_, &value_ref_);
  }
  value_ = value.value_;
}

void Value::DupValue() const {
  if (!IsReference() || !value_.val_ptr) return;
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(value_.val_ptr)->AddRef();
}

void Value::FreeValue() {
  if (IsJSValue()) {
    lynx_value_remove_reference(env_, value_, value_ref_);
    value_ref_ = nullptr;
    return;
  }
  if (!IsReference() || !value_.val_ptr) return;
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(value_.val_ptr)
      ->Release();
}

double Value::Double() const {
  if (value_.type != lynx_value_double) {
    return 0;
  }
  return value_.val_double;
}

int32_t Value::Int32() const {
  if (value_.type != lynx_value_int32) {
    return 0;
  }
  return value_.val_int32;
}

uint32_t Value::UInt32() const {
  if (value_.type != lynx_value_uint32) {
    return 0;
  }
  return value_.val_uint32;
}

uint64_t Value::UInt64() const {
  if (value_.type != lynx_value_uint64) {
    return 0;
  }
  return value_.val_uint64;
}

int64_t Value::Int64() const {
  if (value_.type == lynx_value_int64) return value_.val_int64;
  if (IsJSInteger()) {
    return JSInteger();
  }
  return 0;
}

bool Value::IsJSArray() const {
  if (unlikely(!IsJSValue())) return false;
  bool ret;
  lynx_value_is_array(env_, value_, &ret);
  return ret;
}

bool Value::IsJSTable() const {
  if (unlikely(!IsJSValue())) return false;
  bool ret;
  lynx_value_is_map(env_, value_, &ret);
  return ret;
}

bool Value::IsJSInteger() const {
  if (!IsJSValue()) return false;
  bool ret;
  lynx_value_is_integer(env_, value_, &ret);
  return ret;
}

bool Value::IsJSFunction() const {
  if (!IsJSValue()) return false;
  bool ret;
  lynx_value_is_function(env_, value_, &ret);
  return ret;
}

int Value::GetJSLength() const {
  if (!IsJSValue()) return 0;
  uint32_t len;
  lynx_value_get_length(env_, value_, &len);
  return static_cast<int>(len);
}

bool Value::IsJSFalse() const {
  if (!IsJSValue()) return false;

  return IsJSUndefined() || IsJsNull() || IsJSUninitialized() ||
         (IsJSBool() && !LEPUSBool()) || (IsJSInteger() && JSInteger() == 0) ||
         (IsJSString() && GetJSLength() == 0);
}

int64_t Value::JSInteger() const {
  if (!IsJSValue()) return false;
  int64_t ret;
  lynx_value_get_integer(env_, value_, &ret);
  return ret;
}

std::string Value::ToString() const {
  if (!IsJSValue()) {
    // judge whether it is a lepus string type
    if (IsString()) {
      return StdString();
    }
    // it is not string then return ""
    return "";
  }
  std::string str;
  lynx_value_to_string_utf8(env_, value_, &str);
  return str;
}

void Value::IteratorJSValue(const LepusValueIterator& callback) const {
  if (IsJSValue() && (value_.tag >> 16) == lynx_value_object) {
    ExtendedValueIteratorCallback callback_wrap =
        [&callback](lynx_api_env env, const lynx_value& key,
                    const lynx_value& value) {
          lepus::Value keyWrap(env, key);
          lepus::Value valueWrap(env, value);
          callback(keyWrap, valueWrap);
        };
    IterateExtendedValue(env_, value_, &callback_wrap);
  }
}

bool Value::IsJSValue() const { return value_.type == lynx_value_extended; }

double Value::LEPUSNumber() const {
  DCHECK(IsJSNumber());
  if (unlikely(!IsJSValue())) return 0;
  double ret;
  lynx_value_get_number(env_, value_, &ret);
  return ret;
}

lynx_value_type Value::ToLynxValueType(ValueType type) {
  switch (type) {
    case Value_Nil:
      return lynx_value_null;
    case Value_Double:
      return lynx_value_double;
    case Value_Bool:
      return lynx_value_bool;
    case Value_String:
      return lynx_value_string;
    case Value_Table:
      return lynx_value_map;
    case Value_Array:
      return lynx_value_array;
    case Value_CFunction:
      return lynx_value_function;
    case Value_CPointer:
      return lynx_value_external;
    case Value_Int32:
      return lynx_value_int32;
    case Value_Int64:
      return lynx_value_int64;
    case Value_UInt32:
      return lynx_value_uint32;
    case Value_UInt64:
      return lynx_value_uint64;
    case Value_NaN:
      return lynx_value_nan;
    case Value_RefCounted:
    case Value_Closure:
    case Value_CDate:
    case Value_RegExp:
    case Value_JSObject:
      return lynx_value_object;
    case Value_Undefined:
      return lynx_value_undefined;
    case Value_ByteArray:
      return lynx_value_arraybuffer;
    default:
      return lynx_value_extended;
  }
}

ValueType Value::LegacyTypeFromLynxValue(const lynx_value& value) {
  switch (value.type) {
    case lynx_value_null:
      return Value_Nil;
    case lynx_value_undefined:
      return Value_Undefined;
    case lynx_value_bool:
      return Value_Bool;
    case lynx_value_double:
      return Value_Double;
    case lynx_value_int32:
      return Value_Int32;
    case lynx_value_uint32:
      return Value_UInt32;
    case lynx_value_int64:
      return Value_Int64;
    case lynx_value_uint64:
      return Value_UInt64;
    case lynx_value_nan:
      return Value_NaN;
    case lynx_value_string:
      return Value_String;
    case lynx_value_array:
      return Value_Array;
    case lynx_value_map:
      return Value_Table;
    case lynx_value_arraybuffer:
      return Value_ByteArray;
    case lynx_value_function:
      return Value_CFunction;
    case lynx_value_function_table:
      return Value_FunctionTable;
    case lynx_value_object: {
      RefType type = static_cast<RefType>(value.tag);
      switch (type) {
        case RefType::kJSIObject:
          return Value_JSObject;
        case RefType::kClosure:
          return Value_Closure;
        case RefType::kCDate:
          return Value_CDate;
        case RefType::kRegExp:
          return Value_RegExp;
        default:
          return Value_RefCounted;
          break;
      }
    } break;
    case lynx_value_external:
      return Value_CPointer;
    case lynx_value_extended:
      return Value_TypeCount;
    default:
      break;
  }
  return Value_Nil;
}

Value Value::ToLepusValue(lynx_api_env env, const lynx_value& val,
                          int32_t flag) {
  static Value empty_value;
  if (!env) {
    return empty_value;
  }
  if (val.type != lynx_value_extended) {
    if (likely(flag == 0)) {
      return Value(env, val);
    } else if (flag == 1) {
      return Value::Clone(Value(env, val));
    } else {
      Value ret(env, val);
      if (!ret.MarkConst()) {
        ret = Value::Clone(ret);
      }
      return ret;
    }
  }
  lynx_value_type type;
  lynx_value_typeof(env, val, &type);
  switch (type) {
    case lynx_value_null:
      return lepus::Value();
    case lynx_value_undefined: {
      return lepus::Value(Value::kCreateAsUndefinedTag);
    }
    case lynx_value_bool: {
      bool ret;
      lynx_value_get_bool(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_double: {
      double ret;
      lynx_value_get_double(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_int32: {
      int32_t ret;
      lynx_value_get_int32(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_int64: {
      int64_t ret;
      lynx_value_get_int64(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_string: {
      void* str;
      lynx_value_get_string_ref(env, val, &str);
      auto* base_str = reinterpret_cast<base::RefCountedStringImpl*>(str);
      return lepus::Value(
          base::String::Unsafe::ConstructWeakRefStringFromRawRef(base_str));
    }
    case lynx_value_array: {
      return ToLepusArray(env, val, flag);
    }
    case lynx_value_map: {
      return ToLepusMap(env, val, flag);
    }
    case lynx_value_function: {
      if (flag == 0) {
        return lepus::Value(env, val);
      }
      return empty_value;
    }
    default:
      LOGE("not support type:" << type);
      break;
  }

  return empty_value;
}

Value Value::ToLepusArray(lynx_api_env env, const lynx_value& val,
                          int32_t flag) {
  auto arr = CArray::Create();
  ExtendedValueIteratorCallback callback =
      [&arr, flag](lynx_api_env env, const lynx_value& key,
                   const lynx_value& value) {
        arr->emplace_back(ToLepusValue(env, value, flag));
      };
  IterateExtendedValue(env, val, &callback);
  return Value(std::move(arr));
}

Value Value::ToLepusMap(lynx_api_env env, const lynx_value& val, int32_t flag) {
  auto map = Dictionary::Create();
  ExtendedValueIteratorCallback callback =
      [&map, flag](lynx_api_env env, const lynx_value& key,
                   const lynx_value& value) {
        std::string str;
        lynx_value_to_string_utf8(env, key, &str);
        map->SetValue(std::move(str), ToLepusValue(env, value, flag));
      };
  IterateExtendedValue(env, val, &callback);
  return Value(std::move(map));
}

bool Value::IsLepusValueEqualToExtendedValue(lynx_api_env env,
                                             const lepus::Value& src,
                                             const lynx_value& dst) {
  lynx_value_type type;
  lynx_value_typeof(env, dst, &type);
  if (type == lynx_value_array) {
    if (!src.IsArray()) return false;
    return IsLepusArrayEqualToExtendedArray(env, src.Array().get(), dst);
  } else if (type == lynx_value_map) {
    if (!src.IsTable()) return false;
    return IsLepusDictEqualToExtendedDict(env, src.Table().get(), dst);
  } else if (type == lynx_value_function) {
    return false;
  }

  return src == ToLepusValue(env, dst);
}

bool Value::IsLepusArrayEqualToExtendedArray(lynx_api_env env,
                                             lepus::CArray* src,
                                             const lynx_value& dst) {
  uint32_t len;
  lynx_value_get_length(env, dst, &len);
  if (src->size() != static_cast<size_t>(len)) {
    return false;
  }
  for (uint32_t i = 0; i < src->size(); i++) {
    lynx_value val;
    lynx_api_status status = lynx_value_get_element(env, dst, i, &val);
    if (status != lynx_api_ok) return false;
    lepus::Value dst_element(env, std::move(val));
    if (src->get(i) != dst_element) return false;
  }
  return true;
}

bool Value::IsLepusDictEqualToExtendedDict(lynx_api_env env,
                                           lepus::Dictionary* src,
                                           const lynx_value& dst) {
  uint32_t len;
  lynx_value_get_length(env, dst, &len);
  if (src->size() != static_cast<size_t>(len)) {
    return false;
  }
  for (auto& it : *src) {
    lynx_value val;
    lynx_api_status status =
        lynx_value_get_named_property(env, dst, it.first.c_str(), &val);
    if (status != lynx_api_ok) return false;
    lepus::Value dst_property(env, std::move(val));
    if (it.second != dst_property) return false;
  }
  return true;
}

// TODO(frendy): Remove lynx::tasm:ForEachLepusValue
void Value::ForEachLepusValue(const lepus::Value& value,
                              LepusValueIterator func) {
  if (value.IsJSValue()) {
    value.IteratorJSValue(std::move(func));
    return;
  }

  switch (value.value_.type) {
    case lynx_value_map: {
      auto value_scope_ref_ptr = value.Table();
      auto& table = *value_scope_ref_ptr;
      for (auto& pair : table) {
        auto key = lepus::Value(pair.first);
        func(key, pair.second);
      }
    } break;
    case lynx_value_array: {
      auto value_scope_ref_ptr = value.Array();
      auto& array = *value_scope_ref_ptr;
      for (auto i = decltype(array.size()){}; i < array.size(); ++i) {
        func(lepus::Value{static_cast<int64_t>(i)}, array.get(i));
      }
    } break;
    default: {
      func(lepus::Value{}, value);
    } break;
  }
}

CArray* Value::DummyArray() {
  static thread_local CArray dummy;
  dummy.Reset();
  return &dummy;
}

Dictionary* Value::DummyTable() {
  static thread_local Dictionary dummy;
  dummy.Reset();
  return &dummy;
}

ByteArray* Value::DummyByteArray() {
  static thread_local lepus::ByteArray dummy(nullptr, 0);
  dummy.Reset();
  return &dummy;
}

}  // namespace lepus
}  // namespace lynx
