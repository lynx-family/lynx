// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/java_only_map.h"

#include <android/api-level.h>

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_value.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/base/js_constants.h"
#include "core/build/gen/JavaOnlyMap_jni.h"

namespace lynx {
namespace base {
namespace android {

bool JavaOnlyMap::RegisterJni(JNIEnv* env) { return RegisterNativesImpl(env); }

JavaOnlyMap::JavaOnlyMap() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jni_object_.Reset(env, Java_JavaOnlyMap_create(env).Get());
}

std::unique_ptr<base::android::JavaOnlyMap> JavaOnlyMap::ShallowCopy() {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto map = Java_JavaOnlyMap_shallowCopy(env, jni_object_.Get());
  return std::make_unique<base::android::JavaOnlyMap>(env, map);
}

void JavaOnlyMap::PushString(const std::string& key, const std::string& value) {
  PushString(key.c_str(), value.c_str());
}

void JavaOnlyMap::PushString(const char* key, const char* value) {
  JNIEnv* env = base::android::AttachCurrentThread();

  static const int api_level = android_get_device_api_level();
  // Emoji will make App crash when use `NewStringUTF` API in Android 5.x
  if (api_level < __ANDROID_API_M__) {  // Build.VERSION_CODES.M
    base::android::ScopedLocalJavaRef<jbyteArray> jni_key =
        JNIConvertHelper::ConvertToJNIByteArray(env, key);
    base::android::ScopedLocalJavaRef<jbyteArray> jni_value =
        JNIConvertHelper::ConvertToJNIByteArray(env, value);
    Java_JavaOnlyMap_putByteArrayAsString(env, jni_object_.Get(), jni_key.Get(),
                                          jni_value.Get());
  } else {
    base::android::ScopedLocalJavaRef<jstring> jni_key =
        JNIConvertHelper::ConvertToJNIStringUTF(env, key);
    base::android::ScopedLocalJavaRef<jstring> jni_value =
        JNIConvertHelper::ConvertToJNIStringUTF(env, value);
    Java_JavaOnlyMap_putString(env, jni_object_.Get(), jni_key.Get(),
                               jni_value.Get());
  }
}

void JavaOnlyMap::PushNull(const char* key) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putNull(env, jni_object_.Get(), jni_key.Get());
}

void JavaOnlyMap::PushBoolean(const std::string& key, bool value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putBoolean(env, jni_object_.Get(), jni_key.Get(), value);
}

void JavaOnlyMap::PushInt(const std::string& key, int value) {
  PushInt(key.c_str(), value);
}

void JavaOnlyMap::PushInt(const char* key, int value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putInt(env, jni_object_.Get(), jni_key.Get(), value);
}

void JavaOnlyMap::PushInt64(const char* key, int64_t value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putLong(env, jni_object_.Get(), jni_key.Get(), value);
}

void JavaOnlyMap::PushDouble(const std::string& key, double value) {
  PushDouble(key.c_str(), value);
}

void JavaOnlyMap::PushDouble(const char* key, double value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putDouble(env, jni_object_.Get(), jni_key.Get(), value);
}

void JavaOnlyMap::PushMap(const std::string& key, JavaOnlyMap* value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putMap(env, jni_object_.Get(), jni_key.Get(),
                          value->jni_object());
}

void JavaOnlyMap::PushArray(const std::string& key, JavaOnlyArray* value) {
  PushArray(key.c_str(), value);
}

void JavaOnlyMap::PushArray(const char* key, JavaOnlyArray* value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_JavaOnlyMap_putArray(env, jni_object_.Get(), jni_key.Get(),
                            value->jni_object());
}

void JavaOnlyMap::PushByteArray(const std::string& key, uint8_t* buffer,
                                int length) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jbyteArray> jni_byte_array_ref;
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  if (buffer && length > 0) {
    jni_byte_array_ref = base::android::JNIConvertHelper::ConvertToJNIByteArray(
        env, std::string(*buffer, length));
  }
  Java_JavaOnlyMap_putByteArray(env, jni_object_.Get(), jni_key.Get(),
                                jni_byte_array_ref.Get());
}

void JavaOnlyMap::PushJavaValue(const std::string& key,
                                const JavaValue& value) {
  JavaValue::JavaValueType type = value.type();
  switch (type) {
    case JavaValue::JavaValueType::Null:
      PushNull(key.c_str());
      break;
    case JavaValue::JavaValueType::Boolean:
      PushBoolean(key, value.Bool());
      break;
    case JavaValue::JavaValueType::Double:
      PushDouble(key, value.Double());
      break;
    case JavaValue::JavaValueType::Int32:
      PushInt(key, value.Int32());
      break;
    case JavaValue::JavaValueType::Int64:
      PushInt64(key.c_str(), value.Int64());
      break;
    case JavaValue::JavaValueType::String: {
      JNIEnv* env = base::android::AttachCurrentThread();
      base::android::ScopedLocalJavaRef<jstring> jni_key =
          JNIConvertHelper::ConvertToJNIStringUTF(env, key);
      Java_JavaOnlyMap_putString(env, jni_object_.Get(), jni_key.Get(),
                                 value.JString());
      break;
    }
    case JavaValue::JavaValueType::ByteArray: {
      JNIEnv* env = base::android::AttachCurrentThread();
      base::android::ScopedLocalJavaRef<jstring> jni_key =
          JNIConvertHelper::ConvertToJNIStringUTF(env, key);
      Java_JavaOnlyMap_putByteArray(env, jni_object_.Get(), jni_key.Get(),
                                    value.JByteArray());
      break;
    }
    case JavaValue::JavaValueType::Array:
      PushArray(key, value.Array().get());
      break;
    case JavaValue::JavaValueType::Map:
      PushMap(key, value.Map().get());
      break;
  }
}

