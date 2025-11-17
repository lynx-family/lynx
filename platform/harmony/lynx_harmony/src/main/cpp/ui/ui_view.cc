// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

#include <string>

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

void UIView::OnNodeReady() {
  UIBase::OnNodeReady();
  if (IsOverlayContent()) {
    NodeManager::Instance().RegisterNodeEvent(Node(), NODE_EVENT_ON_ATTACH, 0,
                                              this);
    NodeManager::Instance().RegisterNodeEvent(Node(), NODE_EVENT_ON_DETACH, 0,
                                              this);
    NodeManager::Instance().RegisterNodeEvent(Node(), NODE_ON_TOUCH_INTERCEPT,
                                              0, this);
    NodeManager::Instance().RegisterNodeEvent(Node(), NODE_TOUCH_EVENT,
                                              NODE_TOUCH_EVENT, this);
    NodeManager::Instance().AddNodeEventReceiver(Node(), UIBase::EventReceiver);
    NodeManager::Instance().AddNodeCustomEventReceiver(
        Node(), UIBase::CustomEventReceiver);
  }
}

void UIView::OnNodeEvent(ArkUI_NodeEvent* event) {
  if (IsOverlayContent()) {
    if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_TOUCH_INTERCEPT) {
      auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
      float window_x = OH_ArkUI_PointerEvent_GetWindowX(input_event);
      float window_y = OH_ArkUI_PointerEvent_GetWindowY(input_event);
      if (context_ && context_->GetUIOwner()) {
        float point[2] = {window_x, window_y};
        context_->GetUIOwner()->UpdateRootTarget(this);
        is_consume_event_ = context_->GetUIOwner()->CanConsumeTouchEvent(point);
      }
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_TOUCH_EVENT) {
      if (is_consume_event_) {
        context_->OnTouchEvent(OH_ArkUI_NodeEvent_GetInputEvent(event), this);
      }
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_ATTACH) {
      is_root_attached_ = true;
      context_->ResumeExposure();
    } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_DETACH) {
      is_root_attached_ = false;
      auto dict = lepus::Dictionary::Create();
      dict->SetValue("sendEvent", false);
      context_->StopExposure(lepus::Value(dict));
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
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
