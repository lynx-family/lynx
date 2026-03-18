// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/viewpager/ui_viewpager.h"

#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <cmath>
#include <string>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

const int PAGER_CHANGE_EPSILON = 2;

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

bool IsViewPagerItemTag(const std::string& tag) {
  return tag == "viewpager-item" || tag == "x-viewpager-item-ng";
}

}  // namespace

UIBase* UIViewPager::Make(LynxContext* context, int sign,
                          const std::string& tag) {
  return new UIViewPager(context, ARKUI_NODE_CUSTOM, sign, tag);
}

void UIViewPager::OnPropUpdate(const std::string& name,
                               const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (name == "initial-select-index") {
    initial_select_index_ = static_cast<int32_t>(value.Number());
  } else if (name == "select-index") {
    index_after_reload_ = static_cast<int32_t>(value.Number());
  } else if (name == "enable-scroll" || name == "allow-horizontal-gesture") {
    enable_scroll_ = value.Bool();
    NodeManager::Instance().SetAttributeWithNumberValue(
        scroll_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION, enable_scroll_ ? 1 : 0);
  } else if (name == "enable-nested-scroll") {
    enable_nested_scroll_ = value.Bool();
    if (!enable_nested_scroll_) {
      EnableNestedScroll(false);
    }
  } else if (name == "bounces") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        scroll_, NODE_SCROLL_EDGE_EFFECT,
        static_cast<int32_t>(value.Bool() ? ARKUI_EDGE_EFFECT_SPRING
                                          : ARKUI_EDGE_EFFECT_NONE));
  }
}

