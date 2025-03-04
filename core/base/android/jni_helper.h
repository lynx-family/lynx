// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_BASE_ANDROID_JNI_HELPER_H_
#define CORE_BASE_ANDROID_JNI_HELPER_H_

#include <jni.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/platform/android/java_type.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace base {
namespace android {
class JNIHelper {
 public:
  static lynx::base::android::ScopedLocalJavaRef<jbyteArray>
  ConvertToJNIByteArray(JNIEnv* env, piper::Runtime* rt,
                        const piper::ArrayBuffer& buf);

  static piper::ArrayBuffer ConvertToJSIArrayBuffer(JNIEnv* env,
                                                    piper::Runtime* rt,
                                                    jbyteArray j_obj);

  static ScopedLocalJavaRef<jobject> ConvertSTLStringMapToJavaMap(
      JNIEnv* env, const std::unordered_map<std::string, std::string>& map);
};
}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_JNI_HELPER_H_