int JavaOnlyMap::Size() {
  JNIEnv* env = base::android::AttachCurrentThread();

  return static_cast<int>(Java_JavaOnlyMap_size(env, jni_object_.Get()));
}

bool JavaOnlyMap::Contains(const char* key) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  return static_cast<ReadableType>(Java_JavaOnlyMap_getTypeIndex(
             env, jni_object_.Get(), jni_key.Get())) !=
         lynx::base::android::ReadableType::Null;
}

void JavaOnlyMap::Reset() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jni_object_.Reset(env, Java_JavaOnlyMap_create(env).Get());
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaOnlyMap::JavaOnlyMapGetKeys(JNIEnv* env, jobject map) {
  return Java_JavaOnlyMap_getKeys(env, map);
}

ReadableType JavaOnlyMap::JavaOnlyMapGetTypeAtIndex(JNIEnv* env, jobject map,
                                                    jstring key) {
  return static_cast<ReadableType>(
      Java_JavaOnlyMap_getTypeIndex(env, map, key));
}

std::string JavaOnlyMap::JavaOnlyMapGetStringAtIndex(JNIEnv* env, jobject map,
                                                     jstring key) {
  return base::android::JNIConvertHelper::ConvertToString(
      env, Java_JavaOnlyMap_getString(env, map, key).Get());
}

bool JavaOnlyMap::JavaOnlyMapGetBooleanAtIndex(JNIEnv* env, jobject map,
                                               jstring key) {
  return Java_JavaOnlyMap_getBoolean(env, map, key);
}

int32_t JavaOnlyMap::JavaOnlyMapGetIntAtIndex(JNIEnv* env, jobject map,
                                              jstring key) {
  return Java_JavaOnlyMap_getInt(env, map, key);
}

int64_t JavaOnlyMap::JavaOnlyMapGetLongAtIndex(JNIEnv* env, jobject map,
                                               jstring key) {
  return Java_JavaOnlyMap_getLong(env, map, key);
}

double JavaOnlyMap::JavaOnlyMapGetDoubleAtIndex(JNIEnv* env, jobject map,
                                                jstring key) {
  return Java_JavaOnlyMap_getDouble(env, map, key);
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaOnlyMap::JavaOnlyMapGetMapAtIndex(JNIEnv* env, jobject map, jstring key) {
  return Java_JavaOnlyMap_getMap(env, map, key);
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaOnlyMap::JavaOnlyMapGetArrayAtIndex(JNIEnv* env, jobject map, jstring key) {
  return Java_JavaOnlyMap_getArray(env, map, key);
}

lynx::base::android::ScopedLocalJavaRef<jbyteArray>
JavaOnlyMap::JavaOnlyMapGetByteArrayAtIndex(JNIEnv* env, jobject map,
                                            jstring key) {
  return Java_JavaOnlyMap_getByteArray(env, map, key);
}

void JavaOnlyMap::ForEach(JNIEnv* env, jobject map, ForEachClosure closure) {
  auto keys = base::android::JavaOnlyMap::JavaOnlyMapGetKeys(env, map);

  jclass cls_arraylist = env->GetObjectClass(keys.Get());
  base::android::ScopedGlobalJavaRef<jclass> cls_arraylist_global(
      env, cls_arraylist);
  env->DeleteLocalRef(cls_arraylist);

  jmethodID arraylist_get = env->GetMethodID(cls_arraylist_global.Get(), "get",
                                             "(I)Ljava/lang/Object;");
  jmethodID arraylist_size =
      env->GetMethodID(cls_arraylist_global.Get(), "size", "()I");
  jint module_len = env->CallIntMethod(keys.Get(), arraylist_size);

  for (int i = 0; i < module_len; i++) {
    jstring key_str = static_cast<jstring>(
        env->CallObjectMethod(keys.Get(), arraylist_get, i));
    base::android::ScopedLocalJavaRef<jstring> key_jstring(env, key_str);
    std::string key = base::android::JNIConvertHelper::ConvertToString(
        env, key_jstring.Get());

    closure(env, map, key_jstring.Get(), key);
  }
}

JavaValue JavaOnlyMap::JavaOnlyMapGetJavaValueAtIndex(JNIEnv* env, jobject map,
                                                      jstring key) {
  base::android::ReadableType type =
      base::android::JavaOnlyMap::JavaOnlyMapGetTypeAtIndex(env, map, key);
  switch (type) {
    case Null: {
      return JavaValue();
    }
    case Boolean: {
      return JavaValue(JavaOnlyMapGetBooleanAtIndex(env, map, key));
    }
    case Int: {
      return JavaValue(JavaOnlyMapGetIntAtIndex(env, map, key));
    }
    case Number: {
      return JavaValue(JavaOnlyMapGetDoubleAtIndex(env, map, key));
    }
    case String: {
      return JavaValue(JavaOnlyMapGetStringAtIndex(env, map, key));
    }
    case Map: {
      return JavaValue(std::make_shared<base::android::JavaOnlyMap>(
          env, JavaOnlyMapGetMapAtIndex(env, map, key)));
    }
    case Array: {
      return JavaValue(std::make_shared<base::android::JavaOnlyArray>(
          env, JavaOnlyMapGetArrayAtIndex(env, map, key)));
    }
    case Long: {
      return JavaValue(JavaOnlyMapGetLongAtIndex(env, map, key));
    }
    case ByteArray: {
      lynx::base::android::ScopedLocalJavaRef<jbyteArray> j_byte_array =
          JavaOnlyMapGetByteArrayAtIndex(env, map, key);
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
