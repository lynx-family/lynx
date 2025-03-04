// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/lynx_android_blur.h"

#include <jni.h>

#include "core/base/android/fresco_blur_filter.h"
#include "core/build/gen/BlurUtils_jni.h"

void IterativeBoxBlur(JNIEnv* env, jclass jcaller, jobject bitmap,
                      jint iterations, jint radius) {
  fresco_iterativeBoxBlur(env, jcaller, bitmap, iterations, radius);
}
namespace lynx {
namespace base {
void LynxAndroidBlur::RegisterJNI(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

}  // namespace base
}  // namespace lynx
