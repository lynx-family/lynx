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
  type_ = value.type_;
  val_uint64_t_ = value.val_uint64_t_;
  cell_ = value.cell_;

  value.type_ = Value_Nil;
  value.val_int64_t_ = 0;
  if (value.p_val_ && IsJSValue()) {
    p_val_ = (p_val_ == nullptr) ? new GCPersistent() : p_val_;
    p_val_->Reset(cell_->rt_, value.p_val_->Get());
    value.p_val_->Reset(cell_->rt_);
  }
  value.cell_ = nullptr;
}

Value& Value::operator=(Value&& value) noexcept {
  if (this != &value) {
    this->~Value();
    new (this) Value(std::move(value));
  }
  return *this;
}

Value::Value(const base::String& data)
    : val_str_(base::String::Unsafe::GetUntaggedStringRawRef(data)),
      type_(Value_String) {
  val_str_->AddRef();
}

Value::Value(base::String&& data)
    : val_str_(base::String::Unsafe::GetUntaggedStringRawRef(data)),
      type_(Value_String) {
  if (val_str_ != base::String::Unsafe::GetStringRawRef(data)) {
    val_str_->AddRef();
  }
  base::String::Unsafe::SetStringToEmpty(data);
}

Value::Value(const fml::RefPtr<lepus::LEPUSObject>& data)
    : val_jsobject_(data.get()), type_(Value_JSObject) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::LEPUSObject>&& data)
    : val_jsobject_(data.AbandonRef()), type_(Value_JSObject) {}

Value::Value(const fml::RefPtr<lepus::ByteArray>& data)
    : val_bytearray_(data.get()), type_(Value_ByteArray) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::ByteArray>&& data)
    : val_bytearray_(data.AbandonRef()), type_(Value_ByteArray) {}

Value::Value(const fml::RefPtr<lepus::RefCounted>& data)
    : val_ref_counted_(data.get()), type_(Value_RefCounted) {
  val_ref_counted_->AddRef();
}

Value::Value(fml::RefPtr<lepus::RefCounted>&& data)
    : val_ref_counted_(data.AbandonRef()), type_(Value_RefCounted) {}

Value::Value(bool val) : val_bool_(val), type_(Value_Bool) {}

Value::Value(const char* val) : type_(Value_String) {
  val_str_ = base::RefCountedStringImpl::Unsafe::RawCreate(val);
}

Value::Value(const std::string& str) : type_(Value_String) {
  val_str_ = base::RefCountedStringImpl::Unsafe::RawCreate(str);
}

Value::Value(std::string&& str) : type_(Value_String) {
  val_str_ = base::RefCountedStringImpl::Unsafe::RawCreate(std::move(str));
}

Value::Value(void* data) : val_ptr_(data), type_(Value_CPointer) {}
Value::Value(CFunction val)
    : val_ptr_(reinterpret_cast<void*>(val)), type_(Value_CFunction) {}
Value::Value(bool for_nan, bool val) {
  if (for_nan) {
    val_nan_ = val;
    type_ = Value_NaN;
  }
}

#define NumberConstructor(name, type) \
  Value::Value(type val) : val_##type##_(val), type_(Value_##name) {}

NumberType(NumberConstructor)
#undef NumberConstructor

    Value::Value(uint8_t data)
    : val_uint32_t_(data), type_(Value_UInt32) {
}

Value::Value(const fml::RefPtr<Dictionary>& data)
    : val_table_(data.get()), type_(Value_Table) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<Dictionary>&& data)
    : val_table_(data.AbandonRef()), type_(Value_Table) {}

Value::Value(const fml::RefPtr<CArray>& data)
    : val_carray_(data.get()), type_(Value_Array) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<CArray>&& data)
    : val_carray_(data.AbandonRef()), type_(Value_Array) {}

void Value::ConstructValueFromLepusRef(LEPUSContext* ctx,
                                       const LEPUSValue& val) {
  if (LEPUS_IsLepusRef(val)) {
    type_ = static_cast<ValueType>(LEPUS_GetLepusRefTag(val));
    val_ptr_ = LEPUS_GetLepusRefPoint(val);
    reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr_)->AddRef();
    LEPUSLepusRef* ref =
        reinterpret_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, ref->lepus_val);
    ref->lepus_val = LEPUS_UNDEFINED;
  }
}

