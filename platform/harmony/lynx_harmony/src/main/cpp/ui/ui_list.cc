// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_list.h"

#include <arkui/native_node.h>

#include <algorithm>
#include <limits>
#include <string>

#include "base/include/float_comparison.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/basic_animation/basic_animation.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/ui_component/list/list_container.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/auto_scroller.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIList::UIList(LynxContext* context, int sign, const std::string& tag)
    : BaseScrollContainer(context, sign, tag) {
  container_layout_ = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_ALIGNMENT, static_cast<int32_t>(ARKUI_ALIGNMENT_TOP_START));
  NodeManager::Instance().RegisterNodeEvent(node_, NODE_TOUCH_EVENT,
                                            NODE_TOUCH_EVENT, this);
  NodeManager::Instance().InsertNode(node_, container_layout_, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(node_, NODE_POSITION, 0,
                                                      0);
  // the receiver should be dispatched by the ui on the node_manager，but the
  // list should send the broadcast to the custom_node which is the child of
  // the list.
  NodeManager::Instance().AddNodeEventReceiver(container_layout_,
                                               UIBase::EventReceiver);
  NodeManager::Instance().AddNodeCustomEventReceiver(
      container_layout_, UIBase::CustomEventReceiver);

  for (auto eventType : LIST_NODE_EVENT_TYPES) {
    NodeManager::Instance().RegisterNodeEvent(node_, eventType, eventType,
                                              this);
  }
  NodeManager::Instance().RegisterNodeCustomEvent(
      container_layout_, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE,
      ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, this);
  auto_scroller_ = std::make_shared<AutoScroller>(this);
}

