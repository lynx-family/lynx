// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

#include <memory>

#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
namespace lynx {
namespace tasm {
namespace harmony {
UIRoot::UIRoot(LynxContext* context, int sign, const std::string& tag)
    : UIView(context, ARKUI_NODE_STACK, sign, tag) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_ROOT_CONSTRUCTOR);
  root_proxy_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);
  normal_sibling_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);
  transparent_sibling_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);

  NodeManager::Instance().InsertNode(root_proxy_, normal_sibling_, 0);
  NodeManager::Instance().InsertNode(root_proxy_, transparent_sibling_, 1);
  NodeManager::Instance().InsertNode(normal_sibling_, node_, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(
      transparent_sibling_, NODE_HIT_TEST_BEHAVIOR,
      static_cast<int32_t>(ARKUI_HIT_TEST_MODE_TRANSPARENT));

  NodeManager::Instance().AddNodeEventReceiver(transparent_sibling_,
                                               UIBase::EventReceiver);
  NodeManager::Instance().AddNodeEventReceiver(root_proxy_,
                                               UIBase::EventReceiver);
  NodeManager::Instance().RegisterNodeEvent(transparent_sibling_,
                                            NODE_TOUCH_EVENT, 0, this);
  NodeManager::Instance().RegisterNodeEvent(normal_sibling_, NODE_TOUCH_EVENT,
                                            0, this);
  NodeManager::Instance().RegisterNodeEvent(root_proxy_,
                                            NODE_ON_TOUCH_INTERCEPT, 0, this);
  NodeManager::Instance().RegisterNodeEvent(root_proxy_, NODE_EVENT_ON_ATTACH,
                                            0, this);
  NodeManager::Instance().RegisterNodeEvent(root_proxy_, NODE_EVENT_ON_DETACH,
                                            0, this);
}

UIRoot::~UIRoot() {
  NodeManager::Instance().RemoveNodeEventReceiver(transparent_sibling_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().UnregisterNodeEvent(transparent_sibling_,
                                              NODE_TOUCH_EVENT);
  NodeManager::Instance().UnregisterNodeEvent(normal_sibling_,
                                              NODE_TOUCH_EVENT);
  NodeManager::Instance().UnregisterNodeEvent(root_proxy_,
                                              NODE_ON_TOUCH_INTERCEPT);
  NodeManager::Instance().UnregisterNodeEvent(root_proxy_,
                                              NODE_EVENT_ON_ATTACH);
  NodeManager::Instance().UnregisterNodeEvent(root_proxy_,
                                              NODE_EVENT_ON_DETACH);
  NodeManager::Instance().DisposeNode(root_proxy_);
  NodeManager::Instance().DisposeNode(normal_sibling_);
  NodeManager::Instance().DisposeNode(transparent_sibling_);
}

void UIRoot::UpdateLayout(float left, float top, float width, float height,
                          const float* paddings, const float* margins,
                          const float* sticky, float max_height,
                          uint32_t node_index) {
  UIBase::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  NodeManager::Instance().SetAttributeWithNumberValue(root_proxy_, NODE_WIDTH,
                                                      width);
  NodeManager::Instance().SetAttributeWithNumberValue(root_proxy_, NODE_HEIGHT,
                                                      height);
  NodeManager::Instance().SetAttributeWithNumberValue(root_proxy_,
                                                      NODE_POSITION, left, top);
  NodeManager::Instance().SetAttributeWithNumberValue(normal_sibling_,
                                                      NODE_WIDTH, width);
  NodeManager::Instance().SetAttributeWithNumberValue(normal_sibling_,
                                                      NODE_HEIGHT, height);
  NodeManager::Instance().SetAttributeWithNumberValue(transparent_sibling_,
                                                      NODE_WIDTH, width);
  NodeManager::Instance().SetAttributeWithNumberValue(transparent_sibling_,
                                                      NODE_HEIGHT, height);
  UIBase::RequestLayout();
}

void UIRoot::OnNodeEvent(ArkUI_NodeEvent* event) {
  if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_ATTACH) {
    is_root_attached_ = true;
    context_->ResumeExposure();
  } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_DETACH) {
    is_root_attached_ = false;
    auto dict = lepus::Dictionary::Create();
    dict->SetValue("sendEvent", false);
    context_->StopExposure(lepus::Value(dict));
  } else if (OH_ArkUI_NodeEvent_GetEventType(event) ==
             NODE_ON_TOUCH_INTERCEPT) {
    context_->OnTouchEvent(OH_ArkUI_NodeEvent_GetInputEvent(event),
                           context_->Root());
    if (context_->EventThrough()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          transparent_sibling_, NODE_HIT_TEST_BEHAVIOR,
          static_cast<int32_t>(ARKUI_HIT_TEST_MODE_DEFAULT));
    }
  } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_TOUCH_EVENT) {
    ArkUI_UIInputEvent* touch_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
    if (context_->ShouldBlockNativeEvent()) {
      OH_ArkUI_PointerEvent_SetStopPropagation(touch_event, true);
    }
    if (OH_ArkUI_NodeEvent_GetNodeHandle(event) != transparent_sibling_) {
      return;
    }
    auto touch_action = OH_ArkUI_UIInputEvent_GetAction(touch_event);
    if (touch_action == UI_TOUCH_EVENT_ACTION_UP ||
        touch_action == UI_TOUCH_EVENT_ACTION_CANCEL) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          transparent_sibling_, NODE_HIT_TEST_BEHAVIOR,
          static_cast<int32_t>(ARKUI_HIT_TEST_MODE_TRANSPARENT));
    }
    if (touch_action == UI_TOUCH_EVENT_ACTION_DOWN ||
        context_->EventThrough()) {
      return;
    }
    context_->OnTouchEvent(touch_event, this);
  }
}

ArkUI_NodeHandle UIRoot::GetProxyNode() { return root_proxy_; }

void UIRoot::AttachToNodeContent(NativeNodeContent* content) {
  node_content_ = std::unique_ptr<NativeNodeContent>(content);
  // FIXME(chengjunnan): adpat to draw node.
  OH_ArkUI_NodeContent_AddNode(content->NodeContentHandle(), root_proxy_);
  node_content_->SetUI(this);
}

bool UIRoot::IsRoot() { return true; }

bool UIRoot::IsVisible() { return is_root_attached_ && UIBase::IsVisible(); }

void UIRoot::OnNodeReady() {
  UIBase::OnNodeReady();
  if (!are_gestures_attached_) {
    context_->AttachGesturesToRoot(this);
    are_gestures_attached_ = true;
  }
}

void UIRoot::GetOffsetToScreen(float offset_screen[2]) {
  ArkUI_IntOffset page_offset;
  OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root_proxy_,
                                                      &page_offset);
  float scaled_density = context_->ScaledDensity();
  offset_screen[0] = page_offset.x / scaled_density;
  offset_screen[1] = page_offset.y / scaled_density;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
