// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/android/lepus_message_consumer.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/js_constants.h"
#include "core/base/lynx_trace_categories.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/byte_array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

static const uint8_t TypeNull = 0;
static const uint8_t TypeTrue = 1;
static const uint8_t TypeFalse = 2;
static const uint8_t TypeInt = 3;
static const uint8_t TypeLong = 4;
static const uint8_t TypeDouble = 5;
static const uint8_t TypeString = 6;
static const uint8_t TypeList = 7;
static const uint8_t TypeMap = 8;
static const uint8_t TypeByteArray = 9;
static const uint8_t TypeTemplateData = 10;
static const uint8_t TypeUndefined = 100;

static const int Alignment = 8;

LepusDecoder::LepusDecoder() : index_(0){};

lepus_value LepusDecoder::DecodeMessage(char *buffer, uint32_t len) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusDecoder::DecodeMessage");
  index_ = 0;
  buffer_ = buffer;
  len_ = len;
  in_exception_ = false;
  return forwardValue();
}

std::optional<lynx::piper::Value> LepusDecoder::DecodeJSMessage(
    piper::Runtime &rt, char *buffer, uint32_t len) {
  index_ = 0;
  buffer_ = buffer;
  len_ = len;
  in_exception_ = false;
  return forwardJSValue(rt);
}

lepus_value LepusDecoder::forwardValue() {
  if (in_exception_) return lepus_value();
  uint8_t type = forwardType();
  lepus_value v;
  switch (type) {
    case TypeNull:
      return lepus_value();
    case TypeUndefined:
      v.SetUndefined();
      return v;
    case TypeTrue:
      return lepus_value(true);
    case TypeFalse:
      return lepus_value(false);
    case TypeInt:
      return lepus_value(forwardInteger());
    case TypeLong:
      return lepus_value(forwardLong());
    case TypeDouble:
      return lepus_value(forwardDouble());
    case TypeString:
      return lepus_value(forwardString());
    case TypeList:
      return lepus_value(forwardArray());
    case TypeMap:
      return lepus_value(forwardDictionary());
    case TypeByteArray:
      return lepus_value(forwardByteArray());
    case TypeTemplateData:
      return lepus_value(forwardTemplateData());
  }
  return lepus_value();
}

uint8_t LepusDecoder::forwardType() {
  return static_cast<uint8_t>(buffer_[index_++]);
}

uint16_t LepusDecoder::forwardUInt16() {
  uint16_t result;
  // Because the uint16 data's address is not aligned, which may cause SIGBUG
  // BUS_ADRALN crash, use memcpy to avoid crash.
  memcpy(&result, buffer_ + index_, sizeof(result));
  index_ += sizeof(result);
  return result;
}

int32_t LepusDecoder::forwardInteger() {
  int32_t result;
  // Because the int32_t data's address is not aligned, which may cause SIGBUG
  // BUS_ADRALN crash, use memcpy to avoid crash.
  memcpy(&result, buffer_ + index_, sizeof(result));
  index_ += sizeof(result);
  return result;
}

int64_t LepusDecoder::forwardLong() {
  int64_t result;
  // Because the int64_t data's address is not aligned, which may cause SIGBUG
  // BUS_ADRALN crash, use memcpy to avoid crash.
  memcpy(&result, buffer_ + index_, sizeof(result));
  index_ += sizeof(result);
  return result;
}

double_t LepusDecoder::forwardDouble() {
  const int mod = index_ % Alignment;
  if (mod != 0) {
    index_ += (Alignment - mod);
  }
  double_t result = *(reinterpret_cast<double_t *>(buffer_ + index_));
  index_ += sizeof(double_t);
  return result;
}

int32_t LepusDecoder::forwardSize() {
  if (index_ > len_) {
    in_exception_ = true;
    LOGE("LepusDecoder::forwardSize: forward size overflow: " << index_);
    return 0;
  }
  uint8_t value = *(reinterpret_cast<uint8_t *>(buffer_ + index_));
  int32_t size = 0;
  index_++;
  if (value < 254) {
    size = value;
  } else if (value == 254) {
    size = forwardUInt16();
  } else {
    size = forwardInteger();
  }
  if (size + index_ > len_) {
    in_exception_ = true;
    LOGE("LepusDecoder::forwardSize: find error forward size: " << size);
    return 0;
  }

  return size;
}

