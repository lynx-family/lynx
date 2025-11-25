/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

#ifndef PRIMJS_SRC_NAPI_H_
#define PRIMJS_SRC_NAPI_H_

#include <algorithm>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "js_native_api.h"
#include "napi_module.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif

// VS2015 RTM has bugs with constexpr, so require min of VS2015 Update 3 (known
// good version)
#if !defined(_MSC_VER) || _MSC_FULL_VER >= 190024210
#define NAPI_HAS_CONSTEXPR 1
#endif

// VS2013 does not support char16_t literal strings, so we'll work around it
// using wchar_t strings and casting them. This is safe as long as the character
// sizes are the same.
#if defined(_MSC_VER) && _MSC_VER <= 1800
static_assert(sizeof(char16_t) == sizeof(wchar_t),
              "Size mismatch between char16_t and wchar_t");
#define NAPI_WIDE_TEXT(x) reinterpret_cast<char16_t*>(L##x)
#else
#define NAPI_WIDE_TEXT(x) u##x
#endif

#if !defined(NAPI_CPP_RTTI) && !defined(NAPI_DISABLE_CPP_RTTI)
#if (defined(__clang__) && __has_feature(cxx_rtti)) || defined(__GXX_RTTI) || \
    defined(_CPPRTTI)
#define NAPI_CPP_RTTI
#endif
#endif

#ifdef NAPI_CPP_RTTI
#include <typeindex>
#endif

#ifdef _NOEXCEPT
#define NAPI_NOEXCEPT _NOEXCEPT
#else
#define NAPI_NOEXCEPT noexcept
#endif

#define NAPI_DISALLOW_ASSIGN(CLASS) void operator=(const CLASS&) = delete;
#define NAPI_DISALLOW_COPY(CLASS) CLASS(const CLASS&) = delete;

#define NAPI_DISALLOW_ASSIGN_COPY(CLASS) \
  NAPI_DISALLOW_ASSIGN(CLASS)            \
  NAPI_DISALLOW_COPY(CLASS)

////////////////////////////////////////////////////////////////////////////////
/// N-API C++ Wrapper Classes
///
////////////////////////////////////////////////////////////////////////////////
namespace Napi {

// Forward declarations
template <class T>
class Maybe;
class Env;
template <class T>
class MaybeValue;
class Value;
class Boolean;
class Number;
class String;
class Object;
class Array;
class ArrayBuffer;
class Function;
class Error;
class PropertyDescriptor;
class CallbackInfo;
class TypedArray;

class MemoryManagement;

/**
 * Container class for static utility functions.
 */
class NAPI_EXTERN NAPI {
 private:
  static NAPI_NO_RETURN void FromJustIsNothing();
  static NAPI_NO_RETURN void ToValueEmpty();

  static napi_ref CreateReference(napi_env env, napi_value value,
                                  uint32_t refcount);
  static void DeleteReference(napi_env env, napi_ref ref);
  static napi_value GetReferenceValue(napi_env env, napi_ref ref);
  static uint32_t ReferenceRef(napi_env env, napi_ref ref);
  static uint32_t ReferenceUnRef(napi_env env, napi_ref ref);
  static void* Unwrap(napi_env env, napi_value obj);

  static napi_ref Wrap(napi_env env, napi_value obj, void* data,
                       napi_finalize finalize_cb, void* hint);
  static void* RemoveWrap(napi_env env, napi_value obj);
  static napi_class DefineClass(napi_env env, const char* utf8name,
                                napi_callback ctor, size_t props_count,
                                const napi_property_descriptor* descriptors,
                                void* data, napi_class super_class);

  template <class T>
  friend class Maybe;

  template <class T>
  friend class MaybeValue;

  template <class T>
  friend class Reference;

  template <class T>
  friend class InstanceWrap;

  template <class T>
  friend class ObjectWrap;

  template <typename ContextType, typename DataType,
            void (*CallJs)(Napi::Env, ContextType*, DataType)>
  friend class ThreadSafeFunction;
};

template <class T>
class Maybe {
 public:
  NAPI_INLINE bool IsNothing() const { return !_has_value; }
  NAPI_INLINE bool IsJust() const { return _has_value; }

  NAPI_INLINE operator T() const { return FromJust(); }

  /**
   * An alias for |FromJust|. Will crash if the Maybe<> is nothing.
   */
  NAPI_INLINE T ToChecked() const { return FromJust(); }

  /**
   * Converts this Maybe<> to a value of type T. If this Maybe<> is
   * nothing (empty), |false| is returned and |out| is left untouched.
   */
  NAPI_INLINE bool To(T* out) const {
    if (IsJust()) *out = _value;
    return IsJust();
  }

  /**
   * Converts this Maybe<> to a value of type T. If this Maybe<> is
   * nothing (empty), NAPI will crash the process.
   */
  NAPI_INLINE T FromJust() const {
    if (!IsJust()) NAPI::FromJustIsNothing();
    return _value;
  }

  /**
   * Converts this Maybe<> to a value of type T, using a default value if this
   * Maybe<> is nothing (empty).
   */
  NAPI_INLINE T FromMaybe(const T& default_value) const {
    return _has_value ? _value : default_value;
  }

  NAPI_INLINE bool operator==(const Maybe& other) const {
    return (IsJust() == other.IsJust()) &&
           (!IsJust() || FromJust() == other.FromJust());
  }

  NAPI_INLINE bool operator!=(const Maybe& other) const {
    return !operator==(other);
  }

 private:
  Maybe() : _has_value(false) {}
  explicit Maybe(const T& t) : _has_value(true), _value(t) {}

  bool _has_value;
  T _value;

  template <class U>
  friend Maybe<U> Nothing();
  template <class U>
  friend Maybe<U> Just(const U& u);
};

template <class T>
inline Maybe<T> Nothing() {
  return Maybe<T>();
}

template <class T>
inline Maybe<T> Just(const T& t) {
  return Maybe<T>(t);
}

// A template specialization of Maybe<T> for the case of T = void.
template <>
class Maybe<void> {
 public:
  NAPI_INLINE bool IsNothing() const { return !_is_valid; }
  NAPI_INLINE bool IsJust() const { return _is_valid; }

  NAPI_INLINE bool operator==(const Maybe& other) const {
    return IsJust() == other.IsJust();
  }

  NAPI_INLINE bool operator!=(const Maybe& other) const {
    return !operator==(other);
  }

 private:
  struct JustTag {};

  Maybe() : _is_valid(false) {}
  explicit Maybe(JustTag) : _is_valid(true) {}

  bool _is_valid;

  template <class U>
  friend Maybe<U> Nothing();
  friend Maybe<void> JustVoid();
};

inline Maybe<void> JustVoid() { return Maybe<void>(Maybe<void>::JustTag()); }

/// Environment for N-API values and operations.
///
/// All N-API values and operations must be associated with an environment. An
/// environment instance is always provided to callback functions; that
/// environment must then be used for any creation of N-API values or other
/// N-API operations within the callback. (Many methods infer the environment
/// from the `this` instance that the method is called on.)
///
/// In the future, multiple environments per process may be supported, although
/// current implementations only support one environment per process.
///
/// In the V8 JavaScript engine, a N-API environment approximately corresponds
/// to an Isolate.
class NAPI_EXTERN Env {
 private:
  template <typename T>
  using IfKeyed = std::enable_if_t<std::is_integral<decltype(T::KEY)>::value>;

 public:
  NAPI_INLINE Env(napi_env env) : _env(env) {}

  NAPI_INLINE operator napi_env() const { return _env; }

  Object Global() const;
  Value Undefined() const;
  Value Null() const;
  Object Loader() const;

  bool IsExceptionPending() const;
  Value GetAndClearPendingException();
  Value GetUnhandledRecjectionException();

#ifdef ENABLE_CODECACHE
  static constexpr int CACHE_META_NUMS = 6;
  void InitCodeCache(int capacity, const std::string& filename,
                     std::function<void(bool)> callback);
  void OutputCodeCache();
  void DumpCacheStatus(std::vector<std::pair<std::string, int>>* dump_vec);
  Value RunScriptCache(const char* utf8script, size_t length,
                       const char* filename = nullptr);

  NAPI_INLINE Value RunScriptCache(const char* utf8script,
                                   const char* filename = nullptr);
#endif  // ENABLE_CODECACHE
  Value RunScript(const char* utf8script, size_t length,
                  const char* filename = nullptr);

  NAPI_INLINE Value RunScript(const char* utf8script,
                              const char* filename = nullptr);

  template <typename T, typename Enabled = Env::IfKeyed<T>>
  NAPI_INLINE T* GetInstanceData();

  template <typename T, typename Enabled = Env::IfKeyed<T>>
  NAPI_INLINE void SetInstanceData(T* data);

  template <typename T>
  NAPI_INLINE T* GetInstanceData(uint64_t key);

  template <typename T>
  NAPI_INLINE void SetInstanceData(uint64_t key, T* data);

  template <typename T>
  NAPI_INLINE void AddCleanupHook(void (*cb)(T*), T* data);

  template <typename T>
  NAPI_INLINE void RemoveCleanupHook(void (*cb)(T*), T* data);

  void* GetInstanceData(uint64_t key);
  void SetInstanceData(uint64_t key, void* data, napi_finalize finalize_cb,
                       void* hint);

  void AddCleanupHook(void (*cb)(void*), void* data);
  void RemoveCleanupHook(void (*cb)(void*), void* data);

 private:
  napi_env _env;
};

/// A JavaScript value of unknown type.
///
/// For type-specific operations, convert to one of the Value subclasses using a
/// `To*` or `As()` method. The `To*` methods do type coercion; the `As()`
/// method does not.
///
///     Napi::Value value = ...
///     if (!value.IsString()) throw Napi::TypeError::New(env, "Invalid
///     arg..."); Napi::String str = value.As<Napi::String>(); // Cast to a
///     string value
///
///     Napi::Value anotherValue = ...
///     bool isTruthy = anotherValue.ToBoolean(); // Coerce to a boolean value
class NAPI_EXTERN Value {
 public:
  NAPI_INLINE Value() : _env(nullptr), _value(nullptr) {}

  /// < Wraps a N-API value primitive.
  NAPI_INLINE Value(napi_env env, napi_value value)
      : _env(env), _value(value) {}