Value::Value(LEPUSContext* ctx, const LEPUSValue& val) {
  if (LEPUS_IsLepusRef(val)) {
    ConstructValueFromLepusRef(ctx, val);
    return;
  }

  cell_ = Context::GetContextCellFromCtx(ctx);
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
  type_ = Value_PrimJsValue;
#else
  tag_ = EncodeJSTag(LEPUS_VALUE_GET_TAG(val));
#endif
  val_int64_t_ = LEPUS_VALUE_GET_INT64(val);
  if (cell_->gc_enable_) {
    p_val_ = (p_val_ == nullptr) ? new GCPersistent() : p_val_;
    p_val_->Reset(ctx, val);
  } else {
    LEPUS_DupValue(ctx, val);
  }
}

Value::Value(LEPUSContext* ctx, LEPUSValue&& val) {
  if (LEPUS_IsLepusRef(val)) {
    ConstructValueFromLepusRef(ctx, val);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, val);
    val = LEPUS_UNDEFINED;
    return;
  }

  cell_ = Context::GetContextCellFromCtx(ctx);
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
  type_ = Value_PrimJsValue;
#else
  tag_ = EncodeJSTag(LEPUS_VALUE_GET_TAG(val));
#endif
  val_int64_t_ = LEPUS_VALUE_GET_INT64(val);
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
    return LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, val_ptr_);
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
      if (auto tbl = value.val_table_; tbl != nullptr) {
        for (auto& itr : *tbl) {
          ToLepusValueRecursively(itr.second, deep_convert);
        }
      }
    } else if (value.IsArray()) {
      if (auto arr = value.val_carray_; arr != nullptr) {
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
  switch (type_) {
#define NumberCase(name, type) \
  case Value_##name:           \
    return val_##type##_;

    NumberType(NumberCase)

#undef NumberCase
        default : if (IsJSNumber()) return LEPUSNumber();
  }
  return 0;
}

#if !ENABLE_JUST_LEPUSNG
Value::Value(const fml::RefPtr<lepus::Closure>& data)
    : val_closure_(data.get()), type_(Value_Closure) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<Closure>&& data)
    : val_closure_(data.AbandonRef()), type_(Value_Closure) {}

Value::Value(const fml::RefPtr<lepus::CDate>& data)
    : val_date_(data.get()), type_(Value_CDate) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<CDate>&& data)
    : val_date_(data.AbandonRef()), type_(Value_CDate) {}

Value::Value(const fml::RefPtr<lepus::RegExp>& data)
    : val_regexp_(data.get()), type_(Value_RegExp) {
  data.get()->AddRef();
}

Value::Value(fml::RefPtr<lepus::RegExp>&& data)
    : val_regexp_(data.AbandonRef()), type_(Value_RegExp) {}

fml::RefPtr<lepus::RegExp> Value::RegExp() const {
  if (val_regexp_ != nullptr && type_ == Value_RegExp) {
    return fml::RefPtr<lepus::RegExp>(val_regexp_);
  }
  return lepus::RegExp::Create();
}

fml::RefPtr<lepus::Closure> Value::GetClosure() const {
  if (val_closure_ != nullptr && type_ == Value_Closure) {
    return fml::RefPtr<lepus::Closure>(val_closure_);
  }
  return lepus::Closure::Create(nullptr);
}

fml::RefPtr<lepus::CDate> Value::Date() const {
  if (val_date_ != nullptr && type_ == Value_CDate) {
    return fml::RefPtr<lepus::CDate>(val_date_);
  }
  return lepus::CDate::Create();
}

void Value::SetClosure(const fml::RefPtr<lepus::Closure>& closure) {
  FreeValue();
  this->val_closure_ = closure.get();
  this->type_ = Value_Closure;
  closure->AddRef();
}

void Value::SetClosure(fml::RefPtr<lepus::Closure>&& closure) {
  FreeValue();
  this->val_closure_ = closure.AbandonRef();
  this->type_ = Value_Closure;
}

