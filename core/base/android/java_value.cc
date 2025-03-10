// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/java_value.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/java_only_array.h"

namespace lynx {
namespace base {
namespace android {

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

JavaValue::JavaValue(const std::string &value) : type_(JavaValueType::String) {
  JNIEnv *env = base::android::AttachCurrentThread();
  j_variant_value_ = lynx::base::android::ScopedGlobalJavaRef<jobject>(
      env, JNIConvertHelper::ConvertToJNIStringUTF(env, value).Get());
}

JavaValue::JavaValue(jstring str) : type_(JavaValueType::String) {
  JNIEnv *env = base::android::AttachCurrentThread();
  j_variant_value_ =
      lynx::base::android::ScopedGlobalJavaRef<jobject>(env, str);
}

JavaValue::JavaValue(const uint8_t *value, size_t length)
    : type_(JavaValueType::ByteArray) {
  JNIEnv *env = base::android::AttachCurrentThread();
  j_variant_value_ = lynx::base::android::ScopedGlobalJavaRef<jobject>(
      env, JNIConvertHelper::ConvertToJNIByteArray(
               env, std::string(reinterpret_cast<const char *>(value), length))
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

double JavaValue::Double() const {
  if (IsDouble()) {
    return std::get<jvalue>(j_variant_value_).d;
  }
  return 0;
}

const std::string &JavaValue::String() const {
  if (IsString()) {
    if (string_cache_) {
      return *string_cache_;
    }
    JNIEnv *env = base::android::AttachCurrentThread();
    jstring java_str_ref = reinterpret_cast<jstring>(
        std::get<base::android::ScopedGlobalJavaRef<jobject>>(j_variant_value_)
            .Get());
    const char *str = env->GetStringUTFChars(java_str_ref, NULL);
    if (str) {
      string_cache_ = std::make_optional<std::string>(str);
    }
    env->ReleaseStringUTFChars(java_str_ref, str);
    return *string_cache_;
  }
  return base::String().str();
}
JavaValue JavaValue::GetValueForKey(const std::string &key) const {
  if (!IsMap()) {
    return JavaValue();
  }
  JNIEnv *env = base::android::AttachCurrentThread();
  base::android::ScopedGlobalJavaRef<jstring> j_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  return JavaOnlyMap::JavaOnlyMapGetJavaValueAtIndex(
      env,
      std::get<std::shared_ptr<base::android::JavaOnlyMap>>(j_variant_value_)
          ->jni_object(),
      j_key.Get());
}
uint8_t *JavaValue::ArrayBuffer() const {
  if (!IsArrayBuffer()) {
    return nullptr;
  }
  if (!array_buffer_ptr_cache_.empty()) {
    return array_buffer_ptr_cache_.data();
  }
  JNIEnv *env = base::android::AttachCurrentThread();
  jbyte *j_byte = env->GetByteArrayElements(JByteArray(), JNI_FALSE);
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
  JNIEnv *env = base::android::AttachCurrentThread();
  return JavaOnlyArray::JavaOnlyArrayGetJavaValueAtIndex(
      env,
      std::get<std::shared_ptr<base::android::JavaOnlyArray>>(j_variant_value_)
          ->jni_object(),
      index);
}

}  // namespace android
}  // namespace base
}  // namespace lynx
