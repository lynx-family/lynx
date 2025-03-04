// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_ANDROID_BASE_DEVTOOL_JNI_LOAD_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_ANDROID_BASE_DEVTOOL_JNI_LOAD_H_

#include <jni.h>

#include "devtool/base_devtool/native/android/devtool_global_slot_android.h"
#include "devtool/base_devtool/native/android/devtool_slot_android.h"

namespace basedevtool {
bool RegisterBaseDevToolSoFunctions(JNIEnv* env) {
  bool resultDevToolSlotDelegate =
      lynx::devtool::DevToolSlotDelegate::RegisterJNIUtils(env);
  bool resultDevToolGlobalSlotDelegate =
      lynx::devtool::DevToolGlobalSlotDelegate::RegisterJNIUtils(env);
  return resultDevToolSlotDelegate && resultDevToolGlobalSlotDelegate;
}
}  // namespace basedevtool

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_ANDROID_BASE_DEVTOOL_JNI_LOAD_H_
