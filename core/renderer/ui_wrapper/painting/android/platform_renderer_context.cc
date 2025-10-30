// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"

#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_register_jni.h"

jlong CreateEmbeddedViewContext(JNIEnv* env, jobject jcaller, jobject jThis) {
  return reinterpret_cast<jlong>(
      new lynx::tasm::PlatformRendererContext(env, jThis));
}

namespace lynx {
namespace jni {
bool RegisterJNIForPlatformRendererContext(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
namespace tasm {
void PlatformRendererContext::CreatePlatformRenderer(
    int32_t id, PlatformRendererType type) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_createPlatformRenderer(
      env, local_ref.Get(), id, static_cast<int32_t>(type));
}

void PlatformRendererContext::InsertPlatformRenderer(int32_t parent,
                                                     int32_t child,
                                                     int32_t index) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_insertPlatformRenderer(env, local_ref.Get(),
                                                      parent, child, index);
}

void PlatformRendererContext::RemovePlatformRenderer(int32_t target) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_removePlatformRendererFromParent(
      env, local_ref.Get(), target);
}

void PlatformRendererContext::DestroyPlatformRenderer(int32_t target) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_destroyPlatformRenderer(env, local_ref.Get(),
                                                       target);
}

}  // namespace tasm
}  // namespace lynx