  /// Creates a JS value from a C++ primitive.
  ///
  /// `value` may be any of:
  /// - bool
  /// - Any integer type
  /// - Any floating point type
  /// - const char* (encoded using UTF-8, null-terminated)
  /// - const char16_t* (encoded using UTF-16-LE, null-terminated)
  /// - std::string (encoded using UTF-8)
  /// - std::u16string
  /// - napi::Value
  /// - napi_value
  template <typename T>
  static Value From(napi_env env, const T& value);

  NAPI_INLINE bool IsEmpty() const { return _value == nullptr; }

  /// Converts to a N-API value primitive.
  ///
  /// If the instance is _empty_, this returns `nullptr`.
  NAPI_INLINE operator napi_value() const { return _value; }

  /// Gets the environment the value is associated with.
  NAPI_INLINE Napi::Env Env() const { return Napi::Env(_env); }

  /// Tests if this value strictly equals another value.
  NAPI_INLINE bool operator==(const Value& other) const {
    return StrictEquals(other);
  }

  /// Tests if this value does not strictly equal another value.
  NAPI_INLINE bool operator!=(const Value& other) const {
    return !StrictEquals(other);
  }

  /// Tests if this value strictly equals another value.
  bool StrictEquals(const Value& other) const;

  Maybe<bool> Equals(const Value& other) const;

  napi_valuetype Type() const;  ///< Gets the type of the value.

  bool IsUndefined()
      const;            ///< Tests if a value is an undefined JavaScript value.
  bool IsNull() const;  ///< Tests if a value is a null JavaScript value.
  bool IsBoolean() const;  ///< Tests if a value is a JavaScript boolean.
  bool IsNumber() const;   ///< Tests if a value is a JavaScript number.

  bool IsString() const;  ///< Tests if a value is a JavaScript string.
  bool IsSymbol() const;  ///< Tests if a value is a JavaScript symbol.
  bool IsArray() const;   ///< Tests if a value is a JavaScript array.
  bool IsArrayBuffer()
      const;  ///< Tests if a value is a JavaScript array buffer.

  bool IsTypedArray() const;  ///< Tests if a value is a JavaScript typed array.
  bool IsInt8Array() const;   ///< Tests if a value is a JavaScript int8 array.
  bool IsUint8Array() const;  ///< Tests if a value is a JavaScript uint8 array.
  bool IsUint8ClampedArray()
      const;  ///< Tests if a value is a JavaScript uint8 clamped array.
  bool IsInt16Array() const;  ///< Tests if a value is a JavaScript int16 array.
  bool IsUint16Array()
      const;  ///< Tests if a value is a JavaScript uint16 array.
  bool IsInt32Array() const;  ///< Tests if a value is a JavaScript int32 array.
  bool IsUint32Array()
      const;  ///< Tests if a value is a JavaScript uint32 array.
  bool IsFloat32Array()
      const;  ///< Tests if a value is a JavaScript float32 array.
  bool IsFloat64Array()
      const;  ///< Tests if a value is a JavaScript float64 array.

  bool IsObject() const;    ///< Tests if a value is a JavaScript object.
  bool IsFunction() const;  ///< Tests if a value is a JavaScript function.
  bool IsPromise() const;   ///< Tests if a value is a JavaScript promise.
  bool IsDataView() const;  ///< Tests if a value is a JavaScript data view.
  bool IsBuffer() const;    ///< Tests if a value is a Node buffer.
  bool IsExternal() const;  ///< Tests if a value is a pointer to external data.

  /// Casts to another type of `Napi::Value`, when the actual type is known or
  /// assumed.
  ///
  /// This conversion does NOT coerce the type. Calling any methods
  /// inappropriate for the actual value type may crash.
  template <typename T>
  NAPI_INLINE T As() const {
    return T(_env, _value);
  }

  Boolean ToBoolean() const;  ///< Coerces a value to a JavaScript boolean.
  Number ToNumber() const;    ///< Coerces a value to a JavaScript number.
  String ToString() const;    ///< Coerces a value to a JavaScript string.
  Object ToObject() const;    ///< Coerces a value to a JavaScript object.

 protected:
  /// !cond INTERNAL
  napi_env _env;
  napi_value _value;
  /// !endcond

 private:
  template <class T>
  friend class MaybeValue;

  template <class T>
  friend class Reference;
};

/// A JavaScript boolean value.
class NAPI_EXTERN Boolean : public Value {
 public:
  static Boolean New(napi_env env,  ///< N-API environment
                     bool value     ///< Boolean value
  );

  NAPI_INLINE Boolean() : Napi::Value() {}

  NAPI_INLINE Boolean(napi_env env, napi_value value)
      : Napi::Value(env, value) {}  ///< Wraps a N-API value primitive.

  /// < Converts a Boolean value to a boolean primitive.
  NAPI_INLINE operator bool() const { return Value(); }

  bool Value() const;  ///< Converts a Boolean value to a boolean primitive.
};

/// A JavaScript number value.
class NAPI_EXTERN Number : public Value {
 public:
  static Number New(napi_env env,  ///< N-API environment
                    double value   ///< Number value
  );

  NAPI_INLINE Number() : Napi::Value() {}

  NAPI_INLINE Number(napi_env env, napi_value value)
      : Value(env, value) {}  ///< Wraps a N-API value primitive.

  NAPI_INLINE operator int32_t() const { return Int32Value(); }

  NAPI_INLINE operator uint32_t() const { return Uint32Value(); }

  NAPI_INLINE operator int64_t() const { return Int64Value(); }

  NAPI_INLINE operator uint64_t() const { return static_cast<uint64_t>(Int64Value()); }

  NAPI_INLINE operator float() const { return FloatValue(); }

  NAPI_INLINE operator double() const { return DoubleValue(); }

  int32_t Int32Value()
      const;  ///< Converts a Number value to a 32-bit signed integer value.
  uint32_t Uint32Value()
      const;  ///< Converts a Number value to a 32-bit unsigned integer value.
  int64_t Int64Value()
      const;  ///< Converts a Number value to a 64-bit signed integer value.
  float FloatValue()
      const;  ///< Converts a Number value to a 32-bit floating-point value.
  double DoubleValue()
      const;  ///< Converts a Number value to a 64-bit floating-point value.
};

/// A JavaScript string or symbol value (that can be used as a property name).
class NAPI_EXTERN Name : public Value {
 public:
  NAPI_INLINE Name() : Napi::Value() {}

  NAPI_INLINE Name(napi_env env, napi_value value) : Value(env, value) {}
};

/// A JavaScript string value.
class NAPI_EXTERN String : public Name {
 public:
  /// Creates a new String value from a UTF-8 encoded C string.
  NAPI_INLINE static String New(
      napi_env env,             ///< N-API environment
      const std::string& value  ///< UTF-8 encoded null-terminated C string
  ) {
    return New(env, value.c_str());
  }

  /// Creates a new String value from a UTF-16 encoded C string.
  NAPI_INLINE static String New(
      napi_env env,                ///< N-API environment
      const std::u16string& value  ///< UTF-16 encoded null-terminated C string
  ) {
    return New(env, value.c_str());
  }

  /// Creates a new String value from a UTF-8 encoded C string.
  static String New(
      napi_env env,      ///< N-API environment
      const char* value  ///< UTF-8 encoded null-terminated C string
  );

  /// Creates a new String value from a UTF-16 encoded C string.
  static String New(
      napi_env env,          ///< N-API environment
      const char16_t* value  ///< UTF-16 encoded null-terminated C string
  );

  /// Creates a new String value from a UTF-8 encoded C string with specified
  /// length.
  static String New(napi_env env,       ///< N-API environment
                    const char* value,  ///< UTF-8 encoded C string (not
                                        ///< necessarily null-terminated)
                    size_t length       ///< length of the string in bytes
  );

  /// Creates a new String value from a UTF-16 encoded C string with specified
  /// length.
  static String New(
      napi_env env,           ///< N-API environment
      const char16_t* value,  ///< UTF-16 encoded C string (not
                              ///< necessarily null-terminated)
      size_t length           ///< Length of the string in 2-byte code units
  );

  /// Creates a new String based on the original object's type.
  ///
  /// `value` may be any of:
  /// - const char* (encoded using UTF-8, null-terminated)
  /// - const char16_t* (encoded using UTF-16-LE, null-terminated)
  /// - std::string (encoded using UTF-8)
  /// - std::u16string
  template <typename T>
  static String From(napi_env env, const T& value);

  NAPI_INLINE String() : Name() {}

  NAPI_INLINE String(napi_env env, napi_value value) : Name(env, value) {}

  NAPI_INLINE operator std::string() const { return Utf8Value(); }
  NAPI_INLINE operator std::u16string() const { return Utf16Value(); }

  std::string Utf8Value()
      const;  ///< Converts a String value to a UTF-8 encoded C++ string.
  std::u16string Utf16Value()
      const;  ///< Converts a String value to a UTF-16 encoded C++ string.
};

/// A JavaScript symbol value.
class NAPI_EXTERN Symbol : public Name {
 public:
  /// Creates a new Symbol value with an optional description.
  static Symbol New(
      napi_env env,  ///< N-API environment
      const char* description =
          nullptr  ///< Optional UTF-8 encoded null-terminated C string
                   ///  describing the symbol
  );

  /// Creates a new Symbol value with a description.
  static Symbol New(napi_env env,       ///< N-API environment
                    String description  ///< String value describing the symbol
  );

  /// Creates a new Symbol value with a description.
  static Symbol New(
      napi_env env,           ///< N-API environment
      napi_value description  ///< String value describing the symbol
  );

  /// Get a public Symbol (e.g. Symbol.iterator).
  static Symbol WellKnown(napi_env, const char* name);

  NAPI_INLINE Symbol() : Name() {}

  NAPI_INLINE Symbol(napi_env env, napi_value value) : Name(env, value) {}
};

/// A JavaScript object value.
class NAPI_EXTERN Object : public Value {
 public:
  /// Enables property and element assignments using indexing syntax.
  ///
  /// Example:
  ///
  ///     Napi::Value propertyValue = object1['A'];
  ///     object2['A'] = propertyValue;
  ///     Napi::Value elementValue = array[0];
  ///     array[1] = elementValue;
  template <typename Key>
  class PropertyLValue {
   public:
    /// Converts an L-value to a value.
    NAPI_INLINE operator Value() const {
      return Object(_env, _object).Get(_key);
    }

