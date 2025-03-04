// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_COMMON_ANDROID_LYNX_INSPECTOR_OWNER_NATIVE_GLUE_H_
#define DEVTOOL_LYNX_DEVTOOL_COMMON_ANDROID_LYNX_INSPECTOR_OWNER_NATIVE_GLUE_H_

#include <jni.h>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace devtool {
class LynxInspectorOwnerNativeGlue {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
  static void Reload(JNIEnv* env, jobject obj, bool ignore_cache);
  static void Navigate(JNIEnv* env, jobject obj, jstring url);
  static lynx::base::android::ScopedLocalJavaRef<jstring> GetTemplateUrl(
      JNIEnv* env, jobject obj);
  static void OnConnectionClose(JNIEnv* env, jobject obj);
  static void OnConnectionOpen(JNIEnv* env, jobject obj);
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_COMMON_ANDROID_LYNX_INSPECTOR_OWNER_NATIVE_GLUE_H_
