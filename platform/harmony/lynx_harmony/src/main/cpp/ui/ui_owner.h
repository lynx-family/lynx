// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_OWNER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_OWNER_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/memory/ref_counted.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_dispatcher.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_emitter.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_observer.h"

namespace lynx {
namespace tasm {
namespace harmony {
class UIRoot;
class GestureArenaManager;

class UIOwner {
 public:
  UIOwner();
  using UICreatorFunc = UIBase* (*)(LynxContext*, int, const std::string&);
  static napi_value Init(napi_env env, napi_value exports);
  void CreateUI(int sign, const std::string& tag,
                PropBundleHarmony* painting_data, uint32_t node_index);
  UIBase* CreateJSUI(int sign, const std::string& tag);
  void InsertUI(int parent, int child, int index);
  void RemoveUI(int parent, int child, int index, bool is_move);
  void UpdateUI(int sign, PropBundleHarmony* props);
  void DestroyUI(int parent, int child, int index);
  void OnNodeReady(int sign);

  void UpdateLayout(int sign, float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height, uint32_t node_index);

  void SetContext(const std::shared_ptr<LynxContext>& context) {
    context_ = context;
    context_->SetArkUIContext(ark_ui_context_);
  }

  LynxContext* Context() const { return context_.get(); }
  void OnLayoutFinish(int32_t component_id, int64_t operation_id);
  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset,
                                           bool from_layout);
  void UpdateScrollInfo(int32_t container_id, bool smooth,
                        float estimated_offset, bool scrolling);
  void InsertListItemPaintingNode(int list_sign, int child_sign);
  void RemoveListItemPaintingNode(int list_sign, int child_sign);
  ~UIOwner();

  void UpdateExtraData(
      int sign,
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data);
  int32_t GetTagInfo(const std::string& tag) const;
  UIBase* FindUIBySign(int sign) const;
  UIBase* FindUIByIdSelector(const std::string& id_selector) const;
  UIBase* FindLynxUIByComponentId(const std::string& component_id);
  void InvokeUIMethod(int32_t id, const std::string& method,
                      PropBundleHarmony* args, int32_t callback_id);
  void InvokeUIMethod(
      const std::string& component_id, const std::string& node,
      const std::string& method, const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  void InvokeUIMethod(
      int32_t id, const std::string& method, const pub::Value& args,
      base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback);

  static constexpr int32_t kLynxRootSign = -1;
  static constexpr const char* kDefaultComponentId = "-1";

  UIRoot* Root();

  void CreateNodeContent(UIBase* ui) const;

  void AddUIToExposedMap(UIBase* ui, std::string unique_id,
                         lepus::Value extra_data, bool is_custom_event);
  void RemoveUIFromExposedMap(UIBase* ui, std::string unique_id);
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
  void OnTouchEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                    bool from_overlay = false);
  bool EventThrough();
  bool ShouldBlockNativeEvent();
  void SetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target);
  void UnsetFocusedTarget(const std::weak_ptr<EventTarget>& focused_target);
  void AttachGesturesToRoot(UIBase* root);
  void OnGestureRecognized(UIBase* ui);
  void OnGestureRecognizedWithSign(int sign);
  void SetTapSlop(const std::string& tap_slop);
  void SetHasTouchPseudo(bool has_touch_pseudo);
  void SetLongPressDuration(int32_t long_press_duration);
  void SendEvent(const LynxEvent& event) const;
  void HandleTouchEvent(const TouchEvent& touch_event) const;
  void HandleMultiTouchEvent(const TouchEvent& touch_event) const;
  void HandleCustomEvent(const CustomEvent& custom_event) const;
  void SendPseudoStatusEvent(int id, PseudoStatus pre_status,
                             PseudoStatus current_status) const;
  void OnPseudoStatusChanged(int id, PseudoStatus pre_status,
                             PseudoStatus current_status) const;
  // for gesture handler

  void SetGestureDetectorState(int64_t id, int32_t gesture_id,
                               int32_t state) const;

  void ConsumeGesture(int64_t id, int32_t gesture_id,
                      const lepus::Value& param) const;

