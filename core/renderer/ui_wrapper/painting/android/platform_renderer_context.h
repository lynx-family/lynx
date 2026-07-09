// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/closure.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "base/include/vector.h"
#include "core/public/platform_renderer_type.h"
#include "core/public/pub_value.h"
#include "core/renderer/ui_wrapper/painting/android/native_painting_context_android.h"

namespace lynx {
namespace lepus {
class Value;
}
namespace event {
class Event;
}
namespace tasm {

class PlatformRendererAndroid;
class PaintImage;
class NativePaintingCtxPlatformRef;

class PlatformRendererContext {
 public:
  PlatformRendererContext(JNIEnv* env, jobject j_this)
      : java_ref_(env, j_this) {}

  void Destroy();

  void CreatePlatformRenderer(int32_t id, PlatformRendererType type);
  void CreatePlatformExtendedRenderer(int32_t id, const base::String& tag_name,
                                      jobject init_data);

  void InsertPlatformRenderer(int32_t parent, int32_t child, int32_t index,
                              bool should_update_ui_owner);

  void RemovePlatformRenderer(int32_t parent, int32_t target,
                              bool should_update_ui_owner);

  void DestroyPlatformRenderer(int32_t target);

  void UpdatePlatformRendererFrame(int32_t target, bool need_clip,
                                   const float frame[4],
                                   const float render_offset[2],
                                   const float paddings[4],
                                   const float margins[4],
                                   const float borders[4]);

  // Update platform renderer subtree properties (transform, clip, etc.)
  void UpdatePlatformRendererSubtreeProperties(
      int32_t id, const SubtreeProperty* properties, size_t count);

  // Update platform renderer attributes
  void UpdatePlatformRendererAttributes(int32_t id, jobject prop_bundle,
                                        bool tends_to_flatten);

  void UpdatePlatformRendererExtraData(int32_t id, jobject extra_bundle);

  // Get PlatformRendererAndroid by ID
  PlatformRendererAndroid* GetPlatformRenderer(int32_t id);

  // Register/unregister PlatformRendererAndroid instances
  void RegisterPlatformRenderer(int32_t id, PlatformRendererAndroid* renderer);
  void UnregisterPlatformRenderer(int32_t id);
  fml::RefPtr<PaintImage> CreateImage(
      int32_t id, base::String src, float width, float height,
      int32_t event_mask, bool disable_default_resize,
      std::weak_ptr<NativePaintingCtxPlatformRef> platform_ref);
  void DestroyImage(int32_t id);

  void UpdateTextBundle(int32_t id, intptr_t text_bundle);

  void DestroyTextBundle(int32_t id);

  void FinishTasmOperation(int64_t operation_id);

  void FinishLayoutOperation(int32_t component_id, int64_t operation_id,
                             bool is_first_screen);

  void OnNodeReady(const std::vector<int32_t>& ids);

  int32_t GetTagInfo(const std::string& tag_name);

  std::vector<float> GetRootViewLocationOnScreen();
  std::vector<float> GetScreenSize();
  std::vector<float> GetRendererHostScrollOffset(int32_t sign);
  bool IsRendererHostScrollable(int32_t sign);
  void InvokeUIMethod(
      int32_t id, const std::string& method, const lepus::Value& params,
      base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback);
  void InvokeUIMethodCallback(int32_t callback_id, int32_t code,
                              const lepus::Value& data);

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> java_ref_;
  base::InlineOrderedFlatMap<int32_t, PlatformRendererAndroid*, 64>
      renderer_registry_;
  std::unordered_map<int32_t,
                     base::MoveOnlyClosure<void, int32_t, const pub::Value&>>
      invoke_ui_method_callbacks_;
  int32_t invoke_ui_method_callback_id_{0};
  int32_t GenerateUniqueImageKey() { return ++image_key_; }
  int32_t image_key_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_CONTEXT_H_