void Value::SetDate(const fml::RefPtr<lepus::CDate>& date) {
  FreeValue();
  this->val_date_ = date.get();
  this->type_ = Value_CDate;
  date->AddRef();
}

void Value::SetDate(fml::RefPtr<lepus::CDate>&& date) {
  FreeValue();
  this->val_date_ = date.AbandonRef();
  this->type_ = Value_CDate;
}

void Value::SetRegExp(const fml::RefPtr<lepus::RegExp>& regexp) {
  FreeValue();
  this->type_ = Value_RegExp;
  this->val_regexp_ = regexp.get();
  regexp->AddRef();
}

void Value::SetRegExp(fml::RefPtr<lepus::RegExp>&& regexp) {
  FreeValue();
  this->type_ = Value_RegExp;
  this->val_regexp_ = regexp.AbandonRef();
}
#endif

const std::string& Value::StdString() const {
  if (type_ == Value_String) {
    return val_str_->str();
  } else if (type_ == Value_Bool) {
    return val_bool_ ? base::RefCountedStringImpl::Unsafe::kTrueString().str()
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
  if (type_ == Value_String) {
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(val_str_);
  } else if (type_ == Value_Bool) {
    return val_bool_ ? base::String::Unsafe::ConstructWeakRefStringFromRawRef(
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
  if (type_ == Value_String) {
    return base::String::Unsafe::ConstructStringFromRawRef(val_str_);
  } else if (type_ == Value_Bool) {
    return val_bool_ ? base::String::Unsafe::ConstructStringFromRawRef(
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
  if (val_jsobject_ != nullptr && type_ == Value_JSObject) {
    return fml::RefPtr<lepus::LEPUSObject>(val_jsobject_);
  }
  return lepus::LEPUSObject::Create();
}

fml::RefPtr<lepus::ByteArray> Value::ByteArray() const {
  if (val_bytearray_ != nullptr && type_ == Value_ByteArray) {
    return fml::RefPtr<lepus::ByteArray>(val_bytearray_);
  }
  return lepus::ByteArray::Create();
}

fml::RefPtr<lepus::Dictionary> Value::Table() const {
  if (val_table_ != nullptr && type_ == Value_Table) {
    return fml::RefPtr<lepus::Dictionary>(val_table_);
  }
  return lepus::Dictionary::Create();
}

fml::RefPtr<lepus::CArray> Value::Array() const {
  if (val_carray_ != nullptr && type_ == Value_Array) {
    return fml::RefPtr<lepus::CArray>(val_carray_);
  }
  return lepus::CArray::Create();
}

CFunction Value::Function() const {
  if (likely(type_ == Value_CFunction)) {
    return reinterpret_cast<CFunction>(Ptr());
  }
  return nullptr;
}

void* Value::CPoint() const {
  if (type_ == Value_CPointer) {
    return Ptr();
  }
  if (IsJSCPointer()) {
    return LEPUSCPointer();
  }
  return nullptr;
}

fml::RefPtr<lepus::RefCounted> Value::RefCounted() const {
  if (type_ == Value_RefCounted) {
    return fml::RefPtr<lepus::RefCounted>(val_ref_counted_);
  }
  return nullptr;
}

void Value::SetNan(bool value) {
  FreeValue();
  this->type_ = Value_NaN;
  this->val_nan_ = value;
}

void Value::SetCPoint(void* point) {
  FreeValue();
  this->type_ = Value_CPointer;
  this->val_ptr_ = point;
}

void Value::SetCFunction(CFunction func) {
  FreeValue();
  this->type_ = Value_CFunction;
  this->val_ptr_ = reinterpret_cast<void*>(func);
}

void Value::SetBool(bool value) {
  FreeValue();
  this->type_ = Value_Bool;
  this->val_bool_ = value;
}

void Value::SetString(const base::String& str) {
  FreeValue();
  type_ = Value_String;
  val_str_ = base::String::Unsafe::GetUntaggedStringRawRef(str);
  val_str_->AddRef();
}

void Value::SetString(base::String&& str) {
  FreeValue();
  type_ = Value_String;
  val_str_ = base::String::Unsafe::GetUntaggedStringRawRef(str);
  if (val_str_ != base::String::Unsafe::GetStringRawRef(str)) {
    val_str_->AddRef();
  }
  base::String::Unsafe::SetStringToEmpty(str);
}

void Value::SetTable(const fml::RefPtr<lepus::Dictionary>& dictionary) {
  FreeValue();
  this->val_table_ = dictionary.get();
  this->type_ = Value_Table;
  dictionary->AddRef();
}

void Value::SetTable(fml::RefPtr<lepus::Dictionary>&& dictionary) {
  FreeValue();
  this->val_table_ = dictionary.AbandonRef();
  this->type_ = Value_Table;
}

void Value::SetArray(const fml::RefPtr<lepus::CArray>& ary) {
  FreeValue();
  this->val_carray_ = ary.get();
  this->type_ = Value_Array;
  ary->AddRef();
}

void Value::SetArray(fml::RefPtr<lepus::CArray>&& ary) {
  FreeValue();
  this->val_carray_ = ary.AbandonRef();
  this->type_ = Value_Array;
}

void Value::SetJSObject(const fml::RefPtr<lepus::LEPUSObject>& lepus_obj) {
  FreeValue();
  this->type_ = Value_JSObject;
  this->val_jsobject_ = lepus_obj.get();
  lepus_obj->AddRef();
}

void Value::SetJSObject(fml::RefPtr<lepus::LEPUSObject>&& lepus_obj) {
  FreeValue();
  this->type_ = Value_JSObject;
  this->val_jsobject_ = lepus_obj.AbandonRef();
}

void Value::SetByteArray(const fml::RefPtr<lepus::ByteArray>& src) {
  FreeValue();
  type_ = Value_ByteArray;
  val_bytearray_ = src.get();
  src->AddRef();
}

void Value::SetByteArray(fml::RefPtr<lepus::ByteArray>&& src) {
  FreeValue();
  type_ = Value_ByteArray;
  val_bytearray_ = src.AbandonRef();
}

void Value::SetRefCounted(const fml::RefPtr<lepus::RefCounted>& src) {
  FreeValue();
  type_ = Value_RefCounted;
  val_ref_counted_ = src.get();
  src->AddRef();
}

void Value::SetRefCounted(fml::RefPtr<lepus::RefCounted>&& src) {
  FreeValue();
  type_ = Value_RefCounted;
  val_ref_counted_ = src.AbandonRef();
}

int Value::GetLength() const {
  if (IsJSValue()) {
    return LEPUS_GetLength(cell_->ctx_, WrapJSValue());
  }

  switch (Type()) {
    case lepus::Value_Array:
      return val_carray_ ? static_cast<int>(val_carray_->size()) : 0;
    case lepus::Value_Table:
      return val_table_ ? static_cast<int>(val_table_->size()) : 0;
    case lepus::Value_String:
      return static_cast<int>(val_str_->length_utf8());
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

  if (IsArray() && val_carray_ != nullptr) {
    return val_carray_->set(idx, val);
  }
  return false;
}

bool Value::SetProperty(uint32_t idx, Value&& val) {
  if (IsJSArray()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), idx, val);
  }

  if (IsArray() && val_carray_ != nullptr) {
    return val_carray_->set(idx, std::move(val));
  }
  return false;
}

bool Value::SetProperty(const base::String& key, const Value& val) {
  if (IsJSTable()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), key, val);
  }

  if (IsTable() && val_table_ != nullptr) {
    return val_table_->SetValue(key, val);
  }
  return false;
}

bool Value::SetProperty(base::String&& key, const Value& val) {
  if (IsJSTable()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), key, val);
  }

  if (IsTable() && val_table_ != nullptr) {
    return val_table_->SetValue(std::move(key), val);
  }
  return false;
}

