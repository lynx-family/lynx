// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/lepus_value.h"

#include <math.h>

#include <memory>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/byte_array.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/js_object.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/path_parser.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/regexp.h"
#include "core/runtime/vm/lepus/table.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace lepus {

Value::Value(const Value& value) { Copy(value); }

Value::Value(Value&& value) noexcept {
  if (p_val_ && IsJSValue()) {
    p_val_->Reset(cell_->rt_);
  }
  cell_ = value.cell_;
  value_ = value.value_;

  if (value.p_val_ && IsJSValue()) {
    p_val_ = (p_val_ == nullptr) ? new GCPersistent() : p_val_;
    p_val_->Reset(cell_->rt_, value.p_val_->Get());
    value.p_val_->Reset(cell_->rt_);
  }
  value.cell_ = nullptr;
  value.value_ = {.val_int64 = 0, .type = lynx_value_null, .tag = 0};
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

Value::Value(const fml::RefPtr<lepus::LEPUSObject>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kJSObject)}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::LEPUSObject>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kJSObject)}) {}

Value::Value(const fml::RefPtr<lepus::ByteArray>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_arraybuffer}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::ByteArray>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_arraybuffer}) {}

Value::Value(const fml::RefPtr<lepus::RefCounted>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kRefCounted)}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::RefCounted>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kRefCounted)}) {
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

Value::Value(const fml::RefPtr<CArray>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_array}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<CArray>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_array}) {}

Value::Value(LEPUSContext* ctx, const LEPUSValue& val) {
  if (LEPUS_IsLepusRef(val)) {
    value_ = lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx, val);
    return;
  }

  cell_ = Context::GetContextCellFromCtx(ctx);
  value_ = {
      .val_ptr = reinterpret_cast<lynx_value_ptr>(LEPUS_VALUE_GET_INT64(val)),
      .type = lynx_value_extended,
      .tag = LEPUS_VALUE_GET_TAG(val)};
  if (cell_->gc_enable_) {
    p_val_ = (p_val_ == nullptr) ? new GCPersistent() : p_val_;
    p_val_->Reset(ctx, val);
  } else {
    LEPUS_DupValue(ctx, val);
  }
}

Value::Value(LEPUSContext* ctx, LEPUSValue&& val) {
  if (LEPUS_IsLepusRef(val)) {
    value_ = lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx, val);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, val);
    val = LEPUS_UNDEFINED;
    return;
  }

  cell_ = Context::GetContextCellFromCtx(ctx);
  value_ = {
      .val_ptr = reinterpret_cast<lynx_value_ptr>(LEPUS_VALUE_GET_INT64(val)),
      .type = lynx_value_extended,
      .tag = LEPUS_VALUE_GET_TAG(val)};
  if (cell_->gc_enable_) {
    p_val_ = (p_val_ == nullptr) ? new GCPersistent() : p_val_;
    p_val_->Reset(ctx, val);
  }
  val = LEPUS_UNDEFINED;
}

LEPUSValue Value::ToJSValue(LEPUSContext* ctx, bool deep_convert) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Value::ToJSValue");
  if (IsJSValue()) {
    LEPUSValue v = WrapJSValue();
    LEPUS_DupValue(ctx, v);
    return v;
  }
  if (IsInt32()) {
    return LEPUS_NewInt32(ctx, Int32());
  } else if (IsCPointer()) {
    return LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, value_.val_ptr);
  } else if (IsDouble()) {
    return LEPUS_NewFloat64(ctx, Double());
  }
  return LEPUSValueHelper::ToJsValue(ctx, *this, deep_convert);
}

// nested use of recursive implementation to prevent excessive trace
// instrumentation
Value Value::ToLepusValue(bool deep_convert) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Value::ToLepusValue");
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
  value = LEPUSValueHelper::ToLepusValue(value.context(), value.WrapJSValue(),
                                         flag);
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

#if !ENABLE_JUST_LEPUSNG
Value::Value(const fml::RefPtr<lepus::Closure>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kClosure)}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<Closure>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kClosure)}) {}

Value::Value(const fml::RefPtr<lepus::CDate>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kCDate)}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<CDate>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kCDate)}) {}

