// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_TARGET_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_TARGET_H_

#include <arkui/native_type.h>
#include <arkui/ui_input_event.h>

#include <memory>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

enum class LynxEventPropStatus {
  kUndefined,
  kDisable,
  kEnable,
};

enum class ConsumeSlideDirection {
  kNone,
  kHorizontal,
  kVertical,
  kUp,
  kRight,
  kDown,
  kLeft,
  kAll,
};

enum class PseudoStatus {
  kNone = 0,
  kHover = 1,
  kHoverTransition = 1 << 1,
  kActive = 1 << 3,
  kActiveTransition = 1 << 4,
  kFocus = 1 << 6,
  kFocusTransition = 1 << 7,
  kAll = ~0,
};

class EventTarget {
 public:
  virtual EventTarget* ParentTarget() = 0;

  virtual EventTarget* HitTest(float point[2]) = 0;

  virtual bool ShouldHitTest() = 0;

  virtual void GetPointInTarget(float res[2], EventTarget* parent_target,
                                float point[2]) = 0;

  virtual bool ContainsPoint(float point[2]) = 0;

  virtual bool IsOnResponseChain() = 0;

  virtual void OnFocusChange(bool has_focus, bool is_focus_transition){};

  virtual bool Focusable() { return false; };

  virtual void OnPseudoStatusChanged(PseudoStatus pre_status,
                                     PseudoStatus current_status) = 0;

  virtual PseudoStatus GetPseudoStatus() = 0;

  virtual bool BlockNativeEvent() = 0;

  virtual bool EventThrough() = 0;

  virtual bool IgnoreFocus() = 0;

  virtual ConsumeSlideDirection ConsumeSlideEvent() = 0;

  virtual bool TouchPseudoPropagation() = 0;

  virtual int Sign() const = 0;

  virtual bool IsInterceptGesture() = 0;

  virtual bool HasUI() = 0;

  virtual ArkUI_NodeHandle RootNode() { return nullptr; }

  virtual std::weak_ptr<EventTarget> WeakTarget() = 0;

  virtual int GestureArenaMemberId() { return 0; }

  virtual bool DispatchTouch(const ArkUI_UIInputEvent* event) { return false; }

  EventTarget* FirstUITarget() {
    EventTarget* current_target = this;
    while (current_target && !current_target->HasUI()) {
      current_target = current_target->ParentTarget();
    }
    return current_target;
  }

  virtual ~EventTarget() = default;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_TARGET_H_