    /// Assigns a value to the property. The type of value can be
    /// anything supported by `Object::Set`.
    template <typename ValueType>
    NAPI_INLINE Maybe<void> operator=(ValueType value) {
      return Object(_env, _object).Set(_key, value);
    }

   private:
    PropertyLValue() = delete;
    NAPI_INLINE PropertyLValue(Object object, Key key)
        : _env(object.Env()), _object(object), _key(key) {}
    napi_env _env;
    napi_value _object;
    Key _key;

    friend class Napi::Object;
  };

  /// Creates a new Object value.
  static Object New(napi_env env  ///< N-API environment
  );

  static Object GetOwnPropertyDescriptor(napi_env env, Value obj, Value prop);

  NAPI_INLINE Object() : Value() {}

  NAPI_INLINE Object(napi_env env, napi_value value) : Value(env, value) {}

  /// Gets or sets a named property.
  NAPI_INLINE PropertyLValue<const char*> operator[](
      const char* utf8name  ///< UTF-8 encoded null-terminated property name
  ) {
    return PropertyLValue<const char*>(*this, utf8name);
  }

  /// Gets or sets an indexed property or array element.
  NAPI_INLINE PropertyLValue<uint32_t> operator[](
      uint32_t index  /// Property / element index
  ) {
    return PropertyLValue<uint32_t>(*this, index);
  }

  /// Gets a named property.
  NAPI_INLINE Value operator[](
      const char* utf8name  ///< UTF-8 encoded null-terminated property name
  ) const {
    return Get(utf8name);
  }

  /// Gets an indexed property or array element.
  NAPI_INLINE Value operator[](uint32_t index  ///< Property / element index
  ) const {
    return Get(index);
  }

  /// Checks whether a property is present.
  NAPI_INLINE Maybe<bool> Has(Value key  ///< Property key primitive
  ) const {
    return Has(key.operator napi_value());
  }

  /// Checks whether a property is present.
  Maybe<bool> Has(napi_value key  ///< Property key primitive
  ) const;

  /// Checks whether a named property is present.
  Maybe<bool> Has(
      const char* utf8name  ///< UTF-8 encoded null-terminated property name
  ) const;

  /// Checks whether a own property is present.
  Maybe<bool> HasOwnProperty(napi_value key  ///< Property key primitive
  ) const;

  /// Checks whether a own property is present.
  NAPI_INLINE Maybe<bool> HasOwnProperty(Value key  ///< Property key primitive
  ) const {
    return HasOwnProperty(key.operator napi_value());
  }

  /// Checks whether a own property is present.
  Maybe<bool> HasOwnProperty(
      const char* utf8name  ///< UTF-8 encoded null-terminated property name
  ) const;

  /// Gets a property.
  Value Get(napi_value key  ///< Property key primitive
  ) const;

  /// Gets a property.
  NAPI_INLINE Value Get(Value key  ///< Property key primitive
  ) const {
    return Get(key.operator napi_value());
  }

  /// Gets a named property.
  Value Get(
      const char* utf8name  ///< UTF-8 encoded null-terminated property name
  ) const;

  /// Set a property.
  Maybe<void> Set(napi_value key, napi_value value);

  /// Set a property.
  Maybe<void> Set(const char* utf8name, napi_value value);

  /// Sets a property.
  template <typename ValueType>
  NAPI_INLINE Maybe<void> Set(
      napi_value key,         ///< Property key primitive
      const ValueType& value  ///< Property value primitive
  ) {
    auto v = Value::From(_env, value);
    if (v.IsEmpty()) {
      return Nothing<void>();
    }
    return Set(key, v.operator napi_value());
  }

  /// Sets a named property.
  template <typename ValueType>
  NAPI_INLINE Maybe<void> Set(
      const char* utf8name,  ///< UTF-8 encoded null-terminated property name
      const ValueType& value) {
    auto v = Value::From(_env, value);
    if (v.IsEmpty()) {
      return Nothing<void>();
    }
    return Set(utf8name, v.operator napi_value());
  }

  /// Delete property.
  Maybe<bool> Delete(napi_value key  ///< Property key primitive
  );

  /// Delete property.
  NAPI_INLINE Maybe<bool> Delete(Value key  ///< Property key primitive
  ) {
    return Delete(key.operator napi_value());
  }

  /// Delete property.
  Maybe<bool> Delete(
      const char* utf8name  ///< UTF-8 encoded null-terminated property name
  );

  /// Checks whether an indexed property is present.
  Maybe<bool> Has(uint32_t index  ///< Property / element index
  ) const;

  /// Gets an indexed property or array element.
  Value Get(uint32_t index  ///< Property / element index
  ) const;

  /// Set a property.
  Maybe<void> Set(uint32_t, napi_value value);

  /// Sets an indexed property or array element.
  template <typename ValueType>
  NAPI_INLINE Maybe<void> Set(
      uint32_t index,         ///< Property / element index
      const ValueType& value  ///< Property value primitive
  ) {
    auto v = Value::From(_env, value);
    if (v.IsEmpty()) {
      return Nothing<void>();
    }
    return Set(index, v.operator napi_value());
  }

  /// Deletes an indexed property or array element.
  Maybe<bool> Delete(uint32_t index  ///< Property / element index
  );

  Array GetPropertyNames() const;  ///< Get all property names

  /// Defines a property on the object.
  Maybe<void> DefineProperty(
      const PropertyDescriptor&
          property  ///< Descriptor for the property to be defined
  );

  /// Defines properties on the object.
  Maybe<void> DefineProperties(
      const std::initializer_list<PropertyDescriptor>& properties
      ///< List of descriptors for the properties to be defined
  );

  /// Defines properties on the object.
  Maybe<void> DefineProperties(
      const std::vector<PropertyDescriptor>& properties
      ///< Vector of descriptors for the properties to be defined
  );

  /// Checks if an object is an instance created by a constructor function.
  ///
  /// This is equivalent to the JavaScript `instanceof` operator.
  Maybe<bool> InstanceOf(const Function& constructor  ///< Constructor function
  ) const;

  void AddFinalizer(void* data, napi_finalize cb, void* hint = nullptr);
};

class NAPI_EXTERN Array : public Object {
 public:
  static Array New(napi_env env);
  static Array New(napi_env env, size_t length);

  NAPI_INLINE Array() : Object() {}

  NAPI_INLINE Array(napi_env env, napi_value value) : Object(env, value) {}

  uint32_t Length() const;
};

/// A JavaScript array buffer value.
class NAPI_EXTERN ArrayBuffer : public Object {
 public:
  /// Creates a new ArrayBuffer instance over a new automatically-allocated
  /// buffer.
  static ArrayBuffer New(
      napi_env env,      ///< N-API environment
      size_t byteLength  ///< Length of the buffer to be allocated, in bytes
  );

  /// Creates a new ArrayBuffer instance, using an external buffer with
  /// specified byte length.
  static ArrayBuffer New(
      napi_env env,        ///< N-API environment
      void* externalData,  ///< Pointer to the external buffer
                           ///< to be used by the array
      size_t byteLength    ///< Length of the external buffer to
                           ///< be used by the array, in bytes
  );

  /// Creates a new ArrayBuffer instance, using an external buffer with
  /// specified byte length.
  static ArrayBuffer New(
      napi_env env,        ///< N-API environment
      void* externalData,  ///< Pointer to the external buffer to be used by the
                           ///< array
      size_t byteLength,   ///< Length of the external buffer to be used by the
                           ///< array,
                           ///  in bytes
      napi_finalize
          finalizeCallback,  ///< Function to be called when the array
                             ///< buffer is destroyed;
                             ///  must implement `void operator()(Env
                             ///  env, void* externalData, Hint* hint)`
      void* finalizeHint     ///< Hint (second parameter) to be passed to the
                             ///< finalize callback
  );

  NAPI_INLINE ArrayBuffer() : Object() {}

  NAPI_INLINE ArrayBuffer(napi_env env, napi_value value)
      : Object(env, value) {}  ///< Wraps a N-API value primitive.

  void* Data();         ///< Gets a pointer to the data buffer.
  size_t ByteLength();  ///< Gets the length of the array buffer in bytes.
};

/// A JavaScript typed-array value with unknown array type.
///
/// For type-specific operations, cast to a `TypedArrayOf<T>` instance using the
/// `As()` method:
///
///     Napi::TypedArray array = ...
///     if (t.TypedArrayType() == napi_int32_array) {
///         Napi::Int32Array int32Array = t.As<Napi::Int32Array>();
///     }
class NAPI_EXTERN TypedArray : public Object {
 public:
  NAPI_INLINE TypedArray() : Object(), _type(unknown_array_type), _length(0) {}

  NAPI_INLINE TypedArray(napi_env env, napi_value value)
      : Object(env, value),
        _type(unknown_array_type),
        _length(0) {}  ///< Wraps a N-API value primitive.

  napi_typedarray_type TypedArrayType()
      const;  ///< Gets the type of this typed-array.
  Napi::ArrayBuffer ArrayBuffer() const;  ///< Gets the backing array buffer.

  uint8_t ElementSize()
      const;  ///< Gets the size in bytes of one element in the array.
  size_t ElementLength() const;  ///< Gets the number of elements in the array.
  size_t ByteOffset()
      const;  ///< Gets the offset into the buffer where the array starts.
  size_t ByteLength() const;  ///< Gets the length of the array in bytes.

 protected:
  /// !cond INTERNAL
  napi_typedarray_type _type;
  size_t _length;

  NAPI_INLINE TypedArray(napi_env env, napi_value value,
                         napi_typedarray_type type, size_t length)
      : Object(env, value), _type(type), _length(length) {}