Value::Value(const fml::RefPtr<lepus::RegExp>& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.get()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kRegExp)}) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::RegExp>&& data)
    : value_({.val_ptr = reinterpret_cast<lynx_value_ptr>(data.AbandonRef()),
              .type = lynx_value_object,
              .tag = static_cast<int64_t>(CustomRefCountedType::kRegExp)}) {}

fml::RefPtr<lepus::RegExp> Value::RegExp() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_object &&
      value_.tag == static_cast<int64_t>(CustomRefCountedType::kRegExp)) {
    return fml::RefPtr<lepus::RegExp>(
        reinterpret_cast<lepus::RegExp*>(value_.val_ptr));
  }
  return lepus::RegExp::Create();
}

fml::RefPtr<lepus::Closure> Value::GetClosure() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_object &&
      value_.tag == static_cast<int64_t>(CustomRefCountedType::kClosure)) {
    return fml::RefPtr<lepus::Closure>(
        reinterpret_cast<lepus::Closure*>(value_.val_ptr));
  }
  return lepus::Closure::Create(nullptr);
}

fml::RefPtr<lepus::CDate> Value::Date() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_object &&
      value_.tag == static_cast<int64_t>(CustomRefCountedType::kCDate)) {
    return fml::RefPtr<lepus::CDate>(
        reinterpret_cast<lepus::CDate*>(value_.val_ptr));
  }
  return lepus::CDate::Create();
}

void Value::SetClosure(const fml::RefPtr<lepus::Closure>& closure) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(closure.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kClosure);
  closure->AddRef();
}

void Value::SetClosure(fml::RefPtr<lepus::Closure>&& closure) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(closure.AbandonRef());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kClosure);
}

void Value::SetDate(const fml::RefPtr<lepus::CDate>& date) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(date.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kCDate);
  date->AddRef();
}

void Value::SetDate(fml::RefPtr<lepus::CDate>&& date) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(date.AbandonRef());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kCDate);
}

void Value::SetRegExp(const fml::RefPtr<lepus::RegExp>& regexp) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(regexp.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kRegExp);
  regexp->AddRef();
}

void Value::SetRegExp(fml::RefPtr<lepus::RegExp>&& regexp) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(regexp.AbandonRef());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kRegExp);
}
#endif

const std::string& Value::StdString() const {
  if (value_.type == lynx_value_string) {
    return reinterpret_cast<base::RefCountedStringImpl*>(value_.val_ptr)->str();
  } else if (value_.type == lynx_value_bool) {
    return value_.val_bool
               ? base::RefCountedStringImpl::Unsafe::kTrueString().str()
               : base::RefCountedStringImpl::Unsafe::kFalseString().str();
  } else if (IsJSString()) {
    return LEPUSValueHelper::ToLepusStringRefCountedImpl(cell_->ctx_,
                                                         WrapJSValue())
        ->str();
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
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
        LEPUSValueHelper::ToLepusStringRefCountedImpl(cell_->ctx_,
                                                      WrapJSValue()));
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
    return base::String::Unsafe::ConstructStringFromRawRef(
        LEPUSValueHelper::ToLepusStringRefCountedImpl(cell_->ctx_,
                                                      WrapJSValue()));
  } else if (IsJSBool()) {
    return LEPUSBool()
               ? base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kTrueString())
               : base::String::Unsafe::ConstructStringFromRawRef(
                     &base::RefCountedStringImpl::Unsafe::kFalseString());
  }
  return base::String();
}

fml::RefPtr<lepus::LEPUSObject> Value::LEPUSObject() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_object &&
      value_.tag == static_cast<int>(CustomRefCountedType::kJSObject)) {
    return fml::RefPtr<lepus::LEPUSObject>(
        reinterpret_cast<lepus::LEPUSObject*>(value_.val_ptr));
  }
  return lepus::LEPUSObject::Create();
}

fml::RefPtr<lepus::ByteArray> Value::ByteArray() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_arraybuffer) {
    return fml::RefPtr<lepus::ByteArray>(
        reinterpret_cast<lepus::ByteArray*>(value_.val_ptr));
  }
  return lepus::ByteArray::Create();
}

