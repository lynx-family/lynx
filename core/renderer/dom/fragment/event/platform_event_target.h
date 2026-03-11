// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/event/platform_event_target_exposure.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

class PlatformEventTargetHelper;

enum class LynxEventPropStatus {
  kUndefined,
  kDisable,
  kEnable,
};

enum class LynxPointerEventsValue {
  kAuto,
  kNone,
  // add new type before kUnset
  kUnset,
};

enum class LynxConsumeSlideDirection {
  kNone,
  kHorizontal,
  kVertical,
  kUp,
  kRight,
  kDown,
  kLeft,
  kAll,
};

enum class LynxPseudoStatus {
  kNone = 0,
  kHover = 1,
  kHoverTransition = 1 << 1,
  kActive = 1 << 3,
  kActiveTransition = 1 << 4,
  kFocus = 1 << 6,
  kFocusTransition = 1 << 7,
  kAll = ~0,
};

class PlatformEventTarget : public fml::RefCountedThreadSafeStorage {
  using ChildrenTargetVec =
      base::InlineVector<fml::RefPtr<PlatformEventTarget>, 4>;

 public:
  PlatformEventTarget(PlatformEventTargetHelper* target_helper, int32_t sign,
                      float left, float top, float width, float height)
      : sign_(sign),
        left_(left),
        top_(top),
        width_(width),
        height_(height),
        target_helper_(target_helper) {}
  void ReleaseSelf() const override { delete this; }
  // because the target may be reconstructed, we need to check if the current
  // parent is the target with sign.
  bool operator==(const PlatformEventTarget& other) const {
    return sign_ == other.sign_;
  }
  bool operator!=(const PlatformEventTarget& other) const {
    return !(*this == other);
  }

  int32_t Sign() const { return sign_; }
  float Left() const { return left_; }
  float Top() const { return top_; }
  float Width() const { return width_; }
  float Height() const { return height_; }
  bool IsScrollContainer() const { return is_scroll_container_; }
  float ScrollOffsetX() const { return scroll_offset_x_; }
  float ScrollOffsetY() const { return scroll_offset_y_; }
  float OffsetXForCalcPosition() const { return offset_x_for_calc_position_; }
  float OffsetYForCalcPosition() const { return offset_y_for_calc_position_; }
  bool IsVisible() const { return true; }
  bool IsOverlayContent() const { return false; }
  LynxEventPropStatus EnableExposureUIClip() const {
    return enable_exposure_ui_clip_;
  }
  bool IsScrollable() const { return is_scroll_container_; }
  bool IsRoot() const { return sign_ == kRootId; }
  const base::Vector<PlatformEventName>& EventSet() const { return event_set_; }
  bool UserInteractionEnabled() const { return user_interaction_enabled_; }
  bool NativeInteractionEnabled() const { return native_interaction_enabled_; }
  float ExposureScreenMarginLeft() const {
    return exposure_screen_margin_left_;
  }
  float ExposureScreenMarginRight() const {
    return exposure_screen_margin_right_;
  }
  float ExposureScreenMarginTop() const { return exposure_screen_margin_top_; }
  float ExposureScreenMarginBottom() const {
    return exposure_screen_margin_bottom_;
  }
  float ExposureUIMarginLeft() const { return exposure_ui_margin_left_; }
  float ExposureUIMarginRight() const { return exposure_ui_margin_right_; }
  float ExposureUIMarginTop() const { return exposure_ui_margin_top_; }
  float ExposureUIMarginBottom() const { return exposure_ui_margin_bottom_; }
  float ExposureAreaRatio() const { return exposure_area_ratio_; }
  const std::string& Id() const { return id_; }
  const std::string& ExposureId() const { return exposure_id_; }
  const std::string& ExposureScene() const { return exposure_scene_; }
  const lepus::Value& Dataset() const { return dataset_; }

  void GetExposureTargetRect(float rect[4]) const;
  void GetExposureWindowRect(float rect[4]) const;

  fml::RefPtr<PlatformEventTarget> ParentTarget() { return parent_; }
  void SetParentTarget(fml::RefPtr<PlatformEventTarget> parent) {
    parent_ = parent;
  }
  ChildrenTargetVec& ChildrenTargets() { return children_; }
  void AddChildTarget(fml::RefPtr<PlatformEventTarget> child) {
    if (child == nullptr) {
      return;
    }
    children_.push_back(child);
    child->SetParentTarget(fml::RefPtr<PlatformEventTarget>(this));
  }

