// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_BASE_ANDROID_LYNX_ERROR_ANDROID_H_
#define CORE_BASE_ANDROID_LYNX_ERROR_ANDROID_H_

#include <string>
#include <unordered_map>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {

enum class LynxErrorLevel;

namespace android {

class LynxErrorAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);
  LynxErrorAndroid(
      int error_code, const std::string& error_message,
      const std::string& fix_suggestion, base::LynxErrorLevel level,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only);
  ~LynxErrorAndroid() = default;

  jobject jni_object() { return jni_object_.Get(); }

 private:
  ScopedGlobalJavaRef<jobject> jni_object_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_LYNX_ERROR_ANDROID_H_