base::String LepusDecoder::forwardString() {
  base::String result;
  int32_t forward_size = forwardSize();
  if (in_exception_) {
    return result;
  }
  size_t size = forward_size >= 0 ? static_cast<size_t>(forward_size) : 0;
  result = base::String(buffer_ + index_, size);
  index_ += size;
  return result;
}

fml::RefPtr<lepus::ByteArray> LepusDecoder::forwardByteArray() {
  int32_t forward_size = forwardSize();
  if (in_exception_) {
    return lepus::ByteArray::Create();
  }
  size_t size = forward_size >= 0 ? static_cast<size_t>(forward_size) : 0;
  std::unique_ptr<uint8_t[]> buffer;
  if (size > 0) {
    buffer = std::make_unique<uint8_t[]>(size);
  }
  if (buffer && size > 0) {
    memcpy(buffer.get(), buffer_ + index_, size);
  }
  index_ += size;
  return lepus::ByteArray::Create(std::move(buffer), size);
}

lepus_value LepusDecoder::forwardTemplateData() {
  int64_t result = *(reinterpret_cast<int64_t *>(buffer_ + index_));
  index_ += 8;
  return *reinterpret_cast<lynx::lepus::Value *>(result);
}

fml::RefPtr<lepus::CArray> LepusDecoder::forwardArray() {
  int32_t size = forwardSize();
  auto array = lepus::CArray::Create();
  if (in_exception_) return array;
  for (int32_t index = 0; index < size; index++) {
    array->push_back(forwardValue());
  }
  return array;
}

fml::RefPtr<lepus::Dictionary> LepusDecoder::forwardDictionary() {
  int32_t size = forwardSize();
  auto dict = lepus::Dictionary::Create();
  if (in_exception_) {
    return dict;
  }
  for (int32_t index = 0; index < size; index++) {
    uint8_t type = forwardType();
    DCHECK(type == TypeString);
    dict->SetValue(forwardString(), forwardValue());
  }
  return dict;
}

std::optional<piper::Value> LepusDecoder::forwardJSValue(piper::Runtime &rt) {
  if (in_exception_) return piper::Value();
  uint8_t type = forwardType();
  switch (type) {
    case TypeNull:
      return piper::Value::null();
    case TypeUndefined:
      return piper::Value::undefined();
    case TypeTrue:
      return piper::Value(true);
    case TypeFalse:
      return piper::Value(false);
    case TypeInt:
      return piper::Value(forwardInteger());
    case TypeLong: {
      // In JavaScript,  the max safe integer is 9007199254740991 and the min
      // safe integer is -9007199254740991, so when integer beyond limit, use
      // BigInt Object to define it. More information from
      // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
      int64_t value = forwardLong();
      if (value < piper::kMinJavaScriptNumber ||
          value > piper::kMaxJavaScriptNumber) {
        auto bigint_opt =
            piper::BigInt::createWithString(rt, std::to_string(value));
        if (!bigint_opt) {
          return std::optional<piper::Value>();
        }
        return piper::Value(*bigint_opt);
      } else {
        return piper::Value(static_cast<double>(value));
      }
    }
    case TypeDouble:
      return piper::Value(forwardDouble());
    case TypeString:
      return piper::Value(forwardJSString(rt));
    case TypeList: {
      auto array_opt = forwardJSArray(rt);
      if (!array_opt) {
        return std::optional<piper::Value>();
      }
      return piper::Value(*array_opt);
    }
    case TypeMap: {
      auto obj_opt = forwardJSDictionary(rt);
      if (!obj_opt) {
        return std::optional<piper::Value>();
      }
      return piper::Value(*obj_opt);
    }
    case TypeByteArray:
      return piper::Value(forwardJSByteArray(rt));
  }
  return piper::Value();
}

piper::String LepusDecoder::forwardJSString(piper::Runtime &rt) {
  int32_t forward_size = forwardSize();

  if (in_exception_) {
    return piper::String::createFromUtf8(rt, "");
  }
  size_t size = forward_size >= 0 ? static_cast<size_t>(forward_size) : 0;
  auto result = piper::String::createFromUtf8(
      rt, reinterpret_cast<const uint8_t *>(buffer_ + index_), size);
  index_ += size;
  return result;
}

