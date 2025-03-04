// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/android/base_devtool_jni_load.h"

#include "base/include/platform/android/jni_utils.h"

namespace basedevtool {
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  lynx::base::android::InitVM(vm);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  RegisterBaseDevToolSoFunctions(env);
  return JNI_VERSION_1_6;
}

}  // namespace basedevtool
