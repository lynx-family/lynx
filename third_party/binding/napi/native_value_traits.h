// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_NATIVE_VALUE_TRAITS_H_
#define BINDING_NAPI_NATIVE_VALUE_TRAITS_H_

#include <memory>
#include <string>
#include <vector>

#include "third_party/binding/napi/exception_message.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {
namespace binding {

class ArrayBufferView;

#define IDLTypedArrayDecl(CLAZZ, NAPI_TYPE, C_TYPE) \
  struct IDL##CLAZZ {};

struct IDLBoolean {};
struct IDLNumber {};
struct IDLString {};
struct IDLUnrestrictedFloat {};
struct IDLFloat {};
struct IDLUnrestrictedDouble {};
struct IDLDouble {};
struct IDLObject {};
struct IDLTypedArray {};
struct IDLArrayBuffer {};
struct IDLArrayBufferView {};
NAPI_FOR_EACH_TYPED_ARRAY(IDLTypedArrayDecl)
#undef IDLTypedArrayDecl

void InvalidType(const Napi::Env& env, int32_t index, const char* expecting);

Napi::Value GetArgument(const Napi::CallbackInfo& info, int32_t index);

int NonUndefinedArgumentLength(const Napi::CallbackInfo& info);

std::string Utf16LeToUtf8(const char16_t* u16str, size_t length);

template <typename T, typename SFINAEHelper = void>
struct NativeValueTraits {};

// boolean
template <>
struct NativeValueTraits<IDLBoolean> {
  static Napi::Boolean NativeValue(Napi::Value value, int32_t index = 0);
  static Napi::Boolean NativeValue(const Napi::CallbackInfo& info,
                                   int32_t index = 0);
};

// number
template <>
struct NativeValueTraits<IDLNumber> {
  static Napi::Number NativeValue(Napi::Value value, int32_t index = 0);
  static Napi::Number NativeValue(const Napi::CallbackInfo& info,
                                  int32_t index = 0);
};

// unrestricted float
template <>
struct NativeValueTraits<IDLUnrestrictedFloat> {
  static float NativeValue(Napi::Value value, int32_t index = 0);
  static float NativeValue(const Napi::CallbackInfo& info, int32_t index = 0);
};

// restricted float
template <>
struct NativeValueTraits<IDLFloat> {
  static float NativeValue(Napi::Value value, int32_t index = 0);
  static float NativeValue(const Napi::CallbackInfo& info, int32_t index = 0);
};

// unrestricted double
template <>
struct NativeValueTraits<IDLUnrestrictedDouble> {
  static double NativeValue(Napi::Value value, int32_t index = 0);
  static double NativeValue(const Napi::CallbackInfo& info, int32_t index = 0);
};

// restricted double
template <>
struct NativeValueTraits<IDLDouble> {
  static double NativeValue(Napi::Value value, int32_t index = 0);
  static double NativeValue(const Napi::CallbackInfo& info, int32_t index = 0);
};

// string
template <>
struct NativeValueTraits<IDLString> {
  static Napi::String NativeValue(Napi::Value value, int32_t index = 0);
  static Napi::String NativeValue(const Napi::CallbackInfo& info,
                                  int32_t index = 0);
};

// callback function
template <typename T>
struct IDLFunction {};

template <typename T>
struct NativeValueTraits<IDLFunction<T>> {
  static std::unique_ptr<T> NativeValue(Napi::Value value, int32_t index = 0) {
    if (value.IsFunction()) {
      return std::make_unique<T>(value.As<Napi::Function>());
    } else {
      InvalidType(value.Env(), index, "Callback Function");
      return nullptr;
    }
  }