piper::ArrayBuffer LepusDecoder::forwardJSByteArray(piper::Runtime &rt) {
  int32_t forward_size = forwardSize();
  if (in_exception_) {
    return piper::ArrayBuffer(rt);
  }
  size_t size = forward_size >= 0 ? static_cast<size_t>(forward_size) : 0;
  std::unique_ptr<uint8_t[]> buffer;
  if (size > 0) {
    buffer = std::make_unique<uint8_t[]>(size);
  }
  if (buffer && size > 0) {
    memcpy(buffer.get(), buffer_ + index_, size);
  }
  index_ += size;
  return piper::ArrayBuffer(rt, std::move(buffer), size);
}

std::optional<piper::Array> LepusDecoder::forwardJSArray(piper::Runtime &rt) {
  piper::Scope scope(rt);
  int32_t size = forwardSize();
  if (in_exception_) {
    return piper::Array::createWithLength(rt, 0);
  }
  auto array_opt =
      piper::Array::createWithLength(rt, static_cast<size_t>(size));
  if (!array_opt) {
    return std::optional<piper::Array>();
  }

  for (int32_t index = 0; index < size; index++) {
    auto js_value_opt = forwardJSValue(rt);
    if (!js_value_opt) {
      return std::optional<piper::Array>();
    }
    array_opt->setValueAtIndex(rt, index, *js_value_opt);
  }
  return array_opt;
}

std::optional<piper::Object> LepusDecoder::forwardJSDictionary(
    piper::Runtime &rt) {
  piper::Scope scope(rt);
  int32_t size = forwardSize();
  piper::Object obj = piper::Object(rt);
  if (in_exception_) {
    return obj;
  }
  for (int32_t index = 0; index < size; index++) {
    uint8_t type = forwardType();
    DCHECK(type == TypeString);
    piper::String key = forwardJSString(rt);
    auto js_value_opt = forwardJSValue(rt);
    if (!js_value_opt) {
      return std::optional<piper::Object>();
    }
    obj.setProperty(rt, key, *js_value_opt);
  }
  return obj;
}

std::vector<int8_t> LepusEncoder::EncodeMessage(
    const lynx::lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusEncoder::EncodeMessage",
              [&value](perfetto::EventContext ctx) {
                std::stringstream ss;
                value.PrintValue(ss);
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("lepus_value");
                debug->set_string_value(ss.str());
              });
  std::vector<int8_t> vec;
  WriteValue(vec, value);
  return vec;
}

void LepusEncoder::WriteValue(std::vector<int8_t> &vec,
                              const lynx::lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusEncoder::WriteValue");
  if (value.IsJSValue()) {
    WriteJSValue(vec, value);
    return;
  }
  switch (value.Type()) {
    case lepus::Value_Nil: {
      vec.push_back(TypeNull);
    } break;
    case lepus::Value_Undefined: {
      vec.push_back(TypeUndefined);
    } break;
    case lepus::Value_Bool: {
      if (value.Bool()) {
        vec.push_back(TypeTrue);
      } else {
        vec.push_back(TypeFalse);
      }
    } break;
    case lepus::Value_Int32: {
      int32_t val = value.Number();
      vec.push_back(TypeInt);
      constexpr static int kIntSize = 4;
      const int mod = vec.size() % Alignment;
      if (mod != 0) {
        vec.resize(vec.size() + Alignment - mod);
      }
      int8_t *start = reinterpret_cast<int8_t *>(&val);
      std::copy(start, start + kIntSize, std::back_inserter(vec));
    } break;
    case lepus::Value_Double: {
      double val = value.Number();
      vec.push_back(TypeDouble);
      constexpr static int kDoubleSize = 8;
      const int mod = vec.size() % Alignment;
      if (mod != 0) {
        vec.resize(vec.size() + Alignment - mod);
      }
      int8_t *start = reinterpret_cast<int8_t *>(&val);
      std::copy(start, start + kDoubleSize, std::back_inserter(vec));
    } break;
    case lepus::Value_Int64: {
      int64_t val = value.Int64();
      vec.push_back(TypeLong);
      constexpr static int kInt64Size = 8;
      const int mod = vec.size() % Alignment;
      if (mod != 0) {
        vec.resize(vec.size() + Alignment - mod);
      }
      int8_t *start = reinterpret_cast<int8_t *>(&val);
      std::copy(start, start + kInt64Size, std::back_inserter(vec));
    } break;
    case lepus::Value_String: {
      WriteString(vec, value.String());
    } break;
    case lepus::Value_Array: {
      auto array = value.Array();
      vec.push_back(TypeList);
      WriteSize(vec, array.get()->size());
      for (size_t i = 0; i < array.get()->size(); i++) {
        WriteValue(vec, array.get()->get(i));
      }
    } break;
    case lepus::Value_Table: {
      auto dict = value.Table().get();
      vec.push_back(TypeMap);
      WriteSize(vec, dict->size());
      for (auto &iter : *dict) {
        WriteString(vec, iter.first);
        WriteValue(vec, iter.second);
      }
    } break;
    case lepus::ValueType::Value_PrimJsValue:
    case lepus::ValueType::Value_ByteArray: {
      assert(false);
    } break;
    default: {
      // un supported types.
      LOGE("WriteValue with Unsupported Type: "
           << static_cast<int>(value.Type()));
      vec.push_back(TypeNull);
    } break;
  }
}