UIList::~UIList() {
  for (auto eventType : LIST_NODE_EVENT_TYPES) {
    NodeManager::Instance().UnregisterNodeEvent(node_, eventType);
  }
  NodeManager::Instance().UnregisterNodeCustomEvent(
      container_layout_, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
  NodeManager::Instance().UnregisterNodeEvent(node_, NODE_TOUCH_EVENT);

  NodeManager::Instance().RemoveNodeEventReceiver(container_layout_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().RemoveNodeCustomEventReceiver(
      container_layout_, UIBase::CustomEventReceiver);
  NodeManager::Instance().DisposeNode(container_layout_);
}

void UIList::UpdateProps(PropBundleHarmony* props) {
  BaseScrollContainer::UpdateProps(props);
  if (enable_list_sticky_ && update_sticky_for_diff_) {
    // Generate sticky top/bottom item key set.
    GenerateStickyItemKeySet(sticky_top_item_key_set_, sticky_top_indexes_);
    GenerateStickyItemKeySet(sticky_bottom_item_key_set_,
                             sticky_bottom_indexes_);
  }
}

void UIList::GenerateStickyItemKeySet(
    std::unordered_set<std::string>& sticky_item_key_set,
    const std::vector<int>& sticky_indexes) {
  sticky_item_key_set.clear();
  for (auto index : sticky_indexes) {
    if (index >= 0 && index < static_cast<int>(item_keys_.size())) {
      sticky_item_key_set.insert(item_keys_[index]);
    }
  }
}

void UIList::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  if (name == list::kListContainerInfo) {
    UpdateListContainerInfo(value);
  } else if (name == list::kListVerticalOrientation && value.IsBool()) {
    SetHorizontal(!value.Bool());
  } else if (name == list::kScrollOrientation && value.IsString()) {
    const auto& val = value.StdString();
    if (val == "vertical") {
      SetHorizontal(false);
    } else if (val == "horizontal") {
      SetHorizontal(true);
    } else {
      SetHorizontal(false);
    }
  } else if (name == list::kExperimentalRecycleStickyItem && value.IsBool()) {
    enable_recycle_sticky_item_ = value.Bool();
  } else if (name == list::kSticky && value.IsBool()) {
    enable_list_sticky_ = value.Bool();
  } else if (name == list::kStickyOffset && value.IsNumber()) {
    sticky_offset_ = value.Number();
  } else if (name == list::kExperimentalBatchRenderStrategy &&
             value.IsNumber()) {
    enable_batch_render_ = value.Number() > 0;
  } else if (name == list::kEnableScroll && value.IsBool()) {
    SetEnableScrollInteraction(value.Bool());
  } else if (name == list::kEnableNestedScroll && value.IsBool()) {
    SetNestedScroll(value.Bool());
  } else if (name == list::kEnableScrollBar) {
    SetScrollbar(value.Bool());
  } else if (name == list::kBounces && value.IsBool()) {
    SetBounces(value.Bool());
  } else if (name == list::kNeedVisibleItemInfo && value.IsBool()) {
    enable_need_visible_item_info = value.Bool();
  } else if (name == list::kExperimentalUpdateStickyForDiff && value.IsBool()) {
    update_sticky_for_diff_ = value.Bool();
  } else if (name == list::kItemSnap) {
    if (!value.IsTable() || value.Table()->size() == 0) {
      snap_factor_ = -1;
      // 0.6 is the default friction value.
      NodeManager::Instance().SetAttributeWithNumberValue(
          node_, NODE_SCROLL_FRICTION, 0.6f);
    } else {
      const auto params = value.Table();
      snap_factor_ = params->GetValue("factor").Number();
      snap_offset_ = params->GetValue("offset").Number();
      if (snap_factor_ < 0 || snap_factor_ > 1) {
        auto error = lynx::base::LynxError(
            error::E_COMPONENT_LIST_INVALID_PROPS_ARG,
            Tag() + " item-snap arguments invalid!",
            "The factor should be constrained to the range of [0,1].",
            base::LynxErrorLevel::Error);
        lynx::base::ErrorStorage::GetInstance().SetError(std::move(error));
        snap_factor_ = 0;
      }
      // Set the friction value to 1000.f (a big value) to avoid the fling
      // animation.
      NodeManager::Instance().SetAttributeWithNumberValue(
          node_, NODE_SCROLL_FRICTION, 1000.f);
    }

  } else {
    BaseScrollContainer::OnPropUpdate(name, value);
  }
}

void UIList::OnNodeReady() {
  BaseScrollContainer::OnNodeReady();
  std::pair result = GetScrollOffset();
  UpdateStickyTopView(result.second);
  UpdateStickyBottomView(result.second);
}

void UIList::AutoScrollStopped() {
  SendScrollEndEvent();
  SetScrollState(list::ScrollState::kIdle);
}

void UIList::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (method == "scrollToPosition") {
    int index{-1};
    float offset{0};
    bool smooth{false};
    int aline_to{0};
    if (!args.IsTable()) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("params is not object!"));
      return;
    }
    auto table = args.Table();
    for (const auto& [k, v] : *table) {
      if (k.IsEqual("position") && v.IsNumber()) {
        index = static_cast<int32_t>(v.Number());
      } else if (k.IsEqual("index") && v.IsNumber()) {
        index = static_cast<int32_t>(v.Number());
      } else if (k.IsEqual("offset") && v.IsNumber()) {
        offset = v.Number();
      } else if (k.IsEqual("smooth") && v.IsBool()) {
        smooth = v.Bool();
      } else if (k.IsEqual("alignTo") && v.IsString()) {
        const auto& v_str = v.StdString();
        if (v_str == "middle") {
          aline_to = 1;
        } else if (v_str == "bottom") {
          aline_to = 2;
        }
      }
    }

    scroll_callback_ = std::move(callback);
    ScrollToPosition(index, offset, aline_to, smooth);
    if (!smooth) {
      scroll_callback_(LynxGetUIResult::SUCCESS, lepus_value(""));
      scroll_callback_ = nullptr;
    }

  } else if (method == "autoScroll") {
    if (!args.IsTable()) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("params is not object!"));
      return;
    }
    float rate{0.f};
    bool start{false};
    bool auto_stop{true};

    auto table = args.Table();
    for (auto& [k, v] : *table) {
      if (k.IsEqual("rate")) {
        if (v.IsNumber()) {
          rate = v.Number();
        } else if (v.IsString()) {
          float screen_size[2] = {0};
          context_->ScreenSize(screen_size);
          rate = LynxUnitUtils::ToVPFromUnitValue(v.StdString(), screen_size[0],
                                                  context_->DevicePixelRatio());
        }
      } else if (k.IsEqual("start") && v.IsBool()) {
        start = v.Bool();
      } else if (k.IsEqual("autoStop") && v.IsBool()) {
        auto_stop = v.Bool();
      }
    }
    InvokeAutoScroll(rate, start, auto_stop, std::move(callback));
  } else if (method == "getVisibleCells") {
    auto array = GetVisibleCells();
    callback(LynxGetUIResult::SUCCESS, lepus_value(array));
  } else if (method == "scrollBy") {
    if (!args.IsTable()) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("params is not object!"));
      return;
    }
    float offset{0};
    auto table = args.Table();
    for (const auto& [k, v] : *table) {
      if (k.IsEqual("offset") && v.IsNumber()) {
        offset = v.Number();
      }
    }
    std::vector<float> result = BaseScrollContainer::ScrollBy(offset, offset);
    auto dictionary = lepus::Dictionary::Create();
    if (result.size() >= 4) {
      dictionary->SetValue("consumedX", result[0]);
      dictionary->SetValue("consumedY", result[1]);
      dictionary->SetValue("unconsumedX", result[2]);
      dictionary->SetValue("unconsumedY", result[3]);
    }
    callback(LynxGetUIResult::SUCCESS, lepus_value(std::move(dictionary)));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIList::UpdateListContainerInfo(const lepus::Value& params) {
  if (!params.IsTable()) {
    return;
  }
  auto table = params.Table();
  for (const auto& [k, v] : *table) {
    if (k.IsEqual(list::kDataSourceItemKeys)) {
      item_keys_.clear();
      if (!v.IsArray()) {
        continue;
      }
      auto& key_data = *v.Array();
      for (size_t i = 0; i < key_data.size(); ++i) {
        const auto& item_key = key_data.get(i);
        if (item_key.IsString()) {
          item_keys_.emplace_back(item_key.StdString());
        }
      }
    } else if (k.IsEqual(list::kDataSourceStickyBottom)) {
      sticky_bottom_indexes_.clear();
      if (!v.IsArray()) {
        continue;
      }
      const auto& sticky_bottom_array = *v.Array();
      for (size_t i = 0; i < sticky_bottom_array.size(); ++i) {
        auto& sticky_bottom_item = sticky_bottom_array.get(i);
        if (sticky_bottom_item.IsNumber()) {
          sticky_bottom_indexes_.emplace_back(
              static_cast<int32_t>(sticky_bottom_item.Number()));
        }
      }
    } else if (k.IsEqual(list::kDataSourceStickyTop)) {
      sticky_top_indexes_.clear();
      if (!v.IsArray()) {
        continue;
      }
      const auto& sticky_top_item_array = *v.Array();
      for (size_t i = 0; i < sticky_top_item_array.size(); ++i) {
        auto& sticky_top_item = sticky_top_item_array.get(i);
        if (sticky_top_item.IsNumber()) {
          sticky_top_indexes_.emplace_back(
              static_cast<int32_t>(sticky_top_item.Number()));
        }
      }
    }
  }
}

void UIList::AddChild(lynx::tasm::harmony::UIBase* child, int index) {
  if (index == -1) {
    children_.emplace_back(child);
  } else {
    children_.insert(children_.begin() + index, child);
  }
  child->SetParent(this);
}

