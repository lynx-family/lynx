// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_JAVA_ONLY_ARRAY_H_
#define CORE_BASE_ANDROID_JAVA_ONLY_ARRAY_H_

#include <memory>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace base {
namespace android {

enum ReadableType {
  Null,
  Boolean,
  Int,
  Number,
  String,
  Map,
  Array,
  Long,
  ByteArray,
  PiperData,
  LynxObject,
};

std::string jstring2string(JNIEnv* env, jstring jStr);

class JavaOnlyMap;
class JavaValue;
class JavaOnlyArray {
 public:
  static bool RegisterJni(JNIEnv* env);
  JavaOnlyArray();
  JavaOnlyArray(JNIEnv* env,
                const lynx::base::android::ScopedLocalJavaRef<jobject>& ref)
      : jni_object_(env, ref.Get()) {}

  std::unique_ptr<base::android::JavaOnlyArray> ShallowCopy();
  ~JavaOnlyArray() = default;

  void PushString(const std::string& value);
  void PushBoolean(bool value);
  void PushInt(int value);
  void PushInt64(int64_t value);
  void PushDouble(double value);
  void PushArray(JavaOnlyArray* value);
  void PushByteArray(uint8_t* buffer, int length);
  void PushMap(JavaOnlyMap* value);
  void PushNull();
  void PushJavaValue(const JavaValue& value);
  void Clear();
  void Reset();

  inline jobject jni_object() { return jni_object_.Get(); }

  JavaOnlyArray(JNIEnv* env, jobject jni_object)
      : jni_object_(env, jni_object) {}
  explicit JavaOnlyArray(base::android::ScopedGlobalJavaRef<jobject> ref)
      : jni_object_(ref) {}

  static int32_t JavaOnlyArrayGetSize(JNIEnv* env, jobject array);
  static ReadableType JavaOnlyArrayGetTypeAtIndex(JNIEnv* env, jobject array,
                                                  int32_t index);
  static std::string JavaOnlyArrayGetStringAtIndex(JNIEnv* env, jobject array,
                                                   int32_t index);
  static bool JavaOnlyArrayGetBooleanAtIndex(JNIEnv* env, jobject array,
                                             int32_t index);
  static int32_t JavaOnlyArrayGetIntAtIndex(JNIEnv* env, jobject array,
                                            int32_t index);
  static int64_t JavaOnlyArrayGetLongAtIndex(JNIEnv* env, jobject array,
                                             int32_t index);
  static double JavaOnlyArrayGetDoubleAtIndex(JNIEnv* env, jobject array,
                                              int32_t index);
  static lynx::base::android::ScopedLocalJavaRef<jobject>
  JavaOnlyArrayGetMapAtIndex(JNIEnv* env, jobject array, int32_t index);
  static lynx::base::android::ScopedLocalJavaRef<jobject>
  JavaOnlyArrayGetArrayAtIndex(JNIEnv* env, jobject array, int32_t index);

  static lynx::base::android::ScopedLocalJavaRef<jobject>
  JavaOnlyArrayGetObjectAtIndex(JNIEnv* env, jobject array, int32_t index);
  static lynx::base::android::ScopedLocalJavaRef<jbyteArray>
  JavaOnlyArrayGetByteArrayAtIndex(JNIEnv* env, jobject array, int32_t index);

  static JavaValue JavaOnlyArrayGetJavaValueAtIndex(JNIEnv* env, jobject array,
                                                    size_t index);

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_JAVA_ONLY_ARRAY_H_