  static const napi_typedarray_type unknown_array_type =
      static_cast<napi_typedarray_type>(-1);
};

#define NAPI_FOR_EACH_TYPED_ARRAY(V)                      \
  V(Int8Array, napi_int8_array, int8_t)                   \
  V(Int16Array, napi_int16_array, int16_t)                \
  V(Int32Array, napi_int32_array, int32_t)                \
  V(Uint8ClampedArray, napi_uint8_clamped_array, uint8_t) \
  V(Uint8Array, napi_uint8_array, uint8_t)                \
  V(Uint16Array, napi_uint16_array, uint16_t)             \
  V(Uint32Array, napi_uint32_array, uint32_t)             \
  V(Float32Array, napi_float32_array, float)              \
  V(Float64Array, napi_float64_array, double)

#define TypedArrayDecl(CLAZZ, NAPI_TYPE, C_TYPE)                          \
  class NAPI_EXTERN CLAZZ : public TypedArray {                           \
   public:                                                                \
    NAPI_INLINE C_TYPE& operator[](size_t index) { return _data[index]; } \
    NAPI_INLINE C_TYPE* Data() { return _data; }                          \
                                                                          \
    static CLAZZ New(napi_env env, size_t elementLength);                 \
    static CLAZZ New(napi_env env, size_t elementLength,                  \
                     Napi::ArrayBuffer arrayBuffer, size_t bufferOffset); \
    NAPI_INLINE CLAZZ() : TypedArray(), _data(nullptr) {}                 \
    CLAZZ(napi_env env, napi_value);                                      \
                                                                          \
   private:                                                               \
    C_TYPE* _data;                                                        \
                                                                          \
    NAPI_INLINE CLAZZ(napi_env env, napi_value value, size_t length,      \
                      C_TYPE* data)                                       \
        : TypedArray(env, value, NAPI_TYPE, length), _data(data) {}       \
  };

NAPI_FOR_EACH_TYPED_ARRAY(TypedArrayDecl)

#undef TypedArrayDecl

/// The DataView provides a low-level interface for reading/writing multiple
/// number types in an ArrayBuffer irrespective of the platform's
/// endianness.
class NAPI_EXTERN DataView : public Object {
 public:
  static DataView New(napi_env env, Napi::ArrayBuffer arrayBuffer);
  static DataView New(napi_env env, Napi::ArrayBuffer arrayBuffer,
                      size_t byteOffset);
  static DataView New(napi_env env, Napi::ArrayBuffer arrayBuffer,
                      size_t byteOffset, size_t byteLength);

  NAPI_INLINE DataView() : Object() {}

  DataView(napi_env env,
           napi_value value);  ///< Wraps a N-API value primitive.

  Napi::ArrayBuffer ArrayBuffer() const;  ///< Gets the backing array buffer.
  size_t ByteOffset()
      const;  ///< Gets the offset into the buffer where the array starts.
  NAPI_INLINE size_t ByteLength() const {
    return _length;
  }  ///< Gets the length of the array in bytes.

  NAPI_INLINE void* Data() const { return _data; }

  NAPI_INLINE float GetFloat32(size_t byteOffset) const {
    return ReadData<float>(byteOffset);
  }
  NAPI_INLINE double GetFloat64(size_t byteOffset) const {
    return ReadData<double>(byteOffset);
  }
  NAPI_INLINE int8_t GetInt8(size_t byteOffset) const {
    return ReadData<int8_t>(byteOffset);
  }
  NAPI_INLINE int16_t GetInt16(size_t byteOffset) const {
    return ReadData<int16_t>(byteOffset);
  }
  NAPI_INLINE int32_t GetInt32(size_t byteOffset) const {
    return ReadData<int32_t>(byteOffset);
  }
  NAPI_INLINE uint8_t GetUint8(size_t byteOffset) const {
    return ReadData<uint8_t>(byteOffset);
  }
  NAPI_INLINE uint16_t GetUint16(size_t byteOffset) const {
    return ReadData<uint16_t>(byteOffset);
  }
  NAPI_INLINE uint32_t GetUint32(size_t byteOffset) const {
    return ReadData<uint32_t>(byteOffset);
  }

  NAPI_INLINE void SetFloat32(size_t byteOffset, float value) const {
    WriteData<float>(byteOffset, value);
  }
  NAPI_INLINE void SetFloat64(size_t byteOffset, double value) const {
    WriteData<double>(byteOffset, value);
  }
  NAPI_INLINE void SetInt8(size_t byteOffset, int8_t value) const {
    WriteData<int8_t>(byteOffset, value);
  }
  NAPI_INLINE void SetInt16(size_t byteOffset, int16_t value) const {
    WriteData<int16_t>(byteOffset, value);
  }
  NAPI_INLINE void SetInt32(size_t byteOffset, int32_t value) const {
    WriteData<int32_t>(byteOffset, value);
  }
  NAPI_INLINE void SetUint8(size_t byteOffset, uint8_t value) const {
    WriteData<uint8_t>(byteOffset, value);
  }
  NAPI_INLINE void SetUint16(size_t byteOffset, uint16_t value) const {
    WriteData<uint16_t>(byteOffset, value);
  }
  NAPI_INLINE void SetUint32(size_t byteOffset, uint32_t value) const {
    WriteData<uint32_t>(byteOffset, value);
  }

 private:
  template <typename T>
  T ReadData(size_t byteOffset) const {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(_data) + byteOffset);
  }

  template <typename T>
  void WriteData(size_t byteOffset, T value) const {
    *reinterpret_cast<T*>(static_cast<uint8_t*>(_data) + byteOffset) = value;
  }

  void* _data;
  size_t _length;
};

class NAPI_EXTERN Function : public Object {
 public:
  typedef Value (*Callback)(const CallbackInfo& info);

  static Function New(napi_env env, Callback cb, const char* utf8name = nullptr,
                      void* data = nullptr);

  NAPI_INLINE Function() : Object() {}

  NAPI_INLINE Function(napi_env env, napi_value value) : Object(env, value) {}

  NAPI_INLINE Value
  operator()(const std::initializer_list<napi_value>& args) const {
    return Call(args);
  }

  NAPI_INLINE Value Call(const std::initializer_list<napi_value>& args) const {
    return Call(args.size(), args.begin());
  }
  NAPI_INLINE Value Call(const std::vector<napi_value>& args) const {
    return Call(args.size(), args.data());
  }
  Value Call(size_t argc, const napi_value* args) const;

  NAPI_INLINE Value Call(napi_value recv,
                         const std::initializer_list<napi_value>& args) const {
    return Call(recv, args.size(), args.begin());
  }
  NAPI_INLINE Value Call(napi_value recv,
                         const std::vector<napi_value>& args) const {
    return Call(recv, args.size(), args.data());
  }
  Value Call(napi_value recv, size_t argc, const napi_value* args) const;

  NAPI_INLINE Object New(const std::initializer_list<napi_value>& args) const {
    return New(args.size(), args.begin());
  }
  NAPI_INLINE Object New(const std::vector<napi_value>& args) const {
    return New(args.size(), args.data());
  }
  Object New(size_t argc, const napi_value* args) const;
};

class NAPI_EXTERN Promise : public Object {
 public:
  class NAPI_EXTERN Deferred {
   public:
    static Deferred New(napi_env env);
    ~Deferred();

    // A reference can be moved but cannot be copied.
    NAPI_INLINE Deferred(Deferred&& other) {
      _env = other._env;
      _deferred = other._deferred;
      _promise = other._promise;
      other._env = nullptr;
      other._deferred = nullptr;
      other._promise = nullptr;
    }

    NAPI_INLINE Deferred& operator=(Deferred&& other) {
      _env = other._env;
      _deferred = other._deferred;
      _promise = other._promise;
      other._env = nullptr;
      other._deferred = nullptr;
      other._promise = nullptr;
      return *this;
    }

    NAPI_DISALLOW_ASSIGN_COPY(Deferred)

    NAPI_INLINE Napi::Promise Promise() const {
      return Napi::Promise(_env, _promise);
    }
    NAPI_INLINE Napi::Env Env() const { return _env; }

    Maybe<void> Resolve(napi_value value);
    Maybe<void> Reject(napi_value value);

   private:
    Deferred(napi_env env);
    NAPI_INLINE Deferred()
        : _env(nullptr), _deferred(nullptr), _promise(nullptr) {}

    napi_env _env;
    napi_deferred _deferred;
    napi_value _promise;
  };

  NAPI_INLINE Promise() : Object() {}

  NAPI_INLINE Promise(napi_env env, napi_value value) : Object(env, value) {}
};

class NAPI_EXTERN External : public Value {
 public:
  static External New(napi_env env, void* data, napi_finalize finalize_cb,
                      void* hint);

  NAPI_INLINE External() : Value() {}
  NAPI_INLINE External(napi_env env, napi_value value) : Value(env, value) {}

  void* Data() const;
};

/// Holds a counted reference to a value; initially a weak reference unless
/// otherwise specified, may be changed to/from a strong reference by adjusting
/// the refcount.
///
/// The referenced value is not immediately destroyed when the reference count
/// is zero; it is merely then eligible for garbage-collection if there are no
/// other references to the value.
template <typename T>
class Reference {
 public:
  static NAPI_INLINE Reference<T> New(const T& value,
                                      uint32_t initialRefcount = 0) {
    napi_env env = value.Env();
    napi_value val = value;

    return Reference<T>(env, NAPI::CreateReference(env, val, initialRefcount));
  }

  NAPI_INLINE Reference() : _env(nullptr), _ref(nullptr) {}
  NAPI_INLINE Reference(napi_env env, napi_ref ref) : _env(env), _ref(ref) {}
  NAPI_INLINE ~Reference() {
    if (_ref != nullptr) {
      NAPI::DeleteReference(_env, _ref);
      _ref = nullptr;
    }
  }

  // A reference can be moved but cannot be copied.
  NAPI_INLINE Reference(Reference<T>&& other)
      : _env(other._env), _ref(other._ref) {
    other._env = nullptr;
    other._ref = nullptr;
  }

  NAPI_INLINE Reference<T>& operator=(Reference<T>&& other) {
    Reset();
    _env = other._env;
    _ref = other._ref;
    other._env = nullptr;
    other._ref = nullptr;
    return *this;
  }

  NAPI_DISALLOW_ASSIGN(Reference<T>)

  NAPI_INLINE operator napi_ref() const { return _ref; }

  NAPI_INLINE Napi::Env Env() const { return Napi::Env(_env); }
  NAPI_INLINE bool IsEmpty() const { return _ref == nullptr; }

  // Note when getting the value of a Reference it is usually correct to do so
  // within a HandleScope so that the value handle gets cleaned up efficiently.
  NAPI_INLINE T Value() const {
    return T(_env, NAPI::GetReferenceValue(_env, _ref));
  }

