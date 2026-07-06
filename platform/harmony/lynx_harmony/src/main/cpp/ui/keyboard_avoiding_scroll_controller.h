// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_KEYBOARD_AVOIDING_SCROLL_CONTROLLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_KEYBOARD_AVOIDING_SCROLL_CONTROLLER_H_

#include <cstdint>
#include <unordered_map>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxContext;

class KeyboardAvoidingScrollController {
 public:
  struct Target {
    bool avoid_keyboard{false};
    float spacing{0.f};
  };

  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual UIBase* FindUIBySignForKeyboardAvoiding(int32_t sign) const = 0;
    virtual const std::unordered_map<int32_t, Target>& KeyboardAvoidingTargets()
        const = 0;
    virtual int32_t ActiveKeyboardAvoidingTargetSign() const = 0;
    virtual float KeyboardAvoidingKeyboardTop() = 0;
    virtual LynxContext* KeyboardAvoidingContext() const = 0;
    virtual bool KeyboardAvoidingDestroyed() const = 0;
  };

  explicit KeyboardAvoidingScrollController(Delegate* delegate);

  bool CanHandleTarget(UIBase* owner) const;
  bool ApplyAvoidDistance(UIBase* owner, float keyboard_height, float spacing,
                          bool smooth);
  void CaptureFocusedInputBottomBeforeScrollContentSizeChange(
      UIBase* scroll_ui);
  bool PreserveFocusedInputBottomAfterScrollContentSizeChange(
      UIBase* scroll_ui);
  void ScheduleAvoidDistanceReapplyAfterLayout(UIBase* owner,
                                               float keyboard_height,
                                               float spacing, bool smooth);
  void Clear(bool smooth);
  void Reset();
  bool IsActiveScrollUI(int32_t sign) const;

 private:
  static constexpr int32_t kInvalidSign = -2;
  static constexpr float kEpsilon = 0.5f;
  static constexpr int32_t kContentHeightClearMaxFrames = 60;
  static constexpr intptr_t kPostLayoutReapplyTaskOffset = 100000;

  bool ApplyAvoidDistance(UIBase* owner, float keyboard_height, float spacing,
                          bool smooth, bool schedule_post_layout_reapply);
  UIBase* ScrollUIForOwner(UIBase* owner) const;
  bool IsTargetInScrollUI(UIBase* target, UIBase* scroll_ui) const;
  UIBase* LowestTargetInScrollUI(UIBase* scroll_ui) const;
  float InputBottomInScrollViewContent(UIBase* input, UIBase* scroll_ui,
                                       float current_offset_y) const;

  float BaseContentHeightForScrollUI(UIBase* scroll_ui);
  void SetContentHeightExtraForScrollUI(UIBase* scroll_ui, float extra);
  void SetContentHeightForScrollUI(UIBase* scroll_ui, float content_height);
  void ClearContentHeightForScrollUI(UIBase* scroll_ui);
  void ClearContentHeightForScrollUI(UIBase* scroll_ui,
                                     float base_content_height);
  void ResetContentState();
  void ReapplyContentHeightForScrollUI(UIBase* scroll_ui);

  void ScrollTo(UIBase* scroll_ui, float offset_y, bool smooth);
  void ScrollToAfterContentLayout(UIBase* scroll_ui, int32_t target_sign,
                                  float offset_y, bool smooth);
  void SchedulePostLayoutReapplyIfNeeded(UIBase* scroll_ui, UIBase* target,
                                         float keyboard_height, float spacing,
                                         bool smooth, bool should_schedule);
  void ClearPendingPostLayoutReapply();
  bool IsScrollRequestCurrent(UIBase* scroll_ui, int32_t target_sign) const;
  void PreserveFocusedInputBottomIfNeeded(UIBase* scroll_ui, UIBase* input,
                                          bool should_preserve);
  float ClampScrollOffsetYForScrollUI(UIBase* scroll_ui, float offset_y) const;
  void ResetFocusedInputBottomState();
  void ResetContentSizeChangeAnchor();

  void CaptureOriginalOffsetIfNeeded(UIBase* scroll_ui);
  bool RestoreOffsetForScrollUI(UIBase* scroll_ui, float offset_y, bool smooth);
  bool RestoreOriginalOffsetForScrollUI(UIBase* scroll_ui, bool smooth);
  void ScheduleContentHeightClear(int32_t scroll_sign, int32_t generation,
                                  float base_content_height,
                                  float restore_offset_y,
                                  int32_t remaining_frames);

  Delegate* delegate_{nullptr};
  int32_t scroll_sign_{kInvalidSign};
  float base_content_height_{0.f};
  float content_height_extra_{0.f};
  float original_offset_y_{0.f};
  float focused_input_bottom_in_scroll_view_content_{0.f};
  int32_t clear_generation_{0};
  int32_t post_layout_reapply_generation_{0};
  int32_t focused_input_scroll_sign_{kInvalidSign};
  int32_t focused_input_sign_{kInvalidSign};
  int32_t content_size_change_scroll_sign_{kInvalidSign};
  int32_t content_size_change_input_sign_{kInvalidSign};
  float content_size_change_input_bottom_in_scroll_view_content_{0.f};
  float content_size_change_offset_y_{0.f};
  bool has_content_state_{false};
  bool has_original_offset_{false};
  bool has_focused_input_bottom_{false};
  bool has_content_size_change_anchor_{false};
  int32_t setting_scroll_content_height_depth_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_KEYBOARD_AVOIDING_SCROLL_CONTROLLER_H_
