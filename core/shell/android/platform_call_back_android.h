// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_PLATFORM_CALL_BACK_ANDROID_H_
#define CORE_SHELL_ANDROID_PLATFORM_CALL_BACK_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/common/platform_call_back_manager.h"

namespace lynx {
namespace shell {

class PlatformCallBackAndroid : public PlatformCallBack {
 public:
  PlatformCallBackAndroid(JNIEnv* env, jobject jni_object)
      : jni_object_(env, jni_object){};
  ~PlatformCallBackAndroid() override = default;

  static void RegisterJni(JNIEnv* env);

  void InvokeWithValue(const lepus::Value& value) override;

 private:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_PLATFORM_CALL_BACK_ANDROID_H_
