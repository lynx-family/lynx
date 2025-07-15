// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"

#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_register_jni.h"

jlong CreateEmbeddedViewContext(JNIEnv* env, jobject jcaller, jobject jThis) {
  return reinterpret_cast<jlong>(new lynx::tasm::PlatformRendererContext());
}

namespace lynx {
namespace jni {
bool RegisterJNIForPlatformRendererContext(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx
