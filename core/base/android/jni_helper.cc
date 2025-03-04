// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/android/jni_helper.h"

#include <unordered_set>
#include <vector>

#include "base/include/log/logging.h"
#include "core/base/android/java_only_map.h"

namespace lynx {
namespace base {
namespace android {

piper::ArrayBuffer JNIHelper::ConvertToJSIArrayBuffer(JNIEnv* env,
                                                      piper::Runtime* rt,
                                                      jbyteArray j_obj) {
  jsize length = env->GetArrayLength(j_obj);
  jboolean is_copy = 0;
  jbyte* bytes = env->GetByteArrayElements(j_obj, &is_copy);
  piper::ArrayBuffer array_buffer =
      piper::ArrayBuffer(*rt, reinterpret_cast<const uint8_t*>(bytes), length);
  env->ReleaseByteArrayElements(j_obj, bytes, 0);
  return array_buffer;
}

lynx::base::android::ScopedLocalJavaRef<jbyteArray>
JNIHelper::ConvertToJNIByteArray(JNIEnv* env, piper::Runtime* rt,
                                 const piper::ArrayBuffer& array_buffer) {
  size_t length = array_buffer.length(*rt);
  uint8_t* buffer = array_buffer.data(*rt);
  if (!buffer || length <= 0) {
    return ScopedLocalJavaRef<jbyteArray>();
  }
  jbyteArray jni_byte_array = env->NewByteArray(length);  // NOLINT
  env->SetByteArrayRegion(jni_byte_array, 0, length,
                          reinterpret_cast<jbyte*>(buffer));
  return ScopedLocalJavaRef(env, jni_byte_array);
}

ScopedLocalJavaRef<jobject> JNIHelper::ConvertSTLStringMapToJavaMap(
    JNIEnv* env, const std::unordered_map<std::string, std::string>& map) {
  if (map.empty()) {
    return ScopedLocalJavaRef<jobject>();
  }

  JavaOnlyMap j_map;
  for (const auto& [key, value] : map) {
    j_map.PushString(key.c_str(), value.c_str());
  }
  // j_map.jni_object_ is a ScopedGlobalJavaRef object, it will be delete when
  // j_map released. So we need to call NewLocalRef to get its local reference.
  ScopedLocalJavaRef<jobject> scoped_j_map;
  scoped_j_map.Reset(env, j_map.jni_object());
  return scoped_j_map;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
