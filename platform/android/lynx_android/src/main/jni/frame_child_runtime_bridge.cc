// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include "core/renderer/dom/android/lynx_template_bundle_android.h"
#include "core/template_bundle/lynx_template_bundle.h"

extern "C" JNIEXPORT jobject JNICALL
Java_com_lynx_tasm_FrameChildRuntimeAndroid_nativeCloneBundle(
    JNIEnv* env, jclass, jlong bundle_pointer) {
  auto* bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(
      bundle_pointer);
  if (!bundle) {
    return nullptr;
  }
  auto result = lynx::tasm::ConstructJTemplateBundleFromNative(*bundle);
  return env->NewLocalRef(result.Get());
}
