// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/java_value.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/java_only_array.h"

namespace lynx {
namespace base {
namespace android {

namespace converter {

template <typename T>
static jobject valueOf(JNIEnv* env, jclass c, const char* signature,
                       const T& value) {
  static jmethodID valueOfMethod =
      env->GetStaticMethodID(c, "valueOf", signature);
  return env->CallStaticObjectMethod(c, valueOfMethod, value);
}

template <typename T>
static double doubleValue(JNIEnv* env, jclass c, const T& value) {
  static jmethodID doubleValueMethod =
      env->GetMethodID(c, "doubleValue", "()D");
  return env->CallDoubleMethod(value, doubleValueMethod);
}

jobject charValueOf(JNIEnv* env, jchar value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Character");
  return valueOf(env, cls.Get(), "(C)Ljava/lang/Character;", value);
}

jobject byteValueOf(JNIEnv* env, jbyte value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Byte");
  return valueOf(env, cls.Get(), "(B)Ljava/lang/Byte;", value);
}

jobject booleanValueOf(JNIEnv* env, jboolean value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Boolean");
  return valueOf(env, cls.Get(), "(Z)Ljava/lang/Boolean;", value);
}

jobject shortValueOf(JNIEnv* env, jshort value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Short");
  return valueOf(env, cls.Get(), "(S)Ljava/lang/Short;", value);
}

jobject integerValueOf(JNIEnv* env, jint value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Integer");
  return valueOf(env, cls.Get(), "(I)Ljava/lang/Integer;", value);
}

jobject longValueOf(JNIEnv* env, jlong value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Long");
  return valueOf(env, cls.Get(), "(J)Ljava/lang/Long;", value);
}
jobject floatValueOf(JNIEnv* env, jfloat value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Float");
  return valueOf(env, cls.Get(), "(F)Ljava/lang/Float;", value);
}

jobject doubleValueOf(JNIEnv* env, jdouble value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Double");
  return valueOf(env, cls.Get(), "(D)Ljava/lang/Double;", value);
}

}  // namespace converter

JavaValue::JavaValue(bool value) : type_(JavaValueType::Boolean) {
  jvalue j_value;
  j_value.z = static_cast<jboolean>(value);
  j_variant_value_ = j_value;
}

JavaValue::JavaValue(double value) : type_(JavaValueType::Double) {
  jvalue j_value;
  j_value.d = static_cast<jdouble>(value);
  j_variant_value_ = j_value;
}

JavaValue::JavaValue(float value) : type_(JavaValueType::Float) {
  jvalue j_value;
  j_value.f = static_cast<jfloat>(value);
  j_variant_value_ = j_value;
}

JavaValue::JavaValue(int32_t value) : type_(JavaValueType::Int32) {
  jvalue j_value;
  j_value.i = static_cast<jint>(value);
  j_variant_value_ = j_value;
}

JavaValue::JavaValue(int64_t value) : type_(JavaValueType::Int64) {
  jvalue j_value;
  j_value.j = static_cast<jlong>(value);
  j_variant_value_ = j_value;
}

JavaValue::JavaValue(const std::string& value) : type_(JavaValueType::String) {
  JNIEnv* env = base::android::AttachCurrentThread();
  j_variant_value_ = lynx::base::android::ScopedGlobalJavaRef<jobject>(
      env, JNIConvertHelper::ConvertToJNIStringUTF(env, value).Get());
}

JavaValue::JavaValue(jstring str) : type_(JavaValueType::String) {
  JNIEnv* env = base::android::AttachCurrentThread();
  j_variant_value_ =
      lynx::base::android::ScopedGlobalJavaRef<jobject>(env, str);
}

JavaValue::JavaValue(const uint8_t* value, size_t length)
    : type_(JavaValueType::ByteArray) {
  JNIEnv* env = base::android::AttachCurrentThread();
  j_variant_value_ = lynx::base::android::ScopedGlobalJavaRef<jobject>(
      env, JNIConvertHelper::ConvertToJNIByteArray(
               env, std::string(reinterpret_cast<const char*>(value), length))
               .Get());
}

bool JavaValue::Bool() const {
  if (IsBool()) {
    return std::get<jvalue>(j_variant_value_).z;
  }
  return false;
}

int32_t JavaValue::Int32() const {
  if (IsInt32()) {
    return std::get<jvalue>(j_variant_value_).i;
  }
  return 0;
}

int64_t JavaValue::Int64() const {
  if (IsInt64()) {
    return std::get<jvalue>(j_variant_value_).j;
  }
  return 0;
}

float JavaValue::Float() const {
  if (IsFloat()) {
    return std::get<jvalue>(j_variant_value_).f;
  }
  return 0;
}

double JavaValue::Double() const {
  if (IsDouble()) {
    return std::get<jvalue>(j_variant_value_).d;
  }
  return 0;
}

const std::string& JavaValue::String() const {
  if (IsString()) {
    if (string_cache_) {
      return *string_cache_;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    jstring java_str_ref = reinterpret_cast<jstring>(
        std::get<base::android::ScopedGlobalJavaRef<jobject>>(j_variant_value_)
            .Get());
    const char* str = env->GetStringUTFChars(java_str_ref, NULL);
    if (str) {
      string_cache_ = std::make_optional<std::string>(str);
    }
    env->ReleaseStringUTFChars(java_str_ref, str);
    return *string_cache_;
  }
  return base::String().str();
}
JavaValue JavaValue::GetValueForKey(const std::string& key) const {
  if (!IsMap()) {
    return JavaValue();
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedGlobalJavaRef<jstring> j_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  return JavaOnlyMap::JavaOnlyMapGetJavaValueAtIndex(
      env,
      std::get<std::shared_ptr<base::android::JavaOnlyMap>>(j_variant_value_)
          ->jni_object(),
      j_key.Get());
}
uint8_t* JavaValue::ArrayBuffer() const {
  if (!IsArrayBuffer()) {
    return nullptr;
  }
  if (!array_buffer_ptr_cache_.empty()) {
    return array_buffer_ptr_cache_.data();
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  jbyte* j_byte = env->GetByteArrayElements(JByteArray(), JNI_FALSE);
  if (j_byte == nullptr) {
    return nullptr;
  }
  array_buffer_ptr_cache_ =
      std::vector<uint8_t>(j_byte, j_byte + env->GetArrayLength(JByteArray()));
  env->ReleaseByteArrayElements(JByteArray(), j_byte, 0);
  return array_buffer_ptr_cache_.data();
}
JavaValue JavaValue::GetValueForIndex(uint32_t index) const {
  if (!IsArray()) {
    return JavaValue();
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  return JavaOnlyArray::JavaOnlyArrayGetJavaValueAtIndex(
      env,
      std::get<std::shared_ptr<base::android::JavaOnlyArray>>(j_variant_value_)
          ->jni_object(),
      index);
}

jvalue JavaValue::JByte() const {
  jvalue j_byte;
  // No exception is thrown, truncated from the last 8 bits
  if (IsInt32()) {
    j_byte.b = static_cast<jbyte>(Int32());
  } else if (IsDouble()) {
    j_byte.b = static_cast<jbyte>(Double());
  } else {
    j_byte.b = 0;
  }
  return j_byte;
}

jvalue JavaValue::WrapperJByte() const {
  jvalue j_wrapper_byte;
  JNIEnv* env = base::android::AttachCurrentThread();
  // No exception is thrown, truncated from the last 8 bits
  j_wrapper_byte.l = converter::byteValueOf(env, JByte().b);
  return j_wrapper_byte;
}

jvalue JavaValue::JChar() const {
  jvalue j_char;
  char c_result = '\0';
  if (!String().empty()) {
    c_result = String()[0];
  }
  j_char.c = static_cast<jchar>(c_result);
  return j_char;
}

jvalue JavaValue::WrapperJChar() const {
  jvalue j_wrapper_char;
  char c_result = '\0';
  if (!String().empty()) {
    c_result = String()[0];
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_char.l = converter::charValueOf(env, static_cast<jchar>(c_result));
  return j_wrapper_char;
}

jvalue JavaValue::JBoolean() const {
  jvalue j_boolean;
  j_boolean.z = static_cast<jboolean>(Bool());
  return j_boolean;
}

jvalue JavaValue::WrapperJBoolean() const {
  jvalue j_wrapper_boolean;
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_boolean.l =
      converter::booleanValueOf(env, static_cast<jboolean>(Bool()));
  return j_wrapper_boolean;
}

jvalue JavaValue::JShort() const {
  jvalue j_short;
  if (IsInt32()) {
    j_short.s = static_cast<jshort>(Int32());
  } else if (IsDouble()) {
    j_short.s = static_cast<jshort>(Double());
  } else {
    j_short.s = 0;
  }
  return j_short;
}

jvalue JavaValue::WrapperJShort() const {
  jvalue j_wrapper_short;
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_short.l = converter::shortValueOf(env, JShort().s);
  return j_wrapper_short;
}

jvalue JavaValue::JInt() const {
  jvalue j_int;
  if (IsInt32()) {
    j_int.i = static_cast<jint>(Int32());
  } else if (IsDouble()) {
    j_int.i = static_cast<jint>(Double());
  } else {
    j_int.i = 0;
  }
  return j_int;
}

jvalue JavaValue::WrapperJInt() const {
  jvalue j_wrapper_int;
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_int.l = converter::integerValueOf(env, JInt().i);
  return j_wrapper_int;
}

jvalue JavaValue::JLong() const {
  jvalue j_long;
  if (IsInt64()) {
    j_long.j = static_cast<jlong>(Int64());
  } else if (IsDouble()) {
    j_long.j = static_cast<jlong>(Double());
  } else {
    j_long.j = 0;
  }
  return j_long;
}

jvalue JavaValue::WrapperJLong() const {
  jvalue j_wrapper_long;
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_long.l = converter::longValueOf(env, JLong().j);
  return j_wrapper_long;
}

jvalue JavaValue::JFloat() const {
  return IsFloat() ? std::get<jvalue>(j_variant_value_) : jvalue{};
}

jvalue JavaValue::WrapperJFloat() const {
  jvalue j_wrapper_float;
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_float.l =
      converter::floatValueOf(env, static_cast<jfloat>(Float()));
  return j_wrapper_float;
}

jvalue JavaValue::JDouble() const {
  jvalue j_double;
  j_double.d = static_cast<jdouble>(Double());
  return j_double;
}

jvalue JavaValue::WrapperJDouble() const {
  jvalue j_wrapper_double;
  JNIEnv* env = base::android::AttachCurrentThread();
  j_wrapper_double.l =
      converter::doubleValueOf(env, static_cast<jdouble>(Double()));
  return j_wrapper_double;
}

jvalue JavaValue::JNull() const {
  jvalue j_null;
  j_null.l = nullptr;
  return j_null;
}
int JavaValue::Length() const {
  if (IsArray()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    return JavaOnlyArray::JavaOnlyArrayGetSize(env, Array()->jni_object());
  } else if (IsArrayBuffer()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    return static_cast<int>(env->GetArrayLength(JByteArray()));
  } else {
    return 0;
  }
}

}  // namespace android
}  // namespace base
}  // namespace lynx
