// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ANDROID_LYNX_GET_UI_RESULT_ANDROID_H_
#define CORE_RENDERER_DOM_ANDROID_LYNX_GET_UI_RESULT_ANDROID_H_

#include "core/base/android/android_jni.h"

namespace lynx {
namespace tasm {
class LynxGetUIResult;

class LynxGetUIResultAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);

  LynxGetUIResultAndroid(const LynxGetUIResult& result);

  inline jobject jni_object() { return jni_object_.Get(); }

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ANDROID_LYNX_GET_UI_RESULT_ANDROID_H_