void UIList::RemoveChild(lynx::tasm::harmony::UIBase* child) {
  child->SetParent(nullptr);
  NodeManager::Instance().RemoveNode(container_layout_, child->DrawNode());
  children_.erase(std::remove(children_.begin(), children_.end(), child),
                  children_.end());
  static_cast<UIComponent*>(child)->SetNodeReadyListener(nullptr);
}

void UIList::OnComponentNodeReady(UIComponent* ui_component) {
  // This callback is invoked by component's onNodeReady(), which means
  // component has valid item-key info, so handle component's item-key changed
  // for sticky.
  if (enable_list_sticky_ && update_sticky_for_diff_ && ui_component) {
    const auto& item_key = ui_component->item_key();
    if (sticky_top_item_key_set_.find(item_key) !=
        sticky_top_item_key_set_.end()) {
      // Update sticky top list item map.
      UpdateStickyItemMap(ui_component, sticky_top_item_map_);
    } else if (sticky_bottom_item_key_set_.find(item_key) !=
               sticky_bottom_item_key_set_.end()) {
      // Update sticky bottom list item map.
      UpdateStickyItemMap(ui_component, sticky_bottom_item_map_);
    } else {
      // Not sticky top or bottom list item, remove it from map.
      sticky_top_item_map_.erase(item_key);
      sticky_bottom_item_map_.erase(item_key);
    }
  }
}

void UIList::UpdateStickyItemMap(
    UIComponent* ui_component,
    std::unordered_map<std::string, UIComponent*>& sticky_item_map) {
  if (ui_component) {
    const auto& new_updated_item_key = ui_component->item_key();
    auto it = sticky_item_map.end();
    if ((it = sticky_item_map.find(new_updated_item_key)) !=
            sticky_item_map.end() &&
        it->second == ui_component) {
      // No need to update sticky item map.
      return;
    }
    for (it = sticky_item_map.begin(); it != sticky_item_map.end();) {
      const auto& item_key = it->first;
      UIComponent* component = it->second;
      if (component == ui_component && item_key != new_updated_item_key) {
        // Delete old and insert new <item-key, list-item> pair to finish
        // updating item-key .
        sticky_item_map.erase(it++);
        sticky_item_map.emplace(new_updated_item_key, ui_component);
        break;
      } else {
        ++it;
      }
    }
  }
}

