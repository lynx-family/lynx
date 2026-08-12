// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_

#include <arkui/native_node.h>
#include <multimedia/image_framework/image/image_source_native.h>

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/task_runner.h"
#include "core/base/lynx_export.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/public/event_tracker_proxy.h"
#include "core/public/list_engine_proxy.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/page_options.h"
#include "core/public/perf_controller_proxy.h"
#include "core/public/ui_delegate.h"
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
class UIOwner;
class UIBase;
class UIRoot;
class GestureArenaManager;

class LynxContext {
 public:
  using ConsoleMessageCallback =
      std::function<void(const std::string&, int32_t, int64_t)>;
  using CDPResultCallback = std::function<void(const std::string&)>;
  using InvokeCDPFromSDKCallback =
      std::function<void(const std::string&, CDPResultCallback)>;

  LynxContext(ShadowNodeOwner* node_owner, UIOwner* ui_owner, napi_env env)
      : node_owner_{node_owner}, ui_owner_{ui_owner}, env_(env) {}

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

  void TakeScreenShot(size_t max_width, size_t max_height, int quality,
                      const fml::RefPtr<fml::TaskRunner>& screenshot_runner,
                      TakeSnapshotCompletedCallback callback,
                      const std::string& format = "jpeg");

  void ResetUIOwner() { ui_owner_ = nullptr; }

  UIOwner* GetUIOwner() const { return ui_owner_; }

  const std::shared_ptr<base::VSyncMonitor> VSyncMonitor();

  const std::shared_ptr<shell::ListEngineProxy>& GetListEngineProxy() {
    return list_engine_proxy_;
  }

  const std::shared_ptr<shell::LynxEngineProxy>& GetEngineProxy() {
    return engine_proxy_;
  }

  void ResetNodeOwner() {
    std::unique_lock<std::shared_mutex> guard(node_owner_shared_mutex_);
    node_owner_ = nullptr;
  }

  void SetArkUIContext(ArkUI_ContextHandle ark_ui_context) {
    ark_ui_context_ = ark_ui_context;
  }

  ArkUI_ContextHandle ArkUIContext() const { return ark_ui_context_; }

  void SetEmbeddedMode(EmbeddedMode embedded_mode) {
    embedded_mode_ = embedded_mode;
  }

  bool IsLayoutInElementModeOn() const {
    return (embedded_mode_ & (EmbeddedMode::LAYOUT_IN_ELEMENT |
                              EmbeddedMode::FRAGMENT_LAYER_RENDER)) > 0;
  }

  bool IsFragmentLayerRenderOn() const {
    return (embedded_mode_ & EmbeddedMode::FRAGMENT_LAYER_RENDER) > 0;
  }

  fml::RefPtr<fml::RefCountedThreadSafeStorage> GetTextLayout(
      int32_t sign) const;

  void SetWindowInfo(int32_t window_id, int32_t window_left_px,
                     int32_t window_top_px) {
    window_id_ = window_id;
    window_left_px_ = window_left_px;
    window_top_px_ = window_top_px;
  }

  bool HasWindowInfo() const { return window_id_ >= 0; }
  int32_t WindowId() const { return window_id_; }
  int32_t WindowLeftPx() const { return window_left_px_; }
  int32_t WindowTopPx() const { return window_top_px_; }

  void SetEnableHarmonyNewOverlay(bool enable_new_overlay);

  bool GetEnableHarmonyNewOverlay() { return enable_harmony_new_overlay_; }

  void SetEnableMultiTouch(bool enable_multi_touch);

  void SetTapSlop(const std::string& tap_slop);

  void SetHasTouchPseudo(bool has_touch_pseudo);

  void SetLongPressDuration(int32_t long_press_duration);

  LYNX_EXPORT void SendEvent(const LynxEvent& event) const;

  bool StartEventGenerate(const TouchEvent& touch_event) const;

  void StartEventCapture(int64_t event_id) const;

  void StartEventBubble(int64_t event_id) const;

  void StartEventFire(bool is_stop, int64_t event_id) const;

  void HandleTouchEvent(const TouchEvent& touch_event) const;

  void HandleMultiTouchEvent(const TouchEvent& touch_event) const;

  void HandleCustomEvent(const CustomEvent& custom_event) const;

  void OnPseudoStatusChanged(int id, PseudoStatus pre_status,
                             PseudoStatus current_status) const;

  void HandleGestureEvent(const GestureEvent& custom_event) const;

  void SendGlobalEvent(lepus::Value params) const;

