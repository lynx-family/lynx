// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/keyboard_avoiding_scroll_controller.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"

namespace lynx {
namespace tasm {
namespace harmony {

KeyboardAvoidingScrollController::KeyboardAvoidingScrollController(
    Delegate* delegate)
    : delegate_(delegate) {}

bool KeyboardAvoidingScrollController::CanHandleTarget(UIBase* owner) const {
  return ScrollUIForOwner(owner) != nullptr;
}

bool KeyboardAvoidingScrollController::ApplyAvoidDistance(UIBase* owner,
                                                          float keyboard_height,
                                                          float spacing,
                                                          bool smooth) {
  return ApplyAvoidDistance(owner, keyboard_height, spacing, smooth, true);
}

bool KeyboardAvoidingScrollController::ApplyAvoidDistance(
    UIBase* owner, float keyboard_height, float spacing, bool smooth,
    bool schedule_post_layout_reapply) {
  UIBase* scroll_ui = ScrollUIForOwner(owner);
  if (!scroll_ui) {
    return false;
  }
  if (scroll_sign_ != kInvalidSign && scroll_sign_ != scroll_ui->Sign()) {
    Clear(false);
  }
  scroll_sign_ = scroll_ui->Sign();
  CaptureOriginalOffsetIfNeeded(scroll_ui);
  PreserveFocusedInputBottomIfNeeded(
      scroll_ui, owner,
      keyboard_height > 0.f && (scroll_ui->ScrollContentHeightExtra() > 0.f ||
                                content_height_extra_ > 0.f));

  if (keyboard_height <= 0.f || scroll_ui->height_ <= 0.f) {
    return true;
  }

  float base_content_height = BaseContentHeightForScrollUI(scroll_ui);
  bool had_content_height_extra = scroll_ui->ScrollContentHeightExtra() > 0.f ||
                                  content_height_extra_ > 0.f;
  float min_offset_y = 0.f;
  float current_offset_y = scroll_ui->ScrollY();
  float viewport_height = std::max(0.f, scroll_ui->height_);
  float raw_max_offset_y_without_extra = base_content_height - viewport_height;
  float max_offset_y_without_extra =
      std::max(min_offset_y, raw_max_offset_y_without_extra);

  float visible_height = viewport_height;
  float scroll_rect[4] = {0.f, 0.f, scroll_ui->width_, scroll_ui->height_};
  LynxUIHelper::ConvertRectFromUIToScreen(scroll_rect, scroll_ui, scroll_rect);
  visible_height =
      std::min(visible_height,
               delegate_->KeyboardAvoidingKeyboardTop() - scroll_rect[1]);
  visible_height = std::max(0.f, visible_height);

  float input_bottom_y =
      InputBottomInScrollViewContent(owner, scroll_ui, current_offset_y);
  float desired_offset_y = input_bottom_y + spacing - visible_height;
  float covered_height =
      std::max(0.f, std::ceil(viewport_height - visible_height));
  UIBase* lowest_target = LowestTargetInScrollUI(scroll_ui);
  float lowest_spacing = spacing;
  if (!lowest_target) {
    lowest_target = owner;
  } else if (auto it = delegate_->KeyboardAvoidingTargets().find(
                 lowest_target->Sign());
             it != delegate_->KeyboardAvoidingTargets().end()) {
    lowest_spacing = it->second.spacing;
  }
  float lowest_input_bottom_y = InputBottomInScrollViewContent(
      lowest_target, scroll_ui, current_offset_y);
  float lowest_desired_offset_y =
      lowest_input_bottom_y + lowest_spacing - visible_height;
  float required_extra = covered_height;
  if (lowest_desired_offset_y > max_offset_y_without_extra) {
    required_extra =
        std::max(required_extra,
                 std::max(0.f, std::ceil(lowest_desired_offset_y -
                                         raw_max_offset_y_without_extra)));
  }
  SetContentHeightExtraForScrollUI(scroll_ui, required_extra);

  float max_offset_y =
      std::max(min_offset_y, raw_max_offset_y_without_extra + required_extra);
  float target_offset_y = current_offset_y;
  if (desired_offset_y > current_offset_y) {
    target_offset_y = std::min(
        std::max(std::ceil(desired_offset_y), min_offset_y), max_offset_y);
  }
  if (std::fabs(target_offset_y - current_offset_y) < kEpsilon) {
    SchedulePostLayoutReapplyIfNeeded(
        scroll_ui, owner, keyboard_height, spacing, smooth,
        schedule_post_layout_reapply && had_content_height_extra &&
            required_extra > 0.f);
    return true;
  }
  bool needs_scroll_after_content_layout =
      required_extra > 0.f && target_offset_y > max_offset_y_without_extra;
  if (smooth && needs_scroll_after_content_layout) {
    ScrollToAfterContentLayout(scroll_ui, owner->Sign(), target_offset_y,
                               smooth);
    return true;
  }
  ScrollTo(scroll_ui, target_offset_y, smooth);
  if (needs_scroll_after_content_layout) {
    ScrollToAfterContentLayout(scroll_ui, owner->Sign(), target_offset_y,
                               smooth);
  }
  return true;
}

void KeyboardAvoidingScrollController::
    CaptureFocusedInputBottomBeforeScrollContentSizeChange(UIBase* scroll_ui) {
  ResetContentSizeChangeAnchor();
  if (setting_scroll_content_height_depth_ > 0 || !delegate_ || !scroll_ui ||
      scroll_ui->Sign() != scroll_sign_) {
    return;
  }
  int32_t active_sign = delegate_->ActiveKeyboardAvoidingTargetSign();
  UIBase* active_owner =
      delegate_->FindUIBySignForKeyboardAvoiding(active_sign);
  if (!active_owner || !IsTargetInScrollUI(active_owner, scroll_ui)) {
    return;
  }
  auto target_it = delegate_->KeyboardAvoidingTargets().find(active_sign);
  if (target_it == delegate_->KeyboardAvoidingTargets().end() ||
      !target_it->second.avoid_keyboard) {
    return;
  }
  if (scroll_ui->ScrollContentHeightExtra() <= kEpsilon &&
      content_height_extra_ <= kEpsilon) {
    return;
  }
  float current_offset_y = scroll_ui->ScrollY();
  content_size_change_scroll_sign_ = scroll_ui->Sign();
  content_size_change_input_sign_ = active_owner->Sign();
  content_size_change_input_bottom_in_scroll_view_content_ =
      InputBottomInScrollViewContent(active_owner, scroll_ui, current_offset_y);
  content_size_change_offset_y_ = current_offset_y;
  has_content_size_change_anchor_ = true;
}

bool KeyboardAvoidingScrollController::
    PreserveFocusedInputBottomAfterScrollContentSizeChange(UIBase* scroll_ui) {
  if (setting_scroll_content_height_depth_ > 0 || !delegate_ || !scroll_ui ||
      !has_content_size_change_anchor_ ||
      content_size_change_scroll_sign_ != scroll_ui->Sign()) {
    ResetContentSizeChangeAnchor();
    return false;
  }

  int32_t input_sign = content_size_change_input_sign_;
  float previous_input_bottom =
      content_size_change_input_bottom_in_scroll_view_content_;
  float previous_offset_y = content_size_change_offset_y_;
  ResetContentSizeChangeAnchor();

  UIBase* active_owner = delegate_->FindUIBySignForKeyboardAvoiding(input_sign);
  if (!active_owner || !IsScrollRequestCurrent(scroll_ui, input_sign) ||
      !IsTargetInScrollUI(active_owner, scroll_ui)) {
    return false;
  }

  float current_offset_y = scroll_ui->ScrollY();
  float current_input_bottom =
      InputBottomInScrollViewContent(active_owner, scroll_ui, current_offset_y);
  float delta_bottom = current_input_bottom - previous_input_bottom;
  focused_input_scroll_sign_ = scroll_ui->Sign();
  focused_input_sign_ = active_owner->Sign();
  focused_input_bottom_in_scroll_view_content_ = current_input_bottom;
  has_focused_input_bottom_ = true;

  if (std::fabs(delta_bottom) < kEpsilon) {
    return false;
  }

  float target_offset_y = ClampScrollOffsetYForScrollUI(
      scroll_ui, previous_offset_y + delta_bottom);
  if (std::fabs(target_offset_y - current_offset_y) >= kEpsilon) {
    scroll_ui->ScrollToVerticalOffset(target_offset_y, false);
  }
  return true;
}

void KeyboardAvoidingScrollController::ScheduleAvoidDistanceReapplyAfterLayout(
    UIBase* owner, float keyboard_height, float spacing, bool smooth) {
  UIBase* scroll_ui = ScrollUIForOwner(owner);
  SchedulePostLayoutReapplyIfNeeded(scroll_ui, owner, keyboard_height, spacing,
                                    smooth, true);
}

void KeyboardAvoidingScrollController::Clear(bool smooth) {
  UIBase* scroll_ui =
      delegate_ ? delegate_->FindUIBySignForKeyboardAvoiding(scroll_sign_)
                : nullptr;
  scroll_sign_ = kInvalidSign;
  ClearPendingPostLayoutReapply();
  if (!scroll_ui) {
    ResetContentState();
    has_original_offset_ = false;
    original_offset_y_ = 0.f;
    return;
  }
  bool should_clear_content_height =
      has_content_state_ || scroll_ui->ScrollContentHeightExtra() > 0.f;
  float base_content_height = should_clear_content_height
                                  ? BaseContentHeightForScrollUI(scroll_ui)
                                  : 0.f;
  float restore_offset_y = has_original_offset_
                               ? std::max(0.f, original_offset_y_)
                               : scroll_ui->ScrollY();
  int32_t clear_generation = ++clear_generation_;
  bool is_restoring = RestoreOriginalOffsetForScrollUI(scroll_ui, smooth);
  if (is_restoring && smooth && should_clear_content_height) {
    ScheduleContentHeightClear(scroll_ui->Sign(), clear_generation,
                               base_content_height, restore_offset_y,
                               kContentHeightClearMaxFrames);
  } else if (should_clear_content_height) {
    ClearContentHeightForScrollUI(scroll_ui, base_content_height);
    RestoreOffsetForScrollUI(scroll_ui, restore_offset_y, false);
  }
  ResetContentState();
  has_original_offset_ = false;
  original_offset_y_ = 0.f;
}

void KeyboardAvoidingScrollController::Reset() {
  scroll_sign_ = kInvalidSign;
  ResetContentState();
  ResetContentSizeChangeAnchor();
  has_original_offset_ = false;
  original_offset_y_ = 0.f;
  ++clear_generation_;
  ClearPendingPostLayoutReapply();
}

bool KeyboardAvoidingScrollController::IsActiveScrollUI(int32_t sign) const {
  return scroll_sign_ == sign;
}

UIBase* KeyboardAvoidingScrollController::ScrollUIForOwner(
    UIBase* owner) const {
  UIBase* ui = owner == nullptr ? nullptr : owner->Parent();
  while (ui) {
    if (ui->IsVerticalScrollView()) {
      return ui;
    }
    ui = ui->Parent();
  }
  return nullptr;
}

bool KeyboardAvoidingScrollController::IsTargetInScrollUI(
    UIBase* target, UIBase* scroll_ui) const {
  if (!target || !scroll_ui) {
    return false;
  }
  UIBase* ui = target->Parent();
  while (ui) {
    if (ui == scroll_ui) {
      return true;
    }
    if (ui->IsVerticalScrollView()) {
      return false;
    }
    ui = ui->Parent();
  }
  return false;
}

UIBase* KeyboardAvoidingScrollController::LowestTargetInScrollUI(
    UIBase* scroll_ui) const {
  UIBase* lowest_target = nullptr;
  float lowest_input_bottom = -std::numeric_limits<float>::max();
  float current_offset_y = scroll_ui ? scroll_ui->ScrollY() : 0.f;
  for (const auto& target_entry : delegate_->KeyboardAvoidingTargets()) {
    UIBase* target_ui =
        delegate_->FindUIBySignForKeyboardAvoiding(target_entry.first);
    if (!target_ui || !target_entry.second.avoid_keyboard ||
        !IsTargetInScrollUI(target_ui, scroll_ui)) {
      continue;
    }
    float input_bottom =
        InputBottomInScrollViewContent(target_ui, scroll_ui, current_offset_y);
    if (input_bottom > lowest_input_bottom) {
      lowest_input_bottom = input_bottom;
      lowest_target = target_ui;
    }
  }
  return lowest_target;
}

float KeyboardAvoidingScrollController::InputBottomInScrollViewContent(
    UIBase* input, UIBase* scroll_ui, float current_offset_y) const {
  if (!input || !scroll_ui) {
    return 0.f;
  }
  float input_rect[4] = {0.f, 0.f, input->width_, input->height_};
  LynxUIHelper::ConvertRectFromUIToScreen(input_rect, input, input_rect);
  float scroll_rect[4] = {0.f, 0.f, scroll_ui->width_, scroll_ui->height_};
  LynxUIHelper::ConvertRectFromUIToScreen(scroll_rect, scroll_ui, scroll_rect);
  return input_rect[3] - scroll_rect[1] + current_offset_y;
}

float KeyboardAvoidingScrollController::BaseContentHeightForScrollUI(
    UIBase* scroll_ui) {
  float current_content_height =
      scroll_ui == nullptr ? 0.f
                           : std::max(0.f, scroll_ui->ScrollContentHeight());
  float current_content_height_extra =
      scroll_ui == nullptr
          ? 0.f
          : std::max(0.f, scroll_ui->ScrollContentHeightExtra());
  if (current_content_height_extra > 0.f) {
    float base_content_height =
        std::max(0.f, current_content_height - current_content_height_extra);
    base_content_height_ = base_content_height;
    content_height_extra_ = current_content_height_extra;
    has_content_state_ = true;
    return base_content_height;
  }
  if (has_content_state_) {
    return std::max(0.f, base_content_height_);
  }
  base_content_height_ = current_content_height;
  has_content_state_ = true;
  return current_content_height;
}

void KeyboardAvoidingScrollController::SetContentHeightExtraForScrollUI(
    UIBase* scroll_ui, float extra) {
  if (!scroll_ui) {
    return;
  }
  float base_content_height = BaseContentHeightForScrollUI(scroll_ui);
  float content_height_extra = std::max(0.f, extra);
  content_height_extra_ = content_height_extra;
  if (content_height_extra > 0.f) {
    scroll_ui->SetScrollContentHeightExtra(content_height_extra);
    SetContentHeightForScrollUI(scroll_ui,
                                base_content_height + content_height_extra);
  } else {
    ClearContentHeightForScrollUI(scroll_ui);
  }
  base_content_height_ = base_content_height;
  has_content_state_ = true;
}

void KeyboardAvoidingScrollController::SetContentHeightForScrollUI(
    UIBase* scroll_ui, float content_height) {
  if (!scroll_ui) {
    return;
  }
  float offset_y = scroll_ui->ScrollY();
  float target_content_height = std::max(0.f, content_height);
  float max_offset_y =
      std::max(0.f, target_content_height - std::max(0.f, scroll_ui->height_));
  float target_offset_y = std::min(std::max(offset_y, 0.f), max_offset_y);
  ++setting_scroll_content_height_depth_;
  scroll_ui->SetScrollContentHeight(target_content_height);
  scroll_ui->ScrollToVerticalOffset(target_offset_y, false);
  --setting_scroll_content_height_depth_;
}

void KeyboardAvoidingScrollController::ClearContentHeightForScrollUI(
    UIBase* scroll_ui) {
  if (!scroll_ui || !has_content_state_) {
    return;
  }
  ClearContentHeightForScrollUI(scroll_ui, base_content_height_);
}

void KeyboardAvoidingScrollController::ClearContentHeightForScrollUI(
    UIBase* scroll_ui, float base_content_height) {
  if (!scroll_ui) {
    return;
  }
  scroll_ui->SetScrollContentHeightExtra(0.f);
  SetContentHeightForScrollUI(scroll_ui, base_content_height);
}

void KeyboardAvoidingScrollController::ResetContentState() {
  has_content_state_ = false;
  base_content_height_ = 0.f;
  content_height_extra_ = 0.f;
  ResetFocusedInputBottomState();
  ResetContentSizeChangeAnchor();
}

void KeyboardAvoidingScrollController::ReapplyContentHeightForScrollUI(
    UIBase* scroll_ui) {
  if (!scroll_ui || !has_content_state_ || content_height_extra_ <= 0.f) {
    return;
  }
  SetContentHeightForScrollUI(scroll_ui,
                              base_content_height_ + content_height_extra_);
}

void KeyboardAvoidingScrollController::ScrollTo(UIBase* scroll_ui,
                                                float offset_y, bool smooth) {
  if (!scroll_ui) {
    return;
  }
  ReapplyContentHeightForScrollUI(scroll_ui);
  scroll_ui->ScrollToVerticalOffset(offset_y, smooth);
}

void KeyboardAvoidingScrollController::ScrollToAfterContentLayout(
    UIBase* scroll_ui, int32_t target_sign, float offset_y, bool smooth) {
  if (!scroll_ui || !delegate_ || !delegate_->KeyboardAvoidingContext()) {
    return;
  }
  int32_t scroll_sign = scroll_ui->Sign();
  const auto& monitor = delegate_->KeyboardAvoidingContext()->VSyncMonitor();
  if (!monitor) {
    ScrollTo(scroll_ui, offset_y, smooth);
    return;
  }
  monitor->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<intptr_t>(this),
      [this, scroll_sign, target_sign, offset_y, smooth](int64_t, int64_t) {
        if (delegate_->KeyboardAvoidingDestroyed()) {
          return;
        }
        UIBase* scroll_ui =
            delegate_->FindUIBySignForKeyboardAvoiding(scroll_sign);
        if (!IsScrollRequestCurrent(scroll_ui, target_sign)) {
          return;
        }
        ReapplyContentHeightForScrollUI(scroll_ui);
        if (std::fabs(offset_y - scroll_ui->ScrollY()) >= kEpsilon) {
          ScrollTo(scroll_ui, offset_y, smooth);
        }
      });
}

void KeyboardAvoidingScrollController::SchedulePostLayoutReapplyIfNeeded(
    UIBase* scroll_ui, UIBase* target, float keyboard_height, float spacing,
    bool smooth, bool should_schedule) {
  if (!should_schedule || !scroll_ui || !target || !delegate_ ||
      !delegate_->KeyboardAvoidingContext()) {
    return;
  }
  const auto& monitor = delegate_->KeyboardAvoidingContext()->VSyncMonitor();
  if (!monitor) {
    return;
  }
  int32_t scroll_sign = scroll_ui->Sign();
  int32_t target_sign = target->Sign();
  int32_t generation = ++post_layout_reapply_generation_;
  monitor->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<intptr_t>(this) + kPostLayoutReapplyTaskOffset +
          generation,
      [this, scroll_sign, target_sign, keyboard_height, spacing, smooth,
       generation](int64_t, int64_t) {
        if (delegate_->KeyboardAvoidingDestroyed() ||
            generation != post_layout_reapply_generation_) {
          return;
        }
        UIBase* scroll_ui =
            delegate_->FindUIBySignForKeyboardAvoiding(scroll_sign);
        UIBase* target =
            delegate_->FindUIBySignForKeyboardAvoiding(target_sign);
        if (!target || !IsScrollRequestCurrent(scroll_ui, target_sign)) {
          return;
        }
        ApplyAvoidDistance(target, keyboard_height, spacing, smooth, false);
      });
}

void KeyboardAvoidingScrollController::ClearPendingPostLayoutReapply() {
  ++post_layout_reapply_generation_;
}

bool KeyboardAvoidingScrollController::IsScrollRequestCurrent(
    UIBase* scroll_ui, int32_t target_sign) const {
  return scroll_ui != nullptr && scroll_sign_ == scroll_ui->Sign() &&
         delegate_->ActiveKeyboardAvoidingTargetSign() == target_sign;
}

void KeyboardAvoidingScrollController::PreserveFocusedInputBottomIfNeeded(
    UIBase* scroll_ui, UIBase* input, bool should_preserve) {
  if (!scroll_ui || !input) {
    ResetFocusedInputBottomState();
    return;
  }
  float current_offset_y = scroll_ui->ScrollY();
  float current_input_bottom =
      InputBottomInScrollViewContent(input, scroll_ui, current_offset_y);
  bool can_preserve = should_preserve && has_focused_input_bottom_ &&
                      focused_input_scroll_sign_ == scroll_ui->Sign() &&
                      focused_input_sign_ == input->Sign();
  if (can_preserve) {
    float delta =
        current_input_bottom - focused_input_bottom_in_scroll_view_content_;
    if (std::fabs(delta) >= kEpsilon) {
      float target_offset_y =
          ClampScrollOffsetYForScrollUI(scroll_ui, current_offset_y + delta);
      if (std::fabs(target_offset_y - current_offset_y) >= kEpsilon) {
        scroll_ui->ScrollToVerticalOffset(target_offset_y, false);
      }
    }
  }
  focused_input_scroll_sign_ = scroll_ui->Sign();
  focused_input_sign_ = input->Sign();
  focused_input_bottom_in_scroll_view_content_ = current_input_bottom;
  has_focused_input_bottom_ = true;
}

float KeyboardAvoidingScrollController::ClampScrollOffsetYForScrollUI(
    UIBase* scroll_ui, float offset_y) const {
  if (!scroll_ui) {
    return offset_y;
  }
  float max_offset_y = std::max(0.f, scroll_ui->ScrollContentHeight() -
                                         std::max(0.f, scroll_ui->height_));
  return std::min(std::max(offset_y, 0.f), max_offset_y);
}

void KeyboardAvoidingScrollController::ResetFocusedInputBottomState() {
  focused_input_scroll_sign_ = kInvalidSign;
  focused_input_sign_ = kInvalidSign;
  focused_input_bottom_in_scroll_view_content_ = 0.f;
  has_focused_input_bottom_ = false;
}

void KeyboardAvoidingScrollController::ResetContentSizeChangeAnchor() {
  content_size_change_scroll_sign_ = kInvalidSign;
  content_size_change_input_sign_ = kInvalidSign;
  content_size_change_input_bottom_in_scroll_view_content_ = 0.f;
  content_size_change_offset_y_ = 0.f;
  has_content_size_change_anchor_ = false;
}

void KeyboardAvoidingScrollController::CaptureOriginalOffsetIfNeeded(
    UIBase* scroll_ui) {
  if (!scroll_ui || has_original_offset_) {
    return;
  }
  original_offset_y_ = scroll_ui->ScrollY();
  has_original_offset_ = true;
}

bool KeyboardAvoidingScrollController::RestoreOriginalOffsetForScrollUI(
    UIBase* scroll_ui, bool smooth) {
  if (!scroll_ui || !has_original_offset_) {
    return false;
  }
  return RestoreOffsetForScrollUI(scroll_ui, original_offset_y_, smooth);
}

bool KeyboardAvoidingScrollController::RestoreOffsetForScrollUI(
    UIBase* scroll_ui, float offset_y, bool smooth) {
  if (!scroll_ui) {
    return false;
  }
  float target_offset_y = std::max(0.f, offset_y);
  if (std::fabs(scroll_ui->ScrollY() - target_offset_y) < kEpsilon) {
    return false;
  }
  scroll_ui->ScrollToVerticalOffset(target_offset_y, smooth);
  return true;
}

void KeyboardAvoidingScrollController::ScheduleContentHeightClear(
    int32_t scroll_sign, int32_t generation, float base_content_height,
    float restore_offset_y, int32_t remaining_frames) {
  const auto& monitor =
      delegate_ && delegate_->KeyboardAvoidingContext()
          ? delegate_->KeyboardAvoidingContext()->VSyncMonitor()
          : nullptr;
  if (!monitor) {
    UIBase* scroll_ui = delegate_->FindUIBySignForKeyboardAvoiding(scroll_sign);
    ClearContentHeightForScrollUI(scroll_ui, base_content_height);
    RestoreOffsetForScrollUI(scroll_ui, restore_offset_y, false);
    return;
  }
  monitor->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<intptr_t>(this) + generation,
      [this, scroll_sign, generation, base_content_height, restore_offset_y,
       remaining_frames](int64_t, int64_t) {
        if (delegate_->KeyboardAvoidingDestroyed() ||
            generation != clear_generation_) {
          return;
        }
        if (scroll_sign_ == scroll_sign) {
          return;
        }
        UIBase* scroll_ui =
            delegate_->FindUIBySignForKeyboardAvoiding(scroll_sign);
        bool is_restore_pending =
            scroll_ui && std::fabs(scroll_ui->ScrollY() -
                                   std::max(0.f, restore_offset_y)) >= kEpsilon;
        if (remaining_frames > 0 && is_restore_pending) {
          ScheduleContentHeightClear(scroll_sign, generation,
                                     base_content_height, restore_offset_y,
                                     remaining_frames - 1);
          return;
        }
        ClearContentHeightForScrollUI(scroll_ui, base_content_height);
        RestoreOffsetForScrollUI(scroll_ui, restore_offset_y, false);
      });
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