void UIList::OnMeasure(ArkUI_LayoutConstraint* layout_constraint) {
  // set the layout_constraint for the container view.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_LIST_ON_MEASURE);
  int32_t max = std::numeric_limits<int32_t>::max();
  ArkUI_LayoutConstraint* constraint = OH_ArkUI_LayoutConstraint_Create();
  OH_ArkUI_LayoutConstraint_SetMinHeight(constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMaxHeight(constraint, max);
  OH_ArkUI_LayoutConstraint_SetMinWidth(constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMaxWidth(constraint, max);
  for (const auto child : children_) {
    NodeManager::Instance().MeasureNode(child->DrawNode(), constraint);
  }
  OH_ArkUI_LayoutConstraint_Dispose(constraint);
  NodeManager::Instance().SetMeasuredSize(
      container_layout_, context_->ScaledDensity() * content_width_,
      context_->ScaledDensity() * content_height_);
  if (base::FloatsLarger(pending_delta_offset_, 0)) {
    if (is_horizontal_) {
      ScrollToAsync(pending_delta_offset_, 0);
    } else {
      ScrollToAsync(0, pending_delta_offset_);
    }
    pending_delta_offset_ = 0;
  }
}

void UIList::ScrollToAsync(float delta_x, float delta_y) {
  const auto& vm = context_->VSyncMonitor();
  if (!vm) {
    return;
  }
  vm->AsyncRequestVSync(
      [weak_this = weak_from_this(), delta_x, delta_y](int64_t, int64_t) {
        auto shared_this = weak_this.lock();
        if (!shared_this) {
          return;
        }
        auto* list = static_cast<UIList*>(shared_this.get());
        auto offset = list->GetScrollOffset();
        list->ScrollTo(offset.first + delta_x, offset.second + delta_y, false);
      });
}

std::vector<float> UIList::ScrollBy(float delta_x, float delta_y) {
  SetScrollState(list::ScrollState::kScrollAnimation);
  std::vector<float> result = BaseScrollContainer::ScrollBy(delta_x, delta_y);
  SetScrollState(list::ScrollState::kIdle);
  return result;
}

void UIList::OnNodeEvent(ArkUI_NodeEvent* event) {
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  if (type == NODE_SCROLL_EVENT_ON_SCROLL_START) {
    HandleScrollStartEvent();
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL) {
    GestureRecognized();
    // Handle scroll offset when scroll_state is ARKUI_SCROLL_STATE_IDLE.
    HandleScrollEvent();
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL_STOP) {
    HandleScrollStopEvent();
  } else if (type == NODE_SCROLL_EVENT_ON_WILL_SCROLL) {
    // Handle scroll offset when scroll_state is ARKUI_SCROLL_STATE_SCROLL or
    // ARKUI_SCROLL_STATE_FLING.
    auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
    if (IsEnableNewGesture() && !consume_gesture_) {
      component_event->data[0].f32 = 0.f;
      component_event->data[1].f32 = 0.f;
    }
    HandleWillScrollEvent(component_event);

  } else if (type == NODE_TOUCH_EVENT) {
    DetectSnapScroll(OH_ArkUI_UIInputEvent_GetAction(
        OH_ArkUI_NodeEvent_GetInputEvent(event)));
  } else {
    UIBase::OnNodeEvent(event);
  }
}

bool UIList::IsListItem(UIBase* list_item) const {
  if (list_item != nullptr &&
      (list_item->Tag() == "component" || list_item->Tag() == "list-item")) {
    return true;
  }
  return false;
}

void UIList::DetectSnapScroll(int32_t action) {
  if (snap_factor_ != -1) {
    switch (action) {
      case UI_TOUCH_EVENT_ACTION_DOWN:
      case UI_TOUCH_EVENT_ACTION_MOVE:
        last_scroll_offset_ = GetScrollOffset();
        break;
      case UI_TOUCH_EVENT_ACTION_UP: {
        auto scroll_offset = GetScrollOffset();
        bool vertical = IsVerticalScrollView();
        bool forward = false;
        if (vertical) {
          if (last_scroll_offset_.second == scroll_offset.second &&
              scroll_offset.second == 0) {
            // At top
            forward = false;
          } else {
            forward = scroll_offset.second >= last_scroll_offset_.second;
          }
        } else {
          if (last_scroll_offset_.first == scroll_offset.first &&
              scroll_offset.first == 0) {
            // At Left
            forward = false;
          } else {
            forward = scroll_offset.first >= last_scroll_offset_.first;
          }
        }
        bool has_velocity =
            vertical ? last_scroll_offset_.second != scroll_offset.second
                     : last_scroll_offset_.first != scroll_offset.first;
        auto scroll_target = CalcSnapScroll(forward, has_velocity);
        int32_t scroll_position = std::get<0>(scroll_target);
        if (scroll_position != -1) {
          if (scroll_position >= item_keys_.size()) {
            scroll_position =
                std::max(0, static_cast<int32_t>(item_keys_.size() - 1));
          }
          const auto dict = lepus::Dictionary::Create();
          dict->SetValue("position", scroll_position);
          dict->SetValue("currentScrollLeft", scroll_offset.first);
          dict->SetValue("currentScrollTop", scroll_offset.second);
          dict->SetValue("targetScrollLeft", std::get<1>(scroll_target));
          dict->SetValue("targetScrollTop", std::get<2>(scroll_target));
          CustomEvent event{Sign(), list::kSnap, "detail", lepus_value(dict)};
          context_->SendEvent(event);

          NodeManager::Instance().SetAttributeWithNumberValue(
              node_, NODE_SCROLL_OFFSET, std::get<1>(scroll_target),
              std::get<2>(scroll_target), 250,
              static_cast<int>(ARKUI_CURVE_SMOOTH), 0);
        }
      } break;
      case UI_TOUCH_EVENT_ACTION_CANCEL:
        break;
      default:
        break;
    }
  }
}

UIComponent* UIList::GetItemAtIndex(int32_t index) {
  for (const auto child : children_) {
    if (IsListItem(child)) {
      if (GetIndexFromItemKey(static_cast<UIComponent*>(child)->item_key()) ==
          index) {
        return static_cast<UIComponent*>(child);
      }
    }
  }
  return nullptr;
}

std::tuple<int32_t, float, float> UIList::CalcSnapScroll(bool forward,
                                                         bool has_velocity) {
  bool vertical = IsVerticalScrollView();
  std::pair scroll_offset = GetScrollOffset();

  UIComponent* closest_item_before_position = nullptr;
  UIComponent* closest_item_after_position = nullptr;

  float distance_before = std::numeric_limits<float>::lowest();
  float distance_after = std::numeric_limits<float>::max();

  for (const auto child : children_) {
    if (IsListItem(child)) {
      auto list_item = static_cast<UIComponent*>(child);

      if (vertical ? IsVisibleCellVertical(list_item)
                   : IsVisibleCellHorizontal(list_item)) {
        float distance = DistanceToItem(
            list_item, vertical, snap_factor_, snap_offset_, width_, height_,
            scroll_offset.first, scroll_offset.second);
        if (distance <= 0 && distance > distance_before) {
          // Child is before the position and closer then the previous best
          distance_before = distance;
          closest_item_before_position = list_item;
        }
        if (distance >= 0 && distance < distance_after) {
          // Child is after the position and closer then the previous best
          distance_after = distance;
          closest_item_after_position = list_item;
        }
      }
    }
  }

  int32_t target_position = -1;
  UIComponent* target_item = nullptr;

  if (!has_velocity) {
    if (closest_item_after_position != nullptr &&
        closest_item_before_position != nullptr) {
      if (abs(distance_after) < abs(distance_before)) {
        target_item = closest_item_after_position;
        target_position =
            GetIndexFromItemKey(closest_item_after_position->item_key());
      } else {
        target_item = closest_item_before_position;
        target_position =
            GetIndexFromItemKey(closest_item_before_position->item_key());
      }
    } else if (closest_item_after_position != nullptr) {
      target_item = closest_item_after_position;
      target_position =
          GetIndexFromItemKey(closest_item_after_position->item_key());
    } else if (closest_item_before_position != nullptr) {
      target_item = closest_item_before_position;
      target_position =
          GetIndexFromItemKey(closest_item_before_position->item_key());
    }
  } else {
    // Return the position of the first child from the position, in the
    // direction of the fling
    if (forward && closest_item_after_position != nullptr) {
      target_item = closest_item_after_position;
      target_position =
          GetIndexFromItemKey(closest_item_after_position->item_key());
    } else if (!forward && closest_item_before_position != nullptr) {
      target_item = closest_item_before_position;
      target_position =
          GetIndexFromItemKey(closest_item_before_position->item_key());
    }
  }

  // There is no child in the direction of the fling. Either it doesn't
  // exist (start/end of the list), or it is not yet attached (very rare
  // case when children are larger then the viewport). Extrapolate from the
  // child that is visible to get the position of the view to snap to.
  if (target_item != nullptr) {
    return {target_position,
            vertical ? 0
                     : target_item->left_ -
                           (width_ - target_item->width_) * snap_factor_ +
                           snap_offset_,
            vertical ? target_item->top_ -
                           (height_ - target_item->height_) * snap_factor_ +
                           snap_offset_
                     : 0};
  }

  UIComponent* visible_item =
      forward ? closest_item_before_position : closest_item_after_position;

  if (visible_item == nullptr) {
    return {-1, scroll_offset.first, scroll_offset.second};
  }

  target_position =
      GetIndexFromItemKey(visible_item->item_key()) + (forward ? 1 : -1);

  if (target_position < 0) {
    target_position = 0;
  }

  if (GetItemAtIndex(target_position) == nullptr) {
    return {-1, scroll_offset.first, scroll_offset.second};
  }

  target_item = GetItemAtIndex(target_position);

  return {target_position,
          vertical ? 0
                   : target_item->left_ -
                         (width_ - target_item->width_) * snap_factor_ +
                         snap_offset_,
          vertical ? target_item->top_ -
                         (height_ - target_item->height_) * snap_factor_ +
                         snap_offset_
                   : 0};
}

float UIList::DistanceToItem(UIComponent* list_item, bool vertical,
                             float factor, float offset, float viewport_width,
                             float viewport_height, float scroll_x,
                             float scroll_y) {
  if (vertical) {
    return list_item->top_ + list_item->height_ * factor -
           (scroll_y + viewport_height * factor) + offset;
  } else {
    return list_item->left_ + list_item->width_ * factor -
           (scroll_x + viewport_width * factor) + offset;
  }
}

void UIList::HandleScrollStartEvent() {
  // TODO(zhangkaijie.9): "tag" may need be replaced by prop
  // 'scroll-monitor-tag'
  context_->StartFluencyTrace(Sign(), harmony::kFluencyScrollEvent, "tag");
}

void UIList::HandleScrollEvent() {
  // kDragging and kFling have to be handled in handleWillScrollEvent
  if (scroll_state_ != list::ScrollState::kScrollAnimation) {
    return;
  }

  std::pair result = GetScrollOffset();
  if (!should_block_scroll_) {
    ScrollByListContainer(result.first, result.second, result.first,
                          result.second);
  }
  UpdateStickyTopView(result.second);
  UpdateStickyBottomView(result.second);
  context_->NotifyUIScroll();
}

void UIList::HandleScrollStopEvent() {
  SetScrollState(list::ScrollState::kIdle);
  context_->StopFluencyTrace(Sign());
  SendScrollEndEvent();
}

void UIList::SendScrollEndEvent() {
  CustomEvent event{Sign(), list::kScrollEnd, "detail", lepus_value("")};
  context_->SendEvent(event);
}

void UIList::HandleWillScrollEvent(ArkUI_NodeComponentEvent* component_event) {
  // component_event->data[0].f32 and component_event->data[1].f32 is the delta
  // of content offset that will be
  // consumed.
  float delta_offset_x = component_event->data[0].f32;
  float delta_offset_y = component_event->data[1].f32;
  // ARKUI_SCROLL_STATE_IDLE = 0, ARKUI_SCROLL_STATE_SCROLL = 1,
  // ARKUI_SCROLL_STATE_FLING = 2 We can only get 1 or 2 here.
  int32_t scroll_state = component_event->data[2].i32;
  if (scroll_state == ARKUI_SCROLL_STATE_SCROLL) {
    SetScrollState(list::ScrollState::kDragging);
  } else if (scroll_state == ARKUI_SCROLL_STATE_FLING) {
    SetScrollState(list::ScrollState::kFling);
  }

  if (scroll_state_ == list::ScrollState::kDragging ||
      scroll_state_ == list::ScrollState::kFling) {
    std::pair result = GetScrollOffset();
    if (!should_block_scroll_) {
      ScrollByListContainer(
          result.first + delta_offset_x, result.second + delta_offset_y,
          result.first + delta_offset_x, result.second + delta_offset_y);
    }
    float scroll_offset_y = result.second + delta_offset_y;
    if (IsHorizontal()) {
      component_event->data[0].f32 += delta_offset_;
    } else {
      component_event->data[1].f32 += delta_offset_;
      scroll_offset_y += delta_offset_;
    }
    // delta_offset_ is the delta of content offset got from c++ list in
    // UIList::UpdateContentSizeAndOffset() function, so we need to add it to
    // the component_event->data[0].f32 or component_event->data[1].f32 and
    // reset delta_offset_ to 0.
    delta_offset_ = 0;
    UpdateStickyTopView(scroll_offset_y);
    UpdateStickyBottomView(scroll_offset_y);
    context_->NotifyUIScroll();
  }
}

void UIList::SetScrollState(list::ScrollState state) {
  if (scroll_state_ != state) {
    this->scroll_state_ = state;
    auto param = lepus::Dictionary::Create();
    param->SetValue("state", static_cast<int>(state));
    if (enable_need_visible_item_info) {
      param->SetValue("attachedCells", GetVisibleCells());
    }
    CustomEvent event{Sign(), list::kScrollStateChange, "detail",
                      lepus_value(param)};
    context_->SendEvent(event);
  }
}

void UIList::ScrollByListContainer(float content_offset_x,
                                   float content_offset_y, float original_x,
                                   float original_y) {
  context_->ScrollByListContainer(Sign(), content_offset_x, content_offset_y,
                                  original_x, original_y);
}

void UIList::ScrollToPosition(int index, float offset, int align, bool smooth) {
  context_->ScrollToPosition(Sign(), index, offset, align, smooth);
  if (!smooth) {
    SendScrollEndEvent();
  }
}

void UIList::ScrollStopped() {
  if (scroll_callback_) {
    scroll_callback_(LynxGetUIResult::SUCCESS, lepus_value(""));
    scroll_callback_ = nullptr;
  }
  context_->ScrollStopped(Sign());
  SendScrollEndEvent();
  SetScrollState(list::ScrollState::kIdle);
}

void UIList::UpdateContentSizeAndOffset(float content_size, float delta_x,
                                        float delta_y,
                                        bool is_init_scroll_offset,
                                        bool from_layout) {
  // record last main_size
  float last_content_size = is_horizontal_ ? content_width_ : content_height_;
  // update the latest size
  if (is_horizontal_) {
    if (content_size != content_width_ || height_ != content_height_) {
      UpdateContentSize(content_size, height_);
    }
  } else {
    if (width_ != content_width_ || content_size != content_height_) {
      UpdateContentSize(width_, content_size);
    }
  }
  float delta_scroll_offset = is_horizontal_ ? delta_x : delta_y;
  if (!base::FloatsEqual(delta_scroll_offset, 0.f)) {
    should_block_scroll_ = true;
    if (is_init_scroll_offset) {
      ScrollToAsync(delta_x, delta_y);
    } else {
      if (from_layout) {
        auto offset = GetScrollOffset();
        // The latest "contentSize" can only take effect on the next frame, so
        // need to judge whether you can scroll directly based on the
        // last_content_size.
        if (!base::FloatsLarger(
                GetViewPortSize() + GetScrollDistance() + delta_scroll_offset,
                last_content_size)) {
          ScrollTo(offset.first + delta_x, offset.second + delta_y, false);
        } else {
          // the "delta_y" offset has exceeded the list scroll range area
          pending_delta_offset_ = delta_scroll_offset;
        }
      } else {
        delta_offset_ = delta_scroll_offset;
      }
    }
    should_block_scroll_ = false;
  }
}

void UIList::UpdateContentSize(float width, float height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_LIST_UPDATE_CONTENT_SIZE);
  BaseScrollContainer::UpdateContentSize(width, height);
  NodeManager::Instance().SetAttributeWithNumberValue(container_layout_,
                                                      NODE_SIZE, width, height);
}

void UIList::StartScroller() {
  if (init_smooth_scroll_end_estimated_offset_ < 0) {
    return;
  }

  // set TimingFunctionData
  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func =
      starlight::TimingFunctionType::kEaseInEaseOut;

  // set AnimationData
  starlight::AnimationData data;
  // duration: 250ms
  data.duration = 250;
  data.fill_mode = starlight::AnimationFillModeType::kForwards;
  data.timing_func = timing_function_data;

  if (!smooth_scroller_animator_) {
    smooth_scroller_animator_ =
        std::make_shared<animation::basic::LynxBasicAnimator>(
            data, nullptr,
            animation::basic::LynxBasicAnimator::BasicValueType::FLOAT);

    // register the complete callback
    smooth_scroller_animator_->RegisterEventCallback(
        [weak_this = weak_from_this()]() {
          auto shared_this = weak_this.lock();
          if (!shared_this) {
            return;
          }
          auto list = static_cast<UIList*>(shared_this.get());
          list->ScrollStopped();
        },
        animation::basic::Animation::EventType::End);

    // register the progress callback
    smooth_scroller_animator_->RegisterCustomCallback(
        [weak_this = weak_from_this()](float progress) {
          auto shared_this = weak_this.lock();
          if (!shared_this) {
            return;
          }
          auto list = static_cast<UIList*>(shared_this.get());

          // when clicking a button during the fling process to trigger the
          // smooth process, the execution sequence is
          // startScroller->stopEvent->animatorRunnable.run(), which will cause
          // the scrollState to be incorrect during executing the runnable.
          list->SetScrollState(list::ScrollState::kScrollAnimation);

          float scroll_distance =
              list->init_smooth_scroll_start_offset_ +
              list->content_offset_percent_during_scroll_ * progress *
                  (list->init_smooth_scroll_end_estimated_offset_ -
                   list->init_smooth_scroll_start_offset_);

          if (list->is_horizontal_) {
            list->ScrollTo(scroll_distance, 0, false);
          } else {
            list->ScrollTo(0, scroll_distance, false);
          }
        });
  }

  smooth_scroller_animator_->Start();
}

bool UIList::ShouldCallScroll(float delta_x, float delta_y) {
  bool vertical = !IsHorizontal();
  if ((vertical && delta_y == 0) || (!vertical && delta_x == 0)) {
    return false;
  }
  return true;
}

void UIList::UpdateScrollInfo(bool smooth, float estimated_offset,
                              bool scrolling) {
  if (!scrolling) {
    init_smooth_scroll_end_estimated_offset_ = estimated_offset;
    init_smooth_scroll_start_offset_ = GetScrollDistance();
    StartScroller();
    SetScrollState(list::ScrollState::kScrollAnimation);
  } else {
    if (init_smooth_scroll_end_estimated_offset_ > 0) {
      content_offset_percent_during_scroll_ =
          estimated_offset / init_smooth_scroll_end_estimated_offset_;
    }
  }
}

void UIList::InsertListItemNode(lynx::tasm::harmony::UIComponent* child) {
  if (!child) {
    return;
  }
  if (enable_list_sticky_) {
    if (update_sticky_for_diff_) {
      // This method is invoked in FinishLayoutOperation or by c++ list element
      // which means component has valid item-key info.
      const std::string& item_key = child->item_key();
      // Add <item-key, list-item> to map if current component is sticky item,
      // and add list as component node ready listener.
      if (sticky_top_item_key_set_.find(item_key) !=
          sticky_top_item_key_set_.end()) {
        sticky_top_item_map_.emplace(item_key, child);
        child->SetNodeReadyListener(this);
      } else if (sticky_bottom_item_key_set_.find(item_key) !=
                 sticky_bottom_item_key_set_.end()) {
        sticky_bottom_item_map_.emplace(item_key, child);
        child->SetNodeReadyListener(this);
      }
    } else {
      UpdateStickyComponentInfoForUpdateAction(child);
    }
  }
  NodeManager::Instance().InsertNode(container_layout_, child->DrawNode(), -1);
}

void UIList::RemoveListItemNode(lynx::tasm::harmony::UIComponent* child) {
  if (!child) {
    return;
  }
  if (enable_list_sticky_) {
    if (update_sticky_for_diff_) {
      // Remove <item-key, list-item> from map if current component is sticky
      // item. Note: need set node ready listener to null because this component
      // may be reused by any no sticky item.
      const std::string& item_key = child->item_key();
      auto top_item_iter = sticky_top_item_map_.end();
      auto bottom_item_iter = sticky_bottom_item_map_.end();
      if ((top_item_iter = sticky_top_item_map_.find(item_key)) !=
              sticky_top_item_map_.end() &&
          top_item_iter->second == child) {
        sticky_top_item_map_.erase(top_item_iter);
        child->SetNodeReadyListener(nullptr);
        if (enable_recycle_sticky_item_) {
          ResetStickyItem(child);
        }
      } else if ((bottom_item_iter = sticky_bottom_item_map_.find(item_key)) !=
                     sticky_bottom_item_map_.end() &&
                 bottom_item_iter->second == child) {
        sticky_bottom_item_map_.erase(bottom_item_iter);
        child->SetNodeReadyListener(nullptr);
        if (enable_recycle_sticky_item_) {
          ResetStickyItem(child);
        }
      }
    } else {
      UpdateStickyComponentForRemoveAction(child);
    }
  }
  NodeManager::Instance().RemoveNode(container_layout_, child->DrawNode());
}

void UIList::OnLayoutFinish(UIBase* base, int64_t operation_id) {
  if (!base || enable_batch_render_) {
    return;
  }
  UIComponent* component = static_cast<UIComponent*>(base);
  InsertListItemNode(component);
}

int UIList::GetIndexFromItemKey(const std::string& item_key) const {
  if (item_key.empty()) {
    return -1;
  }
  auto it = std::find(item_keys_.begin(), item_keys_.end(), item_key);

  if (it != item_keys_.end()) {
    // calculate index
    int index = std::distance(item_keys_.begin(), it);
    return index;
  } else {
    return -1;
  }
}

void UIList::UpdateStickyComponentInfoForUpdateAction(UIComponent* component) {
  auto index = GetIndexFromItemKey(component->item_key());
  if (index < 0 || index >= item_keys_.size()) {
    return;
  }
  // update sticky top
  auto top_iterator =
      std::find(sticky_top_indexes_.begin(), sticky_top_indexes_.end(), index);
  if (top_iterator != sticky_top_indexes_.end()) {
    sticky_top_items_[index] = component;
  }
  // update sticky bottom
  auto bottom_iterator = std::find(sticky_bottom_indexes_.begin(),
                                   sticky_bottom_indexes_.end(), index);
  if (bottom_iterator != sticky_bottom_indexes_.end()) {
    sticky_bottom_items_[index] = component;
  }
}

void UIList::UpdateStickyComponentForRemoveAction(UIComponent* component) {
  auto index = GetIndexFromItemKey(component->item_key());
  if (sticky_top_items_.erase(index) && enable_recycle_sticky_item_) {
    ResetStickyItem(component);
  }
  if (sticky_bottom_items_.erase(index) && enable_recycle_sticky_item_) {
    ResetStickyItem(component);
  }
}

void UIList::ResetStickyItem(UIComponent* component) {
  if (component) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        component->DrawNode(), NODE_TRANSLATE, 0, 0, 0);
    NodeManager::Instance().SetAttributeWithNumberValue(
        component->DrawNode(), NODE_Z_INDEX, component->z_index());
  }
}

