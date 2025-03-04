// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_LYNX_ANDROID_BLUR_H_
#define CORE_BASE_ANDROID_LYNX_ANDROID_BLUR_H_

#include <jni.h>

namespace lynx {
namespace base {

class LynxAndroidBlur {
 public:
  LynxAndroidBlur() = delete;
  ~LynxAndroidBlur() = delete;

  static void RegisterJNI(JNIEnv* env);
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_LYNX_ANDROID_BLUR_H_