bool Value::SetProperty(base::String&& key, Value&& val) {
  if (IsJSTable()) {
    return LEPUSValueHelper::SetProperty(cell_->ctx_, WrapJSValue(), key, val);
  }

  if (IsTable() && val_table_ != nullptr) {
    return val_table_->SetValue(std::move(key), std::move(val));
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
    if (val_carray_ != nullptr) {
      return val_carray_->get(idx);
    }
  } else if (type_ == Value_String) {
    if (val_str_->length() > idx) {
      char ss[2] = {val_str_->str()[idx], 0};
      return lepus::Value(base::String(ss, 1));
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
  if (IsTable() && val_table_ != nullptr) {
    return val_table_->GetValue(key);
  }
  return Value();
}

bool Value::Contains(const base::String& key) const {
  if (IsJSTable()) {
    return LEPUSValueHelper::HasProperty(cell_->ctx_, WrapJSValue(), key);
  }
  if (IsTable() && val_table_ != nullptr) {
    return val_table_->Contains(key);
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
  auto update_table = update.val_table_;
  if (update_table == nullptr) {
    return;
  }
  auto target_table = target.IsTable() ? target.val_table_ : nullptr;
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
  ValueType type = src.Type();
  switch (type) {
    case Value_Nil:
      return Value();
    case Value_Undefined: {
      Value v;
      v.SetUndefined();
      return v;
    }
    case Value_Double: {
      double data = src.Number();
      return Value(data);
    }
    case Value_Int32:
      return Value(src.Int32());
    case Value_Int64:
      return Value(src.Int64());
    case Value_UInt32:
      return Value(src.UInt32());
    case Value_UInt64:
      return Value(src.UInt64());
    case Value_Bool:
      return Value(src.Bool());
    case Value_NaN:
      return Value(true, src.NaN());
    case Value_String: {
      return Value(src.String());
    }
    case Value_Table: {
      auto lepus_map = lepus::Dictionary::Create();
      const auto src_tbl = src.val_table_;
      if (src_tbl != nullptr) {
        auto it = src_tbl->begin();
        for (; it != src_tbl->end(); it++) {
          lepus_map->SetValue(it->first, Value::Clone(it->second));
        }
      }
      return Value(std::move(lepus_map));
    }
    case Value_Array: {
      auto ary = CArray::Create();
      const auto src_ary = src.val_carray_;
      if (src_ary != nullptr) {
        ary->reserve(src_ary->size());
        for (size_t i = 0; i < src_ary->size(); ++i) {
          ary->emplace_back(Value::Clone(src_ary->get(i)));
        }
      }
      return Value(std::move(ary));
    }
    case Value_JSObject: {
      return Value(LEPUSObject::Create(src.LEPUSObject()->jsi_object_proxy()));
    }
    case Value_Closure:
    case Value_CFunction:
    case Value_CPointer:
    case Value_RefCounted:
      break;
#if !ENABLE_JUST_LEPUSNG
    case Value_CDate: {
      auto date = CDate::Create(src.Date()->get_date_(), src.Date()->get_ms_(),
                                src.Date()->get_language());
      return Value(std::move(date));
    }
#endif
    default:
      LOGE("!! Value::Clone unknow type: " << type);
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
  ValueType type = src.Type();
  switch (type) {
    case Value_Table: {
      auto lepus_map = lepus::Dictionary::Create();
      const auto src_tbl = src.val_table_;
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
    case Value_Array: {
      auto ary = CArray::Create();
      const auto src_ary = src.val_carray_;
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
  if (left.type_ != right.type_) return false;
  switch (left.type_) {
    case Value_Nil:
      return true;
    case Value_Undefined:
      return true;
    case Value_Double:
      return fabs(left.Number() - right.Number()) < 0.000001;
    case Value_Bool:
      return left.Bool() == right.Bool();
    case Value_NaN:
      return false;
    case Value_String:
      return left.StdString() == right.StdString();
    case Value_CFunction:
      return left.Ptr() == right.Ptr();
    case Value_CPointer:
      return left.Ptr() == right.Ptr();
    case Value_RefCounted:
      return left.RefCounted() == right.RefCounted();
    case Value_Table:
      return *(left.Table().get()) == *(right.Table().get());
    case Value_Array:
      return *(left.Array().get()) == *(right.Array().get());
#if !ENABLE_JUST_LEPUSNG
    case Value_Closure:
      return left.GetClosure() == right.GetClosure();
    case Value_CDate:
      return *(left.Date().get()) == *(right.Date().get());
    case Value_RegExp:
      return left.RegExp()->get_pattern() == right.RegExp()->get_pattern() &&
             left.RegExp()->get_flags() == right.RegExp()->get_flags();
#endif
    case Value_Int32:
    case Value_Int64:
    case Value_UInt32:
    case Value_UInt64:
      // handled, ignore
      break;
    case Value_JSObject:
      return *(left.LEPUSObject().get()) == *(right.LEPUSObject().get());
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
  switch (Type()) {
    case Value_Nil:
      if (ignore_other) {
        output << "";
      } else {
        output << "null";
      }
      break;
    case Value_Undefined:
      if (ignore_other) {
        output << "";
      } else {
        output << "undefined";
      }
      break;
    case Value_Double:
      output << base::StringConvertHelper::DoubleToString(Number());
      break;
    case Value_Int32:
      output << Int32();
      break;
    case Value_Int64:
      output << Int64();
      break;
    case Value_UInt32:
      output << UInt32();
      break;
    case Value_UInt64:
      output << UInt64();
      break;
    case Value_Bool:
      output << (Bool() ? "true" : "false");
      break;
    case Value_String:
      if (pretty) {
        output << "\"" << CString() << "\"";
      } else {
        output << CString();
      }
      break;
    case Value_Table:
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
    case Value_Array:
      output << "[";
      for (size_t i = 0; i < Array()->size(); i++) {
        Array()->get(i).PrintValue(output, ignore_other);
        if (i != (Array()->size() - 1)) {
          output << ",";
        }
      }
      output << "]";
      break;
    case Value_Closure:
    case Value_CFunction:
    case Value_CPointer:
    case Value_RefCounted:
      if (ignore_other) {
        output << "";
      } else {
        output << "closure/cfunction/cpointer/refcounted" << std::endl;
      }
      break;
#if !ENABLE_JUST_LEPUSNG
    case Value_CDate:
      if (ignore_other) {
        output << "";
      } else {
        Date()->print(output);
      }
      break;
    case Value_RegExp:
      if (ignore_other) {
        output << "";
      } else {
        output << "regexp" << std::endl;
        output << "pattern: " << RegExp()->get_pattern().str() << std::endl;
        output << "flags: " << RegExp()->get_flags().str() << std::endl;
      }
      break;
#endif
    case Value_NaN:
      if (ignore_other) {
        output << "";
      } else {
        output << "NaN";
      }
      break;
    case Value_JSObject:
      if (ignore_other) {
        output << "";
      } else {
        output << "LEPUSObject id=" << LEPUSObject()->JSIObjectID();
      }
      break;
    case Value_ByteArray:
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
  switch (type_) {
    case Value_Nil ... Value_String:
    case Value_Closure ... Value_ByteArray:
      // ByteArray and Element objects don't cross thread, and don't need to
      // markConst.
      return true;
    case Value_RefCounted:
      val_ref_counted_->js_object_cache.reset();
      return true;
    case Value_Table:
      return val_table_->MarkConst();
    case Value_Array:
      return val_carray_->MarkConst();
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
  val_uint64_t_ = value.val_uint64_t_;
  type_ = value.Type();
  cell_ = value.cell_;
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
  if (!IsReference() || !val_ptr_) return;
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr_)->AddRef();
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
  if (!IsReference() || !val_ptr_) return;
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr_)->Release();
}

#define NumberValue(name, type)  \
  type Value::name() const {     \
    if (type_ != Value_##name) { \
      return 0;                  \
    }                            \
    return val_##type##_;        \
  }
NormalNumberType(NumberValue)
#undef NumberValue

    int64_t Value::Int64() const {
  if (type_ == Value_Int64) return val_int64_t_;
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
  return type_ == Value_PrimJsValue;
#else
  return cell_ && (type_ > Value_TypeCount || type_ < 0);
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
// #endif
}  // namespace lepus
}  // namespace lynx
