// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_BASE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_BASE_H_

#include <arkui/native_gesture.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <native_drawing/drawing_types.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/base/harmony/harmony_function_loader.h"
#include "core/base/lynx_export.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/ui_wrapper/common/harmony/platform_extra_bundle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/animation/keyframe_animator.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/animation/keyframe_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_handler_delegate.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_config.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/native_node_content.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_exposure.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/basic_shape.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/border_radius.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_offset_calculator.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/transform.h"

namespace lynx {
namespace tasm {
class PropBundleHarmony;

namespace harmony {
class LynxContext;
class GestureArenaManager;

static constexpr const char* const kFluencyScrollEvent = "scroll";

struct OverflowValue {
  bool overflow_x;
  bool overflow_y;
};

class LYNX_EXPORT UIBase : public std::enable_shared_from_this<UIBase>,
                           public EventTarget,
                           public GestureArenaMember,
                           public GestureHandlerDelegate {
 public:
  UIBase(LynxContext* context, ArkUI_NodeType type, int sign,
         const std::string& tag, bool has_customized_layout = false);
  void InitNode(ArkUI_NodeHandle node);
  ~UIBase() override;
  static bool CanDrawBehind();
  ArkUI_NodeHandle Node() const { return node_; }
  ArkUI_NodeHandle DrawNode() const { return draw_node_ ? draw_node_ : node_; }
  virtual void AttachToNodeContent(NativeNodeContent* content);
  void DetachFromNodeContent();
  const std::string& Tag() const { return tag_; }
  int Sign() const override { return sign_; }
  const std::string& IdSelector() const { return id_selector_; }
  bool IsComponent() const { return is_component_; }
  virtual void UpdateLayout(float left, float top, float width, float height,
                            const float* paddings, const float* margins,
                            const float* sticky, float max_height,
                            uint32_t node_index);
  virtual void OnLayoutUpdated() {}
  virtual void SetParent(UIBase* parent);
  UIBase* Parent() const { return parent_; };
  virtual void AddChild(UIBase* child, int index);
  virtual void RemoveChild(UIBase* child);
  virtual void UpdateExtraData(
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) {}
  virtual void OnMeasure(ArkUI_LayoutConstraint* layout_constraint) {}
  virtual void OnLayout() {}
  virtual void FinishLayoutOperation() {}
  virtual void OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node);
  virtual void OnDrawBehind(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node);
  virtual void OnOverlayDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node);
  virtual void UpdateProps(PropBundleHarmony* props);
  virtual void OnNodeEvent(ArkUI_NodeEvent* event);
  virtual void OnNodeReady();
  virtual bool NotifyParent() { return false; }