UIComponent* UIList::GetStickyItemWithIndex(int index, bool is_sticky_top) {
  UIComponent* ui_component = nullptr;
  if (update_sticky_for_diff_) {
    const auto& sticky_item_map =
        is_sticky_top ? sticky_top_item_map_ : sticky_bottom_item_map_;
    if (index >= 0 && index < item_keys_.size()) {
      const std::string& item_key = item_keys_[index];
      auto it = sticky_item_map.find(item_key);
      ui_component = it != sticky_item_map.end() ? it->second : nullptr;
    }
  } else {
    const auto& sticky_items =
        is_sticky_top ? sticky_top_items_ : sticky_bottom_items_;
    auto it = sticky_items.find(index);
    ui_component = it != sticky_items.end() ? it->second : nullptr;
  }
  return ui_component;
}

void UIList::UpdateStickyTopView(float scroll_y) {
  if (!enable_list_sticky_) {
    return;
  }
  float offset = scroll_y + sticky_offset_;
  UIComponent* sticky_top_item = nullptr;
  UIComponent* next_sticky_top_item = nullptr;

  for (auto rit = sticky_top_indexes_.rbegin();
       rit != sticky_top_indexes_.rend(); ++rit) {
    UIComponent* element = GetStickyItemWithIndex(*rit, true);
    if (!element) {
      continue;
    }
    if (base::FloatsLarger(element->top_, offset)) {
      next_sticky_top_item = element;
      ResetStickyItem(element);
    } else if (sticky_top_item) {
      ResetStickyItem(element);
    } else {
      sticky_top_item = element;
    }
  }

  if (sticky_top_item) {
    if (prev_sticky_top_item_ != sticky_top_item) {
      auto param = lepus::Dictionary::Create();
      param->SetValue("top", sticky_top_item->item_key());
      CustomEvent event{Sign(), "stickytop", "detail", lepus_value(param)};
      context_->SendEvent(event);
      prev_sticky_top_item_ = sticky_top_item;
    }

    float sticky_top_y = offset;
    if (next_sticky_top_item) {
      float next_sticky_top_item_distance_from_offset =
          next_sticky_top_item->top_ - offset;
      float squash_sticky_top_delta =
          sticky_top_item->height_ - next_sticky_top_item_distance_from_offset;
      if (squash_sticky_top_delta > 0) {
        // need push sticky top item to upper
        sticky_top_y -= squash_sticky_top_delta;
      }
    }

    NodeManager::Instance().SetAttributeWithNumberValue(
        sticky_top_item->DrawNode(), NODE_TRANSLATE, 0,
        sticky_top_y - sticky_top_item->top_, 0);
    NodeManager::Instance().SetAttributeWithNumberValue(
        sticky_top_item->DrawNode(), NODE_Z_INDEX,
        std::numeric_limits<int32_t>::max());
  }
}

