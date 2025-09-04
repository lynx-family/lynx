// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {
// NODE_SCROLL_EVENT_ON_SCROLL,      NODE_SCROLL_EVENT_ON_SCROLL_START,
//     NODE_SCROLL_EVENT_ON_SCROLL_STOP, NODE_SCROLL_EVENT_ON_SCROLL_EDGE,
//     NODE_SCROLL_EVENT_ON_WILL_SCROLL,
LynxBaseScrollView::LynxBaseScrollView(LynxBaseScrollViewDelegate* delegate)
    : delegate_(delegate) {
  node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_SCROLL);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_BAR_DISPLAY_MODE,
      static_cast<int>(ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF));

  NodeManager::Instance().AddNodeEventReceiver(
      node_, LynxBaseScrollView::EventReceiver);

  NodeManager::Instance().RegisterNodeEvent(node_, NODE_SCROLL_EVENT_ON_SCROLL,
                                            NODE_SCROLL_EVENT_ON_SCROLL, this);
  NodeManager::Instance().RegisterNodeEvent(
      node_, NODE_SCROLL_EVENT_ON_SCROLL_START,
      NODE_SCROLL_EVENT_ON_SCROLL_START, this);
  NodeManager::Instance().RegisterNodeEvent(
      node_, NODE_SCROLL_EVENT_ON_SCROLL_STOP, NODE_SCROLL_EVENT_ON_SCROLL_STOP,
      this);
  NodeManager::Instance().RegisterNodeEvent(
      node_, NODE_SCROLL_EVENT_ON_SCROLL_EDGE, NODE_SCROLL_EVENT_ON_SCROLL_EDGE,
      this);
  NodeManager::Instance().RegisterNodeEvent(
      node_, NODE_SCROLL_EVENT_ON_WILL_SCROLL, NODE_SCROLL_EVENT_ON_WILL_SCROLL,
      this);
  NodeManager::Instance().RegisterNodeEvent(
      node_, NODE_SCROLL_EVENT_ON_DID_SCROLL, NODE_SCROLL_EVENT_ON_DID_SCROLL,
      this);
  NodeManager::Instance().RegisterNodeEvent(node_, NODE_TOUCH_EVENT,
                                            NODE_TOUCH_EVENT, this);

  scroll_content_ = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);
  NodeManager::Instance().InsertNode(node_, scroll_content_, 0);
}

LynxBaseScrollView::~LynxBaseScrollView() {
  NodeManager::Instance().UnregisterNodeEvent(node_,
                                              NODE_SCROLL_EVENT_ON_SCROLL);
  NodeManager::Instance().UnregisterNodeEvent(
      node_, NODE_SCROLL_EVENT_ON_SCROLL_START);
  NodeManager::Instance().UnregisterNodeEvent(node_,
                                              NODE_SCROLL_EVENT_ON_SCROLL_STOP);
  NodeManager::Instance().UnregisterNodeEvent(node_,
                                              NODE_SCROLL_EVENT_ON_SCROLL_EDGE);
  NodeManager::Instance().UnregisterNodeEvent(node_,
                                              NODE_SCROLL_EVENT_ON_WILL_SCROLL);
  NodeManager::Instance().UnregisterNodeEvent(node_,
                                              NODE_SCROLL_EVENT_ON_DID_SCROLL);
  NodeManager::Instance().UnregisterNodeEvent(node_, NODE_TOUCH_EVENT);
  NodeManager::Instance().DisposeNode(node_);
}

void LynxBaseScrollView::OnScroll(ArkUI_NodeEvent* event) {
  if (dragging_) {
    TryToUpdateScrollState(
        LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateDragging);
  } else if (scroll_state_ == LynxBaseScrollViewScrollState::
                                  LynxBaseScrollViewScrollStateDragging) {
    TryToUpdateScrollState(
        LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateFling);
  }
}

void LynxBaseScrollView::OnScrollStart(ArkUI_NodeEvent* event) {}

void LynxBaseScrollView::OnScrollStop(ArkUI_NodeEvent* event) {
  TryToUpdateScrollState(
      LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateIdle);
}

void LynxBaseScrollView::OnScrollEdge(ArkUI_NodeEvent* event) {}

void LynxBaseScrollView::OnWillScroll(ArkUI_NodeEvent* event) {}

void LynxBaseScrollView::OnDidScroll(ArkUI_NodeEvent* event) {
  if (delegate_ != nullptr) {
    delegate_->ScrollViewDidScroll();
  }
}

void LynxBaseScrollView::OnTouchEvent(ArkUI_NodeEvent* event) {
  const auto* touch = OH_ArkUI_NodeEvent_GetInputEvent(event);
  auto action = OH_ArkUI_UIInputEvent_GetAction(touch);
  switch (action) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      dragging_ = true;
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      dragging_ = false;
      break;
    default:
      break;
  }
}

void LynxBaseScrollView::TryToUpdateScrollState(
    LynxBaseScrollViewScrollState newState) {
  LynxBaseScrollViewScrollState oldState = scroll_state_;
  if (oldState != newState) {
    scroll_state_ = newState;
    if (delegate_ != nullptr) {
      delegate_->OnScrollStateChanged(oldState, newState);
    }
  }
}

void LynxBaseScrollView::EventReceiver(ArkUI_NodeEvent* event) {
  auto* scroll = reinterpret_cast<LynxBaseScrollView*>(
      OH_ArkUI_NodeEvent_GetUserData(event));
  if (scroll != nullptr) {
    switch (OH_ArkUI_NodeEvent_GetEventType(event)) {
      case NODE_SCROLL_EVENT_ON_SCROLL:
        scroll->OnScroll(event);
        break;
      case NODE_SCROLL_EVENT_ON_SCROLL_START:
        scroll->OnScrollStart(event);
        break;
      case NODE_SCROLL_EVENT_ON_SCROLL_STOP:
        scroll->OnScrollStop(event);
        break;
      case NODE_SCROLL_EVENT_ON_SCROLL_EDGE:
        scroll->OnScrollEdge(event);
        break;
      case NODE_SCROLL_EVENT_ON_WILL_SCROLL:
        scroll->OnWillScroll(event);
      case NODE_SCROLL_EVENT_ON_DID_SCROLL:
        scroll->OnDidScroll(event);
        break;
      case NODE_TOUCH_EVENT:
        scroll->OnTouchEvent(event);
        break;
      default:
        break;
    }
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
