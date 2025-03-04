// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_PLATFORM_ANDROID_JNI_CONVERT_HELPER_H_
#define BASE_INCLUDE_PLATFORM_ANDROID_JNI_CONVERT_HELPER_H_

#include <jni.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/platform/android/java_type.h"
#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {
namespace android {
class JNIConvertHelper {
 public:
  static lynx::base::android::ScopedLocalJavaRef<jstring> ConvertToJNIStringUTF(
      JNIEnv* env, const std::string& value);

  static lynx::base::android::ScopedLocalJavaRef<jstring> ConvertToJNIStringUTF(
      JNIEnv* env, const char* value);

  static lynx::base::android::ScopedLocalJavaRef<jstring> ConvertToJNIString(
      JNIEnv* env, const jchar* unicode_chars, jsize len);

  static ScopedLocalJavaRef<jstring> U16StringToJNIString(
      JNIEnv* env, const std::u16string& str);

  static std::string ConvertToString(JNIEnv* env, jstring value);

  static std::string ConvertToString(JNIEnv* env, jbyteArray j_binary);

  // TODO(qiuxian):Add unit test when framework is ready.
  static lynx::base::android::ScopedLocalJavaRef<jbyteArray>
  ConvertToJNIByteArray(JNIEnv* env, const std::string& str);

  static std::vector<uint8_t> ConvertJavaBinary(JNIEnv* env,
                                                jbyteArray j_binary);

  inline static int ConvertToInt(JNIEnv* env, jobject j_obj) {
    int value = JType::IntValue(env, j_obj);
    return value;
  }

  inline static long ConvertToLong(JNIEnv* env, jobject j_obj) {
    long value = static_cast<long>(JType::LongValue(env, j_obj));
    return value;
  }

  inline static float ConvertToFloat(JNIEnv* env, jobject j_obj) {
    float value = static_cast<float>(JType::FloatValue(env, j_obj));
    return value;
  }

  inline static double ConvertToDouble(JNIEnv* env, jobject j_obj) {
    double value = static_cast<double>(JType::DoubleValue(env, j_obj));
    return value;
  }

  inline static bool ConvertToBoolean(JNIEnv* env, jobject j_obj) {
    jboolean value = JType::BooleanValue(env, j_obj);
    return static_cast<bool>(value == JNI_TRUE);
  }

  static ScopedLocalJavaRef<jobjectArray> ConvertStringVectorToJavaStringArray(
      JNIEnv* env, const std::vector<std::string>& input);

  static ScopedLocalJavaRef<jobjectArray> ConvertStringVectorToJavaStringArray(
      JNIEnv* env, const std::vector<std::u16string>& input);

  static ScopedLocalJavaRef<jobjectArray> ConvertVectorToBufferArray(
      JNIEnv* env, const std::vector<std::vector<uint8_t>>& vector);

  static std::vector<std::string> ConvertJavaStringArrayToStringVector(
      JNIEnv* env, jobjectArray array);

  static std::unordered_set<std::string> ConvertJavaStringSetToSTLStringSet(
      JNIEnv* env, jobject set);
};
}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_PLATFORM_ANDROID_JNI_CONVERT_HELPER_H_