  std::shared_ptr<GestureArenaManager> GetGestureArenaManager() const;
  void InitGestureArenaManager(LynxContext* context);

  void DispatchTouchEventToGestureArena(std::string type,
                                        std::shared_ptr<TouchEvent> touch_event,
                                        const ArkUI_UIInputEvent* event) const;

  void SetActiveUIToGestureArenaAtDownEvent(std::weak_ptr<EventTarget> target);

  void SetVelocityToGestureArena(float velocity_x, float velocity_y);

  bool CanConsumeTouchEvent(float point[2]);

  void SendGlobalEvent(lepus::Value params) const;
  void PostTaskOnUIThread(base::closure task) const;
  const fml::RefPtr<fml::TaskRunner>& GetUITaskRunner() const;
  const std::shared_ptr<base::VSyncMonitor>& VSyncMonitor();

  // for lynx fluency metrics
  void StartFluencyTrace(int sign, const std::string& scene,
                         const std::string& tag) const;
  void StopFluencyTrace(int sign) const;
  void PostDrawEndTimingFrameCallback() const;

  bool Destroyed() const { return destroyed_; }

  const std::string& Id() { return id_; }
  void AttachPageRoot(NativeNodeContent* native_node_content);
  bool HasAccessibilityExclusiveUI() {
    return !accessibility_exclusive_.empty();
  }
  void AddOrRemoveUIFromExclusiveSet(int32_t sign, bool exclusive);
  bool ContainAccessibilityExclusiveUI(int32_t sign);
  void ResetAccessibilityAttrs();

 private:
  static const std::unordered_map<std::string, UICreatorFunc> behaviors_;
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value AttachPageRoot(napi_env env, napi_callback_info info);
  static napi_value GetId(napi_env, napi_callback_info info);
  static napi_value Destroy(napi_env, napi_callback_info info);
  static napi_value RequestLayout(napi_env env, napi_callback_info info);
  static napi_value CanConsumeTouchEvent(napi_env env, napi_callback_info info);
  void DestroySubTree(UIBase* root);
  void MarkHasUIOperations(UIBase* ui);
  void MarkHasUIOperationsBottomUp(UIBase* ui);
  void RequestLayout();
  void UpdateComponentIdMap(UIBase* ui, PropBundleHarmony* painting_data);

  int GetJSNodeType(int sign, const std::string& tag) const;
  std::unordered_map<int32_t, std::shared_ptr<UIBase>> ui_holder_;
  std::unordered_set<int32_t> accessibility_exclusive_;
  std::unordered_map<std::string, int32_t> component_map_;
  std::unordered_map<int32_t, std::weak_ptr<UIBase>> layout_changed_nodes_;
  napi_env env_{nullptr};
  napi_ref js_this_{nullptr};
  napi_ref js_create_{nullptr};
  napi_ref js_create_node_content_{nullptr};
  napi_ref js_start_fluency_trace_{nullptr};
  napi_ref js_stop_fluency_trace_{nullptr};
  napi_ref post_draw_end_timing_frame_callback_{nullptr};
  napi_ref js_get_node_type_{nullptr};

  std::shared_ptr<LynxContext> context_{nullptr};
  std::unique_ptr<EventDispatcher> event_dispatcher_ =
      std::make_unique<EventDispatcher>(this);
  std::shared_ptr<EventEmitter> event_emitter_ =
      std::make_shared<EventEmitter>(this);
  std::unique_ptr<UIObserver> ui_observer_ = std::make_unique<UIObserver>(this);
  // TODO(@renzhongyue) implement the lynxContext wrapper and store the
  // ark_ui_context into the lynxContext.
  ArkUI_ContextHandle ark_ui_context_{nullptr};
  std::shared_ptr<UIBase> root_{nullptr};

  std::shared_ptr<base::VSyncMonitor> vsync_monitor_{nullptr};
  std::string id_;

  bool destroyed_ = false;
  std::shared_ptr<GestureArenaManager> gesture_arena_manager_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_OWNER_H_
