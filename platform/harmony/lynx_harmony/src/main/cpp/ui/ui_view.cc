// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

#include <deviceinfo.h>

#include <string>

#include "base/include/log/logging.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {
constexpr int kKeyEventSupportVersion = 14;
constexpr int kAxisEventSupportVersion = 17;
}  // namespace

UIBase* UIView::Make(LynxContext* context, int sign, const std::string& tag) {
  return new UIView(context, ARKUI_NODE_CUSTOM, sign, tag);
}

void UIView::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  // TODO(renzhongyue): prop setters here
  UIBase::OnPropUpdate(name, value);
}

void UIView::OnNodeReady() { UIBase::OnNodeReady(); }

void UIView::OnNodeEvent(ArkUI_NodeEvent* event) {
  if (IsOverlayContent()) {
    if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_TOUCH_INTERCEPT) {
      auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
      float display_x = OH_ArkUI_PointerEvent_GetDisplayX(input_event);
      float display_y = OH_ArkUI_PointerEvent_GetDisplayY(input_event);
      if (context_ && context_->GetUIOwner()) {
        // Hit testing rebases screen coordinates against the root position.
        float point[2] = {display_x, display_y};
        is_consume_event_ =
            context_->GetUIOwner()->CanConsumeTouchEventAtRoot(point, this);
        if (OH_ArkUI_UIInputEvent_GetAction(input_event) ==
            UI_TOUCH_EVENT_ACTION_DOWN) {
          LOGI("[OverlayContentSlot] phase=native-intercept-down sign="
               << Sign() << " displayX=" << display_x << " displayY="
               << display_y << " canConsume=" << is_consume_event_)
        }
      }
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_TOUCH_EVENT) {
      auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
      if (is_consume_event_) {
        context_->OnTouchEvent(input_event, this);
      }
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_MOUSE) {
      context_->OnMouseEvent(OH_ArkUI_NodeEvent_GetInputEvent(event), this);
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_AXIS) {
      context_->OnAxisEvent(OH_ArkUI_NodeEvent_GetInputEvent(event), this);
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_KEY_EVENT) {
      context_->OnKeyEvent(OH_ArkUI_NodeEvent_GetInputEvent(event));
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_ATTACH) {
      is_root_attached_ = true;
      context_->NotifyUIScroll();
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_DETACH) {
      is_root_attached_ = false;
      context_->NotifyUIScroll();
    }
  }
}

UIView::UIView(LynxContext* context, ArkUI_NodeType type, int sign,
               const std::string& tag)
    : UIBase(context, type, sign, tag) {
  // default overflow.
  overflow_ = {true, true};
}

UIView::~UIView() {
  if (IsOverlayContent()) {
    NodeManager::Instance().RemoveNodeEventReceiver(Node(),
                                                    UIBase::EventReceiver);
    NodeManager::Instance().RemoveNodeCustomEventReceiver(
        Node(), UIBase::CustomEventReceiver);
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_TOUCH_EVENT);
    NodeManager::Instance().UnregisterNodeEvent(Node(),
                                                NODE_ON_TOUCH_INTERCEPT);
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_ON_MOUSE);
    if (OH_GetSdkApiVersion() >= kKeyEventSupportVersion) {
      NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_ON_KEY_EVENT);
    }
    if (OH_GetSdkApiVersion() >= kAxisEventSupportVersion) {
      NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_ON_AXIS);
    }
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_EVENT_ON_ATTACH);
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_EVENT_ON_DETACH);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
