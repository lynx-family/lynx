// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_emitter.h"

#include <limits>
#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/bubble_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {
constexpr int64_t kMaxEventID = std::numeric_limits<int64_t>::max() - 1;
constexpr int64_t kCurrentLynxPageOnlyEventID =
    std::numeric_limits<int64_t>::min();
}  // namespace

void EventEmitter::SendEvent(const LynxEvent& event) {
  switch (event.EventType()) {
    case LynxEventType::kTouch: {
      const TouchEvent& touch_event = static_cast<const TouchEvent&>(event);
      SendTouchEvent(touch_event);
      break;
    }
    case LynxEventType::kCustom: {
      ui_owner_->HandleCustomEvent(static_cast<const CustomEvent&>(event));
      break;
    }
    case LynxEventType::kPointer:
    case LynxEventType::kMouse:
    case LynxEventType::kWheel:
    case LynxEventType::kKeyboard: {
      SendBubbleEvent(static_cast<const BubbleEvent&>(event));
      break;
    }
    case LynxEventType::kNone:
    default: {
      break;
    }
  }
}

void EventEmitter::SendBubbleEvent(const BubbleEvent& bubble_event) {
  ui_owner_->HandleBubbleEvent(bubble_event);
}

void EventEmitter::SendTouchEvent(const TouchEvent& touch_event) {
  TouchEvent event = touch_event;
  bool dispatch_in_current_lynx_page_only =
      event.EventID() == kCurrentLynxPageOnlyEventID;
  if (dispatch_in_current_lynx_page_only) {
    event.SetEventID(0);
  }
  auto target = event.Target().lock();
  if (target) {
    auto* children_lynx_page_ui = target->ChildrenLynxPageUI();
    bool has_children_lynx_page_ui =
        children_lynx_page_ui && !children_lynx_page_ui->empty();
    bool has_parent_lynx_page_ui = !dispatch_in_current_lynx_page_only &&
                                   !target->ParentLynxPageUI().expired();
    bool has_target_child_lynx_page_ui = false;
    if (children_lynx_page_ui) {
      auto target_child_it = children_lynx_page_ui->find(target.get());
      has_target_child_lynx_page_ui =
          target_child_it != children_lynx_page_ui->end() &&
          !target_child_it->second.expired();
    }
    bool should_dispatch_through_lynx_page =
        dispatch_in_current_lynx_page_only
            ? has_target_child_lynx_page_ui
            : has_parent_lynx_page_ui || has_children_lynx_page_ui;
    if (should_dispatch_through_lynx_page) {
      if (!has_parent_lynx_page_ui) {
        // Only the root page starts a new cross-page event sequence.
        event_id_ = (event_id_ + 1) % kMaxEventID;
      }
      TouchEvent event_with_id = event;
      event_with_id.SetEventID(event_id_);
      target->SetEventID(event_id_);
      ui_owner_->StartEventGenerate(event_with_id);
      if (!has_target_child_lynx_page_ui) {
        auto root_lynx_page_ui = target->RootLynxPageUI().lock();
        if (root_lynx_page_ui) {
          root_lynx_page_ui->StartEventCapture(event_id_);
        }
      }
      return;
    }
  }

  if (event.IsMultiTouch()) {
    ui_owner_->HandleMultiTouchEvent(event);
  } else {
    ui_owner_->HandleTouchEvent(event);
  }
}

void EventEmitter::SendPseudoStatusEvent(int id, PseudoStatus pre_status,
                                         PseudoStatus current_status) const {
  if (pre_status == current_status) {
    return;
  }
  ui_owner_->OnPseudoStatusChanged(id, pre_status, current_status);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
