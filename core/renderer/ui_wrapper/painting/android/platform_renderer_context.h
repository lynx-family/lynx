// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_type.h"
namespace lynx::tasm {

class PlatformRendererContext {
 public:
  PlatformRendererContext(JNIEnv* env, jobject j_this)
      : java_ref_(env, j_this) {}

  void CreatePlatformRenderer(int32_t id, PlatformRendererType type);

  void InsertPlatformRenderer(int32_t parent, int32_t child, int32_t index);

  void RemovePlatformRenderer(int32_t target);

  void DestroyPlatformRenderer(int32_t target);

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> java_ref_;
};

}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_
