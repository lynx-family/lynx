// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "base/include/vector.h"
#include "core/public/platform_renderer_type.h"
#include "core/renderer/ui_wrapper/painting/android/native_painting_context_android.h"

namespace lynx::tasm {

class PlatformRendererAndroid;

class PlatformRendererContext {
 public:
  PlatformRendererContext(JNIEnv* env, jobject j_this)
      : java_ref_(env, j_this) {}

  void CreatePlatformRenderer(int32_t id, PlatformRendererType type);

  void InsertPlatformRenderer(int32_t parent, int32_t child, int32_t index);

  void RemovePlatformRenderer(int32_t target);

  void DestroyPlatformRenderer(int32_t target);

  void UpdatePlatformRendererFrame(int32_t target, const float frame[4],
                                   const float render_offset[2]);

  // Get PlatformRendererAndroid by ID
  PlatformRendererAndroid* GetPlatformRenderer(int32_t id);

  // Register/unregister PlatformRendererAndroid instances
  void RegisterPlatformRenderer(int32_t id, PlatformRendererAndroid* renderer);
  void UnregisterPlatformRenderer(int32_t id);
  void CreateImage(int32_t id, base::String src, float width, float height);
  void DestroyImage(int32_t id);

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> java_ref_;
  base::InlineOrderedFlatMap<int32_t, PlatformRendererAndroid*, 64>
      renderer_registry_;
};

}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_
