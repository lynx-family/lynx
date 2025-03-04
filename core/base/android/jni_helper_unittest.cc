// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/android/jni_helper.h"

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace android {

TEST(ConvertSTLStringMapToJavaMapTest, ConvertEmptyMap) {
  JNIEnv* env = AttachCurrentThread();

  std::unordered_map<std::string, std::string> empty_map;
  ScopedLocalJavaRef<jobject> j_empty_map =
      JNIHelper::ConvertSTLStringMapToJavaMap(env, empty_map);
  ASSERT_EQ(j_empty_map.Get(), nullptr);
}

TEST(ConvertSTLStringMapToJavaMapTest, ConvertNonEmptyMap) {
  JNIEnv* env = AttachCurrentThread();

  std::unordered_map<std::string, std::string> map;
  map.emplace("key1", "value1");
  map.emplace("key2", "value2");
  map.emplace("key3", "value3");
  ScopedLocalJavaRef<jobject> j_map =
      JNIHelper::ConvertSTLStringMapToJavaMap(env, map);
  auto key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key1");
  auto value =
      JavaOnlyMap::JavaOnlyMapGetStringAtIndex(env, j_map.Get(), key.Get());
  ASSERT_EQ(value, "value1");
  key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key2");
  value = JavaOnlyMap::JavaOnlyMapGetStringAtIndex(env, j_map.Get(), key.Get());
  ASSERT_EQ(value, "value2");
  key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key3");
  value = JavaOnlyMap::JavaOnlyMapGetStringAtIndex(env, j_map.Get(), key.Get());
  ASSERT_EQ(value, "value3");
  key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key4");
  value = JavaOnlyMap::JavaOnlyMapGetStringAtIndex(env, j_map.Get(), key.Get());
  ASSERT_EQ(value, "");
}

TEST(ConvertSTLStringMapToJavaMapTest, ConvertMapWithEmptyKey) {
  JNIEnv* env = AttachCurrentThread();

  std::unordered_map<std::string, std::string> map;
  map.emplace("", "value");
  ScopedLocalJavaRef<jobject> j_map =
      JNIHelper::ConvertSTLStringMapToJavaMap(env, map);
  auto key = JNIConvertHelper::ConvertToJNIStringUTF(env, "");
  auto value =
      JavaOnlyMap::JavaOnlyMapGetStringAtIndex(env, j_map.Get(), key.Get());
  ASSERT_EQ(value, "value");
}

TEST(ConvertSTLStringMapToJavaMapTest, ConvertMapWithEmptyValue) {
  JNIEnv* env = AttachCurrentThread();

  std::unordered_map<std::string, std::string> map;
  map.emplace("key", "");
  ScopedLocalJavaRef<jobject> j_map =
      JNIHelper::ConvertSTLStringMapToJavaMap(env, map);
  auto key = JNIConvertHelper::ConvertToJNIStringUTF(env, "key");
  auto value =
      JavaOnlyMap::JavaOnlyMapGetStringAtIndex(env, j_map.Get(), key.Get());
  ASSERT_EQ(value, "");
}

}  // namespace android
}  // namespace base
}  // namespace lynx
