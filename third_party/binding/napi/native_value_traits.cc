// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/native_value_traits.h"

#include <cmath>

#include "third_party/binding/napi/array_buffer_view.h"

namespace lynx {
namespace binding {

void InvalidType(const Napi::Env &env, int32_t index, const char *expecting) {
  char pretty_name[12];
  std::snprintf(pretty_name, 12, "argument %d", index);
  ExceptionMessage::InvalidType(env, pretty_name, expecting);
}

Napi::Value GetArgument(const Napi::CallbackInfo &info, int32_t index) {
  return info[index];
}

// boolean
Napi::Boolean NativeValueTraits<IDLBoolean>::NativeValue(Napi::Value value,
                                                         int32_t index) {
  return value.ToBoolean();
}

Napi::Boolean NativeValueTraits<IDLBoolean>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return value.ToBoolean();
}

// number
Napi::Number NativeValueTraits<IDLNumber>::NativeValue(Napi::Value value,
                                                       int32_t index) {
  if (value.IsNumber()) {
    return value.As<Napi::Number>();
  }
  return value.ToNumber();
}

Napi::Number NativeValueTraits<IDLNumber>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// unrestricted float
float NativeValueTraits<IDLUnrestrictedFloat>::NativeValue(Napi::Value value,
                                                           int32_t index) {
  Napi::Number value_ = NativeValueTraits<IDLNumber>::NativeValue(value);
  return value_.FloatValue();
}

float NativeValueTraits<IDLUnrestrictedFloat>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// restricted float
float NativeValueTraits<IDLFloat>::NativeValue(Napi::Value value,
                                               int32_t index) {
  Napi::Number value_ = NativeValueTraits<IDLNumber>::NativeValue(value);
  float result = value_.FloatValue();
  if (std::isnan(result) || std::isinf(result)) {
    InvalidType(value.Env(), index, "restricted float");
    return 0;
  }
  return result;
}

float NativeValueTraits<IDLFloat>::NativeValue(const Napi::CallbackInfo &info,
                                               int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// unrestricted double
double NativeValueTraits<IDLUnrestrictedDouble>::NativeValue(Napi::Value value,
                                                             int32_t index) {
  Napi::Number value_ = NativeValueTraits<IDLNumber>::NativeValue(value);
  return value_.DoubleValue();
}

double NativeValueTraits<IDLUnrestrictedDouble>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// restricted double
double NativeValueTraits<IDLDouble>::NativeValue(Napi::Value value,
                                                 int32_t index) {
  Napi::Number value_ = NativeValueTraits<IDLNumber>::NativeValue(value);
  double result = value_.DoubleValue();
  if (std::isnan(result) || std::isinf(result)) {
    InvalidType(value.Env(), index, "Restricted Double");
    return 0;
  }
  return result;
}

double NativeValueTraits<IDLDouble>::NativeValue(const Napi::CallbackInfo &info,
                                                 int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// string
Napi::String NativeValueTraits<IDLString>::NativeValue(Napi::Value value,
                                                       int32_t index) {
  if (value.IsString()) {
    return value.As<Napi::String>();
  }
  return value.ToString();
}

Napi::String NativeValueTraits<IDLString>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// object
Napi::Object NativeValueTraits<IDLObject>::NativeValue(Napi::Value value,
                                                       int32_t index) {
  if (value.IsObject()) {
    return value.As<Napi::Object>();
  } else {
    InvalidType(value.Env(), index, "Object");
    return Napi::Object();
  }
}

Napi::Object NativeValueTraits<IDLObject>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// typedarray
#define TypedArrayNativeValueTraitsImpl(CLAZZ, NAPI_TYPE, C_TYPE)           \
  Napi::CLAZZ NativeValueTraits<IDL##CLAZZ>::NativeValue(Napi::Value value, \
                                                         int32_t index) {   \
    if (value.Is##CLAZZ()) {                                                \
      return value.As<Napi::CLAZZ>();                                       \
    } else {                                                                \
      InvalidType(value.Env(), index, #CLAZZ);                              \
      return Napi::CLAZZ();                                                 \
    }                                                                       \
  }                                                                         \
                                                                            \
  Napi::CLAZZ NativeValueTraits<IDL##CLAZZ>::NativeValue(                   \
      const Napi::CallbackInfo &info, int32_t index) {                      \
    Napi::Value value = GetArgument(info, index);                           \
    return NativeValue(value, index);                                       \
  }

NAPI_FOR_EACH_TYPED_ARRAY(TypedArrayNativeValueTraitsImpl)
#undef TypedArrayNativeValueTraitsImpl

// arraybuffer
Napi::ArrayBuffer NativeValueTraits<IDLArrayBuffer>::NativeValue(
    Napi::Value value, int32_t index) {
  if (value.IsArrayBuffer()) {
    return value.As<Napi::ArrayBuffer>();
  } else {
    InvalidType(value.Env(), index, "ArrayBuffer");
    return Napi::ArrayBuffer();
  }
}

Napi::ArrayBuffer NativeValueTraits<IDLArrayBuffer>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

// arraybufferview
ArrayBufferView NativeValueTraits<IDLArrayBufferView>::NativeValue(
    Napi::Value value, int32_t index) {
  if (value.IsTypedArray() || value.IsDataView()) {
    if (value.IsTypedArray()) {
      return ArrayBufferView::From(value.As<Napi::TypedArray>());
    } else {
      return ArrayBufferView::From(value.As<Napi::DataView>());
    }
  } else {
    InvalidType(value.Env(), index, "ArrayBufferView");
    return ArrayBufferView();
  }
}

ArrayBufferView NativeValueTraits<IDLArrayBufferView>::NativeValue(
    const Napi::CallbackInfo &info, int32_t index) {
  Napi::Value value = GetArgument(info, index);
  return NativeValue(value, index);
}

std::string Utf16LeToUtf8(const char16_t* u16str, size_t length) {
  // Implementation copied from base/ string_utils.cc
  if (length <= 0) {
    return "";
  }
  const char16_t* p = u16str;
  std::u16string::size_type len = length;
  if (p[0] == 0xFEFF) {
    p += 1;
    len -= 1;
  }

  std::string u8str;
  u8str.reserve(len * 3);

  char16_t u16char;
  for (std::u16string::size_type i = 0; i < len; ++i) {
    // Assume little-endian
    u16char = p[i];

    // 1 utf8 code units
    if (u16char < 0x0080) {
      // u16char <= 0x007f
      // U- 0000 0000 ~ 0000 07ff : 0xxx xxxx
      u8str.push_back(static_cast<char>(u16char & 0x00FF));
      continue;
    }
    // 2 utf8 code units
    if (u16char >= 0x0080 && u16char <= 0x07FF) {
      // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
      u8str.push_back(static_cast<char>(((u16char >> 6) & 0x1F) | 0xC0));
      u8str.push_back(static_cast<char>((u16char & 0x3F) | 0x80));
      continue;
    }
    // Surrogate pair
    if (u16char >= 0xD800 && u16char <= 0xDBFF) {
      if (i + 1 >= len) {
        // TODO(yulitao): Need to add an unknown glyph?
        // FML_LOG(ERROR) << "incomplete utf16 encoding.";
        continue;
      }
      // * U-00010000 - U-001FFFFF: 1111 0xxx 10xxxxxx 10xxxxxx 10xxxxxx
      uint32_t highSur = u16char;
      uint32_t lowSur = p[++i];
      // From surrogate pair to unicode codepoint
      // 1. Deal with high surrogate(minus 0xD800, with effective 10bit left)
      // 2. Deal with low surrogate(minus 0xDC00, with effective 10bit left)
      // 3. Plus 0x10000, results in unicode codepoint.
      uint32_t codePoint = highSur - 0xD800;
      codePoint <<= 10;
      codePoint |= lowSur - 0xDC00;
      codePoint += 0x10000;
      // Encode to utf8 with 4 bytes.
      u8str.push_back(static_cast<char>((codePoint >> 18) | 0xF0));
      u8str.push_back(static_cast<char>(((codePoint >> 12) & 0x3F) | 0x80));
      u8str.push_back(static_cast<char>(((codePoint >> 06) & 0x3F) | 0x80));
      u8str.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
      continue;
    }
    // 3 utf8 code units
    {
      // * U-0000E000 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
      u8str.push_back(static_cast<char>(((u16char >> 12) & 0x0F) | 0xE0));
      u8str.push_back(static_cast<char>(((u16char >> 6) & 0x3F) | 0x80));
      u8str.push_back(static_cast<char>((u16char & 0x3F) | 0x80));
      continue;
    }
  }

  return u8str;
}

}  // namespace binding
}  // namespace lynx