  NAPI_INLINE uint32_t Ref() { return NAPI::ReferenceRef(_env, _ref); }
  NAPI_INLINE uint32_t Unref() { return NAPI::ReferenceUnRef(_env, _ref); }
  NAPI_INLINE void Reset() {
    if (_ref != nullptr) {
      NAPI::DeleteReference(_env, _ref);
      _ref = nullptr;
    }
  }
  NAPI_INLINE void Reset(const T& value, uint32_t refcount = 0) {
    Reset();
    _env = value.Env();
    napi_value val = value;
    _ref = NAPI::CreateReference(_env, val, refcount);
  }

 protected:
  /// !cond INTERNAL
  napi_env _env;
  napi_ref _ref;
  /// !endcond
};

using ObjectReference = Reference<Object>;

using FunctionReference = Reference<Function>;

// Shortcuts to creating a new reference with inferred type and refcount = 0.

template <typename T>
NAPI_INLINE Reference<T> Weak(T value) {
  return Reference<T>::New(value, 0);
}

NAPI_INLINE ObjectReference Weak(Object value) {
  return Reference<Object>::New(value, 0);
}

NAPI_INLINE FunctionReference Weak(Function value) {
  return Reference<Function>::New(value, 0);
}

template <typename T>
NAPI_INLINE Reference<T> Persistent(T value) {
  return Reference<T>::New(value, 1);
}

NAPI_INLINE ObjectReference Persistent(Object value) {
  return Reference<Object>::New(value, 1);
}

NAPI_INLINE FunctionReference Persistent(Function value) {
  return Reference<Function>::New(value, 1);
}

/// A JavaScript error object.
///
/// ### Handling Errors Without C++ Exceptions
///
/// JS Engine calls may raise _pending_ JavaScript exceptions and return _empty_
/// `Value`s. Calling code should check `Value::IsEmpty()` before attempting to
/// use a returned value, and may use methods on the `Env` class to check for,
/// get, and clear a pending JavaScript exception. If the pending exception is
/// not cleared, it will be thrown when the native callback returns to
/// JavaScript.
///
/// #### Example 1B - Throwing a JS exception
///
///     Napi::Env env = ...
///     Napi::Error::New(env, "Example exception").ThrowAsJavaScriptException();
///     return;
///
/// After throwing a JS exception, the code should generally return immediately
/// from the native callback, after performing any necessary cleanup.
///
/// #### Example 2B - Propagating a N-API JS exception:
///
///     Napi::Function jsFunctionThatThrows = someObj.As<Napi::Function>();
///     Napi::Value result = jsFunctionThatThrows({ arg1, arg2 });
///     if (result.IsEmpty()) return;
///
/// An empty value result from a N-API call indicates an error occurred, and a
/// JavaScript exception is pending. To let the exception propagate, the code
/// should generally return immediately from the native callback, after
/// performing any necessary cleanup.
///
/// #### Example 3B - Handling a N-API JS exception:
///
///     Napi::Function jsFunctionThatThrows = someObj.As<Napi::Function>();
///     Napi::Value result = jsFunctionThatThrows({ arg1, arg2 });
///     if (result.IsEmpty()) {
///       Napi::Error e = env.GetAndClearPendingException();
///       cerr << "Caught JavaScript exception: " + e.Message();
///     }
///
/// Since the exception was cleared here, it will not be propagated as a
/// JavaScript exception after the native callback returns.
class NAPI_EXTERN Error : public Object {
 public:
  static Error New(napi_env env);
  static Error New(napi_env env, const char* message);

  Error(napi_env env, napi_value value) : Object(env, value) {}

  void ThrowAsJavaScriptException() const;

 protected:
  /// !cond INTERNAL
  typedef napi_status (*create_error_fn)(napi_env env, napi_value code,
                                         napi_value msg, napi_value* result);

  static napi_value Create(napi_env env, const char* message, size_t length,
                           create_error_fn create_error);
  /// !endcond
};

class NAPI_EXTERN TypeError : public Error {
 public:
  static TypeError New(napi_env env, const char* message);

  NAPI_INLINE TypeError(napi_env env, napi_value value) : Error(env, value) {}
};

class NAPI_EXTERN RangeError : public Error {
 public:
  static RangeError New(napi_env env, const char* message);

  NAPI_INLINE RangeError(napi_env env, napi_value value) : Error(env, value) {}
};

class NAPI_EXTERN CallbackInfo {
 public:
  CallbackInfo(napi_env env, napi_callback_info info);
  ~CallbackInfo();

  // Disallow copying to prevent multiple free of _dynamicArgs
  NAPI_DISALLOW_ASSIGN_COPY(CallbackInfo)

  NAPI_INLINE Napi::Env Env() const { return Napi::Env(_env); }
  Value NewTarget() const;
  bool IsConstructCall() const;
  NAPI_INLINE size_t Length() const { return _argc; }
  NAPI_INLINE const Value operator[](size_t index) const {
    return index < _argc ? Value(_env, _argv[index]) : Env().Undefined();
  }
  NAPI_INLINE Value This() const {
    return _this == nullptr ? Env().Undefined() : Object(_env, _this);
  }
  NAPI_INLINE void* Data() const { return _data; }
  NAPI_INLINE void SetData(void* data) { _data = data; }

 private:
  const size_t _staticArgCount = 6;
  napi_env _env;
  napi_callback_info _info;
  napi_value _this;
  size_t _argc;
  napi_value* _argv;
  napi_value _staticArgs[6];
  napi_value* _dynamicArgs;
  void* _data;
};

class NAPI_EXTERN PropertyDescriptor {
 public:
  typedef void (*SetterCallback)(const CallbackInfo& info, const Value&);

