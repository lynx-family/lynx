// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_H_

#include <string>
#include <vector>

#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

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
  PlatformEventTarget(int32_t sign, float left, float top, float width,
                      float height)
      : sign_(sign), left_(left), top_(top), width_(width), height_(height) {}
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
  float RendererOffsetX() const { return renderer_offset_x_; }
  float RendererOffsetY() const { return renderer_offset_y_; }
  bool IsScrollContainer() const { return is_scroll_container_; }
  float ScrollOffsetX() const { return scroll_offset_x_; }
  float ScrollOffsetY() const { return scroll_offset_y_; }
  float OffsetXForCalcPosition() const { return offset_x_for_calc_position_; }
  float OffsetYForCalcPosition() const { return offset_y_for_calc_position_; }

  void SetRendererOffsetX(float renderer_offset_x) {
    renderer_offset_x_ = renderer_offset_x;
  }
  void SetRendererOffsetY(float renderer_offset_y) {
    renderer_offset_y_ = renderer_offset_y;
  }

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
  bool ShouldHitTest();
  void GetPointInTarget(float target_point[2],
                        fml::RefPtr<PlatformEventTarget> parent_target,
                        float point[2]);
  bool ContainsPoint(float point[2]);
  void OnResponseChain();
  void OffResponseChain();
  bool IsOnResponseChain();
  void OnFocusChange(bool has_focus, bool is_focus_transition);
  bool Focusable();
  void OnPseudoStatusChanged(LynxPseudoStatus pre_status,
                             LynxPseudoStatus current_status);
  LynxPseudoStatus GetPseudoStatus();
  bool TouchPseudoPropagation();
  std::vector<std::string> EventSet();

  bool BlockNativeEvent(float point[2]);
  bool EventThrough(float point[2]);
  bool IgnoreFocus();
  LynxConsumeSlideDirection ConsumeSlideEvent();
  LynxPointerEventsValue PointerEvents();

 private:
  // target props
  int32_t sign_;
  float left_{0.f};
  float top_{0.f};
  float width_{0.f};
  float height_{0.f};
  float renderer_offset_x_{0.f};
  float renderer_offset_y_{0.f};
  bool is_scroll_container_{false};
  float scroll_offset_x_{0.f};
  float scroll_offset_y_{0.f};
  float offset_x_for_calc_position_{0.f};
  float offset_y_for_calc_position_{0.f};

  // event/expose target tree
  fml::RefPtr<PlatformEventTarget> parent_{nullptr};
  ChildrenTargetVec children_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_H_