fml::RefPtr<lepus::Dictionary> Value::Table() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_map) {
    return fml::RefPtr<lepus::Dictionary>(
        reinterpret_cast<lepus::Dictionary*>(value_.val_ptr));
  }
  return lepus::Dictionary::Create();
}

fml::RefPtr<lepus::CArray> Value::Array() const {
  if (value_.val_ptr != nullptr && value_.type == lynx_value_array) {
    return fml::RefPtr<lepus::CArray>(
        reinterpret_cast<lepus::CArray*>(value_.val_ptr));
  }
  return lepus::CArray::Create();
}

CFunction Value::Function() const {
  if (likely(value_.type == lynx_value_function)) {
    return reinterpret_cast<CFunction>(Ptr());
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

fml::RefPtr<lepus::RefCounted> Value::RefCounted() const {
  if (value_.type == lynx_value_object &&
      value_.tag == static_cast<int>(CustomRefCountedType::kRefCounted)) {
    return fml::RefPtr<lepus::RefCounted>(
        reinterpret_cast<lepus::RefCounted*>(value_.val_ptr));
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

void Value::SetJSObject(const fml::RefPtr<lepus::LEPUSObject>& lepus_obj) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(lepus_obj.get());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kJSObject);
  lepus_obj->AddRef();
}

void Value::SetJSObject(fml::RefPtr<lepus::LEPUSObject>&& lepus_obj) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(lepus_obj.AbandonRef());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kJSObject);
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
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kRefCounted);
  src->AddRef();
}

void Value::SetRefCounted(fml::RefPtr<lepus::RefCounted>&& src) {
  FreeValue();
  value_.val_ptr = reinterpret_cast<lynx_value_ptr>(src.AbandonRef());
  value_.type = lynx_value_object;
  value_.tag = static_cast<int64_t>(CustomRefCountedType::kRefCounted);
}

int Value::GetLength() const {
  if (value_.val_ptr == nullptr) {
    return 0;
  }
  if (IsJSValue()) {
    return LEPUS_GetLength(cell_->ctx_, WrapJSValue());
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
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), idx, val);
  }

  if (IsArray() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::CArray*>(value_.val_ptr)->set(idx, val);
  }
  return false;
}

bool Value::SetProperty(uint32_t idx, Value&& val) {
  if (IsJSArray()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), idx, val);
  }

  if (IsArray() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::CArray*>(value_.val_ptr)
        ->set(idx, std::move(val));
  }
  return false;
}

bool Value::SetProperty(const base::String& key, const Value& val) {
  if (IsJSTable()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), key, val);
  }

  if (IsTable() && value_.val_ptr != nullptr) {
    reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->SetValue(key, val);
  }
  return false;
}

bool Value::SetProperty(base::String&& key, const Value& val) {
  if (IsJSTable()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), key, val);
  }

  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)
        ->SetValue(std::move(key), val);
  }
  return false;
}

bool Value::SetProperty(base::String&& key, Value&& val) {
  if (IsJSTable()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), key, val);
  }

  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)
        ->SetValue(std::move(key), std::move(val));
  }
  return false;
}

Value Value::GetProperty(uint32_t idx) const {
  if (IsJSArray()) {
    LEPUSContext* ctx = cell_->ctx_;
    return lepus::Value(
        ctx, LEPUSValueHelper::GetPropertyJsValue(ctx, WrapJSValue(), idx));
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
    LEPUSContext* ctx = cell_->ctx_;
    return lepus::Value(ctx, LEPUSValueHelper::GetPropertyJsValue(
                                 ctx, WrapJSValue(), key.c_str()));
  }
  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->GetValue(key);
  }
  return Value();
}

bool Value::Contains(const base::String& key) const {
  if (IsJSTable()) {
    return LEPUSValueHelper::HasProperty(cell_->ctx_, WrapJSValue(), key);
  }
  if (IsTable() && value_.val_ptr != nullptr) {
    return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->Contains(key);
  }
  return false;
}