void UIList::UpdateStickyBottomView(float scroll_y) {
  if (!enable_list_sticky_) {
    return;
  }
  float offset = scroll_y + height_ - sticky_offset_;
  UIComponent* sticky_bottom_item = nullptr;
  UIComponent* next_sticky_bottom_item = nullptr;

  for (auto index : sticky_bottom_indexes_) {
    UIComponent* element = GetStickyItemWithIndex(index, false);
    if (!element) {
      continue;
    }
    if (base::FloatsLarger(offset, element->top_ + element->height_)) {
      next_sticky_bottom_item = element;
      ResetStickyItem(element);
    } else if (sticky_bottom_item != nullptr) {
      ResetStickyItem(element);
    } else {
      sticky_bottom_item = element;
    }
  }

  if (sticky_bottom_item) {
    if (prev_sticky_bottom_item_ != sticky_bottom_item) {
      auto param = lepus::Dictionary::Create();
      param->SetValue("bottom", sticky_bottom_item->item_key());
      CustomEvent event{Sign(), "stickybottom", "detail", lepus_value(param)};
      context_->SendEvent(event);
      prev_sticky_bottom_item_ = sticky_bottom_item;
    }

    float sticky_top_y = offset - sticky_bottom_item->height_;
    if (next_sticky_bottom_item) {
      float next_sticky_item_distance_from_offset =
          offset - next_sticky_bottom_item->top_ -
          next_sticky_bottom_item->height_;
      float squash_sticky_top_delta =
          sticky_bottom_item->height_ - next_sticky_item_distance_from_offset;
      if (squash_sticky_top_delta > 0) {
        // need push sticky top item to upper
        sticky_top_y += squash_sticky_top_delta;
      }
    }

    NodeManager::Instance().SetAttributeWithNumberValue(
        sticky_bottom_item->DrawNode(), NODE_TRANSLATE, 0,
        sticky_top_y - sticky_bottom_item->top_, 0);
    NodeManager::Instance().SetAttributeWithNumberValue(
        sticky_bottom_item->DrawNode(), NODE_Z_INDEX,
        std::numeric_limits<int32_t>::max());
  }
}