  void SetConsoleMessageCallback(ConsoleMessageCallback callback);

  void ShowMessageOnConsole(const std::string& message, int32_t level) const;

  void SetInvokeCDPFromSDKCallback(InvokeCDPFromSDKCallback callback);

  void InvokeCDPFromSDK(const std::string& cdp_msg,
                        CDPResultCallback callback) const;

  void SetEnableEventThrough(bool enable_event_through);

  bool EnableEventThrough();

  void SetEnableHarmonyVisibleAreaChangeForExposure(
      bool enable_harmony_visible_area_change_for_exposure);

  bool EnableHarmonyVisibleAreaChangeForExposure();

  void SetEnableExposureWhenReload(bool enable_exposure_when_reload);

  bool EnableExposureWhenReload();

  void SetEnableTransformedTouchPosition(
      bool enable_transformed_touch_position);

  BASE_EXPORT bool EnableTransformedTouchPosition();

  void CallJSApiCallbackWithValue(int32_t callback_id,
                                  const lepus::Value& params) const;

  void CallJSFunction(const std::string& module_id,
                      const std::string& method_id,
                      lepus::Value&& params) const;

  void CallJSIntersectionObserver(int32_t observer_id, int32_t callback_id,
                                  lepus::Value params) const;

  LYNX_EXPORT float ScaledDensity() const;

  const std::shared_ptr<pub::LynxResourceLoader>& GetResourceLoader() const {
    return resource_loader_;
  }

  UIBase* FindUIBySign(int sign) const;

  LYNX_EXPORT UIBase* FindUIByIdSelector(const std::string& id_selector) const;

  LYNX_EXPORT napi_env GetNapiEnv() const;

  void InvokeUIMethod(
      const std::string& component_id, const std::string& node,
      const std::string& method, const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  ShadowNode* FindShadowNodeBySign(int sign) const;
  LYNX_EXPORT void FindShadowNodeAndRunTask(
      int sign, base::MoveOnlyClosure<void, ShadowNode*>) const;

  void ScrollByListContainer(int32_t tag, float x, float y, float original_x,
                             float original_y);

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth);
  void ScrollStopped(int32_t tag);

  void AddUIToExposedMap(UIBase* ui, std::string unique_id = "",
                         lepus::Value extra_data = lepus::Value(),
                         bool is_custom_event = false);

  void RemoveUIFromExposedMap(UIBase* ui, std::string unique_id = "");
  void RefreshUIInExposedMap(UIBase* ui, std::string unique_id = "");

  void TriggerExposureCheck();

  void StopExposure(const lepus::Value& options);

  void ResumeExposure();

  void OnRootAttachedToViewTree();

  void OnRootDetachedFromViewTree();

  void SetObserverFrameRate(const lepus::Value& options);

  void CreateUIIntersectionObserver(int intersection_observer_id,
                                    const std::string& js_component_id,
                                    const lepus::Value& options);

  UIIntersectionObserver* GetUIIntersectionObserver(
      int intersection_observer_id);

  void NotifyUIScroll();

  LYNX_EXPORT void ScreenSize(float size[2]) const;

  float DevicePixelRatio() const { return ScaledDensity(); }

  float DefaultFontSize() const {
    // rendering with the platform layer, the layouts_unit_per_px is set to 1.
    const float layouts_unit_per_px = 1.f;
    return layouts_unit_per_px * DEFAULT_FONT_SIZE_DP;
  }

