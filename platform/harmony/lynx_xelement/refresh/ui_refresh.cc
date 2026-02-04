// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/refresh/ui_refresh.h"

#include <string>
#include <utility>

#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIRefreshHeader::UIRefreshHeader(LynxContext* context, int sign,
                                 const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

UIRefreshHeader::~UIRefreshHeader() { refresh_view_ = nullptr; }

void UIRefreshHeader::FrameDidChanged() {
  NodeManager::Instance().SetAttributeWithNumberValue(node_, NODE_WIDTH,
                                                      width_);
  NodeManager::Instance().SetAttributeWithNumberValue(node_, NODE_HEIGHT,
                                                      height_);
  if (refresh_view_ != nullptr) {
    refresh_view_->UpdateRefreshViewHeight();
  }
}

void UIRefresh::InsertNode(UIBase* child, int index) {
  const auto& tag = child->Tag();
  if (tag == "refresh-header") {
    ArkUI_AttributeItem item = {.object = child->DrawNode()};
    NodeManager::Instance().SetAttribute(Node(), NODE_REFRESH_CONTENT, &item);
    if (!enable_refresh_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          child->DrawNode(), NODE_VISIBILITY,
          static_cast<int32_t>(ARKUI_VISIBILITY_HIDDEN));
    }
    refresh_header_ = child;
    static_cast<UIRefreshHeader*>(child)->refresh_view_ = this;
  } else {
    NodeManager::Instance().InsertNode(refresh_container_, child->DrawNode(),
                                       0);
  }
}

void UIRefresh::RemoveNode(UIBase* child) {
  if (child->Tag() == "refresh-header") {
    ArkUI_AttributeItem item = {.object = nullptr};
    NodeManager::Instance().SetAttribute(Node(), NODE_REFRESH_CONTENT, &item);
    refresh_header_ = nullptr;
  } else {
    NodeManager::Instance().RemoveNode(refresh_container_, child->DrawNode());
  }
}

UIRefresh::UIRefresh(LynxContext* context, int sign, const std::string& tag)
    : UIView(context, ARKUI_NODE_REFRESH, sign, tag) {
  overflow_ = {false, false};
  refresh_container_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);

  NodeManager::Instance().InsertNode(Node(), refresh_container_, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_REFRESH_PULL_DOWN_RATIO, 0.5);

  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_TOUCH_EVENT,
                                            NODE_TOUCH_EVENT, this);
  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_REFRESH_ON_REFRESH,
                                            NODE_REFRESH_ON_REFRESH, this);
  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_REFRESH_STATE_CHANGE,
                                            NODE_REFRESH_STATE_CHANGE, this);
  NodeManager::Instance().RegisterNodeEvent(
      Node(), NODE_REFRESH_ON_OFFSET_CHANGE, NODE_REFRESH_ON_OFFSET_CHANGE,
      this);
}

UIRefresh::~UIRefresh() {
  NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_TOUCH_EVENT);
  NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_REFRESH_ON_REFRESH);
  NodeManager::Instance().UnregisterNodeEvent(Node(),
                                              NODE_REFRESH_STATE_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(Node(),
                                              NODE_REFRESH_ON_OFFSET_CHANGE);
  NodeManager::Instance().DisposeNode(refresh_container_);
  refresh_header_ = nullptr;
}

void UIRefresh::UpdateRefreshViewHeight() {
  if (refresh_header_ != nullptr) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        Node(), NODE_REFRESH_OFFSET, refresh_header_->height_);
  }
}

void UIRefresh::OnPropUpdate(const std::string& name,
                             const lepus::Value& value) {
  UIView::OnPropUpdate(name, value);
  if (name == "enable-refresh") {
    enable_refresh_ = value.Bool();
    if (!enable_refresh_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          Node(), NODE_REFRESH_REFRESHING, 0);
    }

    for (auto child : Children()) {
      if (child->Tag() == "refresh-header") {
        NodeManager::Instance().SetAttributeWithNumberValue(
            child->DrawNode(), NODE_VISIBILITY,
            static_cast<int32_t>(enable_refresh_ ? ARKUI_VISIBILITY_VISIBLE
                                                 : ARKUI_VISIBILITY_HIDDEN));
        break;
      }
    }
  } else if (name == "harmony-pull-down-ratio") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        Node(), NODE_REFRESH_PULL_DOWN_RATIO, value.Number());
  }
}

