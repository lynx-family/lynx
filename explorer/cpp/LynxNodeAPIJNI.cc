// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <string>

#include "LynxNodeAPI.h"

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_explorer_modules_LynxNodeAPIModule_nativeRequireNodeAddon(
    JNIEnv* env, jobject thiz, jlong napiEnv, jstring addonName) {
  if (napiEnv == 0 || addonName == nullptr) return;

  const char* c_addonName = env->GetStringUTFChars(addonName, nullptr);
  if (c_addonName == nullptr) return;

  std::string addonNameStr(c_addonName);
  env->ReleaseStringUTFChars(addonName, c_addonName);

  lynx::explorer::LynxNodeAPI::GetInstance().RequireNodeAddon(
      reinterpret_cast<void*>(napiEnv), addonNameStr);
}
