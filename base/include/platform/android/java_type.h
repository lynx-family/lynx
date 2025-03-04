// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_ANDROID_JAVA_TYPE_H_
#define BASE_INCLUDE_PLATFORM_ANDROID_JAVA_TYPE_H_

#include <jni.h>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {
namespace android {

static constexpr char kShortType = 'S';
static constexpr char kIntType = 'I';
static constexpr char kLongType = 'J';
static constexpr char kFloatType = 'F';
static constexpr char kDoubleType = 'D';
static constexpr char kCharType = 'C';
static constexpr char kBooleanType = 'Z';
static constexpr char kByteType = 'B';
static constexpr char kVoidType = 'V';
static constexpr char kArrayFlag = '[';

static constexpr char kStringType = 's';
static constexpr char kObjectType = 'O';

class JType {
 public:
  static ScopedLocalJavaRef<jobject> NewByte(JNIEnv* env, jbyte value);
  static ScopedLocalJavaRef<jobject> NewChar(JNIEnv* env, jchar value);
  static ScopedLocalJavaRef<jobject> NewBoolean(JNIEnv* env, jboolean value);
  static ScopedLocalJavaRef<jobject> NewShort(JNIEnv* env, jshort value);
  static ScopedLocalJavaRef<jobject> NewInt(JNIEnv* env, jint value);
  static ScopedLocalJavaRef<jobject> NewLong(JNIEnv* env, jlong value);
  static ScopedLocalJavaRef<jobject> NewFloat(JNIEnv* env, jfloat value);
  static ScopedLocalJavaRef<jobject> NewDouble(JNIEnv* env, jdouble value);

  static ScopedLocalJavaRef<jstring> NewString(JNIEnv* env, const char* str);

  static jbyte ByteValue(JNIEnv* env, jobject value);
  static jchar CharValue(JNIEnv* env, jobject value);
  static jboolean BooleanValue(JNIEnv* env, jobject value);
  static jshort ShortValue(JNIEnv* env, jobject value);
  static jint IntValue(JNIEnv* env, jobject value);
  static jlong LongValue(JNIEnv* env, jobject value);
  static jfloat FloatValue(JNIEnv* env, jobject value);
  static jdouble DoubleValue(JNIEnv* env, jobject value);

  static ScopedLocalJavaRef<jobjectArray> NewObjectArray(JNIEnv* env,
                                                         int length);
  static ScopedLocalJavaRef<jdoubleArray> NewDoubleArray(JNIEnv* env,
                                                         int length);
  static ScopedLocalJavaRef<jobjectArray> NewStringArray(JNIEnv* env,
                                                         int length);

  static void Init(JNIEnv* env, char type);
  static void ReleaseAll(JNIEnv* env);

 private:
  static void EnsureInstance(JNIEnv* env, char type);

  static jclass byte_clazz;
  static jmethodID byte_ctor;
  static jfieldID byte_valueField;

  static jclass char_clazz;
  static jmethodID char_ctor;
  static jfieldID char_valueField;

  static jclass boolean_clazz;
  static jmethodID boolean_ctor;
  static jfieldID boolean_valueField;

  static jclass short_clazz;
  static jmethodID short_ctor;
  static jfieldID short_valueField;

  static jclass int_clazz;
  static jmethodID int_ctor;
  static jfieldID int_valueField;

  static jclass long_clazz;
  static jmethodID long_ctor;
  static jfieldID long_valueField;

  static jclass float_clazz;
  static jmethodID float_ctor;
  static jfieldID float_valueField;

  static jclass double_clazz;
  static jmethodID double_ctor;
  static jfieldID double_valueField;

  static jclass string_clazz;
  static jclass object_clazz;
};

}  // namespace android
}  // namespace base
}  // namespace lynx
#endif  // BASE_INCLUDE_PLATFORM_ANDROID_JAVA_TYPE_H_
