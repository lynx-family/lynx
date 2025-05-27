// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_

#include <arkui/native_node.h>
#include <multimedia/image_framework/image/image_source_native.h>

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/task_runner.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/perf_controller_proxy.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/services/fluency/harmony/fluency_trace_helper_harmony.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/font/font_face_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_intersection_observer.h"

namespace lynx {
namespace shell {
class EmbedderPlatformHarmony;
}

namespace tasm {
namespace harmony {
class ShadowNode;
class LynxEvent;
class TouchEvent;
class CustomEvent;
class GestureEvent;
class EventTarget;
class ShadowNodeOwner;
class TextMeasureCache;
class UIOwner;
class UIBase;
class UIRoot;
class GestureArenaManager;

class LynxContext {
 public:
  LynxContext(ShadowNodeOwner* node_owner, UIOwner* ui_owner)
      : node_owner_{node_owner}, ui_owner_{ui_owner} {}

  void SetContextDelegate(
      const std::shared_ptr<PubLynxContextDelegate>& delegate) {
    delegate_ = delegate;
  }

  ~LynxContext();

  // TODO(chenyouhui): Remove embedder after UIRendererManager is added.
  void SetEmbedder(shell::EmbedderPlatformHarmony* embedder) {
    std::unique_lock<std::shared_mutex> guard(embedder_shared_mutex_);
    embedder_ = embedder;
  }

  void ResetEmbedder() {
    std::unique_lock<std::shared_mutex> guard(embedder_shared_mutex_);
    embedder_ = nullptr;
  }

  void ResetUIOwner() { ui_owner_ = nullptr; }

  UIOwner* GetUIOwner() const { return ui_owner_; }

  const std::shared_ptr<base::VSyncMonitor> VSyncMonitor();

  void ResetNodeOwner() {
    std::unique_lock<std::shared_mutex> guard(node_owner_shared_mutex_);
    node_owner_ = nullptr;
  }

  void SetArkUIContext(ArkUI_ContextHandle ark_ui_context) {
    ark_ui_context_ = ark_ui_context;
  }

  ArkUI_ContextHandle ArkUIContext() const { return ark_ui_context_; }

  void SetTapSlop(const std::string& tap_slop);

  void SetHasTouchPseudo(bool has_touch_pseudo);

  void SetLongPressDuration(int32_t long_press_duration);

  void SendEvent(const LynxEvent& event) const;

  void HandleTouchEvent(const TouchEvent& touch_event) const;

  void HandleMultiTouchEvent(const TouchEvent& touch_event) const;

  void HandleCustomEvent(const CustomEvent& custom_event) const;

  void OnPseudoStatusChanged(int id, PseudoStatus pre_status,
                             PseudoStatus current_status) const;

  void HandleGestureEvent(const GestureEvent& custom_event) const;

  void SendGlobalEvent(lepus::Value params) const;

  void CallJSApiCallbackWithValue(int32_t callback_id,
                                  const lepus::Value& params) const;

  void CallJSFunction(const std::string& module_id,
                      const std::string& method_id,
                      lepus::Value&& params) const;

  void CallJSIntersectionObserver(int32_t observer_id, int32_t callback_id,
                                  lepus::Value params) const;

  BASE_EXPORT float ScaledDensity() const;

  const std::shared_ptr<pub::LynxResourceLoader>& GetResourceLoader() const {
    return resource_loader_;
  }

  UIBase* FindUIBySign(int sign) const;

  UIBase* FindUIByIdSelector(const std::string& id_selector) const;

  napi_env GetNapiEnv() const;

  void InvokeUIMethod(
      const std::string& component_id, const std::string& node,
      const std::string& method, const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  ShadowNode* FindShadowNodeBySign(int sign) const;
  void FindShadowNodeAndRunTask(int sign,
                                base::MoveOnlyClosure<void, ShadowNode*>) const;

  void ScrollByListContainer(int32_t tag, float x, float y, float original_x,
                             float original_y);

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth);
  void ScrollStopped(int32_t tag);

  void AddUIToExposedMap(UIBase* ui, std::string unique_id = "",
                         lepus::Value extra_data = lepus::Value(),
                         bool is_custom_event = false);

  void RemoveUIFromExposedMap(UIBase* ui, std::string unique_id = "");

  void TriggerExposureCheck();

  void StopExposure(const lepus::Value& options);

  void ResumeExposure();

  void SetObserverFrameRate(const lepus::Value& options);

  void CreateUIIntersectionObserver(int intersection_observer_id,
                                    const std::string& js_component_id,
                                    const lepus::Value& options);

