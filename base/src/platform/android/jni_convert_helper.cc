// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/jni_convert_helper.h"

#include "base/include/fml/macros.h"

namespace lynx {
namespace base {
namespace android {

#define ASSERT_NO_EXCEPTION() \
  LYNX_BASE_CHECK(env->ExceptionCheck() == JNI_FALSE);

lynx::base::android::ScopedLocalJavaRef<jstring>
JNIConvertHelper::ConvertToJNIStringUTF(JNIEnv* env, const std::string& value) {
  jstring str = env->NewStringUTF(value.c_str());  // NOLINT
  return lynx::base::android::ScopedLocalJavaRef<jstring>(env, str);
}

lynx::base::android::ScopedLocalJavaRef<jstring>
JNIConvertHelper::ConvertToJNIStringUTF(JNIEnv* env, const char* value) {
  jstring str = env->NewStringUTF(value);  // NOLINT
  return lynx::base::android::ScopedLocalJavaRef<jstring>(env, str);
}

lynx::base::android::ScopedLocalJavaRef<jstring>
JNIConvertHelper::ConvertToJNIString(JNIEnv* env, const jchar* unicode_chars,
                                     jsize len) {
  jstring str = env->NewString(unicode_chars, len);  // NOLINT
  return lynx::base::android::ScopedLocalJavaRef<jstring>(env, str);
}

lynx::base::android::ScopedLocalJavaRef<jbyteArray>
JNIConvertHelper::ConvertToJNIByteArray(JNIEnv* env, const std::string& str) {
  jbyteArray array = env->NewByteArray(str.length());  // NOLINT
  env->SetByteArrayRegion(array, 0, str.length(),
                          reinterpret_cast<const jbyte*>(str.c_str()));
  return lynx::base::android::ScopedLocalJavaRef<jbyteArray>(env, array);
}

std::vector<uint8_t> JNIConvertHelper::ConvertJavaBinary(JNIEnv* env,
                                                         jbyteArray j_binary) {
  std::vector<uint8_t> binary;
  if (j_binary != nullptr) {
    auto* temp = env->GetByteArrayElements(j_binary, JNI_FALSE);
    size_t len = env->GetArrayLength(j_binary);
    if (len > 0) {
      auto begin = reinterpret_cast<const uint8_t*>(temp);
      binary.assign(begin, begin + len);
    }
    env->ReleaseByteArrayElements(j_binary, temp, JNI_FALSE);
  }
  return binary;
}

std::string JNIConvertHelper::ConvertToString(JNIEnv* env, jstring j_str) {
  std::string res;
  if (j_str != nullptr) {
    const char* str = env->GetStringUTFChars(j_str, JNI_FALSE);
    if (str) {
      res = std::string(str);
    }
    env->ReleaseStringUTFChars(j_str, str);
  }
  return res;
}

std::string JNIConvertHelper::ConvertToString(JNIEnv* env,
                                              jbyteArray j_binary) {
  std::string str;
  if (j_binary != nullptr) {
    auto* temp = env->GetByteArrayElements(j_binary, JNI_FALSE);
    size_t len = env->GetArrayLength(j_binary);
    str.assign(reinterpret_cast<char*>(temp), len);
    env->ReleaseByteArrayElements(j_binary, temp, JNI_FALSE);
  }
  return str;
}

ScopedLocalJavaRef<jobjectArray>
JNIConvertHelper::ConvertStringVectorToJavaStringArray(
    JNIEnv* env, const std::vector<std::string>& input) {
  auto size = input.size();
  jobjectArray result = env->NewObjectArray(
      static_cast<jsize>(size), env->FindClass("java/lang/String"), nullptr);

  for (size_t i = 0; i < size; ++i) {
    auto j_str =
        base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, input[i]);
    env->SetObjectArrayElement(result, i, j_str.Get());
  }
  return ScopedLocalJavaRef<jobjectArray>(env, result);
}

ScopedLocalJavaRef<jstring> JNIConvertHelper::U16StringToJNIString(
    JNIEnv* env, const std::u16string& str) {
  auto result = ScopedLocalJavaRef<jstring>(
      env, env->NewString(reinterpret_cast<const jchar*>(str.data()),  // NOLINT
                          str.length()));
  return result;
}

ScopedLocalJavaRef<jobjectArray>
JNIConvertHelper::ConvertStringVectorToJavaStringArray(
    JNIEnv* env, const std::vector<std::u16string>& input) {
  auto size = input.size();
  jobjectArray result = env->NewObjectArray(
      static_cast<jsize>(size), env->FindClass("java/lang/String"), nullptr);

  for (size_t i = 0; i < size; ++i) {
    auto j_str =
        base::android::JNIConvertHelper::U16StringToJNIString(env, input[i]);
    env->SetObjectArrayElement(result, i, j_str.Get());
  }
  return ScopedLocalJavaRef<jobjectArray>(env, result);
}

ScopedLocalJavaRef<jobjectArray> ConvertVectorToBufferArray(
    JNIEnv* env, const std::vector<std::vector<uint8_t>>& vector) {
  ScopedLocalJavaRef<jclass> byte_buffer_clazz(
      env, env->FindClass("java/nio/ByteBuffer"));
  LYNX_BASE_DCHECK(!byte_buffer_clazz.IsNull());
  jobjectArray java_array =
      env->NewObjectArray(vector.size(), byte_buffer_clazz.Get(), NULL);
  ASSERT_NO_EXCEPTION();
  for (size_t i = 0; i < vector.size(); ++i) {
    uint8_t* data = const_cast<uint8_t*>(vector[i].data());
    ScopedLocalJavaRef<jobject> item(
        env, env->NewDirectByteBuffer(reinterpret_cast<void*>(data),
                                      vector[i].size()));
    env->SetObjectArrayElement(java_array, i, item.Get());
  }
  return ScopedLocalJavaRef<jobjectArray>(env, java_array);
}

std::vector<std::string> JNIConvertHelper::ConvertJavaStringArrayToStringVector(
    JNIEnv* env, jobjectArray array) {
  std::vector<std::string> result;
  if (array == nullptr) {
    return result;
  }
  jsize len = env->GetArrayLength(array);
  for (size_t i = 0; i < static_cast<size_t>(len); ++i) {
    auto java_obj =
        ScopedLocalJavaRef<jobject>(env, env->GetObjectArrayElement(array, i));
    result.emplace_back(
        ConvertToString(env, static_cast<jstring>(java_obj.Get())));
  }
  return result;
}

std::unordered_set<std::string>
JNIConvertHelper::ConvertJavaStringSetToSTLStringSet(JNIEnv* env, jobject set) {
  std::unordered_set<std::string> ret_set;

  auto set_class =
      ScopedLocalJavaRef<jclass>(env, env->FindClass("java/util/Set"));

  jmethodID set_to_array =
      env->GetMethodID(set_class.Get(), "toArray", "()[Ljava/lang/Object;");

  auto str_array = ScopedLocalJavaRef<jobjectArray>(
      env, static_cast<jobjectArray>(env->CallObjectMethod(set, set_to_array)));

  if (str_array.Get() == nullptr) {
    return ret_set;
  }

  size_t len = env->GetArrayLength(str_array.Get());
  for (size_t i = 0; i < len; ++i) {
    auto java_obj = ScopedLocalJavaRef<jobject>(
        env, env->GetObjectArrayElement(str_array.Get(), i));
    ret_set.emplace(ConvertToString(env, static_cast<jstring>(java_obj.Get())));
  }
  return ret_set;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
