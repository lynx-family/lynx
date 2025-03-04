// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_CONTEXT_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_CONTEXT_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>
#include <unordered_set>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/public/layout_ctx_platform_impl.h"

namespace lynx {
namespace tasm {

class PlatformBundleHolderAndroid;

class LayoutContextAndroid : public LayoutCtxPlatformImpl {
 public:
  static bool RegisterJNI(JNIEnv* env);
  LayoutContextAndroid(JNIEnv* env, jobject impl);
  ~LayoutContextAndroid() override;

  int CreateLayoutNode(int id, const std::string& tag, PropBundle* props,
                       bool allow_inline) override;
  void InsertLayoutNode(int parent, int child, int index) override;
  void RemoveLayoutNode(int parent, int child, int index) override;
  void MoveLayoutNode(int parent, int child, int from_index,
                      int to_index) override;
  void UpdateLayoutNode(int id, PropBundle* painting_data) override;
  void OnLayoutBefore(int tag) override;
  void OnLayout(int tag, float left, float top, float width, float height,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override;
  void ScheduleLayout(base::closure callback) override;
  void DestroyLayoutNodes(const std::unordered_set<int>& ids) override;
  void Destroy() override;
  void SetFontFaces(const FontFacesMap& fontfaces) override;
  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) override;
  std::unique_ptr<PlatformExtraBundle> GetPlatformExtraBundle(
      int32_t id) override;

  std::unique_ptr<PlatformExtraBundleHolder> ReleasePlatformBundleHolder()
      override;

  void TriggerLayout();

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> impl_;
  std::unique_ptr<PlatformBundleHolderAndroid> bundle_holder_;

  base::closure trigger_layout_callback_;

  LayoutContextAndroid(const LayoutContextAndroid&) = delete;
  LayoutContextAndroid& operator=(const LayoutContextAndroid&) = delete;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_CONTEXT_ANDROID_H_