  static std::unique_ptr<T> NativeValue(const Napi::CallbackInfo& info,
                                        int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

// object
template <>
struct NativeValueTraits<IDLObject> {
  static Napi::Object NativeValue(Napi::Value value, int32_t index = 0);
  static Napi::Object NativeValue(const Napi::CallbackInfo& info,
                                  int32_t index = 0);
};

// arraybuffer
template <>
struct NativeValueTraits<IDLArrayBuffer> {
  static Napi::ArrayBuffer NativeValue(Napi::Value value, int32_t index = 0);
  static Napi::ArrayBuffer NativeValue(const Napi::CallbackInfo& info,
                                       int32_t index = 0);
};

// arraybufferview
template <>
struct NativeValueTraits<IDLArrayBufferView> {
  static ArrayBufferView NativeValue(Napi::Value value, int32_t index = 0);
  static ArrayBufferView NativeValue(const Napi::CallbackInfo& info,
                                     int32_t index = 0);
};

// typedarray
#define TypedArrayNativeValueTraitsDecl(CLAZZ, NAPI_TYPE, C_TYPE)         \
  template <>                                                             \
  struct NativeValueTraits<IDL##CLAZZ> {                                  \
    static Napi::CLAZZ NativeValue(Napi::Value value, int32_t index = 0); \
    static Napi::CLAZZ NativeValue(const Napi::CallbackInfo& info,        \
                                   int32_t index = 0);                    \
  };

NAPI_FOR_EACH_TYPED_ARRAY(TypedArrayNativeValueTraitsDecl)
#undef TypedArrayNativeValueTraitsDecl

// dictionary
template <typename T>
struct IDLDictionary {};

template <typename T>
struct NativeValueTraits<IDLDictionary<T>> {
  typedef decltype(T::ToImpl(Napi::Value())) ImplType;
  static ImplType NativeValue(Napi::Value value, int32_t index = 0) {
    ImplType result = T::ToImpl(value);
    if (value.Env().IsExceptionPending()) {
      return nullptr;
    }
    return result;
  }
  static ImplType NativeValue(const Napi::CallbackInfo& info,
                              int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

// wrapped object
template <typename T>
T* SafeUnwrap(Napi::Value value) {
  // Used when an exception is not expected ('soft' errors), e.g. union matching
  // and overload resolution.
  if (!value.IsObject()) {
    return nullptr;
  }
  auto* wrappable =
      Napi::ObjectWrap<Napi::ScriptWrappable>::Unwrap(value.As<Napi::Object>());
  if (T::IsInstance(wrappable)) {
    return static_cast<T*>(wrappable);
  }
  return nullptr;
}

template <typename T>
struct NativeValueTraits<
    T, typename std::enable_if_t<std::is_base_of<BridgeBase, T>::value>> {
  typedef decltype(Napi::ObjectWrap<T>::Unwrap(Napi::Object())
      ->ToImplUnsafe()) ImplType;
  static ImplType NativeValue(Napi::Value value, int32_t index = 0) {
    if (auto* bridge = SafeUnwrap<T>(value)) {
      return bridge->ToImplUnsafe();
    } else {
      InvalidType(value.Env(), index, T::InterfaceName());
      return nullptr;
    }
  }
  static ImplType NativeValue(const Napi::CallbackInfo& info,
                              int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

// sequence
template <typename T>
struct IDLSequence {};

template <typename T>
struct NativeValueTraits<
    IDLSequence<T>, typename std::enable_if_t<!std::is_integral<T>::value>> {
  typedef decltype(NativeValueTraits<T>::NativeValue(Napi::Value(),
                                                     int32_t())) ElementType;
  static std::vector<ElementType> NativeValue(Napi::Value value,
                                              int32_t index = 0) {
    if (value.IsArray()) {
      std::vector<ElementType> dst;
      auto array = value.As<Napi::Array>();
      auto len = array.Length();
      dst.resize(len);
      for (uint32_t i = 0; i < len; i++) {
        Napi::Value val = array[i];
        dst[i] = NativeValueTraits<T>::NativeValue(val, index);
        if (val.Env().IsExceptionPending()) {
          return std::vector<ElementType>();
        }
      }
      return dst;
    } else {
      InvalidType(value.Env(), index, "Array");
      return std::vector<ElementType>();
    }
  }
  static std::vector<ElementType> NativeValue(const Napi::CallbackInfo& info,
                                              int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

template <typename T>
struct NativeValueTraits<
    IDLSequence<T>, typename std::enable_if_t<std::is_integral<T>::value>> {
  static std::vector<T> NativeValue(Napi::Value value, int32_t index = 0) {
    if (value.IsArray()) {
      std::vector<T> dst;
      auto array = value.As<Napi::Array>();
      auto len = array.Length();
      dst.resize(len);
      for (uint32_t i = 0; i < len; i++) {
        Napi::Value val = array[i];
        dst[i] = NativeValueTraits<IDLNumber>::NativeValue(val, index);
        if (val.Env().IsExceptionPending()) {
          return std::vector<T>();
        }
      }
      return dst;
    } else {
      InvalidType(value.Env(), index, "Array");
      return std::vector<T>();
    }
  }
  static std::vector<T> NativeValue(const Napi::CallbackInfo& info,
                                    int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

template <>
struct NativeValueTraits<uint64_t> {
  static uint64_t NativeValue(Napi::Value value, int32_t index = 0) {
    return NativeValueTraits<IDLNumber>::NativeValue(value, index);
  }
  static uint64_t NativeValue(const Napi::CallbackInfo& info,
                              int32_t index = 0) {
    return NativeValueTraits<IDLNumber>::NativeValue(info, index);
  }
};

template <typename K, typename V>
struct IDLRecord {};

template <typename K, typename V>
struct NativeValueTraits<IDLRecord<K, V>> {
  typedef decltype(NativeValueTraits<K>::NativeValue(Napi::Value(),
                                                     int32_t())) KeyType;
  typedef decltype(NativeValueTraits<V>::NativeValue(Napi::Value(),
                                                     int32_t())) ValueType;
  static std::vector<std::pair<std::string, ValueType>> NativeValue(
      Napi::Value value, int32_t index = 0) {
    // Converts a JavaScript value |O| to an IDL record<K, V> value. In C++, a
    // record is represented as a std::vector<std::pair<k, v>>.
    // See https://webidl.spec.whatwg.org/#es-record
    Napi::Env env = value.Env();

    // "2. Let result be a new empty instance of record<K, V>."
    std::vector<std::pair<std::string, ValueType>> result;

    // "1. If Type(O) is not Object, throw a TypeError."
    if (!value.IsObject()) {
      InvalidType(env, index, "Array");
      return result;
    }
    Napi::Object obj = value.ToObject();

    // "3. Let keys be ? O.[[OwnPropertyKeys]]()."
    Napi::Array keys = obj.GetPropertyNames();

    // "4. Repeat, for each element key of keys in List order:"
    for (uint32_t i = 0; i < keys.Length(); ++i) {
      // "4.2.1. Let typedKey be key converted to an IDL value of type K."
      std::string key = NativeValueTraits<K>::NativeValue(keys[i]).Utf8Value();
      if (env.IsExceptionPending()) {
        return result;
      }

      // "4.2.2. Let value be ? Get(O, key)."
      Napi::Value value = obj[key.c_str()];

      // "4.2.3. Let typedValue be value converted to an IDL value of type V."
      ValueType typed_value = NativeValueTraits<V>::NativeValue(value);
      if (env.IsExceptionPending()) {
        return result;
      }
      // Skipping: "4.2.4. If typedKey is already a key in result, set its value
      // to typedValue"
      // "4.2.5. Otherwise, append to result a mapping (typedKey, typedValue)."
      result.push_back({key, typed_value});
    }
    return result;
  }
  static std::vector<std::pair<std::string, ValueType>> NativeValue(
      const Napi::CallbackInfo& info, int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

// nullable
template <typename T>
struct IDLNullable {
  typedef decltype(NativeValueTraits<T>::NativeValue(Napi::Value(),
                                                     int32_t())) ImplType;
};

// nullable sequence, dictionary, typedarray, arraybuffer, arraybufferview,
// object, string, function, number
template <typename T>
struct NativeValueTraits<IDLNullable<T>,
                         typename std::enable_if_t<!std::is_pointer<
                             typename IDLNullable<T>::ImplType>::value>> {
  using ImplType =
      typename std::conditional<std::is_same<T, IDLString>::value, std::string,
                                typename IDLNullable<T>::ImplType>::type;
  static ImplType NativeValue(Napi::Value value, int32_t index = 0) {
    if (value.IsNull() || value.IsUndefined()) {
      return ImplType();
    } else {
      return NativeValueTraits<T>::NativeValue(value, index);
    }
  }
  static ImplType NativeValue(const Napi::CallbackInfo& info,
                              int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

// nullable wrapped object
template <typename T>
struct NativeValueTraits<IDLNullable<T>,
                         typename std::enable_if_t<std::is_pointer<
                             typename IDLNullable<T>::ImplType>::value>> {
  typedef typename IDLNullable<T>::ImplType ImplType;
  static ImplType NativeValue(Napi::Value value, int32_t index = 0) {
    if (value.IsNull() || value.IsUndefined()) {
      return nullptr;
    } else {
      return NativeValueTraits<T>::NativeValue(value, index);
    }
  }
  static ImplType NativeValue(const Napi::CallbackInfo& info,
                              int32_t index = 0) {
    Napi::Value value = GetArgument(info, index);
    return NativeValue(value, index);
  }
};

template <typename T>
auto ToRawPtr(T&& ptr) -> decltype(ptr.get()) {
  static_assert(std::is_same_v<T, std::unique_ptr<typename T::element_type>>,
                "Type must be std::unique_ptr or raw pointer");
  return ptr.release();
}

template <typename T>
T* ToRawPtr(T* ptr) {
  return ptr;
}

inline std::nullptr_t ToRawPtr(std::nullptr_t ptr) { return nullptr; }

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_NAPI_NATIVE_VALUE_TRAITS_H_
