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
    JavaValue val = JavaValue::Undefined();
    ASSERT_FALSE(val.IsNull());
    ASSERT_TRUE(val.IsUndefined());
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

TEST(JavaValueTest, JavaDataConvertor) {
  JNIEnv* env = AttachCurrentThread();
  {
    JavaValue byte_java_value(10);
    ASSERT_TRUE(byte_java_value.IsInt32());
    jvalue common_j_value = byte_java_value.JByte();
    ASSERT_EQ(common_j_value.b, 10);

    JavaValue out_of_range_1(128);
    ASSERT_TRUE(out_of_range_1.IsInt32());
    jvalue out_of_range_j_value_1 = out_of_range_1.JByte();
    ASSERT_EQ(out_of_range_j_value_1.b, -128);

    JavaValue out_of_range_2(-129);
    ASSERT_TRUE(out_of_range_2.IsInt32());
    jvalue out_of_range_j_value_2 = out_of_range_2.JByte();
    ASSERT_EQ(out_of_range_j_value_2.b, static_cast<uint8_t>(127));

    JavaValue byte_wrapper_java_value(10);
    ASSERT_TRUE(byte_wrapper_java_value.IsInt32());
    jvalue common_wrapper_j_value = byte_wrapper_java_value.WrapperJByte();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Byte");
    jmethodID byte_value_method =
        env->GetMethodID(cls.Get(), "byteValue", "()B");
    int8_t result =
        env->CallByteMethod(common_wrapper_j_value.l, byte_value_method);
    ASSERT_EQ(result, 10);
  }

  {
    const std::string str = "foo";
    JavaValue char_java_value(str);
    ASSERT_TRUE(char_java_value.IsString());
    jvalue char_j_value = char_java_value.JChar();
    ASSERT_EQ(char_j_value.c, 'f');

    JavaValue char_java_value_null = JavaValue();
    ASSERT_FALSE(char_java_value_null.IsString());
    jvalue char_j_value_null = char_java_value_null.JChar();
    ASSERT_EQ(char_j_value_null.c, '\0');

    JavaValue char_wrapper_java_value(str);
    ASSERT_TRUE(char_wrapper_java_value.IsString());
    jvalue char_wrapper_j_value = char_wrapper_java_value.WrapperJChar();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Character");
    jmethodID char_value_method =
        env->GetMethodID(cls.Get(), "charValue", "()C");
    char result =
        env->CallCharMethod(char_wrapper_j_value.l, char_value_method);
    ASSERT_EQ(result, 'f');
  }

  {
    JavaValue bool_java_value(false);
    ASSERT_TRUE(bool_java_value.IsBool());
    jvalue bool_j_value = bool_java_value.JBoolean();
    ASSERT_FALSE(bool_j_value.z);

    JavaValue bool_wrapper_java_value(false);
    ASSERT_TRUE(bool_wrapper_java_value.IsBool());
    jvalue bool_wrapper_j_value = bool_wrapper_java_value.WrapperJBoolean();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Boolean");
    jmethodID bool_value_method =
        env->GetMethodID(cls.Get(), "booleanValue", "()Z");
    char result =
        env->CallBooleanMethod(bool_wrapper_j_value.l, bool_value_method);
    ASSERT_EQ(result, false);
  }

  {
    JavaValue short_java_value(10);
    ASSERT_TRUE(short_java_value.IsInt32());
    jvalue short_j_value = short_java_value.JShort();
    ASSERT_EQ(short_j_value.s, 10);

    JavaValue short_wrapper_java_value(10);
    ASSERT_TRUE(short_wrapper_java_value.IsInt32());
    jvalue short_wrapper_j_value = short_wrapper_java_value.WrapperJShort();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Short");
    jmethodID short_value_method =
        env->GetMethodID(cls.Get(), "shortValue", "()S");
    short result =
        env->CallShortMethod(short_wrapper_j_value.l, short_value_method);
    ASSERT_EQ(result, 10);
  }

  {
    JavaValue int_java_value(10);
    ASSERT_TRUE(int_java_value.IsInt32());
    jvalue int_j_value = int_java_value.JInt();
    ASSERT_EQ(int_j_value.i, 10);

    JavaValue int_wrapper_java_value(10);
    ASSERT_TRUE(int_wrapper_java_value.IsInt32());
    jvalue int_wrapper_j_value = int_wrapper_java_value.WrapperJInt();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Integer");
    jmethodID int_value_method = env->GetMethodID(cls.Get(), "intValue", "()I");
    int32_t result =
        env->CallIntMethod(int_wrapper_j_value.l, int_value_method);
    ASSERT_EQ(result, 10);
  }

  {
    JavaValue long_java_value(9223372036854775806);
    ASSERT_TRUE(long_java_value.IsInt64());
    jvalue long_j_value = long_java_value.JLong();
    ASSERT_EQ(long_j_value.j, 9223372036854775806);

    JavaValue long_wrapper_java_value(9223372036854775806);
    ASSERT_TRUE(long_wrapper_java_value.IsInt64());
    jvalue long_wrapper_j_value = long_wrapper_java_value.WrapperJLong();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Long");
    jmethodID long_value_method =
        env->GetMethodID(cls.Get(), "longValue", "()J");
    int64_t result =
        env->CallLongMethod(long_wrapper_j_value.l, long_value_method);
    ASSERT_EQ(result, 9223372036854775806);
  }

  {
    JavaValue float_java_value(3.14f);
    ASSERT_TRUE(float_java_value.IsFloat());
    jvalue float_j_value = float_java_value.JFloat();
    ASSERT_EQ(float_j_value.f, 3.14f);

    JavaValue float_wrapper_java_value(3.14f);
    ASSERT_TRUE(float_wrapper_java_value.IsFloat());
    jvalue float_wrapper_j_value = float_wrapper_java_value.WrapperJFloat();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Float");
    jmethodID float_value_method =
        env->GetMethodID(cls.Get(), "floatValue", "()F");
    float result =
        env->CallFloatMethod(float_wrapper_j_value.l, float_value_method);
    ASSERT_EQ(result, 3.14f);
  }

  {
    JavaValue double_java_value(3.1415926);
    ASSERT_TRUE(double_java_value.IsDouble());
    jvalue double_j_value = double_java_value.JDouble();
    ASSERT_EQ(double_j_value.d, 3.1415926);

    JavaValue double_wrapper_java_value(3.1415926);
    ASSERT_TRUE(double_wrapper_java_value.IsDouble());
    jvalue double_wrapper_j_value = double_wrapper_java_value.WrapperJDouble();
    auto cls = base::android::GetGlobalClass(env, "java/lang/Double");
    jmethodID double_value_method =
        env->GetMethodID(cls.Get(), "doubleValue", "()D");
    double result =
        env->CallDoubleMethod(double_wrapper_j_value.l, double_value_method);
    ASSERT_EQ(result, 3.1415926);
  }
}

}  // namespace android
}  // namespace base
}  // namespace lynx
