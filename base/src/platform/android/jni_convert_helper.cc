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

lynx::base::android::ScopedLocalJavaRef<jbyteArray>
JNIConvertHelper::ConvertToJNIByteArray(JNIEnv* env, const void* data,
                                        int32_t size) {
  jbyteArray array = env->NewByteArray(size);  // NOLINT
  env->SetByteArrayRegion(array, 0, size, reinterpret_cast<const jbyte*>(data));
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

bool JNIConvertHelper::ConvertJavaBinary(
    JNIEnv* env, jbyteArray j_binary,
    std::function<void*(int32_t size)> allocator) {
  if (j_binary != nullptr) {
    auto* temp = env->GetByteArrayElements(j_binary, nullptr);
    size_t len = env->GetArrayLength(j_binary);
    if (len > 0) {
      auto data = allocator(len);
      if (data && temp) {
        std::memcpy(data, temp, len);
        return true;
      }
    }
  }

  return false;
}

bool JNIConvertHelper::ConvertJavaDirectByteBuffer(
    JNIEnv* env, jobject j_buffer,
    std::function<void*(int32_t size)> allocator) {
  if (j_buffer != nullptr) {
    auto* temp = env->GetDirectBufferAddress(j_buffer);
    size_t len = env->GetDirectBufferCapacity(j_buffer);
    if (len > 0 && temp != nullptr) {
      auto data = allocator(len);
      if (data) {
        std::memcpy(data, temp, len);
      }
      return true;
    }
  }

  return false;
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JNIConvertHelper::ConvertToJavaDirectByteBuffer(JNIEnv* env, const void* data,
                                                int32_t size) {
  auto buffer = env->NewDirectByteBuffer(const_cast<void*>(data), size);
  return lynx::base::android::ScopedLocalJavaRef<jobject>(env, buffer);
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

std::unique_ptr<std::unordered_map<std::string, std::string>>
JNIConvertHelper::ConvertJavaStringHashMapToSTLStringMap(JNIEnv* env,
                                                         jobject javaMap) {
  if (!javaMap) {
    return nullptr;
  }

  // get class
  jclass mapClass = env->FindClass("java/util/HashMap");
  jclass setClass = env->FindClass("java/util/Set");
  jclass iteratorClass = env->FindClass("java/util/Iterator");
  jclass entryClass = env->FindClass("java/util/Map$Entry");
  jclass stringClass = env->FindClass("java/lang/String");
  if (!mapClass || !setClass || !iteratorClass || !entryClass || !stringClass) {
    return nullptr;
  }

  // get method id
  jmethodID entrySetMethod =
      env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
  jmethodID iteratorMethod =
      env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
  jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
  jmethodID nextMethod =
      env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");
  jmethodID getKeyMethod =
      env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
  jmethodID getValueMethod =
      env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");
  if (!entrySetMethod || !iteratorMethod || !hasNextMethod || !nextMethod ||
      !getKeyMethod || !getValueMethod) {
    return nullptr;
  }

  // get entrySet
  jobject entrySet = env->CallObjectMethod(javaMap, entrySetMethod);
  if (env->ExceptionCheck() || !entrySet) {
    env->ExceptionClear();
    return nullptr;
  }

  // get iterator
  jobject iterator = env->CallObjectMethod(entrySet, iteratorMethod);
  if (env->ExceptionCheck() || !iterator) {
    env->ExceptionClear();
    env->DeleteLocalRef(entrySet);
    return nullptr;
  }

  auto cppMapPtr =
      std::make_unique<std::unordered_map<std::string, std::string>>();
  // convert
  while (env->CallBooleanMethod(iterator, hasNextMethod)) {
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      break;
    }

    jobject entry = env->CallObjectMethod(iterator, nextMethod);
    if (env->ExceptionCheck() || !entry) {
      env->ExceptionClear();
      continue;
    }

    // get string key
    jobject keyObj = env->CallObjectMethod(entry, getKeyMethod);
    if (env->ExceptionCheck() || !keyObj ||
        !env->IsInstanceOf(keyObj, stringClass)) {
      if (env->ExceptionCheck()) {
        env->ExceptionClear();
      }
      env->DeleteLocalRef(entry);
      if (keyObj) env->DeleteLocalRef(keyObj);
      continue;
    }

    // get string value
    jobject valueObj = env->CallObjectMethod(entry, getValueMethod);
    if (env->ExceptionCheck() || !valueObj ||
        !env->IsInstanceOf(valueObj, stringClass)) {
      if (env->ExceptionCheck()) {
        env->ExceptionClear();
      }
      env->DeleteLocalRef(entry);
      env->DeleteLocalRef(keyObj);
      if (valueObj) env->DeleteLocalRef(valueObj);
      continue;
    }

    jstring javaKey = static_cast<jstring>(keyObj);
    jstring javaValue = static_cast<jstring>(valueObj);

    std::string key =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, javaKey);
    std::string value =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, javaValue);

    cppMapPtr->emplace(std::move(key), std::move(value));

    // clear local ref
    env->DeleteLocalRef(javaKey);
    env->DeleteLocalRef(javaValue);
    env->DeleteLocalRef(entry);
  }

  // clear local ref
  env->DeleteLocalRef(iterator);
  env->DeleteLocalRef(entrySet);
  env->DeleteLocalRef(mapClass);
  env->DeleteLocalRef(setClass);
  env->DeleteLocalRef(iteratorClass);
  env->DeleteLocalRef(entryClass);
  env->DeleteLocalRef(stringClass);

  return cppMapPtr;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
