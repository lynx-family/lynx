// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/android/android_jni.h"

#include "base/include/debug/lynx_error.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace android {

TEST(AndroidJNITest, HasJNIException) {
  JNIEnv* env = AttachCurrentThread();
  auto key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key1");
  JavaOnlyMap j_map;

  // The HasJNIException flag is intially set to false
  ASSERT_FALSE(HasJNIException());

  // Get value from a empty JavaOnlyMap will throw a jni exception
  bool bool_value = JavaOnlyMap::JavaOnlyMapGetBooleanAtIndex(
      env, j_map.jni_object(), key.Get());
  ASSERT_TRUE(HasJNIException());

  // Successfully calling a JNI method resets the HasJNIException flag
  j_map.PushBoolean("key1", false);
  bool_value = JavaOnlyMap::JavaOnlyMapGetBooleanAtIndex(
      env, j_map.jni_object(), key.Get());
  ASSERT_FALSE(HasJNIException());
}

TEST(AndroidJNITest, GetExceptionInfo) {
  JNIEnv* env = AttachCurrentThread();
  auto key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key1");
  JavaOnlyMap j_map;

  // Get value from a empty JavaOnlyMap will throw a jni exception
  JavaOnlyMap::JavaOnlyMapGetBooleanAtIndex(env, j_map.jni_object(), key.Get());
  ASSERT_TRUE(HasJNIException());

  const auto& error = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(
      error->error_message_.find("java.lang.NullPointerException: key: key1"),
      std::string::npos);
}

}  // namespace android
}  // namespace base
}  // namespace lynx