std::unordered_map<std::string, UIRefresh::UIMethod>
    UIRefresh::ui_refresh_method_map_ = {
        {"finishRefresh", &UIRefresh::FinishRefresh},
        {"autoStartRefresh", &UIRefresh::AutoStartRefresh}};

void UIRefresh::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = ui_refresh_method_map_.find(method);
      it != ui_refresh_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIRefresh::FinishRefresh(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_REFRESH_REFRESHING, 0);
  callback(LynxGetUIResult::SUCCESS, lepus::Value());
}

void UIRefresh::AutoStartRefresh(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (enable_refresh_) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        Node(), NODE_REFRESH_REFRESHING, 1);
  }
  callback(LynxGetUIResult::SUCCESS, lepus::Value());

  // legacy API
  OnHeaderReleased();
}

void UIRefresh::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIView::OnNodeEvent(event);
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);

  if (type == NODE_TOUCH_EVENT) {
    auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
    const auto action = OH_ArkUI_UIInputEvent_GetAction(input_event);
    action_last_loop_ = action;
    switch (action) {
      case UI_TOUCH_EVENT_ACTION_UP: {
        const auto& vm = context_->VSyncMonitor();
        if (vm) {
          vm->AsyncRequestVSync(
              [weak_self = weak_from_this()](int64_t, int64_t) {
                auto self = weak_self.lock();
                if (!self) {
                  return;
                }
                auto ui_refresh = std::static_pointer_cast<UIRefresh>(self);
                ui_refresh->action_last_loop_ = UI_TOUCH_EVENT_ACTION_CANCEL;
              });
        }
        if (refresh_state_ == 2) {
          OnRefreshStateChanged(LYNX_UI_REFRESH_STATE_OVER_DRAG_RELEASE);
        }
      } break;
      default:
        break;
    }
  } else if (type == NODE_REFRESH_ON_REFRESH) {
    if (!enable_refresh_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          Node(), NODE_REFRESH_REFRESHING, 0);
    } else {
      auto dict = lepus::Dictionary::Create();
      dict->SetValue("isManual", action_last_loop_ == UI_TOUCH_EVENT_ACTION_UP);
      CustomEvent custom_event{Sign(), "startrefresh", "detail",
                               lepus_value(dict)};
      context_->SendEvent(custom_event);
    }
  } else if (type == NODE_REFRESH_STATE_CHANGE) {
    auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
    refresh_state_ = component_event->data[0].i32;

    if (refresh_state_ == 3) {
      OnRefreshStateChanged(LYNX_UI_REFRESH_STATE_REFRESHING);
      // legacy API
      OnHeaderReleased();
    } else if (refresh_state_ == 0 || refresh_state_ == 4) {
      OnRefreshStateChanged(LYNX_UI_REFRESH_STATE_IDLE);
    }
  } else if (type == NODE_REFRESH_ON_OFFSET_CHANGE) {
    GestureRecognized();
    context_->NotifyUIScroll();
    auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
    refresh_offset_ = component_event->data[0].f32;
    OnRefreshOffsetChanged();
  }
}

void UIRefresh::OnRefreshStateChanged(LYNX_UI_REFRESH_STATE state) const {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("state", state);
  CustomEvent event{Sign(), "refreshstatechange", "detail", lepus_value(dict)};
  context_->SendEvent(event);
}

void UIRefresh::OnHeaderReleased() const {
  // legacy API
  auto dict = lepus::Dictionary::Create();
  CustomEvent e{Sign(), "headerreleased", "detail", lepus_value(dict)};
  context_->SendEvent(e);
}

void UIRefresh::OnRefreshOffsetChanged() const {
  auto dict = lepus::Dictionary::Create();
  // 64 is the default range set by HarmonyOS
  const double total_distance =
      refresh_header_ == nullptr ? 64 : refresh_header_->height_;
  dict->SetValue("offsetPercent",
                 total_distance > 0 ? refresh_offset_ / total_distance : 0);

  dict->SetValue("isDragging", refresh_state_ == 1 || refresh_state_ == 2);
  CustomEvent event{Sign(), "headeroffset", "detail", lepus_value(dict)};
  context_->SendEvent(event);

  // legacy API
  CustomEvent legacyEvent{Sign(), "headershow", "detail", lepus_value(dict)};
  context_->SendEvent(legacyEvent);
}

bool UIRefresh::IsScrollable() { return true; }

float UIRefresh::ScrollY() { return -refresh_offset_; }

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
