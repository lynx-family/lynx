// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/android/java_value.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace android {

TEST(JavaValueTest, ConstructorAndGetterAndSetter) {
  JNIEnv* env = AttachCurrentThread();
  {
    JavaValue val = JavaValue();
    ASSERT_TRUE(val.IsNull());
    ASSERT_TRUE(val.IsPrimitiveType());
  }
  {
    JavaValue val = JavaValue(true);
    ASSERT_TRUE(val.IsBool());
    ASSERT_TRUE(val.IsPrimitiveType());
    ASSERT_TRUE(val.Bool());
  }
  {
    JavaValue val = JavaValue(0.01);
    ASSERT_TRUE(val.IsNumber());
    ASSERT_TRUE(val.IsDouble());
    ASSERT_TRUE(val.Double() == 0.01);
  }
  {
    JavaValue val = JavaValue(static_cast<int32_t>(1));
    ASSERT_TRUE(val.IsNumber());
    ASSERT_TRUE(val.IsInt32());
    ASSERT_TRUE(val.Int32() == 1);
  }
  {
    JavaValue val = JavaValue(static_cast<int64_t>(12345678));
    ASSERT_TRUE(val.IsNumber());
    ASSERT_TRUE(val.IsInt64());
    ASSERT_TRUE(val.Int64() == 12345678);
  }
  {
    JavaValue val = JavaValue(std::string("hello world"));
    ASSERT_TRUE(val.IsString());
    ASSERT_TRUE(val.String() == "hello world");
  }
  {
    JavaValue val = JavaValue(
        JNIConvertHelper::ConvertToJNIStringUTF(env, "hello world").Get());
    ASSERT_TRUE(val.IsString());
    // get the string twice to test if cache is valid in JavaValue.
    ASSERT_TRUE(val.String() == "hello world");
    ASSERT_TRUE(val.String() == "hello world");
    ASSERT_TRUE(val.JString() != nullptr);
  }
  {
    std::string str = "hello world";
    JavaValue val =
        JavaValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
    ASSERT_TRUE(val.IsArrayBuffer());
    ASSERT_TRUE(val.JByteArray() != nullptr);
  }
  {
    JavaValue val = JavaValue(std::make_shared<base::android::JavaOnlyArray>());
    ASSERT_TRUE(val.IsArray());
    ASSERT_TRUE(val.Array()->jni_object() != nullptr);
  }
  {
    JavaValue val = JavaValue(std::make_shared<base::android::JavaOnlyMap>());
    ASSERT_TRUE(val.IsMap());
    ASSERT_TRUE(val.Map()->jni_object() != nullptr);
  }
}

}  // namespace android
}  // namespace base
}  // namespace lynx