  virtual void OnDestroy() {}
  virtual LynxContext* GetContext() { return context_; };
  virtual void OnLayoutFinish(UIBase* base, int64_t operation_id) {}
  virtual void UpdateSticky(const float* sticky);
  virtual void OnListCellAppear(const std::string& item_key, UIBase* ui_list);
  virtual void OnListCellDisAppear(const std::string& item_key, UIBase* ui_list,
                                   bool is_exist);
  virtual void OnListCellPrepareForReuse(const std::string& item_key,
                                         UIBase* ui_list);
  virtual void WillRemoveFromUIParent();
  virtual void OnEnterForeground() {}
  virtual void OnEnterBackground() {}
  virtual bool NeedWindowStateChangeEvent() const { return false; }
  bool CheckStickyOnParentScroll(float scroll_left, float scroll_top);
  void RemoveFromParent();
  void RequestLayout();
  void Invalidate();
  PlatformLength ToVPFromUnitValue(const std::string& value);
  virtual void SetEvents(const std::vector<lepus::Value>& events);
  virtual void InvokeMethod(
      const std::string& method, const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  UIBase* FindViewById(const std::string& id, bool by_ref_id,
                       bool traverse_children = true);
  void SetIsComponent(bool value) { is_component_ = value; }
  std::vector<UIBase*> Children() const { return children_; }
  void SetVisibility(bool visible);
  std::weak_ptr<EventTarget> WeakTarget() override { return weak_from_this(); }

  // for gesture handler
  void SetGestureDetectorState(int gesture_id, int state) override;
  void ConsumeGesture(int gesture_id, const lepus::Value& params) override;
  bool CanConsumeGesture(float deltaX, float deltaY) override { return false; };
  std::vector<float> GestureScrollBy(float delta_x, float delta_y) override;
  std::vector<float> ScrollBy(float delta_x, float delta_y) override;
  int GestureArenaMemberId() override { return gesture_arena_member_id_; };
  bool IsAtBorder(bool isStart) override { return false; };
  const GestureMap& GetGestureDetectorMap() override {
    return gesture_detectors_;
  };
  const GestureHandlerMap& GetGestureHandlers() override;
  bool IsEnableNewGesture() { return gesture_arena_member_id_ > 0; }
  void SetGestureDetectors(const GestureMap& gestureDetectors);
  std::shared_ptr<GestureArenaManager> GetGestureArenaManager();

  virtual bool IsVerticalScrollView() { return false; };
  EventTarget* HitTest(float point[2]) override;
  bool ShouldHitTest() override;
  bool ContainsPoint(float point[2]) override;
  bool IsOnResponseChain() override { return is_on_response_chain_; };
  virtual void GetOriginRect(float origin_rect[4]);
  void GetTargetPoint(float target_point[2], float point[2], float scroll[2],
                      float target_origin_rect[4], Transform* target_transform);
  Transform* GetTransform() { return transform_.get(); }
  float TranslateZ() const;
  virtual float ViewLeft() const;
  virtual float ViewTop() const;
  float GetBorderLeftWidth() const {
    return background_drawable_ ? background_drawable_->GetBorderLeftWidth()
                                : 0.f;
  }
  float GetBorderRightWidth() const {
    return background_drawable_ ? background_drawable_->GetBorderRightWidth()
                                : 0.f;
  }
  float GetBorderTopWidth() const {
    return background_drawable_ ? background_drawable_->GetBorderTopWidth()
                                : 0.f;
  }
  float GetBorderBottomWidth() const {
    return background_drawable_ ? background_drawable_->GetBorderBottomWidth()
                                : 0.f;
  }
  uint32_t GetBorderLeftColor() const {
    return background_drawable_ ? background_drawable_->GetBorderLeftColor()
                                : 0;
  }
  uint32_t GetBorderRightColor() const {
    return background_drawable_ ? background_drawable_->GetBorderRightColor()
                                : 0;
  }
  uint32_t GetBorderTopColor() const {
    return background_drawable_ ? background_drawable_->GetBorderTopColor() : 0;
  }
  uint32_t GetBorderBottomColor() const {
    return background_drawable_ ? background_drawable_->GetBorderBottomColor()
                                : 0;
  }
  bool HasCustomizedLayout() const { return has_customized_layout_; }
  virtual bool HasJSObject() { return false; }
  NativeNodeContent* NodeContent() const { return node_content_.get(); }
  const OverflowValue& Overflow() { return overflow_; }
  float ScrollX() override { return 0; }
  int8_t GetScrollContainerDirection() override {
    return GestureConstants::DIRECTION_UNDETERMINED;
  }
  float ScrollY() override { return 0; }
  bool IsInterceptGesture() override;
  virtual float OffsetXForCalcPosition();
  virtual float OffsetYForCalcPosition();
  virtual bool IsVisible();
  virtual bool IsScrollable();
  virtual bool IsList() const { return false; }
  virtual bool IsOverlayContent() const { return is_overlay_content_; }
  void SetIsOverlayContent(bool is_overlay_content) {
    is_overlay_content_ = is_overlay_content;
  }
  void GestureRecognized();
  const std::string& ExposureID() { return exposure_id_; }
  const std::string& ExposureScene() { return exposure_scene_; }
  float ExposureScreenMarginLeft() { return exposure_screen_margin_left_; }
  float ExposureScreenMarginRight() { return exposure_screen_margin_right_; }
  float ExposureScreenMarginTop() { return exposure_screen_margin_top_; }
  float ExposureScreenMarginBottom() { return exposure_screen_margin_bottom_; }
  float ExposureUIMarginLeft() { return exposure_ui_margin_left_; }
  float ExposureUIMarginRight() { return exposure_ui_margin_right_; }
  float ExposureUIMarginTop() { return exposure_ui_margin_top_; }
  float ExposureUIMarginBottom() { return exposure_ui_margin_bottom_; }
  float ExposureArea() { return exposure_area_; }
  LynxEventPropStatus EnableExposureUIClip() {
    return enable_exposure_ui_clip_;
  }
  const lepus::Value& Dataset() { return dataset_; }
  bool HasAppearEvent() { return has_appear_event_; }
  bool HasDisappearEvent() { return has_disappear_event_; }
  std::string ExposureUIKey(const std::string& unique_id,
                            bool is_add = true) const;
  void GetExposureUIRect(float rect[4]);
  void GetExposureWindowRect(float rect[4]);
  bool IsVisibleForExposure(
      std::unordered_map<int, UIExposure::CommonAncestorUIRect>&
          common_ancestor_ui_rect_map,
      float offset_screen[2]);
  EventTarget* ParentTarget() override;
  void GetPointInTarget(float res[2], EventTarget* parent_target,
                        float point[2]) override;
  LynxPointerEventsValue PointerEvents() override;
  bool NativeInteractionEnabled() override;
  bool BlockNativeEvent(float point[2]) override;
  bool EventThrough(float point[2]) override;
  bool IgnoreFocus() override;
  ConsumeSlideDirection ConsumeSlideEvent() override;
  bool TouchPseudoPropagation() override;
  void OnPseudoStatusChanged(PseudoStatus pre_status,
                             PseudoStatus current_status) override;
  PseudoStatus GetPseudoStatus() override;
  bool HasUI() override { return true; };
  std::vector<std::string> EventSet() override { return events_; };
  void OnResponseChain() override { is_on_response_chain_ = true; };
  void OffResponseChain() override { is_on_response_chain_ = false; };
  starlight::ImageRenderingType RenderingType();
  bool SkipRedirection() const { return skip_redirection_; }

  ArkUI_NodeHandle RootNode() override {
    return is_overlay_content_ ? node_ : nullptr;
  };

  float left_{0};
  float top_{0};
  float width_{0};
  float height_{0};
  float padding_left_{0};
  float padding_top_{0};
  float padding_right_{0};
  float padding_bottom_{0};
  float margin_left_{0};
  float margin_top_{0};
  float margin_right_{0};
  float margin_bottom_{0};
  std::vector<float> sticky_value_;
  float opacity_{1.f};
  bool render_group_{false};
  void GetTransformValue(float left, float right, float top, float bottom,
                         std::vector<float>& point);
  void GetLocationOnScreen(std::pair<float, float>& point);
  virtual bool IsRoot() { return false; };
  void GetBoundingClientRect(float* res, bool to_screen = false);
  void GetBoundingClientRect(const lepus::Value& args, float result[4]);
  void ResetAccessibilityAttrs();
  virtual void OnKeyboardWillShow(float height) {}
  virtual void OnKeyboardWillHide() {}
  bool NeedClip() const {
    return !overflow_.overflow_x && !overflow_.overflow_y;
  }
  virtual bool HasContent() { return false; }
  void OnResourceLoadCallback(const lepus::Value& value);
  void SetAnimation(const lepus::Value& value);
  const lepus::Value& GetKeyframes(const std::string& name);
  float GetArkUIProperty(AnimationProperty type) const;
  void SetAnimationProperty(AnimationProperty type, float value);
  void SendAnimationEvent(const char* event, const std::string& name);

 protected:
  static void EventReceiver(ArkUI_NodeEvent* event);
  static void CustomEventReceiver(ArkUI_NodeCustomEvent* event);
  virtual void OnPropUpdate(const std::string& name, const lepus::Value& value);
  virtual void InsertNode(UIBase* child, int index);
  virtual void RemoveNode(UIBase* child);
  virtual bool DefaultOverflowValue() { return false; }
  virtual void FrameDidChanged();
  virtual void ScrollIntoView(bool smooth, const UIBase* target,
                              const std::string& block,
                              const std::string& inline_value) {}
  virtual void EnableSticky() {}
  virtual void SetImageRendering(const lepus::Value& value);
  bool NeedDrawNode();
  bool NeedDraw(ArkUI_NodeHandle node);
  void SetAccessibilityLabelDirtyFlag();
  virtual const std::string& GetAccessibilityLabel() const {
    return accessibility_label_;
  }
  void Destroy();

  ArkUI_NodeHandle node_{nullptr};
  UIBase* parent_{nullptr};
  std::vector<UIBase*> children_;
  LynxContext* context_;
  OverflowValue overflow_ = {false, false};
  std::unique_ptr<NativeNodeContent> node_content_{nullptr};
  LynxPointerEventsValue pointer_events_{LynxPointerEventsValue::kUnset};
  bool block_native_event_{false};
  ConsumeSlideDirection consume_slide_event_{ConsumeSlideDirection::kNone};
  LynxEventPropStatus ignore_focus_{LynxEventPropStatus::kUndefined};
  LynxEventPropStatus event_through_{LynxEventPropStatus::kUndefined};
  std::vector<std::vector<PlatformLength>> event_through_active_regions_;
  std::vector<std::vector<PlatformLength>> block_native_event_areas_;
  /** Used to control whether the viewport clipping of this node is considered
   * during exposure detection.  */
  LynxEventPropStatus enable_exposure_ui_clip_{LynxEventPropStatus::kUndefined};

  std::unique_ptr<BackgroundDrawable> background_drawable_{nullptr};
  std::unique_ptr<BackgroundDrawable> mask_drawable_{nullptr};
  std::vector<std::string> events_;
  ArkUI_NodeType node_type_;
  starlight::ImageRenderingType rendering_type_{
      starlight::ImageRenderingType::kAuto};
  int gesture_arena_member_id_{0};
  bool consume_gesture_{true};
  bool block_list_event_{false};
  bool skip_redirection_{false};
  LynxInterceptGestureStatus gesture_status_{
      LynxInterceptGestureStatus::LynxInterceptGestureStateUnset};
  enum class LynxAccessibilityMode {
    kAuto,
    kEnable,
    kDisable,
    kDisableForDescendants
  };
  void InitAccessibilityAttrs(LynxAccessibilityMode mode,
                              const std::string& traits);
  // Returns whether this view has content which overlaps.
  virtual bool HasOverlappingRendering() { return true; }
  bool HasBackground() const {
    return background_drawable_ || has_background_color_;
  }

 private:
  void SetIdSelector(const lepus::Value& value);
  void SetReactRef(const lepus::Value& value);
  void SetBackgroundColor(const lepus::Value& value);
  void SetOpacity(const lepus::Value& value);
  void SetGroup(const lepus::Value& value);
  void SetVisibility(const lepus::Value& value);
  void CreateOrUpdateBackground();
  void CreateOrUpdateMask();
  void SetTransform(const lepus::Value& value);
  void SetTransformOrigin(const lepus::Value& value);
  void ApplyTransform();
  void SetBorderRadius(const lepus::Value& value);
  void SetBorderTopLeftRadius(const lepus::Value& value);
  void SetBorderTopRightRadius(const lepus::Value& value);
  void SetBorderBottomRightRadius(const lepus::Value& value);
  void SetBorderBottomLeftRadius(const lepus::Value& value);
  bool ComputeOverflowValue(const lepus::Value& value);
  void SetBorderStyle(const lepus::Value& value);
  void SetBorderLeftStyle(const lepus::Value& value);
  void SetBorderRightStyle(const lepus::Value& value);
  void SetBorderTopStyle(const lepus::Value& value);
  void SetBorderBottomStyle(const lepus::Value& value);
  void SetBorderColor(const lepus::Value& value);
  void SetBorderLeftColor(const lepus::Value& value);
  void SetBorderRightColor(const lepus::Value& value);
  void SetBorderTopColor(const lepus::Value& value);
  void SetBorderWidth(const lepus::Value& value);
  void SetBorderBottomColor(const lepus::Value& value);
  void SetBorderLeftWidth(const lepus::Value& value);
  void SetBorderRightWidth(const lepus::Value& value);
  void SetBorderTopWidth(const lepus::Value& value);
  void SetBorderBottomWidth(const lepus::Value& value);
  void SetBackgroundClip(const lepus::Value& value);
  void SetBackgroundOrigin(const lepus::Value& value);
  void SetBackgroundImage(const lepus::Value& value);
  void SetBackgroundSize(const lepus::Value& value);
  void SetBackgroundPosition(const lepus::Value& value);
  void SetBackgroundRepeat(const lepus::Value& value);
  void SetMaskClip(const lepus::Value& value);
  void SetMaskOrigin(const lepus::Value& value);
  void SetMaskImage(const lepus::Value& value);
  void SetMaskSize(const lepus::Value& value);
  void SetMaskPosition(const lepus::Value& value);
  void SetMaskRepeat(const lepus::Value& value);
  void SetOverflow(const lepus::Value& value);
  void SetOverflowX(const lepus::Value& value);
  void SetOverflowY(const lepus::Value& value);
  void SetBoxShadow(const lepus::Value& value);
  void SetFilter(const lepus::Value& value);
  void ApplyOverflowClip();
  void InitDrawNode();
  void UpdateDrawNodeFrame();
  void DestroyDrawNode();
  UIBase* GetRelativeUI(const std::string& relativeId);
  void GetBoundingClientRect(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void ScrollIntoView(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void TakeScreenshot(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  void SetPointerEvents(const lepus::Value& value);
  void SetUserInteractionEnabled(const lepus::Value& value);
  void SetNativeInteractionEnabled(const lepus::Value& value);
  void SetHitSlop(const lepus::Value& value);
  void SetIgnoreFocus(const lepus::Value& value);
  void SetEventThrough(const lepus::Value& value);
  void SetEventThroughActiveRegions(const lepus::Value& value);
  void SetBlockNativeEvent(const lepus::Value& value);
  void SetBlockNativeEventAreas(const lepus::Value& value);
  void SetConsumeSlideEvent(const lepus::Value& value);
  void SetEnableTouchPseudoPropagation(const lepus::Value& value);
  void TransformFromViewToRootView(UIBase* ui, std::pair<float, float>& point);
  void MapPointWithTransform(std::pair<float, float>& point);
  void SetExposureID(const lepus::Value& value);
  void SetExposureScene(const lepus::Value& value);
  void SetExposureScreenMarginLeft(const lepus::Value& value);
  void SetExposureScreenMarginRight(const lepus::Value& value);
  void SetExposureScreenMarginTop(const lepus::Value& value);
  void SetExposureScreenMarginBottom(const lepus::Value& value);
  void SetExposureUIMarginLeft(const lepus::Value& value);
  void SetExposureUIMarginRight(const lepus::Value& value);
  void SetExposureUIMarginTop(const lepus::Value& value);
  void SetExposureUIMarginBottom(const lepus::Value& value);
  void SetExposureArea(const lepus::Value& value);
  void SetEnableExposureUIClip(const lepus::Value& value);
  void SetDataset(const lepus::Value& value);
  void SetClipPath(const lepus::Value& value);
  void SetOffsetPath(const lepus::Value& value);
  void SetOffsetDistance(const lepus::Value& value);
  void SetOffsetRotate(const lepus::Value& value);
  void UpdateOffsetPathCacheIfNeeded();
  std::optional<transforms::Matrix44> GetOffsetMatrix() const;
  void SetPerspective(const lepus::Value& value);
  void SetBlockListEvent(const lepus::Value& value);
  void SetSkipRedirection(const lepus::Value& value);
  float GetPerspectiveValue();
  void SetAccessibilityElement(const lepus::Value& value);
  void SetAccessibilityLabel(const lepus::Value& value);
  void SetAccessibilityTraits(const lepus::Value& value);
  void SetAccessibilityElementsHidden(const lepus::Value& value);
  void SetAccessibilityExclusiveFocus(const lepus::Value& value);
  void SetAccessibilityId(const lepus::Value& value);
  void SetCrossLanguageOption(const lepus::Value& value);
  void OnNodeReadyForAccessibility();
  void SendLayoutChangeEvent();
  bool IsImportantForAccessibility();
  ArkUI_NodeType GetAccessibilityType(const std::string& traits);
  void Base64EncodeTask(
      OH_PixelmapNative* pixel_map, const std::string& format,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void ApplyOverflowClipPath(float width, float height,
                             const std::string& path);
  void ApplyOverflowClipRectangle(float width, float height, float left,
                                  float top);
  void ParseRegionArray(const lepus::Value& value,
                        std::vector<std::vector<PlatformLength>>& regions);

  int sign_;
  using PropSetter = void (UIBase::*)(const lepus::Value& value);
  static std::unordered_map<std::string, PropSetter> prop_setters_;
  using UIMethod = void (UIBase::*)(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  static std::unordered_map<std::string, UIMethod> ui_method_map_;
  std::unique_ptr<BorderRadius> border_radius_;
  std::unique_ptr<Transform> transform_;

  std::string tag_;
  uint32_t dirty_flags_{0};
  std::string id_selector_;
  std::string react_ref_id_;
  bool is_component_{false};
  ArkUI_NodeHandle draw_node_{nullptr};
  uint32_t background_color_{};
  bool has_background_color_{false};

  bool user_interaction_enabled_{true};
  bool is_overlay_content_{false};
  bool native_interaction_enabled_{true};
  float hit_slop_left_{0};
  float hit_slop_right_{0};
  float hit_slop_top_{0};
  float hit_slop_bottom_{0};
  bool enable_touch_pseudo_propagation_{true};
  PseudoStatus pseudo_status_{PseudoStatus::kNone};
  bool is_on_response_chain_{false};
  const bool has_customized_layout_;
  std::string exposure_id_;
  std::string exposure_scene_;
  float exposure_screen_margin_left_{0};
  float exposure_screen_margin_right_{0};
  float exposure_screen_margin_top_{0};
  float exposure_screen_margin_bottom_{0};
  float exposure_ui_margin_left_{0};
  float exposure_ui_margin_right_{0};
  float exposure_ui_margin_top_{0};
  float exposure_ui_margin_bottom_{0};
  float exposure_area_{0};
  lepus::Value perspective_{};
  // TODO(hexionghui): use optional to optimize
  std::unordered_map<std::string, lepus::Value> old_exposure_props_;
  std::unordered_map<std::string, lepus::Value> new_exposure_props_;
  bool has_appear_event_{false};
  bool has_disappear_event_{false};
  std::optional<TransformOrigin> transform_origin_ = std::nullopt;
  bool has_layout_change_event_{false};
  float translation_z_{.0f};
  lepus::Value dataset_{lepus::Value(lepus::Dictionary::Create())};
  // for gesture handler
  GestureMap gesture_detectors_;
  GestureHandlerMap gesture_handlers_;
  std::unique_ptr<BasicShape> basic_shape_{nullptr};
  static constexpr float kOffsetRotateAuto = -1024.0f;
  std::unique_ptr<BasicShape> offset_basic_shape_{nullptr};
  std::unique_ptr<LynxOffsetCalculator> lynx_offset_calculator_{nullptr};
  float offset_distance_{0.f};
  float offset_rotate_{kOffsetRotateAuto};
  bool is_auto_offset_rotate_{true};
  std::string accessibility_id_;
  std::string accessibility_label_;
  LynxAccessibilityMode accessibility_mode_{LynxAccessibilityMode::kDisable};
  std::string accessibility_traits_{"none"};
  enum class LynxAccessibilityStatus {
    kDefault,
    kFocus,
    kDisable,
  };
  LynxAccessibilityStatus accessibility_status_{
      LynxAccessibilityStatus::kDefault};

  struct EncodeAsyncContexts {
    napi_async_work async_work = nullptr;
    OH_PixelmapNative* pixel_map = nullptr;
    std::string format;
    uint32_t width;
    uint32_t height;
    bool res{false};
    std::string data;
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback;
    EncodeAsyncContexts(
        OH_PixelmapNative* pixel_map, const std::string& format,
        base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback)
        : pixel_map(pixel_map), format(format), callback(std::move(callback)) {}
  };
  std::unique_ptr<KeyframeManager> keyframe_manager_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_BASE_H_