  void OnTouchEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                    bool from_overlay = false);

  void OnEventCapture(long target_id, bool is_catch, int64_t event_id);

  void OnEventBubble(long target_id, bool is_catch, int64_t event_id);

  void OnEventFire(long target_id, bool is_stop, int64_t event_id);

  void SetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target);

  void UnsetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target);

  // for lynx fluency metrics
  void StartFluencyTrace(int sign, const std::string& scene,
                         const std::string& tag);
  void StopFluencyTrace(int sign);

  LYNX_EXPORT UIRoot* Root();

  const std::string& OwnerId();

  std::shared_ptr<FontFaceManager> GetFontFaceManager() const;

  void CreateNodeContent(UIBase* ui) const;

  void OnLynxCreate(
      const std::shared_ptr<shell::ListEngineProxy>& list_engine_proxy,
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
      const std::shared_ptr<shell::EventTrackerProxy>& event_tracker_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner,
      bool is_embedded_mode = false);
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
  LYNX_EXPORT void RunOnUIThread(base::closure task) const;

  LYNX_EXPORT void RunOnTASMThread(base::closure task) const;

  // The task will run immediately if on layout thread, or it will post to
  // layout task runner.
  LYNX_EXPORT void RunOnLayoutThread(base::closure task) const;

  const fml::RefPtr<fml::TaskRunner>& GetUITaskRunner() const {
    return ui_task_runner_;
  }

  const fml::RefPtr<fml::TaskRunner>& GetLayoutTaskRunner() const {
    return layout_task_runner_;
  }

  bool EventThrough();
  bool ShouldBlockNativeEvent();
  void AttachGesturesToRoot(UIBase* root);
  void AttachGesturesToOverlayRoot(UIBase* root, int32_t level);
  void DetachGesturesFromRoot(UIBase* root);
  void OnGestureRecognized(UIBase* ui);
  void OnGestureRecognizedWithSign(int sign);
  void UpdateNativeInteractionEnabledForTree(UIBase* root);
  void SetKeyframes(const lepus::Value& value);
  const lepus::Value& GetKeyframes(const std::string& name);

  using UICreatorFunc = UIBase* (*)(LynxContext*, int, const std::string&);
  using LayoutNodeCreatorFuc = ShadowNode* (*)(int, const std::string&);
  struct NodeInfo {
    UICreatorFunc ui_creator{nullptr};
    LayoutNodeCreatorFuc layout_node_creator{nullptr};
    int node_type{LayoutNodeType::COMMON};
  };

  const LynxContext::NodeInfo* GetNodeInfo(const std::string& node_name);
  void RegisterNodeInfo(const std::string& node_name, NodeInfo node_info);

  LYNX_EXPORT int32_t GetInstanceId() const;
  fluency::harmony::FluencyTraceHelperHarmony& GetFluencyTraceHelper();

  void SetEnableTextOverflow(bool enable) { enable_text_overflow_ = enable; }
  bool IsEnableTextOverflow() const { return enable_text_overflow_; }
  void SetEnableNewSticky(bool enable) { enable_new_sticky_ = enable; }
  bool GetEnableNewSticky() const { return enable_new_sticky_; }
  LYNX_EXPORT static std::unordered_map<std::string, NodeInfo>&
  GetCAPINodeInfoMap();

  void SetExtensionDelegate(pub::LynxExtensionDelegate* extension_delegate) {
    extension_delegate_ = extension_delegate;
  }

  LYNX_EXPORT pub::LynxExtensionDelegate* GetExtensionDelegate() const {
    return extension_delegate_;
  }

 private:
  ShadowNodeOwner* node_owner_;
  shell::EmbedderPlatformHarmony* embedder_{nullptr};
  UIOwner* ui_owner_;
  fluency::harmony::FluencyTraceHelperHarmony fluency_trace_helper_;
  ArkUI_ContextHandle ark_ui_context_{nullptr};
  int32_t window_id_{-1};
  int32_t window_left_px_{0};
  int32_t window_top_px_{0};
  std::unordered_map<std::string, NodeInfo> dynamic_node_info_map_;
  bool enable_text_overflow_{false};
  bool enable_new_sticky_{false};
  bool enable_harmony_new_overlay_{false};
  EmbeddedMode embedded_mode_{EmbeddedMode::UNSET};

  std::shared_ptr<shell::ListEngineProxy> list_engine_proxy_{nullptr};
  std::shared_ptr<shell::LynxEngineProxy> engine_proxy_{nullptr};
  std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy_{nullptr};
  std::shared_ptr<shell::PerfControllerProxy> perf_controller_proxy_{nullptr};
  std::shared_ptr<shell::EventTrackerProxy> event_tracker_proxy_{nullptr};
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_{nullptr};
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
  fml::RefPtr<fml::TaskRunner> layout_task_runner_;

  mutable std::shared_mutex node_owner_shared_mutex_;
  mutable std::shared_mutex embedder_shared_mutex_;
  mutable std::mutex console_message_callback_mutex_;
  mutable std::mutex invoke_cdp_from_sdk_callback_mutex_;

  std::shared_ptr<PubLynxContextDelegate> delegate_{nullptr};
  ConsoleMessageCallback console_message_callback_;
  InvokeCDPFromSDKCallback invoke_cdp_from_sdk_callback_;
  pub::LynxExtensionDelegate* extension_delegate_{nullptr};
  napi_env env_;

  std::optional<lepus::Value> keyframes_;
  bool enable_event_through_{false};
  bool enable_harmony_visible_area_change_for_exposure_{false};
  bool enable_exposure_when_reload_{false};
  bool enable_transformed_touch_position_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_
