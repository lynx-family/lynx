// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/java_only_array.h"

#include <memory>
#include <utility>

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/java_value.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/base/js_constants.h"
#include "core/base/json/json_util.h"
#include "core/build/gen/JavaOnlyArray_jni.h"

namespace lynx {
namespace base {
namespace android {

bool JavaOnlyArray::RegisterJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

JavaOnlyArray::JavaOnlyArray() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jni_object_.Reset(env, Java_JavaOnlyArray_create(env).Get());
}

std::unique_ptr<base::android::JavaOnlyArray> JavaOnlyArray::ShallowCopy() {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto array = Java_JavaOnlyArray_shallowCopy(env, jni_object_.Get());
  return std::make_unique<base::android::JavaOnlyArray>(env, array);
}

void JavaOnlyArray::PushString(const std::string& value) {
  JNIEnv* env = base::android::AttachCurrentThread();

  static const int api_level = android_get_device_api_level();
  // Emoji will make App crash when use `NewStringUTF` API in Android 5.x
  if (api_level < __ANDROID_API_M__) {
    base::android::ScopedLocalJavaRef<jbyteArray> jni_value =
        JNIConvertHelper::ConvertToJNIByteArray(env, value);
    Java_JavaOnlyArray_pushByteArrayAsString(env, jni_object_.Get(),
                                             jni_value.Get());
  } else {
    base::android::ScopedLocalJavaRef<jstring> jni_value =
        JNIConvertHelper::ConvertToJNIStringUTF(env, value);
    Java_JavaOnlyArray_pushString(env, jni_object_.Get(), jni_value.Get());
  }
}

void JavaOnlyArray::PushBoolean(bool value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushBoolean(env, jni_object_.Get(), value);
}

void JavaOnlyArray::PushInt64(int64_t value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushLong(env, jni_object_.Get(), value);
}

void JavaOnlyArray::PushInt(int value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushInt(env, jni_object_.Get(), value);
}

void JavaOnlyArray::PushDouble(double value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushDouble(env, jni_object_.Get(), value);
}

void JavaOnlyArray::PushArray(JavaOnlyArray* value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushArray(env, jni_object_.Get(), value->jni_object());
}

void JavaOnlyArray::PushByteArray(uint8_t* buffer, int length) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jbyteArray> jni_byte_array_ref;
  if (buffer && length > 0) {
    jni_byte_array_ref = base::android::JNIConvertHelper::ConvertToJNIByteArray(
        env, std::string(*buffer, length));
  }
  Java_JavaOnlyArray_pushByteArray(env, jni_object_.Get(),
                                   jni_byte_array_ref.Get());
}

void JavaOnlyArray::PushMap(JavaOnlyMap* value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushMap(env, jni_object_.Get(), value->jni_object());
}

void JavaOnlyArray::PushNull() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_pushNull(env, jni_object_.Get());
}

void JavaOnlyArray::PushJavaValue(const JavaValue& value) {
  JavaValue::JavaValueType type = value.type();
  switch (type) {
    case JavaValue::JavaValueType::Null:
      PushNull();
      break;
    case JavaValue::JavaValueType::Boolean:
      PushBoolean(value.Bool());
      break;
    case JavaValue::JavaValueType::Double:
      PushDouble(value.Double());
      break;
    case JavaValue::JavaValueType::Int32:
      PushInt(value.Int32());
      break;
    case JavaValue::JavaValueType::Int64:
      PushInt64(value.Int64());
      break;
    case JavaValue::JavaValueType::String: {
      JNIEnv* env = base::android::AttachCurrentThread();
      static const int api_level = android_get_device_api_level();
      // Emoji will make App crash when use `NewStringUTF` API in Android 5.x
      if (api_level < __ANDROID_API_M__) {
        PushString(value.String());
      } else {
        Java_JavaOnlyArray_pushString(env, jni_object_.Get(), value.JString());
      }
      break;
    }
    case JavaValue::JavaValueType::ByteArray: {
      JNIEnv* env = base::android::AttachCurrentThread();
      Java_JavaOnlyArray_pushByteArray(env, jni_object_.Get(),
                                       value.JByteArray());
      break;
    }
    case JavaValue::JavaValueType::Array:
      PushArray(value.Array().get());
      break;
    case JavaValue::JavaValueType::Map:
      PushMap(value.Map().get());
      break;
  }
}

void JavaOnlyArray::Clear() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JavaOnlyArray_clear(env, jni_object());
}

void JavaOnlyArray::Reset() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jni_object_.Reset(env, Java_JavaOnlyArray_create(env).Get());
}

int32_t JavaOnlyArray::JavaOnlyArrayGetSize(JNIEnv* env, jobject array) {
  return Java_JavaOnlyArray_size(env, array);
}

