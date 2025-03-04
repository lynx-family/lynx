// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_NODE_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_NODE_ANDROID_H_

#include <jni.h>

#include "base/include/platform/android/scoped_java_ref.h"
namespace lynx {
namespace tasm {
class LayoutNodeAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);
  static lynx::base::android::ScopedLocalJavaRef<jfloatArray> Measure(
      JNIEnv* env, jobject obj, jfloat width, int widthMode, jfloat height,
      int heightMode, jboolean finalMeasure);
  static void Align(JNIEnv* env, jobject obj);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_NODE_ANDROID_H_
