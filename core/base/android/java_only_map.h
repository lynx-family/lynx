// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_JAVA_ONLY_MAP_H_
#define CORE_BASE_ANDROID_JAVA_ONLY_MAP_H_
#include <memory>
#include <string>

#include "base/include/closure.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "core/base/android/java_only_array.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace base {
namespace android {
class JavaOnlyArray;
class JavaValue;
class JavaOnlyMap {
 public:
  // return type: void
  // arguments: JNIENV, JavaOnlyMap, jstring key, std::string key
  using ForEachClosure = base::MoveOnlyClosure<void, JNIEnv*, jobject, jstring,
                                               const std::string&>;

  static bool RegisterJni(JNIEnv* env);
  JavaOnlyMap();

  JavaOnlyMap(JNIEnv* env,
              const lynx::base::android::ScopedLocalJavaRef<jobject>& ref)
      : jni_object_(env, ref.Get()) {}

  std::unique_ptr<base::android::JavaOnlyMap> ShallowCopy();

  ~JavaOnlyMap() = default;

  void PushString(const std::string& key, const std::string& value);
  void PushString(const char* key, const char* value);
  void PushNull(const char* key);
  void PushBoolean(const std::string& key, bool value);
  void PushInt(const std::string& key, int value);
  void PushInt(const char* key, int value);
  void PushInt64(const char* key, int64_t value);
  void PushDouble(const std::string& key, double value);
  void PushDouble(const char* key, double value);
  void PushMap(const std::string& key, JavaOnlyMap* value);
  void PushArray(const std::string& key, JavaOnlyArray* value);
  void PushArray(const char* key, JavaOnlyArray* value);
  void PushByteArray(const std::string& key, uint8_t* buffer, int length);
  void PushJavaValue(const std::string& key, const JavaValue& value);

  bool Contains(const char* key) const;

  int Size();

  void Reset();

  inline jobject jni_object() { return jni_object_.Get(); }

  static lynx::base::android::ScopedLocalJavaRef<jobject> JavaOnlyMapGetKeys(
      JNIEnv* env, jobject map);
  static ReadableType JavaOnlyMapGetTypeAtIndex(JNIEnv* env, jobject map,
                                                jstring key);
  static std::string JavaOnlyMapGetStringAtIndex(JNIEnv* env, jobject map,
                                                 jstring key);
  static bool JavaOnlyMapGetBooleanAtIndex(JNIEnv* env, jobject map,
                                           jstring key);
  static int32_t JavaOnlyMapGetIntAtIndex(JNIEnv* env, jobject map,
                                          jstring key);
  static int64_t JavaOnlyMapGetLongAtIndex(JNIEnv* env, jobject map,
                                           jstring key);
  static double JavaOnlyMapGetDoubleAtIndex(JNIEnv* env, jobject map,
                                            jstring key);
  static lynx::base::android::ScopedLocalJavaRef<jobject>
  JavaOnlyMapGetMapAtIndex(JNIEnv* env, jobject map, jstring key);
  static lynx::base::android::ScopedLocalJavaRef<jobject>
  JavaOnlyMapGetArrayAtIndex(JNIEnv* env, jobject map, jstring key);
  static lynx::base::android::ScopedLocalJavaRef<jbyteArray>
  JavaOnlyMapGetByteArrayAtIndex(JNIEnv* env, jobject map, jstring key);

  static JavaValue JavaOnlyMapGetJavaValueAtIndex(JNIEnv* env, jobject map,
                                                  jstring key);

  static void ForEach(JNIEnv* env, jobject map, ForEachClosure closure);

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_JAVA_ONLY_MAP_H_