  static PropertyDescriptor Accessor(
      Napi::Env env, Napi::Object obj, const char* utf8name,
      Function::Callback getter, SetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  static PropertyDescriptor Accessor(
      Napi::Env env, Napi::Object obj, Name name, Function::Callback getter,
      SetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  static PropertyDescriptor Function(
      Napi::Env env, Napi::Object obj, const char* utf8name,
      Function::Callback cb, napi_property_attributes attributes = napi_default,
      void* data = nullptr);

  static PropertyDescriptor Function(
      Napi::Env env, Napi::Object obj, Name name, Function::Callback cb,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  static PropertyDescriptor Value(
      const char* utf8name, napi_value value,
      napi_property_attributes attributes = napi_default);

  static PropertyDescriptor Value(
      napi_value name, napi_value value,
      napi_property_attributes attributes = napi_default);

  NAPI_INLINE PropertyDescriptor(napi_property_descriptor desc) : _desc(desc) {}

  NAPI_INLINE operator napi_property_descriptor&() { return _desc; }
  NAPI_INLINE operator const napi_property_descriptor&() const { return _desc; }

 private:
  napi_property_descriptor _desc;
};

template <typename T, typename TCallback>
struct MethodCallbackData {
  TCallback callback;
  void* data;
};

template <typename T, typename TGetterCallback, typename TSetterCallback>
struct AccessorCallbackData {
  TGetterCallback getterCallback;
  TSetterCallback setterCallback;
  void* data;
};

/// Property descriptor for use with `ObjectWrap::DefineClass()`.
///
/// This is different from the standalone `PropertyDescriptor` because it is
/// specific to each `ObjectWrap<T>` subclass. This prevents using descriptors
/// from a different class when defining a new class (preventing the callbacks
/// from having incorrect `this` pointers).
template <typename T>
class ClassPropertyDescriptor {
 public:
  ClassPropertyDescriptor(napi_property_descriptor desc) : _desc(desc) {}

  operator napi_property_descriptor&() { return _desc; }
  operator const napi_property_descriptor&() const { return _desc; }

 private:
  napi_property_descriptor _desc;
};

template <typename T>
class InstanceWrap {
 public:
  static T* Unwrap(Object wrapper);

  typedef ClassPropertyDescriptor<T> PropertyDescriptor;

  typedef Value (T::*InstanceCallback)(const CallbackInfo& info);
  typedef void (T::*InstanceSetterCallback)(const CallbackInfo& info,
                                            const Value& value);

  static PropertyDescriptor InstanceMethod(
      const char* utf8name, InstanceCallback method,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  static PropertyDescriptor InstanceMethod(
      Name name, InstanceCallback method,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  static PropertyDescriptor InstanceAccessor(
      const char* utf8name, InstanceCallback getter,
      InstanceSetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default, void* data = nullptr);
  static PropertyDescriptor InstanceAccessor(
      Name name, InstanceCallback getter,
      InstanceSetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  static PropertyDescriptor InstanceValue(
      const char* utf8name, napi_value value,
      napi_property_attributes attributes = napi_default);

  static PropertyDescriptor InstanceValue(
      Name name, napi_value value,
      napi_property_attributes attributes = napi_default);

 protected:
  static void AttachPropData(Napi::Object obj, size_t props_count,
                             const napi_property_descriptor* props);

 private:
  using This = InstanceWrap<T>;
  typedef MethodCallbackData<T, InstanceCallback> InstanceMethodCallbackData;
  typedef AccessorCallbackData<T, InstanceCallback, InstanceSetterCallback>
      InstanceAccessorCallbackData;

  static napi_value InstanceMethodCallbackWrapper(napi_env env,
                                                  napi_callback_info info);
  static napi_value InstanceGetterCallbackWrapper(napi_env env,
                                                  napi_callback_info info);
  static napi_value InstanceSetterCallbackWrapper(napi_env env,
                                                  napi_callback_info info);
};

class NAPI_EXTERN Class {
 public:
  NAPI_INLINE Class() : _env(nullptr), _class(nullptr) {}
  NAPI_INLINE Class(napi_env env, napi_class clazz)
      : _env(env), _class(clazz) {}

  ~Class();

  // An Class can be moved but cannot be copied.
  NAPI_INLINE Class(Class&& other) {
    _env = other._env;
    other._env = nullptr;
    _class = other._class;
    other._class = nullptr;
  }

  NAPI_INLINE Class& operator=(Class&& other) {
    _env = other._env;
    other._env = nullptr;
    _class = other._class;
    other._class = nullptr;
    return *this;
  }

  NAPI_DISALLOW_ASSIGN_COPY(Class)

  Function Get(napi_env env);

  NAPI_INLINE operator napi_class() const { return _class; }

 private:
  napi_env _env;
  napi_class _class;
};

template <typename...>
struct always_false {
  static constexpr bool value = false;
};

class NAPI_EXTERN ScriptWrappable {
 public:
  ScriptWrappable();
  virtual ~ScriptWrappable();

  template <typename T>
  T* Cast() {
#ifdef NAPI_CPP_RTTI
    if (*_isa_index == typeid(T)) {
      return reinterpret_cast<T*>(_isa);
    }
    T* ptr = dynamic_cast<T*>(this);
    if (ptr) {
      _isa = reinterpret_cast<void*>(ptr);
      *_isa_index = typeid(T);
    }
    return ptr;
#else

#ifndef NAPI_DISABLE_CPP_RTTI
    static_assert(always_false<T>::value,
                  "`T* ScriptWrappable::Cast()` is dangerous without RTTI, "
                  "specify `NAPI_DISABLE_CPP_RTTI` to disable the assertion");
#endif

    return static_cast<T*>(this);
#endif
  }

 private:
  void* _isa;

#ifdef NAPI_CPP_RTTI
  std::type_index* _isa_index;
#else
 protected:
  void* type_id() { return _isa; }
  void set_type_id(void* index) { _isa = index; }

 private:
  void* _isa_index = nullptr;
#endif
};

static_assert(sizeof(ScriptWrappable) == 3 * sizeof(void*),
              "sizeof ScriptWrappable must be vtable ptr + void* + void*");

/// Base class to be extended by C++ classes exposed to JavaScript; each C++
/// class instance gets "wrapped" by a JavaScript object that is managed by
/// this class.
///
/// At initialization time, the `DefineClass()` method must be used to
/// hook up the accessor and method callbacks. It takes a list of
/// property descriptors, which can be constructed via the various
/// static methods on the base class.
///
/// #### Example:
///
///     class Example: public Napi::ObjectWrap<Example> {
///       public:
///         static void Initialize(Napi::Env& env, Napi::Object& target) {
///           Napi::Function constructor = DefineClass(env, "Example", {
///             InstanceAccessor<&Example::GetSomething,
///             &Example::SetSomething>("value"),
///             InstanceMethod<&Example::DoSomething>("doSomething"),
///           });
///           target.Set("Example", constructor);
///         }
///
///         Example(const Napi::CallbackInfo& info); // Constructor
///         Napi::Value GetSomething(const Napi::CallbackInfo& info);
///         void SetSomething(const Napi::CallbackInfo& info, const
///         Napi::Value& value); Napi::Value DoSomething(const
///         Napi::CallbackInfo& info);
///     }
template <typename T>
class ObjectWrap final : public InstanceWrap<T>,
                         public T,
                         public Reference<Object> {
 public:
  typedef ClassPropertyDescriptor<T> PropertyDescriptor;

  typedef Napi::Value (*StaticMethodCallback)(const CallbackInfo& info);
  typedef void (*StaticSetterCallback)(const CallbackInfo& info,
                                       const Napi::Value& value);

  NAPI_INLINE static Class DefineClass(
      Napi::Env env, const char* utf8name,
      const std::initializer_list<PropertyDescriptor>& properties,
      void* data = nullptr, napi_class super_class = nullptr) {
    return DefineClass(
        env, utf8name, properties.size(),
        reinterpret_cast<const napi_property_descriptor*>(properties.begin()),
        data, super_class);
  }

  NAPI_INLINE
  static Class DefineClass(Napi::Env env, const char* utf8name,
                           const std::vector<PropertyDescriptor>& properties,
                           void* data = nullptr,
                           napi_class super_class = nullptr) {
    return DefineClass(
        env, utf8name, properties.size(),
        reinterpret_cast<const napi_property_descriptor*>(properties.data()),
        data, super_class);
  }

  NAPI_INLINE static PropertyDescriptor StaticMethod(
      const char* utf8name, StaticMethodCallback method,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  NAPI_INLINE static PropertyDescriptor StaticMethod(
      Name name, StaticMethodCallback method,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  NAPI_INLINE static PropertyDescriptor StaticAccessor(
      const char* utf8name, StaticMethodCallback getter,
      StaticSetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  NAPI_INLINE static PropertyDescriptor StaticAccessor(
      Name name, Function::Callback getter,
      StaticSetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default, void* data = nullptr);

  NAPI_INLINE static PropertyDescriptor StaticValue(
      const char* utf8name, napi_value value,
      napi_property_attributes attributes = napi_default);

  NAPI_INLINE static PropertyDescriptor StaticValue(
      Name name, napi_value value,
      napi_property_attributes attributes = napi_default);

 private:
  using This = ObjectWrap<T>;
  using StaticMethodCallbackData = MethodCallbackData<T, StaticMethodCallback>;
  using StaticAccessorCallbackData =
      AccessorCallbackData<T, StaticMethodCallback, StaticSetterCallback>;

  ObjectWrap(const CallbackInfo& callbackInfo);

  static Class DefineClass(Napi::Env env, const char* utf8name,
                           size_t props_count,
                           const napi_property_descriptor* props, void* data,
                           napi_class super_class);

  static napi_value ConstructorCallbackWrapper(napi_env env,
                                               napi_callback_info info);
  static void FinalizeCallback(napi_env env, void* data, void* hint);

  static napi_value StaticMethodCallbackWrapper(napi_env env,
                                                napi_callback_info info);
  static napi_value StaticGetterCallbackWrapper(napi_env env,
                                                napi_callback_info info);
  static napi_value StaticSetterCallbackWrapper(napi_env env,
                                                napi_callback_info info);
};

class NAPI_EXTERN ContextScope {
 public:
  NAPI_INLINE ContextScope(napi_env env, napi_context_scope scope)
      : _env(env), _scope(scope) {}
  explicit ContextScope(Napi::Env env);
  ~ContextScope();

  // Disallow copying to prevent double close of napi_handle_scope
  NAPI_DISALLOW_ASSIGN_COPY(ContextScope)

  NAPI_INLINE operator napi_context_scope() const { return _scope; }

  NAPI_INLINE Napi::Env Env() const { return _env; }

 private:
  napi_env _env;
  napi_context_scope _scope;
};

class NAPI_EXTERN HandleScope {
 public:
  NAPI_INLINE HandleScope(napi_env env, napi_handle_scope scope)
      : _env(env), _scope(scope) {}
  explicit HandleScope(Napi::Env env);
  ~HandleScope();

  // Disallow copying to prevent double close of napi_handle_scope
  NAPI_DISALLOW_ASSIGN_COPY(HandleScope)

  NAPI_INLINE operator napi_handle_scope() const { return _scope; }

  NAPI_INLINE Napi::Env Env() const { return _env; }

 private:
  napi_env _env;
  napi_handle_scope _scope;
};

class NAPI_EXTERN EscapableHandleScope {
 public:
  NAPI_INLINE EscapableHandleScope(napi_env env,
                                   napi_escapable_handle_scope scope)
      : _env(env), _scope(scope) {}
  explicit EscapableHandleScope(Napi::Env env);
  ~EscapableHandleScope();

  // Disallow copying to prevent double close of napi_escapable_handle_scope
  NAPI_DISALLOW_ASSIGN_COPY(EscapableHandleScope)

  NAPI_INLINE operator napi_escapable_handle_scope() const { return _scope; }

  NAPI_INLINE Napi::Env Env() const { return _env; }

  Value Escape(napi_value escapee);

 private:
  napi_env _env;
  napi_escapable_handle_scope _scope;
};

class NAPI_EXTERN ErrorScope {
 public:
  NAPI_INLINE ErrorScope(napi_env env, napi_error_scope scope)
      : _env(env), _scope(scope) {}
  explicit ErrorScope(Napi::Env env);
  ~ErrorScope();

  // Disallow copying to prevent double close of napi_escapable_handle_scope
  NAPI_DISALLOW_ASSIGN_COPY(ErrorScope)

  NAPI_INLINE operator napi_error_scope() const { return _scope; }

  NAPI_INLINE Napi::Env Env() const { return _env; }

 private:
  napi_env _env;
  napi_error_scope _scope;
};

class NAPI_EXTERN AsyncWorker {
 public:
  virtual ~AsyncWorker();

  // An async worker can be moved but cannot be copied.
  NAPI_INLINE AsyncWorker(AsyncWorker&& other) {}
  NAPI_INLINE AsyncWorker& operator=(AsyncWorker&& other) {
    _env = other._env;
    other._env = nullptr;
    _work = other._work;
    other._work = nullptr;
    return *this;
  }
  NAPI_DISALLOW_ASSIGN_COPY(AsyncWorker)

  NAPI_INLINE operator napi_async_work() const { return _work; }

  NAPI_INLINE Napi::Env Env() const { return _env; }

  void Queue();
  void Cancel();

 protected:
  explicit AsyncWorker(Napi::Env env);

  virtual void OnWorkComplete(Napi::Env env, napi_status status);
  virtual void Execute() = 0;
  virtual void OnOK() = 0;

 private:
  napi_env _env;
  napi_async_work _work;
};

// A ThreadSafeFunction by default has no context (nullptr) and can
// accept any type (void) to its CallJs.
template <typename ContextType = std::nullptr_t, typename DataType = void,
          void (*CallJs)(Napi::Env, ContextType*, DataType) = nullptr>
class ThreadSafeFunction {
 public:
  NAPI_DISALLOW_ASSIGN_COPY(ThreadSafeFunction)

  ~ThreadSafeFunction();

  // This API may only be called from the main thread.
  // Creates a new threadsafe function with:
  //   Callback [passed] Finalizer [passed]
  template <typename Finalizer, typename FinalizerDataType = std::nullptr_t>
  static ThreadSafeFunction<ContextType, DataType, CallJs>* New(
      napi_env env, ContextType* context, Finalizer finalizeCallback,
      FinalizerDataType* data = nullptr);

  operator napi_threadsafe_function() const;

  // This API may be called from any thread.
  template <typename... Args>
  napi_status BlockingCall(Args&&... args) const;

  // This API may be called from any thread.
  template <typename... Args>
  napi_status NonBlockingCall(Args&&... args) const;

  // This API may be called from any thread.
  ContextType* GetContext() const;

 private:
  ThreadSafeFunction(napi_env env, napi_threadsafe_function tsfn);

  static void CallJsInternal(napi_env env, void* context,
                             void* data) NAPI_NOEXCEPT;

 protected:
  decltype(napi_env__::napi_call_threadsafe_function) _call;
  decltype(napi_env__::napi_get_threadsafe_function_context) _get_context;
  decltype(napi_env__::napi_delete_threadsafe_function) _delete;
  napi_threadsafe_function _tsfn;
};

// Memory management.
class NAPI_EXTERN MemoryManagement {
 public:
  static int64_t AdjustExternalMemory(Env env, int64_t change_in_bytes);
};

// Version management
class VersionManagement {
 public:
  static uint32_t GetNapiVersion(Env env);
};

#ifdef ENABLE_CODECACHE
Value Env::RunScriptCache(const char* utf8script, const char* filename) {
  return RunScriptCache(utf8script, NAPI_AUTO_LENGTH, filename);
}
#endif  // ENABLE_CODECACHE

Value Env::RunScript(const char* utf8script, const char* filename) {
  return RunScript(utf8script, NAPI_AUTO_LENGTH, filename);
}

template <typename T, typename Enabled>
T* Env::GetInstanceData() {
  return static_cast<T*>(GetInstanceData(T::KEY));
}

template <typename T, typename Enabled>
void Env::SetInstanceData(T* data) {
  return SetInstanceData(
      T::KEY, data,
      [](napi_env env, void* data, void*) { delete static_cast<T*>(data); },
      nullptr);
}

template <typename T>
void Env::SetInstanceData(uint64_t key, T* data) {
  return SetInstanceData(
      key, data,
      [](napi_env env, void* data, void*) { delete static_cast<T*>(data); },
      nullptr);
}

template <typename T>
inline T* Env::GetInstanceData(uint64_t key) {
  return static_cast<T*>(GetInstanceData(key));
}

template <typename T>
void Env::AddCleanupHook(void (*cb)(T*), T* data) {
  return AddCleanupHook(reinterpret_cast<void (*)(void* arg)>(cb), data);
}

template <typename T>
void Env::RemoveCleanupHook(void (*cb)(T*), T* data) {
  return RemoveCleanupHook(reinterpret_cast<void (*)(void* arg)>(cb), data);
}

template <typename T>
T* InstanceWrap<T>::Unwrap(Object wrapper) {
  static_assert(std::is_base_of<ScriptWrappable, T>::value,
                "T must inherit ScriptWrappable");
  ScriptWrappable* unwrapped =
      reinterpret_cast<ScriptWrappable*>(NAPI::Unwrap(wrapper.Env(), wrapper));
  return unwrapped ? unwrapped->Cast<T>() : nullptr;
}

template <typename T>
ClassPropertyDescriptor<T> InstanceWrap<T>::InstanceMethod(
    const char* utf8name, InstanceCallback method,
    napi_property_attributes attributes, void* data) {
  InstanceMethodCallbackData* callbackData =
      new InstanceMethodCallbackData({method, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.utf8name = utf8name;
  desc.method = This::InstanceMethodCallbackWrapper;
  desc.data = callbackData;
  desc.attributes = attributes;
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> InstanceWrap<T>::InstanceMethod(
    Name name, InstanceCallback method, napi_property_attributes attributes,
    void* data) {
  InstanceMethodCallbackData* callbackData =
      new InstanceMethodCallbackData({method, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.name = name;
  desc.method = This::InstanceMethodCallbackWrapper;
  desc.data = callbackData;
  desc.attributes = attributes;
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> InstanceWrap<T>::InstanceAccessor(
    const char* utf8name, InstanceCallback getter,
    InstanceSetterCallback setter, napi_property_attributes attributes,
    void* data) {
  InstanceAccessorCallbackData* callbackData =
      new InstanceAccessorCallbackData({getter, setter, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.utf8name = utf8name;
  desc.getter =
      getter != nullptr ? This::InstanceGetterCallbackWrapper : nullptr;
  desc.setter =
      setter != nullptr ? This::InstanceSetterCallbackWrapper : nullptr;
  desc.data = callbackData;
  desc.attributes = attributes;
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> InstanceWrap<T>::InstanceAccessor(
    Name name, InstanceCallback getter, InstanceSetterCallback setter,
    napi_property_attributes attributes, void* data) {
  InstanceAccessorCallbackData* callbackData =
      new InstanceAccessorCallbackData({getter, setter, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.name = name;
  desc.getter =
      getter != nullptr ? This::InstanceGetterCallbackWrapper : nullptr;
  desc.setter =
      setter != nullptr ? This::InstanceSetterCallbackWrapper : nullptr;
  desc.data = callbackData;
  desc.attributes = attributes;
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> InstanceWrap<T>::InstanceValue(
    const char* utf8name, napi_value value,
    napi_property_attributes attributes) {
  napi_property_descriptor desc = napi_property_descriptor();
  desc.utf8name = utf8name;
  desc.value = value;
  desc.attributes = attributes;
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> InstanceWrap<T>::InstanceValue(
    Name name, napi_value value, napi_property_attributes attributes) {
  napi_property_descriptor desc = napi_property_descriptor();
  desc.name = name;
  desc.value = value;
  desc.attributes = attributes;
  return desc;
}

template <typename T>
void InstanceWrap<T>::AttachPropData(Napi::Object obj, size_t props_count,
                                     const napi_property_descriptor* props) {
  for (size_t i = 0; i < props_count; i++) {
    auto& p = props[i];
    if ((p.attributes & napi_static) == 0) {
      if (p.method == This::InstanceMethodCallbackWrapper) {
        obj.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<InstanceMethodCallbackData*>(data);
        });
      } else if (p.getter == This::InstanceGetterCallbackWrapper ||
                 p.setter == This::InstanceSetterCallbackWrapper) {
        obj.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<InstanceAccessorCallbackData*>(data);
        });
      }
    }
  }
}

template <typename T>
napi_value InstanceWrap<T>::InstanceMethodCallbackWrapper(
    napi_env env, napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);
  InstanceMethodCallbackData* callbackData =
      reinterpret_cast<InstanceMethodCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  T* instance = This::Unwrap(callbackInfo.This().As<Object>());
  auto cb = callbackData->callback;
  if (instance) {
    return (instance->*cb)(callbackInfo);
  } else {
    Napi::TypeError::New(callbackInfo.Env(),
                         "callback's caller 's Type is invalid")
        .ThrowAsJavaScriptException();
    return nullptr;
  }
  //  return instance ? (instance->*cb)(callbackInfo) : nullptr;
}

template <typename T>
napi_value InstanceWrap<T>::InstanceGetterCallbackWrapper(
    napi_env env, napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);
  InstanceAccessorCallbackData* callbackData =
      reinterpret_cast<InstanceAccessorCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  T* instance = This::Unwrap(callbackInfo.This().As<Object>());
  auto cb = callbackData->getterCallback;
  if (instance) {
    return (instance->*cb)(callbackInfo);
  } else {
    Napi::TypeError::New(callbackInfo.Env(),
                         "callback's caller 's Type is invalid")
        .ThrowAsJavaScriptException();
    return nullptr;
  }
  //  return instance ? instance->*cb(callbackInfo) : nullptr;
}

template <typename T>
napi_value InstanceWrap<T>::InstanceSetterCallbackWrapper(
    napi_env env, napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);
  InstanceAccessorCallbackData* callbackData =
      reinterpret_cast<InstanceAccessorCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  T* instance = This::Unwrap(callbackInfo.This().As<Object>());
  auto cb = callbackData->setterCallback;
  if (instance) {
    (instance->*cb)(callbackInfo, callbackInfo[0]);
  }
  return nullptr;
}

template <typename T>
ObjectWrap<T>::ObjectWrap(const Napi::CallbackInfo& callbackInfo)
    : T(callbackInfo) {
  napi_env env = callbackInfo.Env();
  napi_value wrapper = callbackInfo.This();

  static_assert(std::is_base_of<ScriptWrappable, T>::value,
                "T must inherit ScriptWrappable");
  void* ptr = static_cast<ScriptWrappable*>(this);

  napi_ref ref = NAPI::Wrap(env, wrapper, ptr, FinalizeCallback, nullptr);

  Reference<Object>* instanceRef = this;
  *instanceRef = Reference<Object>(env, ref);
}

template <typename T>
napi_value ObjectWrap<T>::ConstructorCallbackWrapper(napi_env env,
                                                     napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);

  if (!callbackInfo.IsConstructCall()) {
    TypeError::New(env, "Class constructors cannot be invoked without 'new'")
        .ThrowAsJavaScriptException();
    return nullptr;
  }

  new This(callbackInfo);

  return callbackInfo.This();
}

template <typename T>
napi_value ObjectWrap<T>::StaticMethodCallbackWrapper(napi_env env,
                                                      napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);
  StaticMethodCallbackData* callbackData =
      reinterpret_cast<StaticMethodCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  return (*callbackData->callback)(callbackInfo);
}

template <typename T>
napi_value ObjectWrap<T>::StaticGetterCallbackWrapper(napi_env env,
                                                      napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);
  StaticAccessorCallbackData* callbackData =
      reinterpret_cast<StaticAccessorCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  return (*callbackData->getterCallback)(callbackInfo);
}

template <typename T>
napi_value ObjectWrap<T>::StaticSetterCallbackWrapper(napi_env env,
                                                      napi_callback_info info) {
  CallbackInfo callbackInfo(env, info);
  StaticAccessorCallbackData* callbackData =
      reinterpret_cast<StaticAccessorCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  (*callbackData->setterCallback)(callbackInfo, callbackInfo[0]);
  return nullptr;
}

template <typename T>
void ObjectWrap<T>::FinalizeCallback(napi_env env, void* data, void* /*hint*/) {
  ScriptWrappable* instance = static_cast<ScriptWrappable*>(data);
  delete instance;
}

template <typename T>
ClassPropertyDescriptor<T> ObjectWrap<T>::StaticMethod(
    const char* utf8name, StaticMethodCallback method,
    napi_property_attributes attributes, void* data) {
  StaticMethodCallbackData* callbackData =
      new StaticMethodCallbackData({method, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.utf8name = utf8name;
  desc.method = This::StaticMethodCallbackWrapper;
  desc.data = callbackData;
  desc.attributes =
      static_cast<napi_property_attributes>(attributes | napi_static);
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> ObjectWrap<T>::StaticMethod(
    Name name, StaticMethodCallback method, napi_property_attributes attributes,
    void* data) {
  StaticMethodCallbackData* callbackData =
      new StaticMethodCallbackData({method, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.name = name;
  desc.method = This::StaticMethodCallbackWrapper;
  desc.data = callbackData;
  desc.attributes =
      static_cast<napi_property_attributes>(attributes | napi_static);
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> ObjectWrap<T>::StaticAccessor(
    const char* utf8name, StaticMethodCallback getter,
    StaticSetterCallback setter, napi_property_attributes attributes,
    void* data) {
  StaticAccessorCallbackData* callbackData =
      new StaticAccessorCallbackData({getter, setter, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.utf8name = utf8name;
  desc.getter = getter != nullptr ? This::StaticGetterCallbackWrapper : nullptr;
  desc.setter = setter != nullptr ? This::StaticSetterCallbackWrapper : nullptr;
  desc.data = callbackData;
  desc.attributes =
      static_cast<napi_property_attributes>(attributes | napi_static);
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> ObjectWrap<T>::StaticAccessor(
    Name name, StaticMethodCallback getter, StaticSetterCallback setter,
    napi_property_attributes attributes, void* data) {
  StaticAccessorCallbackData* callbackData =
      new StaticAccessorCallbackData({getter, setter, data});

  napi_property_descriptor desc = napi_property_descriptor();
  desc.name = name;
  desc.getter = getter != nullptr ? This::StaticGetterCallbackWrapper : nullptr;
  desc.setter = setter != nullptr ? This::StaticSetterCallbackWrapper : nullptr;
  desc.data = callbackData;
  desc.attributes =
      static_cast<napi_property_attributes>(attributes | napi_static);
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> ObjectWrap<T>::StaticValue(
    const char* utf8name, napi_value value,
    napi_property_attributes attributes) {
  napi_property_descriptor desc = napi_property_descriptor();
  desc.utf8name = utf8name;
  desc.value = value;
  desc.attributes =
      static_cast<napi_property_attributes>(attributes | napi_static);
  return desc;
}

template <typename T>
ClassPropertyDescriptor<T> ObjectWrap<T>::StaticValue(
    Name name, napi_value value, napi_property_attributes attributes) {
  napi_property_descriptor desc = napi_property_descriptor();
  desc.name = name;
  desc.value = value;
  desc.attributes =
      static_cast<napi_property_attributes>(attributes | napi_static);
  return desc;
}

template <typename T>
Class ObjectWrap<T>::DefineClass(Napi::Env env, const char* utf8name,
                                 const size_t props_count,
                                 const napi_property_descriptor* props,
                                 void* data, napi_class super_class) {
  Class clazz(env,
              NAPI::DefineClass(env, utf8name, This::ConstructorCallbackWrapper,
                                props_count, props, data, super_class));

  auto fun = clazz.Get(env);

  InstanceWrap<T>::AttachPropData(fun, props_count, props);

  for (size_t i = 0; i < props_count; i++) {
    auto& p = props[i];
    if ((p.attributes & napi_static) != 0) {
      if (p.method == This::StaticMethodCallbackWrapper) {
        fun.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<StaticMethodCallbackData*>(data);
        });
      } else if (p.getter == This::StaticGetterCallbackWrapper ||
                 p.setter == This::StaticSetterCallbackWrapper) {
        fun.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<StaticAccessorCallbackData*>(data);
        });
      }
    }
  }

  return clazz;
}

namespace details {
template <typename ContextType = void,
          typename Finalizer = std::function<void(Env, void*, ContextType*)>,
          typename FinalizerDataType = void>
struct ThreadSafeFinalize {
  static inline void FinalizeFinalizeWrapperWithDataAndContext(
      napi_env env, void* rawFinalizeData, void* rawContext) NAPI_NOEXCEPT {
    if (rawFinalizeData == nullptr) return;
    ThreadSafeFinalize* finalizeData =
        static_cast<ThreadSafeFinalize*>(rawFinalizeData);
    finalizeData->callback(Env(env), finalizeData->data,
                           static_cast<ContextType*>(rawContext));
    delete finalizeData;
  }

  FinalizerDataType* data;
  Finalizer callback;
};
}  // namespace details

////////////////////////////////////////////////////////////////////////////////
// ThreadSafeFunction<ContextType,DataType,CallJs> class
////////////////////////////////////////////////////////////////////////////////

// static, with Callback [missing] Resource [missing] Finalizer [passed]
template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
template <typename Finalizer, typename FinalizerDataType>
ThreadSafeFunction<ContextType, DataType, CallJs>*
ThreadSafeFunction<ContextType, DataType, CallJs>::New(
    napi_env env, ContextType* context, Finalizer finalizeCallback,
    FinalizerDataType* data) {
  napi_threadsafe_function tsfn;

  auto* finalizeData = new details::ThreadSafeFinalize<ContextType, Finalizer,
                                                       FinalizerDataType>(
      {data, finalizeCallback});
  napi_status status = NAPI_ENV_CALL(
      create_threadsafe_function, env, finalizeData,
      details::ThreadSafeFinalize<ContextType, Finalizer, FinalizerDataType>::
          FinalizeFinalizeWrapperWithDataAndContext,
      context, CallJsInternal, &tsfn);
  if (status != napi_ok) {
    delete finalizeData;
    // TODO FATAL
  }

  return new ThreadSafeFunction<ContextType, DataType, CallJs>(env, tsfn);
}

template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
ThreadSafeFunction<ContextType, DataType, CallJs>::ThreadSafeFunction(
    napi_env env, napi_threadsafe_function tsfn)
    : _call(env->napi_call_threadsafe_function),
      _get_context(env->napi_get_threadsafe_function_context),
      _delete(env->napi_delete_threadsafe_function),
      _tsfn(tsfn) {}

template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
ThreadSafeFunction<ContextType, DataType, CallJs>::~ThreadSafeFunction() {
  _delete(_tsfn);
}

template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
ThreadSafeFunction<ContextType, DataType,
                   CallJs>::operator napi_threadsafe_function() const {
  return _tsfn;
}

template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
template <typename... Args>
napi_status ThreadSafeFunction<ContextType, DataType, CallJs>::BlockingCall(
    Args&&... args) const {
  DataType* data = new DataType(std::forward<Args>(args)...);
  napi_status status = _call(_tsfn, data, napi_tsfn_blocking);
  if (status != napi_ok) {
    delete data;
  }
  return status;
}

template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
template <typename... Args>
napi_status ThreadSafeFunction<ContextType, DataType, CallJs>::NonBlockingCall(
    Args&&... args) const {
  DataType* data = new DataType(std::forward<Args>(args)...);
  napi_status status = _call(_tsfn, data, napi_tsfn_nonblocking);
  if (status != napi_ok) {
    delete data;
  }
  return status;
}

template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
ContextType* ThreadSafeFunction<ContextType, DataType, CallJs>::GetContext()
    const {
  void* context;
  _get_context(_tsfn, &context);
  // TODO FATAL
  return static_cast<ContextType*>(context);
}

// static
template <typename ContextType, typename DataType,
          void (*CallJs)(Napi::Env, ContextType*, DataType)>
void ThreadSafeFunction<ContextType, DataType, CallJs>::CallJsInternal(
    napi_env env, void* context, void* data) NAPI_NOEXCEPT {
  DataType* ptr = static_cast<DataType*>(data);
  CallJs(env, static_cast<ContextType*>(context), *ptr);
  delete ptr;
}

////////////////////////////////////////////////////////////////////////////////
// Automagic value creation
////////////////////////////////////////////////////////////////////////////////

namespace details {
template <typename T>
struct vf_number {
  static Number From(napi_env env, T value) {
    return Number::New(env, static_cast<double>(value));
  }
};

template <>
struct vf_number<bool> {
  static Boolean From(napi_env env, bool value) {
    return Boolean::New(env, value);
  }
};

struct vf_utf8_charp {
  static String From(napi_env env, const char* value) {
    return String::New(env, value);
  }
};

struct vf_utf16_charp {
  static String From(napi_env env, const char16_t* value) {
    return String::New(env, value);
  }
};

struct vf_utf8_string {
  static String From(napi_env env, const std::string& value) {
    return String::New(env, value);
  }
};

struct vf_utf16_string {
  static String From(napi_env env, const std::u16string& value) {
    return String::New(env, value);
  }
};

template <typename T>
struct vf_fallback {
  static Value From(napi_env env, const T& value) { return Value(env, value); }
};

template <typename...>
struct disjunction : std::false_type {};
template <typename B>
struct disjunction<B> : B {};
template <typename B, typename... Bs>
struct disjunction<B, Bs...>
    : std::conditional<bool(B::value), B, disjunction<Bs...>>::type {};

template <typename T>
struct can_make_string
    : disjunction<typename std::is_convertible<T, const char*>::type,
                  typename std::is_convertible<T, const char16_t*>::type,
                  typename std::is_convertible<T, std::string>::type,
                  typename std::is_convertible<T, std::u16string>::type> {};
}  // namespace details

template <typename T>
Value Value::From(napi_env env, const T& value) {
  using Helper = typename std::conditional<
      std::is_integral<T>::value || std::is_floating_point<T>::value,
      details::vf_number<T>,
      typename std::conditional<details::can_make_string<T>::value, String,
                                details::vf_fallback<T>>::type>::type;
  return Helper::From(env, value);
}

template <typename T>
String String::From(napi_env env, const T& value) {
  struct Dummy {};
  using Helper = typename std::conditional<
      std::is_convertible<T, const char*>::value, details::vf_utf8_charp,
      typename std::conditional<
          std::is_convertible<T, const char16_t*>::value,
          details::vf_utf16_charp,
          typename std::conditional<
              std::is_convertible<T, std::string>::value,
              details::vf_utf8_string,
              typename std::conditional<
                  std::is_convertible<T, std::u16string>::value,
                  details::vf_utf16_string, Dummy>::type>::type>::type>::type;
  return Helper::From(env, value);
}

}  // namespace Napi

// Register an add-on based on an initializer function.
#define NODE_API_MODULE(modname, regfunc)                                \
  static napi_value __napi_##regfunc(napi_env env, napi_value exports) { \
    return regfunc(Napi::Env(env), Napi::Object(env, exports));          \
  }                                                                      \
  NAPI_MODULE_PRIMJS(modname, __napi_##regfunc)

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif

#endif  // SRC_NAPI_H_