UIViewPager::~UIViewPager() {
  NodeManager::Instance().UnregisterNodeCustomEvent(
      Node(), ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
  NodeManager::Instance().UnregisterNodeCustomEvent(
      Node(), ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT);
  NodeManager::Instance().RemoveNodeCustomEventReceiver(
      Node(), UIBase::CustomEventReceiver);
  NodeManager::Instance().UnregisterNodeEvent(scroll_,
                                              NODE_SCROLL_EVENT_ON_SCROLL);
  NodeManager::Instance().UnregisterNodeEvent(
      scroll_, NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN);
  NodeManager::Instance().UnregisterNodeEvent(scroll_, NODE_TOUCH_EVENT);
  NodeManager::Instance().UnregisterNodeEvent(scroll_,
                                              NODE_SCROLL_EVENT_ON_SCROLL_STOP);
  NodeManager::Instance().RemoveNodeEventReceiver(scroll_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().DisposeNode(scroll_);
  NodeManager::Instance().DisposeNode(row_);
}

void UIViewPager::InsertNode(UIBase* child, int index) {
  if (IsViewPagerItemTag(child->Tag())) {
    NodeManager::Instance().InsertNode(row_, child->DrawNode(), index);
  } else {
    LOGE(
        "viewpager can only have viewpager-item/x-viewpager-item-ng as its "
        "child !");
  }
  layout_changed_ = true;
  should_reload_data_ = true;
}

void UIViewPager::RemoveNode(UIBase* child) {
  NodeManager::Instance().RemoveNode(row_, child->DrawNode());
  layout_changed_ = true;
  should_reload_data_ = true;
}

void UIViewPager::UpdateLayout(float left, float top, float width, float height,
                               const float* paddings, const float* margins,
                               const float* sticky, float max_height,
                               uint32_t node_index) {
  if (base::FloatsNotEqual(width, width_)) {
    viewpager_width_changed_with_marked_index_ = index_;
  }
  UIBase::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  layout_changed_ = true;
}

void UIViewPager::FinishLayoutOperation() {
  UIBase::FinishLayoutOperation();
  if (layout_changed_) {
    NodeManager::Instance().SetAttributeWithNumberValue(scroll_, NODE_WIDTH,
                                                        width_);
    NodeManager::Instance().SetAttributeWithNumberValue(scroll_, NODE_HEIGHT,
                                                        height_);
    NodeManager::Instance().RequestLayout(scroll_);
    NodeManager::Instance().SetAttributeWithNumberValue(
        row_, NODE_WIDTH, width_ * Children().size());
    NodeManager::Instance().SetAttributeWithNumberValue(row_, NODE_HEIGHT,
                                                        height_);
    int index = 0;
    for (const auto child : Children()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          child->DrawNode(), NODE_POSITION, width_ * index++, 0);
    }
  }

  int target_index = -1;

  if (viewpager_width_changed_with_marked_index_ != -1) {
    target_index = viewpager_width_changed_with_marked_index_;
    viewpager_width_changed_with_marked_index_ = -1;
  }

  // legacy API compatible
  if (should_reload_data_ && index_after_reload_ != -1) {
    should_reload_data_ = false;
    target_index = index_after_reload_;
  }

  if (first_render_) {
    target_index = initial_select_index_;
  }

  if (target_index != -1) {
    ScrollToIndex(target_index, false);
  }
}

void UIViewPager::OnNodeReady() {
  UIBase::OnNodeReady();
  // RequestLayout to trigger onLayout and execute the internal scroll logic
  RequestLayout();
}

bool UIViewPager::IsScrollable() { return true; }

float UIViewPager::ScrollX() {
  return NodeManager::Instance().GetAttribute<float>(scroll_,
                                                     NODE_SCROLL_OFFSET, 0);
}

void UIViewPager::ScrollToIndex(int32_t index, bool animated) {
  if (layout_changed_) {
    pending_scroll_index_ = index;
    pending_scroll_animated_ = animated;
    // It is required to start scroll after onLayout
    return;
  }
  tab_was_selected_before_touch_ = true;
  ArkUI_AttributeItem item;
  ArkUI_NumberValue value[] = {
      {.f32 = index * width_},
      {.f32 = 0.0},
      {.i32 = animated ? 250 : 0},
      {.i32 = ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH},
      {.i32 = 0}};
  item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  NodeManager::Instance().SetAttribute(this->scroll_, NODE_SCROLL_OFFSET,
                                       &item);
}

int32_t UIViewPager::ClampIndex(int32_t index) const {
  if (index > Children().size() - 1) {
    index = Children().size() - 1;
  }
  if (index < 0) {
    index = 0;
  }
  return index;
}

std::unordered_map<std::string, UIViewPager::UIMethod>
    UIViewPager::viewpager_ui_method_map_ = {
        {"selectTab", &UIViewPager::SelectTab},
};

void UIViewPager::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = viewpager_ui_method_map_.find(method);
      it != viewpager_ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIViewPager::SelectTab(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (args.IsTable()) {
    int32_t index = 0;
    bool smooth = true;

    const auto params = args.Table();
    if (params->Contains("index")) {
      index = static_cast<int32_t>(args.Table()->GetValue("index").Number());
    } else {
      const auto ret = lepus::Dictionary::Create();
      ret->SetValue("err", "index is not assigned");
      callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
      return;
    }
    if (index < 0 || index >= Children().size()) {
      const auto ret = lepus::Dictionary::Create();
      std::stringstream msg;
      msg << "index out of range [0, " << Children().size() << ")";
      ret->SetValue("err", msg.str());
      callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
      return;
    }
    if (params->Contains("smooth")) {
      smooth = args.Table()->GetValue("smooth").Bool();
    }

    const auto event_params = lepus::Dictionary::Create();
    event_params->SetValue("index", index);
    event_params->SetValue("isDragged", false);
    CustomEvent event{Sign(), "willchange", "detail",
                      lepus_value(event_params)};
    context_->SendEvent(event);

    if (index != index_) {
      emit_target_changes_only_during_scroll_ = index;
    }
    ScrollToIndex(index, smooth);
    callback(LynxGetUIResult::SUCCESS, lepus::Value());
  } else {
    const auto ret = lepus::Dictionary::Create();
    ret->SetValue("err", "index is not assigned");
    callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
  }
}

UIViewPager::UIViewPager(LynxContext* context, ArkUI_NodeType type, int sign,
                         const std::string& tag)
    : UIBase(context, type, sign, tag) {
  scroll_ = NodeManager::Instance().CreateNode(ARKUI_NODE_SCROLL);

  NodeManager::Instance().SetAttributeWithNumberValue(
      scroll_, NODE_SCROLL_SCROLL_DIRECTION,
      static_cast<int32_t>(ARKUI_SCROLL_DIRECTION_HORIZONTAL));
  NodeManager::Instance().SetAttributeWithNumberValue(
      scroll_, NODE_SCROLL_BAR_DISPLAY_MODE,
      static_cast<int32_t>(ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF));
  NodeManager::Instance().SetAttributeWithNumberValue(
      scroll_, NODE_SCROLL_ENABLE_PAGING, 1);
  NodeManager::Instance().AddNodeEventReceiver(scroll_, UIBase::EventReceiver);
  row_ = NodeManager::Instance().CreateNode(ARKUI_NODE_ROW);
  NodeManager::Instance().AddNodeCustomEventReceiver(
      node_, UIBase::CustomEventReceiver);
  NodeManager::Instance().RegisterNodeCustomEvent(
      node_, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE,
      ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, this);

  NodeManager::Instance().RegisterNodeCustomEvent(
      node_, ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT,
      ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT, this);

  NodeManager::Instance().InsertNode(Node(), scroll_, 0);

  NodeManager::Instance().InsertNode(scroll_, row_, 0);
  NodeManager::Instance().RegisterNodeEvent(
      scroll_, NODE_SCROLL_EVENT_ON_SCROLL, NODE_SCROLL_EVENT_ON_SCROLL, this);
  NodeManager::Instance().RegisterNodeEvent(
      scroll_, NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN,
      NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN, this);
  NodeManager::Instance().RegisterNodeEvent(scroll_, NODE_TOUCH_EVENT,
                                            NODE_TOUCH_EVENT, this);
  NodeManager::Instance().RegisterNodeEvent(
      scroll_, NODE_SCROLL_EVENT_ON_SCROLL_STOP,
      NODE_SCROLL_EVENT_ON_SCROLL_STOP, this);
}

void UIViewPager::EnableNestedScroll(bool enable) const {
  ArkUI_ScrollNestedMode mode = enable ? ARKUI_SCROLL_NESTED_MODE_SELF_FIRST
                                       : ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
  NodeManager::Instance().SetAttributeWithNumberValue(
      scroll_, NODE_SCROLL_NESTED_SCROLL, static_cast<int32_t>(mode),
      static_cast<int32_t>(mode));
}

void UIViewPager::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIBase::OnNodeEvent(event);
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  if (type == NODE_TOUCH_EVENT) {
    const auto* touch = OH_ArkUI_NodeEvent_GetInputEvent(event);
    auto action = OH_ArkUI_UIInputEvent_GetAction(touch);
    switch (action) {
      case UI_TOUCH_EVENT_ACTION_DOWN:
        emit_target_changes_only_during_scroll_ = -1;
        last_offset_on_touch_up_ = -1;
        tab_was_selected_before_touch_ = false;
        if (enable_nested_scroll_) {
          // ArkUI can not handle nested fling properly，enable nested scroll
          // only while dragging to workaround。
          EnableNestedScroll(true);
        }
        break;
      case UI_TOUCH_EVENT_ACTION_MOVE:
        break;
      case UI_TOUCH_EVENT_ACTION_UP:
        last_offset_on_touch_up_ = NodeManager::Instance().GetAttribute<float>(
            scroll_, NODE_SCROLL_OFFSET, 0);
        break;
      case UI_TOUCH_EVENT_ACTION_CANCEL:
        last_offset_on_touch_up_ = NodeManager::Instance().GetAttribute<float>(
            scroll_, NODE_SCROLL_OFFSET, 0);
        break;
      default:
        break;
    }

  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL) {
    GestureRecognized();
    context_->NotifyUIScroll();
    const int current_offset = NodeManager::Instance().GetAttribute<float>(
        scroll_, NODE_SCROLL_OFFSET, 0);

    if (width_ > 0) {
      const float progress = 1.0 * current_offset / width_;
      const auto dict = lepus::Dictionary::Create();
      std::ostringstream stream;
      stream << std::fixed << std::setprecision(2) << progress;
      dict->SetValue("offset", stream.str());
      CustomEvent custom_event{Sign(), "offsetchange", "detail",
                               lepus_value(dict)};
      context_->SendEvent(custom_event);
      const int current_index = index_;
      int next_index = std::floor(progress);
      const int ceil_next_index = std::ceil(progress);

      if (last_offset_on_touch_up_ > 0 &&
          current_offset != last_offset_on_touch_up_) {
        const int will_change_index = current_offset > last_offset_on_touch_up_
                                          ? ceil_next_index
                                          : next_index;
        const auto param = lepus::Dictionary::Create();
        param->SetValue("index", will_change_index);
        param->SetValue("isDragged", true);
        custom_event =
            CustomEvent{Sign(), "willchange", "detail", lepus_value(param)};
        context_->SendEvent(custom_event);
        last_offset_on_touch_up_ = -1;
        if (will_change_index == 0 ||
            will_change_index == Children().size() - 1) {
          if (enable_nested_scroll_) {
            // ArkUI can not handle nested fling during pagination
            // properly，disable nested scroll if will page to border, and
            // enable it at next touch down
            EnableNestedScroll(false);
          }
        }
      }

      if (next_index < current_index) {
        if ((progress - next_index) * width_ < PAGER_CHANGE_EPSILON) {
        } else {
          next_index = ceil_next_index;
        }
      } else {
        if ((ceil_next_index - progress) * width_ < PAGER_CHANGE_EPSILON) {
          next_index = ceil_next_index;
        } else {
        }
      }

      if (next_index >= 0 && next_index < Children().size() &&
          next_index != index_) {
        index_ = next_index;
        if (emit_target_changes_only_during_scroll_ != -1 &&
            next_index != emit_target_changes_only_during_scroll_) {
        } else {
          const auto param = lepus::Dictionary::Create();
          param->SetValue("index", index_);
          param->SetValue("isDragged", !tab_was_selected_before_touch_);
          custom_event =
              CustomEvent{Sign(), "change", "detail", lepus_value(param)};
          context_->SendEvent(custom_event);
        }
      }
    }
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL_STOP) {
    emit_target_changes_only_during_scroll_ = -1;
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN) {
  }
}
void UIViewPager::OnLayout() {
  NodeManager::Instance().LayoutNode(scroll_, 0, 0);
  NodeManager::Instance().SetLayoutPosition(scroll_, 0, 0);
  layout_changed_ = false;
  first_render_ = false;
  if (pending_scroll_index_ != -1) {
    ScrollToIndex(pending_scroll_index_, pending_scroll_animated_);
    pending_scroll_index_ = -1;
  }
}

void UIViewPager::OnMeasure(ArkUI_LayoutConstraint* layout_constraint) {
  NodeManager::Instance().MeasureNode(scroll_, layout_constraint);
  NodeManager::Instance().SetMeasuredSize(Node(),
                                          context_->ScaledDensity() * width_,
                                          context_->ScaledDensity() * height_);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