  UIIntersectionObserver* GetUIIntersectionObserver(
      int intersection_observer_id);

  void NotifyUIScroll();

  void ScreenSize(float size[2]) const;

  float DevicePixelRatio() const { return ScaledDensity(); }

  float DefaultFontSize() const {
    // rendering with the platform layer, the layouts_unit_per_px is set to 1.
    const float layouts_unit_per_px = 1.f;
    return layouts_unit_per_px * DEFAULT_FONT_SIZE_DP;
  }

  void OnTouchEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                    bool from_overlay = false);

  void SetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target);

  void UnsetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target);

  // for lynx fluency metrics
  void StartFluencyTrace(int sign, const std::string& scene,
                         const std::string& tag);
  void StopFluencyTrace(int sign);

  BASE_EXPORT UIRoot* Root();

  const std::string& OwnerId();

  std::shared_ptr<FontFaceManager> GetFontFaceManager() const;

  TextMeasureCache* GetTextMeasureCache() const;

  void CreateNodeContent(UIBase* ui) const;

  void OnLynxCreate(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner);
  void PostTaskOnUIThread(base::closure task) const;

  // The task will run immediately if on main thread, or it will post to ui task
  // runner, and current thread will be hanged until the task finishes on ui
  // thread.
  // XXX(renzhongyue): we do not provide a method to post sync task on layout
  // thread. We do not want ui thread hanged by layout thread and it can provide
  // potential dead lock while the ui thread and layout thread are waiting each
  // other.
  void PostSyncTaskOnUIThread(base::closure task) const;

  // The task will run immediately if on main thread, or it will post to ui task
  // runner.
  void RunOnUIThread(base::closure task) const;

  // The task will run immediately if on layout thread, or it will post to
  // layout task runner.
  void RunOnLayoutThread(base::closure task) const;

  const fml::RefPtr<fml::TaskRunner>& GetUITaskRunner() const {
    return ui_task_runner_;
  }

  const fml::RefPtr<fml::TaskRunner>& GetLayoutTaskRunner() const {
    return layout_task_runner_;
  }

  bool EventThrough();
  bool ShouldBlockNativeEvent();
  void AttachGesturesToRoot(UIBase* root);
  void OnGestureRecognized(UIBase* ui);
  void OnGestureRecognizedWithSign(int sign);

  using UICreatorFunc = UIBase* (*)(LynxContext*, int, const std::string&);
  using LayoutNodeCreatorFuc = ShadowNode* (*)(int, const std::string&);
  struct NodeInfo {
    UICreatorFunc ui_creator{nullptr};
    LayoutNodeCreatorFuc layout_node_creator{nullptr};
    int node_type{LayoutNodeType::COMMON};
  };

  const LynxContext::NodeInfo* GetNodeInfo(const std::string& node_name);
  void RegisterNodeInfo(const std::string& node_name, NodeInfo node_info);

  int32_t GetInstanceId() const;
  fluency::harmony::FluencyTraceHelperHarmony& GetFluencyTraceHelper();

  void SetEnableTextOverflow(bool enable) { enable_text_overflow_ = enable; }
  bool IsEnableTextOverflow() const { return enable_text_overflow_; }
  static std::unordered_map<std::string, NodeInfo>& GetCAPINodeInfoMap();

  void SetExtensionDelegate(pub::LynxExtensionDelegate* extension_delegate) {
    extension_delegate_ = extension_delegate;
  }

  BASE_EXPORT pub::LynxExtensionDelegate* GetExtensionDelegate() const {
    return extension_delegate_;
  }

 private:
  ShadowNodeOwner* node_owner_;
  shell::EmbedderPlatformHarmony* embedder_{nullptr};
  UIOwner* ui_owner_;
  fluency::harmony::FluencyTraceHelperHarmony fluency_trace_helper_;
  ArkUI_ContextHandle ark_ui_context_{nullptr};
  std::unordered_map<std::string, NodeInfo> dynamic_node_info_map_;
  bool enable_text_overflow_{false};

  std::shared_ptr<shell::LynxEngineProxy> engine_proxy_{nullptr};
  std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy_{nullptr};
  std::shared_ptr<shell::PerfControllerProxy> perf_controller_proxy_{nullptr};
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_{nullptr};
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
  fml::RefPtr<fml::TaskRunner> layout_task_runner_;

  mutable std::shared_mutex node_owner_shared_mutex_;
  mutable std::shared_mutex embedder_shared_mutex_;

  std::shared_ptr<PubLynxContextDelegate> delegate_{nullptr};
  pub::LynxExtensionDelegate* extension_delegate_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_