void UIList::InvokeAutoScroll(
    float rate, bool start, bool auto_stop,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto_scroller_) {
    SetScrollState(list::ScrollState::kScrollAnimation);
    auto_scroller_->AutoScroll(rate, start, auto_stop, std::move(callback));
  }
}

fml::RefPtr<lepus::CArray> UIList::GetVisibleCells() const {
  auto array = lepus::CArray::Create();
  bool vertical = !IsHorizontal();
  float scroll_x = GetScrollOffset().first;
  float scroll_y = GetScrollOffset().second;

  for (const auto child : children_) {
    if (IsListItem(child)) {
      if (vertical
              ? IsVisibleCellVertical(static_cast<UIComponent*>(child))
              : IsVisibleCellHorizontal(static_cast<UIComponent*>(child))) {
        auto param = lepus::Dictionary::Create();
        param->SetValue("id", child->Sign());
        auto index =
            GetIndexFromItemKey(static_cast<UIComponent*>(child)->item_key());
        param->SetValue("position", index);
        param->SetValue("index", index);
        param->SetValue("itemKey",
                        static_cast<UIComponent*>(child)->item_key());
        param->SetValue("top", child->top_ - scroll_y);
        param->SetValue("bottom", child->top_ + child->height_ - scroll_y);
        param->SetValue("left", child->left_ - scroll_x);
        param->SetValue("right", child->left_ + child->width_ - scroll_x);
        array->emplace_back(std::move(param));
      }
    }
  }
  return array;
}

bool UIList::IsVisibleCellVertical(UIComponent* component) const {
  float min_y = component->ViewTop();
  float max_y = min_y + component->height_;
  float offset_min_y = GetScrollOffset().second;
  float offset_max_y = offset_min_y + height_;
  return ((min_y <= offset_min_y && max_y >= offset_min_y) ||
          (min_y <= offset_max_y && max_y >= offset_max_y) ||
          (min_y >= offset_min_y && max_y <= offset_max_y));
}

bool UIList::IsVisibleCellHorizontal(UIComponent* component) const {
  float min_x = component->ViewLeft();
  float max_x = min_x + component->width_;
  float offset_min_x = GetScrollOffset().first;
  float offset_max_x = offset_min_x + width_;
  return ((min_x <= offset_min_x && max_x >= offset_min_x) ||
          (min_x <= offset_max_x && max_x >= offset_max_x) ||
          (min_x >= offset_min_x && max_x <= offset_max_x));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