ReadableType JavaOnlyArray::JavaOnlyArrayGetTypeAtIndex(JNIEnv* env,
                                                        jobject array,
                                                        int32_t index) {
  return static_cast<ReadableType>(
      Java_JavaOnlyArray_getTypeIndex(env, array, index));
}

std::string JavaOnlyArray::JavaOnlyArrayGetStringAtIndex(JNIEnv* env,
                                                         jobject array,
                                                         int32_t index) {
  return base::android::JNIConvertHelper::ConvertToString(
      env, Java_JavaOnlyArray_getString(env, array, index).Get());
}

bool JavaOnlyArray::JavaOnlyArrayGetBooleanAtIndex(JNIEnv* env, jobject array,
                                                   int32_t index) {
  return Java_JavaOnlyArray_getBoolean(env, array, index);
}

int32_t JavaOnlyArray::JavaOnlyArrayGetIntAtIndex(JNIEnv* env, jobject array,
                                                  int32_t index) {
  return Java_JavaOnlyArray_getInt(env, array, index);
}

int64_t JavaOnlyArray::JavaOnlyArrayGetLongAtIndex(JNIEnv* env, jobject array,
                                                   int32_t index) {
  return Java_JavaOnlyArray_getLong(env, array, index);
}

double JavaOnlyArray::JavaOnlyArrayGetDoubleAtIndex(JNIEnv* env, jobject array,
                                                    int32_t index) {
  return Java_JavaOnlyArray_getDouble(env, array, index);
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaOnlyArray::JavaOnlyArrayGetMapAtIndex(JNIEnv* env, jobject array,
                                          int32_t index) {
  return Java_JavaOnlyArray_getMap(env, array, index);
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaOnlyArray::JavaOnlyArrayGetObjectAtIndex(JNIEnv* env, jobject array,
                                             int32_t index) {
  return Java_JavaOnlyArray_getObject(env, array, index);
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaOnlyArray::JavaOnlyArrayGetArrayAtIndex(JNIEnv* env, jobject array,
                                            int32_t index) {
  return Java_JavaOnlyArray_getArray(env, array, index);
}

lynx::base::android::ScopedLocalJavaRef<jbyteArray>
JavaOnlyArray::JavaOnlyArrayGetByteArrayAtIndex(JNIEnv* env, jobject array,
                                                int32_t index) {
  return Java_JavaOnlyArray_getByteArray(env, array, index);
}

std::string jstring2string(JNIEnv* env, jstring jStr) {
  if (!jStr) return "";
  const char* str = env->GetStringUTFChars(jStr, JNI_FALSE);
  std::string result = str;
  env->ReleaseStringUTFChars(jStr, str);
  return str;
}
JavaValue JavaOnlyArray::JavaOnlyArrayGetJavaValueAtIndex(JNIEnv* env,
                                                          jobject array,
                                                          size_t index) {
  ReadableType type = JavaOnlyArrayGetTypeAtIndex(env, array, index);
  switch (type) {
    case Null: {
      return JavaValue();
    }
    case Boolean: {
      return JavaValue(JavaOnlyArrayGetBooleanAtIndex(env, array, index));
    }
    case Int: {
      return JavaValue(JavaOnlyArrayGetIntAtIndex(env, array, index));
    }
    case Number: {
      return JavaValue(JavaOnlyArrayGetDoubleAtIndex(env, array, index));
    }
    case String: {
      return JavaValue(JavaOnlyArrayGetStringAtIndex(env, array, index));
    }
    case Map: {
      return JavaValue(std::make_shared<base::android::JavaOnlyMap>(
          env, JavaOnlyArrayGetMapAtIndex(env, array, index)));
    }
    case Array: {
      return JavaValue(std::make_shared<base::android::JavaOnlyArray>(
          env, JavaOnlyArrayGetArrayAtIndex(env, array, index)));
    }
    case Long: {
      return JavaValue(JavaOnlyArrayGetLongAtIndex(env, array, index));
    }
    case ByteArray: {
      lynx::base::android::ScopedLocalJavaRef<jbyteArray> j_byte_array =
          JavaOnlyArrayGetByteArrayAtIndex(env, array, index);
      auto* array_ptr =
          env->GetByteArrayElements(j_byte_array.Get(), JNI_FALSE);
      size_t len = env->GetArrayLength(j_byte_array.Get());
      JavaValue ret =
          JavaValue(reinterpret_cast<const uint8_t*>(array_ptr), len);
      env->ReleaseByteArrayElements(j_byte_array.Get(), array_ptr, JNI_FALSE);
      return ret;
    }
    case PiperData:
    case LynxObject: {
      // TODO(wujintian): Support PiperData and LynxObject
      return JavaValue();
    }
  }
}

}  // namespace android
}  // namespace base
}  // namespace lynx
