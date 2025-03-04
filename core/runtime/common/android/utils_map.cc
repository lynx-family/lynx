// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <utility>

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/base/js_constants.h"
#include "core/build/gen/JavaOnlyMap_jni.h"
#include "core/runtime/bindings/jsi/modules/android/platform_jsi/lynx_platform_jsi_object_android.h"
#include "core/runtime/common/utils.h"

namespace lynx {
namespace piper {
bool JSBUtilsMapRegisterJNI(JNIEnv* env) { return RegisterNativesImpl(env); }

std::optional<piper::Object> jsObjectFromJavaOnlyMap(JNIEnv* env, jobject map,
                                                     piper::Runtime* rt) {
  piper::Scope scope(*rt);
  piper::Object obj = piper::Object(*rt);
  auto keys = Java_JavaOnlyMap_getKeys(env, map);
  if (keys.IsNull()) {
    LOGE(
        "JavaOnlyMap jsObjectFromJavaOnlyMap error! Get null object from "
        "getKeys API. It may be caused by OOM.");
    return std::optional<piper::Object>();
  }
  base::android::ScopedGlobalJavaRef<jobject> keys_global(env, keys.Get());
  keys.Reset();

  jclass cls_arraylist = env->GetObjectClass(keys_global.Get());
  base::android::ScopedGlobalJavaRef<jclass> cls_arraylist_global(
      env, cls_arraylist);
  env->DeleteLocalRef(cls_arraylist);

  jmethodID arraylist_get = env->GetMethodID(cls_arraylist_global.Get(), "get",
                                             "(I)Ljava/lang/Object;");
  jmethodID arraylist_size =
      env->GetMethodID(cls_arraylist_global.Get(), "size", "()I");
  jint module_len = env->CallIntMethod(keys_global.Get(), arraylist_size);

  for (int i = 0; i < module_len; i++) {
    jstring key_str = static_cast<jstring>(
        env->CallObjectMethod(keys_global.Get(), arraylist_get, i));
    base::android::ScopedGlobalJavaRef<jstring> keyJString(env, key_str);
    std::string keyChar;
    {
      const char* str = env->GetStringUTFChars(key_str, JNI_FALSE);
      keyChar = str;
      env->ReleaseStringUTFChars(key_str, str);
    }
    env->DeleteLocalRef(key_str);

    jint typeIndex = Java_JavaOnlyMap_getTypeIndex(env, map, keyJString.Get());
    lynx::base::android::ReadableType type =
        static_cast<base::android::ReadableType>(typeIndex);

    bool is_successful = true;
    switch (type) {
      case lynx::base::android::ReadableType::Null:
        is_successful = obj.setProperty(*rt, keyChar.c_str(), nullptr);
        break;
      case lynx::base::android::ReadableType::String: {
        std::string stdString;
        {
          auto jStr = Java_JavaOnlyMap_getString(env, map, keyJString.Get());
          const char* str = env->GetStringUTFChars(jStr.Get(), JNI_FALSE);
          stdString = str;
          env->ReleaseStringUTFChars(jStr.Get(), str);
        }
        is_successful = obj.setProperty(*rt, keyChar.c_str(), stdString);
      } break;
      case lynx::base::android::ReadableType::Boolean: {
        bool b = Java_JavaOnlyMap_getBoolean(env, map, keyJString.Get());
        is_successful = obj.setProperty(*rt, keyChar.c_str(), b);
        break;
      }
      case lynx::base::android::ReadableType::Int: {
        jint n = Java_JavaOnlyMap_getInt(env, map, keyJString.Get());
        is_successful = obj.setProperty(*rt, keyChar.c_str(), n);
        break;
      }
      case lynx::base::android::ReadableType::Long: {
        // In JavaScript,  the max safe integer is 9007199254740991 and the min
        // safe integer is -9007199254740991, so when integer beyond limit, use
        // BigInt Object to define it. More information from
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
        jlong n = Java_JavaOnlyMap_getLong(env, map, keyJString.Get());
        if (n < piper::kMinJavaScriptNumber ||
            n > piper::kMaxJavaScriptNumber) {
          auto bigint = piper::BigInt::createWithString(*rt, std::to_string(n));
          if (!bigint) {
            return std::optional<piper::Object>();
          }
          is_successful =
              obj.setProperty(*rt, keyChar.c_str(), std::move(*bigint));
        } else {
          // cast to double
          double d = Java_JavaOnlyMap_getDouble(env, map, keyJString.Get());
          is_successful = obj.setProperty(*rt, keyChar.c_str(), d);
        }
        break;
      }
      case lynx::base::android::ReadableType::Number: {
        double d = Java_JavaOnlyMap_getDouble(env, map, keyJString.Get());
        is_successful = obj.setProperty(*rt, keyChar.c_str(), d);
        break;
      }
      case lynx::base::android::ReadableType::Array: {
        auto arr = Java_JavaOnlyMap_getArray(env, map, keyJString.Get());
        base::android::ScopedGlobalJavaRef<jobject> arr_global(env, arr.Get());
        arr.Reset();
        auto array_opt = jsArrayFromJavaOnlyArray(env, arr_global.Get(), rt);
        if (!array_opt) {
          return std::optional<piper::Object>();
        }
        is_successful =
            obj.setProperty(*rt, keyChar.c_str(), std::move(*array_opt));
        break;
      }
      case lynx::base::android::ReadableType::Map: {
        auto inner = Java_JavaOnlyMap_getMap(env, map, keyJString.Get());
        base::android::ScopedGlobalJavaRef<jobject> inner_global(env,
                                                                 inner.Get());
        inner.Reset();
        auto map_opt = jsObjectFromJavaOnlyMap(env, inner_global.Get(), rt);
        if (!map_opt) {
          return std::optional<piper::Object>();
        }
        is_successful =
            obj.setProperty(*rt, keyChar.c_str(), std::move(*map_opt));
        break;
      }
      case lynx::base::android::ReadableType::ByteArray: {
        base::android::ScopedLocalJavaRef<jbyteArray> byte_array =
            Java_JavaOnlyMap_getByteArray(env, map, keyJString.Get());
        is_successful =
            obj.setProperty(*rt, keyChar.c_str(),
                            base::android::JNIHelper::ConvertToJSIArrayBuffer(
                                env, rt, byte_array.Get()));
        break;
      }
      case lynx::base::android::ReadableType::PiperData: {
        base::android::ScopedLocalJavaRef<jobject> piper_data =
            Java_JavaOnlyMap_getPiperData(env, map, keyJString.Get());
        auto js_object_opt = base::android::PiperData::jsObjectFromPiperData(
            env, rt, std::move(piper_data));
        if (!js_object_opt) {
          return std::optional<piper::Object>();
        }
        obj.setProperty(*rt, keyChar.c_str(), std::move(*js_object_opt));
        break;
      }
      case lynx::base::android::ReadableType::LynxObject: {
        // LynxObject should not appear in JavaOnlyMap
        is_successful = false;
        break;
      }
    }
    if (!is_successful) {
      return std::optional<piper::Object>();
    }
  }

  return obj;
}

void PushByteArrayToJavaMap(piper::Runtime* rt, const std::string& key,
                            const piper::ArrayBuffer& array_buffer,
                            base::android::JavaOnlyMap* jmap) {
  // TODO(qiuxian):Add unit test when framework is ready.
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  base::android::ScopedLocalJavaRef<jbyteArray> jni_byte_array =
      base::android::JNIHelper::ConvertToJNIByteArray(env, rt, array_buffer);
  Java_JavaOnlyMap_putByteArray(env, jmap->jni_object(), jni_key.Get(),
                                jni_byte_array.Get());
}

}  // namespace piper
}  // namespace lynx
