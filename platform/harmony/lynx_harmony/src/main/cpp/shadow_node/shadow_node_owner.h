// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_SHADOW_NODE_OWNER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_SHADOW_NODE_OWNER_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/public/layout_node_manager.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/font/font_face_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_measure_cache.h"

namespace lynx {
namespace tasm {

namespace harmony {

class LynxContext;

class LayoutVSyncProxy : public std::enable_shared_from_this<LayoutVSyncProxy> {
 public:
  LayoutVSyncProxy() {
    vsync_monitor_ = base::VSyncMonitor::Create();
    vsync_monitor_->BindToCurrentThread();
    vsync_monitor_->Init();
  }
  void ScheduleLayout(base::closure callback);
  const std::shared_ptr<base::VSyncMonitor>& VSyncMonitor() const {
    return vsync_monitor_;
  }

 private:
  std::shared_ptr<base::VSyncMonitor> vsync_monitor_;
  bool layout_scheduled_{false};
  base::closure trigger_layout_callback_;
};

class ShadowNodeOwner : public std::enable_shared_from_this<ShadowNodeOwner> {
 public:
  ShadowNodeOwner();
  ~ShadowNodeOwner();
  static napi_value Init(napi_env env, napi_value exports);

  int CreateShadowNode(int sign, const std::string& tag,
                       PropBundleHarmony* props, bool allow_inline);

  void InsertLayoutNode(int parent, int child, int index);
  void RemoveLayoutNode(int parent, int child, int index);
  void MoveLayoutNode(int parent, int child, int from_index, int to_index);
  void UpdateLayoutNode(int id, PropBundleHarmony* painting_data);
  void OnLayoutBefore(int id);
  LayoutResult MeasureNode(int id, float width, int32_t width_mode,
                           float height, int32_t height_mode,
                           bool final_measure);
  void OnLayout(int id, float left, float top, float width, float height);
  void ScheduleLayout(base::closure callback);
  void DestroyNode(int sign);
  void Destroy();
  fml::RefPtr<fml::RefCountedThreadSafeStorage> GetExtraBundle(int sign);
  void SetContext(const std::shared_ptr<LynxContext>& context);

  LynxContext* Context() const { return context_.get(); }

  ShadowNode* FindShadowNodeBySign(int sign) const;

  void AddFontFace(std::string font_family, FontFace font_face);
  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager);

  void UpdateRootSize(float width, float height);

  std::shared_ptr<FontFaceManager> GetFontFaceManager() const {
    return font_face_manager_;
  }

  TextMeasureCache* GetTextMeasureCache() const {
    return text_measure_cache_.get();
  }

  const fml::RefPtr<fml::TaskRunner>& GetLayoutTaskRunner() const;

  const std::shared_ptr<base::VSyncMonitor>& VSyncMonitor();
  void FindShadowNodeAndRunTask(int sign,
                                base::MoveOnlyClosure<void, ShadowNode*> task);

  void NotifySystemFontUpdated();

 private:
  ShadowNode* CreateJSShadowNode(int sign, const std::string& tag,
                                 PropBundleHarmony* props);

  void NativeCallJSTask(base::closure task, bool sync = true);
  void JSCallNativeTask(base::closure task, bool sync = true);
  void DestroyNode(const fml::RefPtr<ShadowNode>& node);

  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value FindJSShadowNodeBySign(napi_env env,
                                           napi_callback_info info);
  static napi_value MeasureLayoutNode(napi_env env, napi_callback_info info);
  static napi_value AlignLayoutNode(napi_env env, napi_callback_info info);
  static napi_value Destroy(napi_env env, napi_callback_info info);
  const std::shared_ptr<LayoutVSyncProxy>& GetLayoutVSyncProxy();
  napi_ref js_{nullptr};
  napi_ref create_{nullptr};
  napi_ref destroy_{nullptr};
  napi_env env_{nullptr};
  std::unordered_map<int, fml::RefPtr<ShadowNode>> node_holder_;
  std::unique_ptr<TextMeasureCache> text_measure_cache_{nullptr};
  std::shared_ptr<FontFaceManager> font_face_manager_{nullptr};
  std::shared_ptr<LynxContext> context_{nullptr};
  LayoutNodeManager* layout_node_manager_{nullptr};
  std::shared_ptr<LayoutVSyncProxy> vsync_proxy_;
  float root_width_{0.f};
  float root_height_{0.f};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_SHADOW_NODE_OWNER_H_