  fml::RefPtr<PlatformEventTarget> HitTest(float point[2]);
  bool ShouldHitTest() const;
  void GetPointInTarget(float target_point[2],
                        fml::RefPtr<PlatformEventTarget> parent_target,
                        float point[2]);
  bool ContainsPoint(float point[2]) const;
  bool IsVisibleForExposure(
      std::unordered_map<int32_t, CommonAncestorRect>& common_ancestor_rect_map,
      float root_view_origin_on_screen[2], const float window_rect[4]) const;
  void OnResponseChain();
  void OffResponseChain();
  bool IsOnResponseChain() const;

  void OnFocusChange(bool has_focus, bool is_focus_transition);
  bool Focusable() const;
  void OnPseudoStatusChanged(LynxPseudoStatus pre_status,
                             LynxPseudoStatus current_status);
  LynxPseudoStatus GetPseudoStatus() const;
  bool TouchPseudoPropagation() const;

  bool EventThrough(float point[2]) const;
  bool IgnoreFocus() const;
  LynxPointerEventsValue PointerEvents() const;
  bool BlockNativeEvent(float point[2]) const;
  LynxConsumeSlideDirection ConsumeSlideEvent() const;

  void SetEventSet(base::Vector<PlatformEventName> event_set) {
    event_set_ = std::move(event_set);
  }

  void SetUserInteractionEnabled(bool enabled) {
    user_interaction_enabled_ = enabled;
  }
  void SetNativeInteractionEnabled(bool enabled) {
    native_interaction_enabled_ = enabled;
  }
  void SetExposureScreenMarginLeft(float value) {
    exposure_screen_margin_left_ = value;
  }
  void SetExposureScreenMarginRight(float value) {
    exposure_screen_margin_right_ = value;
  }
  void SetExposureScreenMarginTop(float value) {
    exposure_screen_margin_top_ = value;
  }
  void SetExposureScreenMarginBottom(float value) {
    exposure_screen_margin_bottom_ = value;
  }
  void SetExposureUIMarginLeft(float value) {
    exposure_ui_margin_left_ = value;
  }
  void SetExposureUIMarginRight(float value) {
    exposure_ui_margin_right_ = value;
  }
  void SetExposureUIMarginTop(float value) { exposure_ui_margin_top_ = value; }
  void SetExposureUIMarginBottom(float value) {
    exposure_ui_margin_bottom_ = value;
  }
  void SetExposureArea(float value) { exposure_area_ratio_ = value; }
  void SetEnableExposureUIClip(LynxEventPropStatus value) {
    enable_exposure_ui_clip_ = value;
  }
  void SetId(std::string value) { id_ = std::move(value); }
  void SetExposureId(std::string value) { exposure_id_ = std::move(value); }
  void SetExposureScene(std::string value) {
    exposure_scene_ = std::move(value);
  }
  void SetDataset(lepus::Value dataset) { dataset_ = std::move(dataset); }

 private:
  void GetOrUpdateTargetScreenRect(
      std::unordered_map<int32_t, CommonAncestorRect>& common_ancestor_rect_map,
      const fml::RefPtr<PlatformEventTarget>& target, float out_rect[4],
      float root_view_origin_on_screen[2]) const;

  // target props
  int32_t sign_;
  float left_{0.f};
  float top_{0.f};
  float width_{0.f};
  float height_{0.f};
  bool is_scroll_container_{false};
  float scroll_offset_x_{0.f};
  float scroll_offset_y_{0.f};
  float offset_x_for_calc_position_{0.f};
  float offset_y_for_calc_position_{0.f};
  base::Vector<PlatformEventName> event_set_;
  bool user_interaction_enabled_{true};
  bool native_interaction_enabled_{true};
  float exposure_screen_margin_left_{0.f};
  float exposure_screen_margin_right_{0.f};
  float exposure_screen_margin_top_{0.f};
  float exposure_screen_margin_bottom_{0.f};
  float exposure_ui_margin_left_{0.f};
  float exposure_ui_margin_right_{0.f};
  float exposure_ui_margin_top_{0.f};
  float exposure_ui_margin_bottom_{0.f};
  float exposure_area_ratio_{0.f};
  LynxEventPropStatus enable_exposure_ui_clip_{LynxEventPropStatus::kUndefined};
  std::string id_;
  std::string exposure_id_;
  std::string exposure_scene_;
  lepus::Value dataset_;

  // event/expose target tree
  fml::RefPtr<PlatformEventTarget> parent_{nullptr};
  ChildrenTargetVec children_;
  PlatformEventTargetHelper* target_helper_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_H_
