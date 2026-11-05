// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_dispatcher.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

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
    auto* owner = context_->GetUIOwner();
    auto* dispatcher = owner ? owner->GetEventDispatcher() : nullptr;
    if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_TOUCH_INTERCEPT) {
      auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
      if (dispatcher && dispatcher->RawInputExclusive()) {
        OH_ArkUI_PointerEvent_SetInterceptHitTestMode(input_event, HTM_BLOCK);
        return;
      }
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
      if (dispatcher && dispatcher->RawInputExclusive()) {
        OH_ArkUI_PointerEvent_SetStopPropagation(input_event, true);
        context_->OnTouchEvent(input_event, this);
        return;
      }
      if (is_consume_event_) {
        context_->OnTouchEvent(input_event, this);
      }
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_ATTACH) {
      is_root_attached_ = true;
      if (auto input_context = context_->GetNativePaintingContext()) {
        input_context->SetPlatformEventRootActive(Sign(), true);
        input_context->NotifyInputRootStateChanged(Sign(), true, false);
      }
      context_->NotifyUIScroll();
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_DETACH) {
      is_root_attached_ = false;
      if (auto input_context = context_->GetNativePaintingContext()) {
        input_context->SetPlatformEventRootActive(Sign(), false);
      }
      context_->NotifyUIScroll();
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_FOCUS ||
               OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_BLUR) {
      const bool focused =
          OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_FOCUS &&
          dispatcher && dispatcher->InputWindowFocused();
      if (auto input_context = context_->GetNativePaintingContext()) {
        input_context->NotifyInputRootStateChanged(Sign(), is_root_attached_,
                                                   focused);
      }
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
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_EVENT_ON_ATTACH);
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_EVENT_ON_DETACH);
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_ON_FOCUS);
    NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_ON_BLUR);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
