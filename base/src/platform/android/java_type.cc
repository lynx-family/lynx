// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/java_type.h"

#include "base/include/platform/android/jni_convert_helper.h"
namespace lynx {
namespace base {
namespace android {

ScopedLocalJavaRef<jstring> JType::NewString(JNIEnv* env, const char* str) {
  return JNIConvertHelper::ConvertToJNIStringUTF(env, str);
}

ScopedLocalJavaRef<jobjectArray> JType::NewObjectArray(JNIEnv* env,
                                                       int length) {
  EnsureInstance(env, kObjectType);
  return ScopedLocalJavaRef<jobjectArray>(
      env, env->NewObjectArray(length, object_clazz, nullptr));
}

ScopedLocalJavaRef<jdoubleArray> JType::NewDoubleArray(JNIEnv* env,
                                                       int length) {
  return ScopedLocalJavaRef<jdoubleArray>(env, env->NewDoubleArray(length));
}

ScopedLocalJavaRef<jobjectArray> JType::NewStringArray(JNIEnv* env,
                                                       int length) {
  EnsureInstance(env, kStringType);
  return ScopedLocalJavaRef<jobjectArray>(
      env, env->NewObjectArray(length, string_clazz, nullptr));
}

ScopedLocalJavaRef<jobject> JType::NewByte(JNIEnv* env, jbyte value) {
  EnsureInstance(env, kByteType);
  jobject result = env->NewObject(byte_clazz, byte_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewChar(JNIEnv* env, jchar value) {
  EnsureInstance(env, kCharType);
  jobject result = env->NewObject(char_clazz, char_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewBoolean(JNIEnv* env, jboolean value) {
  EnsureInstance(env, kBooleanType);
  jobject result = env->NewObject(boolean_clazz, boolean_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewShort(JNIEnv* env, jshort value) {
  EnsureInstance(env, kShortType);
  jobject result = env->NewObject(short_clazz, short_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewInt(JNIEnv* env, jint value) {
  EnsureInstance(env, kIntType);
  jobject result = env->NewObject(int_clazz, int_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewLong(JNIEnv* env, jlong value) {
  EnsureInstance(env, kLongType);
  jobject result = env->NewObject(long_clazz, long_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewFloat(JNIEnv* env, jfloat value) {
  EnsureInstance(env, kFloatType);
  jobject result = env->NewObject(float_clazz, float_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

ScopedLocalJavaRef<jobject> JType::NewDouble(JNIEnv* env, jdouble value) {
  EnsureInstance(env, kDoubleType);
  jobject result = env->NewObject(double_clazz, double_ctor, value);
  return ScopedLocalJavaRef<jobject>(env, result);
}

jbyte JType::ByteValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kByteType);
  return env->GetByteField(value, byte_valueField);
}

jchar JType::CharValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kCharType);
  return env->GetCharField(value, char_valueField);
}

jboolean JType::BooleanValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kBooleanType);
  return env->GetBooleanField(value, boolean_valueField);
}

jshort JType::ShortValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kShortType);
  return env->GetShortField(value, short_valueField);
}

jint JType::IntValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kIntType);
  return env->GetIntField(value, int_valueField);
}

jlong JType::LongValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kLongType);
  return env->GetLongField(value, long_valueField);
}

jfloat JType::FloatValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kFloatType);
  return env->GetFloatField(value, float_valueField);
}

jdouble JType::DoubleValue(JNIEnv* env, jobject value) {
  EnsureInstance(env, kDoubleType);
  return env->GetDoubleField(value, double_valueField);
}

void JType::EnsureInstance(JNIEnv* env, char type) { Init(env, type); }

void JType::Init(JNIEnv* env, char type) {
  // TODO(Provide a fallback mechanism to prevent possible field name changes)
  switch (type) {
    case kByteType:
      if (byte_clazz == nullptr) {
        byte_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Byte"));
        byte_ctor = env->GetMethodID(byte_clazz, "<init>", "(B)V");
        byte_valueField = env->GetFieldID(byte_clazz, "value", "B");
      }
      break;
    case kCharType:
      if (char_clazz == nullptr) {
        char_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Character"));
        char_ctor = env->GetMethodID(char_clazz, "<init>", "(C)V");
        char_valueField = env->GetFieldID(char_clazz, "value", "C");
      }
      break;
    case kBooleanType:
      if (boolean_clazz == nullptr) {
        boolean_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Boolean"));
        boolean_ctor = env->GetMethodID(boolean_clazz, "<init>", "(Z)V");
        boolean_valueField = env->GetFieldID(boolean_clazz, "value", "Z");
      }
      break;
    case kShortType:
      if (short_clazz == nullptr) {
        short_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Short"));
        short_ctor = env->GetMethodID(short_clazz, "<init>", "(S)V");
        short_valueField = env->GetFieldID(short_clazz, "value", "S");
      }
      break;
    case kIntType:
      if (int_clazz == nullptr) {
        int_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Integer"));
        int_ctor = env->GetMethodID(int_clazz, "<init>", "(I)V");
        int_valueField = env->GetFieldID(int_clazz, "value", "I");
      }
      break;
    case kLongType:
      if (long_clazz == nullptr) {
        long_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Long"));
        long_ctor = env->GetMethodID(long_clazz, "<init>", "(J)V");
        long_valueField = env->GetFieldID(long_clazz, "value", "J");
      }
      break;
    case kFloatType:
      if (float_clazz == nullptr) {
        float_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Float"));
        float_ctor = env->GetMethodID(float_clazz, "<init>", "(F)V");
        float_valueField = env->GetFieldID(float_clazz, "value", "F");
      }
      break;
    case kDoubleType:
      if (double_clazz == nullptr) {
        double_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Double"));
        double_ctor = env->GetMethodID(double_clazz, "<init>", "(D)V");
        double_valueField = env->GetFieldID(double_clazz, "value", "D");
      }
      break;
    case kStringType:
      if (string_clazz == nullptr) {
        string_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/String"));
      }
      break;
    case kObjectType:
      if (object_clazz == nullptr) {
        object_clazz =
            // NOLINTNEXTLINE
            (jclass)env->NewGlobalRef(env->FindClass("java/lang/Object"));
      }
      break;
    default:
      break;
  }
}

jclass JType::byte_clazz;
jmethodID JType::byte_ctor;
jfieldID JType::byte_valueField;

jclass JType::char_clazz;
jmethodID JType::char_ctor;
jfieldID JType::char_valueField;

jclass JType::boolean_clazz;
jmethodID JType::boolean_ctor;
jfieldID JType::boolean_valueField;

jclass JType::short_clazz;
jmethodID JType::short_ctor;
jfieldID JType::short_valueField;

jclass JType::int_clazz;
jmethodID JType::int_ctor;
jfieldID JType::int_valueField;

jclass JType::long_clazz;
jmethodID JType::long_ctor;
jfieldID JType::long_valueField;

jclass JType::float_clazz;
jmethodID JType::float_ctor;
jfieldID JType::float_valueField;

jclass JType::double_clazz;
jmethodID JType::double_ctor;
jfieldID JType::double_valueField;

jclass JType::string_clazz;
jclass JType::object_clazz;

}  // namespace android
}  // namespace base
}  // namespace lynx