void Value::MergeValue(lepus::Value& target, const lepus::Value& update) {
  if (update.IsJSTable()) {
    tasm::ForEachLepusValue(
        update, [&target](const Value& key, const Value& val) {
          // the update key may be a path
          auto path = lepus::ParseValuePath(key.StdString());
          if (!path.empty()) {
            UpdateValueByPath(target, val.ToLepusValue(), path);
          }
        });
    return;
  }
  // check target's first level variable.
  // 1. if update key is not path, simply add new k-v pair for the first level
  // 2. if update key is value path, clone the first level k-v pair and update
  //     the exact value.
  auto update_table =
      reinterpret_cast<lepus::Dictionary*>(update.value_.val_ptr);
  if (update_table == nullptr) {
    return;
  }
  auto target_table =
      target.IsTable()
          ? reinterpret_cast<lepus::Dictionary*>(target.value_.val_ptr)
          : nullptr;
  for (auto it = update_table->begin(); it != update_table->end(); ++it) {
    auto result = lepus::ParseValuePath(it->first.str());
    if (result.size() == 1) {
      target.SetProperty(it->first, it->second);
    } else if (result.size() > 1) {
      if (target_table != nullptr) {
        auto front_value = result.begin();
        lepus_value old_value = target_table->GetValue(*front_value);
        if ((old_value.IsTable() && old_value.Table()->IsConst()) ||
            (old_value.IsArray() && old_value.Array()->IsConst())) {
          old_value = lepus_value::Clone(old_value);
        }
        result.erase(front_value);
        UpdateValueByPath(old_value, it->second, result);
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
    return LEPUSValueHelper::DeepCopyJsValue(src.cell_->ctx_, src.WrapJSValue(),
                                             clone_as_jsvalue);
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
      CustomRefCountedType ref_type =
          static_cast<CustomRefCountedType>(src.value_.tag);
      switch (ref_type) {
        case CustomRefCountedType::kJSObject:
          return Value(
              LEPUSObject::Create(src.LEPUSObject()->jsi_object_proxy()));
#if !ENABLE_JUST_LEPUSNG
        case CustomRefCountedType::kCDate: {
          auto date =
              CDate::Create(src.Date()->get_date_(), src.Date()->get_ms_(),
                            src.Date()->get_language());
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Value::ShallowCopy");
  if (src.IsJSValue()) {
    return LEPUSValueHelper::ShallowCopyJsValue(
        src.cell_->ctx_, src.WrapJSValue(), clone_as_jsvalue);
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

Value Value::CreateObject(Context* ctx) {
  if (ctx && ctx->IsLepusNGContext()) {
    LEPUSContext* lctx = ctx->context();
    return lepus::Value(lctx, LEPUS_NewObject(lctx));
  }
  return Value(lepus::Dictionary::Create());
}

Value Value::CreateArray(Context* ctx) {
  if (ctx && ctx->IsLepusNGContext()) {
    LEPUSContext* lctx = ctx->context();
    return lepus::Value(lctx, LEPUS_NewArray(lctx));
  }
  return Value(lepus::CArray::Create());
}

bool operator==(const Value& left, const Value& right) {
  if (&left == &right) {
    return true;
  }
  // process JSValue type
  if (left.IsJSValue() && right.IsJSValue()) {
    return LEPUSValueHelper::IsJsValueEqualJsValue(
        left.context(), left.WrapJSValue(), right.WrapJSValue());
  } else if (right.IsJSValue()) {
    return LEPUSValueHelper::IsLepusEqualJsValue(right.cell_->ctx_, left,
                                                 right.WrapJSValue());
  } else if (left.IsJSValue()) {
    return LEPUSValueHelper::IsLepusEqualJsValue(left.cell_->ctx_, right,
                                                 left.WrapJSValue());
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
      CustomRefCountedType ref_type =
          static_cast<CustomRefCountedType>(left.value_.tag);
      switch (ref_type) {
        case CustomRefCountedType::kRefCounted:
          return left.RefCounted() == right.RefCounted();
        case CustomRefCountedType::kJSObject:
          return *(left.LEPUSObject().get()) == *(right.LEPUSObject().get());
#if !ENABLE_JUST_LEPUSNG
        case CustomRefCountedType::kClosure:
          return left.GetClosure() == right.GetClosure();
        case CustomRefCountedType::kCDate:
          return *(left.Date().get()) == *(right.Date().get());
        case CustomRefCountedType::kRegExp:
          return left.RegExp()->get_pattern() ==
                     right.RegExp()->get_pattern() &&
                 left.RegExp()->get_flags() == right.RegExp()->get_flags();
#endif
        default:
          break;
      }
    } break;
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

void Value::PrintValue(std::ostream& output, bool ignore_other,
                       bool pretty) const {
  if (IsJSValue()) {
    LEPUSValueHelper::PrintValue(output, cell_->ctx_, WrapJSValue());
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
    case lynx_value_map:
      output << "{";
      for (auto it = Table()->begin(); it != Table()->end(); it++) {
        if (it != Table()->begin()) {
          output << ",";
        }
        if (pretty) {
          output << "\"" << it->first.str() << "\""
                 << ":";
        } else {
          output << it->first.str() << ":";
        }
        it->second.PrintValue(output, ignore_other);
      }
      output << "}";
      break;
    case lynx_value_array:
      output << "[";
      for (size_t i = 0; i < Array()->size(); i++) {
        Array()->get(i).PrintValue(output, ignore_other);
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
      CustomRefCountedType ref_type =
          static_cast<CustomRefCountedType>(value_.tag);
      switch (ref_type) {
        case CustomRefCountedType::kRefCounted:
        case CustomRefCountedType::kClosure:
          if (ignore_other) {
            output << "";
          } else {
            output << "closure/cfunction/cpointer/refcounted" << std::endl;
          }
          break;
        case CustomRefCountedType::kJSObject:
          if (ignore_other) {
            output << "";
          } else {
            output << "LEPUSObject id=" << LEPUSObject()->JSIObjectID();
          }
          break;
#if !ENABLE_JUST_LEPUSNG
        case CustomRefCountedType::kCDate:
          if (ignore_other) {
            output << "";
          } else {
            Date()->print(output);
          }
          break;
        case CustomRefCountedType::kRegExp: {
          output << "regexp" << std::endl;
          output << "pattern: " << RegExp()->get_pattern().str() << std::endl;
          output << "flags: " << RegExp()->get_flags().str() << std::endl;
        } break;
#endif

        default:
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
    case lynx_value_external:
      // ByteArray and Element objects don't cross thread, and don't need to
      // markConst.
      return true;
    case lynx_value_object: {
      CustomRefCountedType ref_type =
          static_cast<CustomRefCountedType>(value_.tag);
      if (ref_type == CustomRefCountedType::kRefCounted) {
        reinterpret_cast<lepus::RefCounted*>(value_.val_ptr)
            ->js_object_cache.reset();
      }
      return true;
    }
    case lynx_value_map:
      return reinterpret_cast<lepus::Dictionary*>(value_.val_ptr)->MarkConst();
    case lynx_value_array:
      return reinterpret_cast<lepus::CArray*>(value_.val_ptr)->MarkConst();
    default:
      // JSValue
      if (LEPUS_VALUE_HAS_REF_COUNT(WrapJSValue())) {
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

  if (p_val_ && IsJSValue()) {
    p_val_->Reset(cell_->rt_);
  }
  cell_ = value.cell_;
  value_ = value.value_;
  if (value.p_val_ && IsJSValue()) {
    p_val_ = (p_val_ == nullptr) ? new GCPersistent() : p_val_;
    p_val_->Reset(cell_->rt_, value.p_val_->Get());
  }
}

void Value::DupValue() const {
  if (IsJSValue()) {
    if (!cell_->gc_enable_) {
      LEPUSValue val = WrapJSValue();
      LEPUS_DupValueRT(cell_->rt_, val);
    }
    return;
  }
  if (!IsReference() || !value_.val_ptr) return;
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(value_.val_ptr)->AddRef();
}

void Value::FreeValue() {
  if (unlikely(p_val_)) {
    if (IsJSValue() && cell_->rt_) {
      p_val_->Reset(cell_->rt_);
    }
    delete p_val_;
    p_val_ = nullptr;
  }
  if (IsJSValue()) {
    if (unlikely(!cell_->rt_)) return;
    if (!cell_->gc_enable_) {
      LEPUSValue val = WrapJSValue();
      LEPUS_FreeValueRT(cell_->rt_, val);
    }
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
  if (unlikely(!cell_)) return false;
  LEPUSValue temp_val = WrapJSValue();
  return LEPUS_IsArray(cell_->ctx_, temp_val) ||
         (LEPUS_GetLepusRefTag(temp_val) == Value_Array);
}

bool Value::IsJSTable() const {
  if (unlikely(!cell_)) return false;
  LEPUSValue temp_val = WrapJSValue();
  return LEPUS_IsObject(temp_val) ||
         (LEPUS_GetLepusRefTag(temp_val) == Value_Table);
}

bool Value::IsJSInteger() const {
  if (!IsJSValue()) return false;
  LEPUSValue temp_val = WrapJSValue();
  if (LEPUS_IsInteger(temp_val)) return true;
  if (LEPUS_IsNumber(temp_val)) {
    double val;
    LEPUS_ToFloat64(cell_->ctx_, &val, temp_val);
    if (base::StringConvertHelper::IsInt64Double(val)) {
      return true;
    }
  }
  return false;
}

bool Value::IsJSFunction() const {
  if (!IsJSValue()) return false;
  return LEPUS_IsFunction(cell_->ctx_, WrapJSValue());
}

int Value::GetJSLength() const {
  if (!IsJSValue()) return 0;
  LEPUSValue temp_val = WrapJSValue();
  return LEPUS_GetLength(cell_->ctx_, temp_val);
}

bool Value::IsJSFalse() const {
  if (!IsJSValue()) return false;

  return IsJSUndefined() || IsJsNull() ||
         (LEPUS_VALUE_IS_UNINITIALIZED(WrapJSValue())) ||
         (IsJSBool() && !LEPUSBool()) || (IsJSInteger() && JSInteger() == 0) ||
         (IsJSString() && GetJSLength() == 0);
}

int64_t Value::JSInteger() const {
  if (!IsJSValue()) return false;
  LEPUSValue temp_val = WrapJSValue();
  if (LEPUS_VALUE_GET_TAG(temp_val) == LEPUS_TAG_INT) {
    return LEPUS_VALUE_GET_INT(temp_val);
  }
  if (LEPUS_IsInteger(temp_val)) {
    int64_t val;
    LEPUS_ToInt64(cell_->ctx_, &val, temp_val);
    return val;
  } else {
    DCHECK(LEPUS_IsNumber(temp_val));
    double val;
    LEPUS_ToFloat64(cell_->ctx_, &val, temp_val);
    return static_cast<int64_t>(val);
  }
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
  return LEPUSValueHelper::ToStdString(cell_->ctx_, WrapJSValue());
}

void Value::IteratorJSValue(const LepusValueIterator& callback) const {
  if (LEPUSValueHelper::IsJsObject(WrapJSValue())) {
    JSValueIteratorCallback callback_wrap =
        [&callback](LEPUSContext* ctx, LEPUSValue& key, LEPUSValue& value) {
          lepus::Value keyWrap(ctx, key);
          lepus::Value valueWrap(ctx, value);
          callback(keyWrap, valueWrap);
        };
    LEPUSValueHelper::IteratorJsValue(cell_->ctx_, WrapJSValue(),
                                      &callback_wrap);
  }
}

bool Value::IsJSValue() const {
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
  return value_.type == lynx_value_extended;
#else
  return cell_ && value_.type == lynx_value_extended;
#endif
}

double Value::LEPUSNumber() const {
  DCHECK(IsJSNumber());
  if (unlikely(!cell_)) return 0;
  LEPUSValue temp_val = WrapJSValue();
  double val;
  LEPUS_ToFloat64(cell_->ctx_, &val, temp_val);
  return val;
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
    case lynx_value_object: {
      CustomRefCountedType type = static_cast<CustomRefCountedType>(value.tag);
      switch (type) {
        case CustomRefCountedType::kRefCounted:
          return Value_RefCounted;
        case CustomRefCountedType::kJSObject:
          return Value_JSObject;
        case CustomRefCountedType::kClosure:
          return Value_Closure;
        case CustomRefCountedType::kCDate:
          return Value_CDate;
        case CustomRefCountedType::kRegExp:
          return Value_RegExp;
        default:
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

// #endif
}  // namespace lepus
}  // namespace lynx