void LepusEncoder::WriteString(std::vector<int8_t> &vec,
                               const base::String &string) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusEncoder::WriteString",
              [&string](perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("write_string");
                debug->set_string_value(string.str());
              });
  const std::string &str = string.str();
  vec.push_back(TypeString);
  WriteSize(vec, str.size());
  std::copy(str.begin(), str.end(), std::back_inserter(vec));
}

void LepusEncoder::WriteSize(std::vector<int8_t> &vec, const size_t size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusEncoder::WriteSize",
              [&size](perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("write_size");
                debug->set_string_value(std::to_string(size));
              });
  if (size < 0) {
    LOGE("try write size less than zero.");
    return;
  }
  if (size < 254) {
    vec.push_back(static_cast<signed char &&>(size));
  } else if (size <= 0xffff) {
    vec.push_back(static_cast<signed char &&>(254));
    vec.push_back(static_cast<signed char &&>(size));
    vec.push_back(static_cast<signed char &&>(size >> 8));
  } else {
    vec.push_back(static_cast<signed char &&>(255));
    vec.push_back(static_cast<signed char &&>(size));
    vec.push_back(static_cast<signed char &&>(size >> 8));
    vec.push_back(static_cast<signed char &&>(size >> 16));
    vec.push_back(static_cast<signed char &&>(size >> 24));
  }
}

void LepusEncoder::WriteJSValue(std::vector<int8_t> &vec,
                                const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusEncoder::WriteJSValue",
              [&value](perfetto::EventContext ctx) {
                std::stringstream ss;
                value.PrintValue(ss);
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("value");
                debug->set_string_value(ss.str());
              });
  if (value.IsJSValue()) {
    if (value.IsJSArray()) {
      WriteJSArray(vec, value);
    } else if (value.IsJSTable()) {
      WriteJSTable(vec, value);
    } else if (value.IsJSFunction()) {
      LOGE("WriteValue with Unsupported type: JSFunction.");
      vec.push_back(TypeNull);
    } else {
      WriteValue(vec, value.ToLepusValue());
    }
  }
}

void LepusEncoder::WriteJSArray(std::vector<int8_t> &vec,
                                const lepus::Value &value) {
  if (value.IsJSArray()) {
    vec.push_back(TypeList);
    uint32_t jsarray_size = value.GetJSLength();
    WriteSize(vec, jsarray_size);
    value.IteratorJSValue(
        [&](const lepus::Value &_index, const lepus::Value &element) {
          this->WriteValue(vec, element);
        });
  } else {
    vec.push_back(TypeNull);
  }
}

void LepusEncoder::WriteJSTable(std::vector<int8_t> &vec,
                                const lepus::Value &value) {
  if (value.IsJSTable()) {
    vec.push_back(TypeMap);
    uint32_t js_object_length = value.GetLength();
    WriteSize(vec, js_object_length);
    value.IteratorJSValue(
        [&](const lepus::Value &key, const lepus::Value &element) {
          this->WriteString(vec, key.String());
          this->WriteValue(vec, element);
        });
  } else {
    vec.push_back(TypeNull);
  }
}

}  // namespace tasm
}  // namespace lynx
