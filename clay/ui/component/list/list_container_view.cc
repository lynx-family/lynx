// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_container_view.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/list_scroller.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/lynx_module/type_utils.h"

namespace clay {

namespace details {
static constexpr const char kDataSourceItemKeys[] = "itemkeys";
static constexpr const char kDataSourceStickyStart[] = "stickyStart";
static constexpr const char kDataSourceStickyEnd[] = "stickyEnd";
}  // namespace details

ListContainerView::ListContainerView(int32_t id, PageView* page_view)
    : WithTypeInfo(id, ScrollDirection::kVertical, page_view,
                   std::make_unique<RenderScroll>()) {
  tag_ = "ListContainerView";
  AddEventCallback(event_attr::kEventScrollStateChange);
}

void ListContainerView::SetAttribute(const char* attr_c,
                                     const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kListContainerInfo) {
    const auto& info = attribute_utils::GetMap(value);
    if (auto it = info.find(details::kDataSourceItemKeys); it != info.end()) {
      const auto& itemkeys = attribute_utils::GetArray(it->second);
      item_keys_.clear();
      item_key_map_.clear();
      for (size_t i = 0; i < itemkeys.size(); i++) {
        auto s = attribute_utils::GetCString(itemkeys[i]);
        item_keys_.push_back(s);
        item_key_map_[s] = i;
      }
    }
    if (auto it = info.find(details::kDataSourceStickyStart);
        it != info.end()) {
      const auto& itemkeys = attribute_utils::GetArray(it->second);
      sticky_top_indexes_.clear();
      for (size_t i = 0; i < itemkeys.size(); i++) {
        sticky_top_indexes_.push_back(attribute_utils::GetInt(itemkeys[i]));
      }
    }
    if (auto it = info.find(details::kDataSourceStickyEnd); it != info.end()) {
      const auto& itemkeys = attribute_utils::GetArray(it->second);
      sticky_bottom_indexes_.clear();
      for (size_t i = 0; i < itemkeys.size(); i++) {
        sticky_bottom_indexes_.push_back(attribute_utils::GetInt(itemkeys[i]));
      }
    }
  } else if (kw == KeywordID::kExperimentalBatchRenderStrategy) {
    enable_batch_render_strategy_ = attribute_utils::GetInt(value) > 0;
  } else if (kw == KeywordID::kSticky) {
    enable_list_sticky_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kStickyOffset) {
    sticky_offset_ = attribute_utils::GetDouble(value);
  } else if (kw == KeywordID::kExperimentalRecycleStickyItem) {
    enable_recycle_sticky_item_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kExperimentalUpdateStickyForDiff) {
    update_sticky_for_diff_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kEnableInsertPlatformViewOperation) {
    enable_insert_platform_view_operation_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kNeedVisibleItemInfo) {
    need_visible_item_info_ = attribute_utils::GetBool(value);
  } else {
    ScrollView::SetAttribute(attr_c, value);
  }
}

int ListContainerView::GetIndexFromItemKey(std::string item_key) {
  if (item_key.empty() || item_key_map_.find(item_key) == item_key_map_.end()) {
    return -1;
  } else {
    return item_key_map_[item_key];
  }
}

void ListContainerView::EraseStickyItem(BaseView* view) {
  if (!view || !view->Is<Component>() || !enable_list_sticky_) return;
  auto* component = static_cast<Component*>(view);
  if (update_sticky_for_diff_) {
    if (auto item_key = view->ItemKey(); !item_key.empty()) {
#define ERASE_STICKY_ITEM(sticky_map_ref)                     \
  do {                                                        \
    auto it = (sticky_map_ref).find(item_key);                \
    if (it != (sticky_map_ref).end() && it->second == view) { \
      (sticky_map_ref).erase(it);                             \
      component->SetNodeReadyListener(nullptr);               \
      if (enable_recycle_sticky_item_) {                      \
        ResetStickyItem(component);                           \
      }                                                       \
      return;                                                 \
    }                                                         \
  } while (0)
      ERASE_STICKY_ITEM(sticky_top_item_map_);
      ERASE_STICKY_ITEM(sticky_bottom_item_map_);
#undef ERASE_STICKY_ITEM
    }
  } else {
    UpdateStickyInfoForDeletedChild(component, sticky_top_items_);
    UpdateStickyInfoForDeletedChild(component, sticky_bottom_items_);
  }
}

void ListContainerView::RemoveListItemPaintingNode(BaseView* view) {
  RemoveChild(view);
  EraseStickyItem(view);
}

void ListContainerView::InsertListItemPaintingNode(BaseView* view) {
  // In multi thread, the child component has been rendered when invoking
  // onLayoutFinish() but may not have any layout info, because we block the
  // child component layout info's flushing which triggered by starlight
  // engine, so we cannot add child view to the list until getting the real
  // layout info of the child component.
  // TODO(dongjiajian): considering async layout here.
  InsertListItemPaintingNodeInternal(view);
  view->render_object()->SetRepaintBoundary(true);
  if (view->Is<Component>()) {
    auto component = static_cast<Component*>(view);
    if (auto index = component->GetZIndex()) {
      component->render_object()->SetPaintingOrder(index.value());
    }
    if (enable_list_sticky_) {
      if (update_sticky_for_diff_) {
        std::string itemKey = view->ItemKey();
        if (!itemKey.empty()) {
          if (sticky_top_item_key_set_.count(itemKey)) {
            sticky_top_item_map_[itemKey] = component;
            component->SetNodeReadyListener(this);
          } else if (sticky_bottom_item_key_set_.count(itemKey)) {
            sticky_bottom_item_map_[itemKey] = component;
            component->SetNodeReadyListener(this);
          }
        }
      } else {
        int index = GetIndexFromItemKey(component->ItemKey());
        UpdateStickyInfoForUpdatedChild(component, sticky_top_items_,
                                        sticky_top_indexes_, index);
        UpdateStickyInfoForUpdatedChild(component, sticky_bottom_items_,
                                        sticky_bottom_indexes_, index);
      }
    }
  }
  if (delegate_) {
    delegate_->OnListViewDidLayout();
  }
}

void ListContainerView::InsertListItemPaintingNodeInternal(BaseView* view) {
  if (view && !view->Parent()) {
    ScrollView::AddChild(view, child_count());
  }
}

void ListContainerView::ScrollToPosition(
    bool smooth, int position, float offset, AlignTo align_to,
    const std::string& id,
    const std::function<void(uint32_t, const std::string&)>& callback) {
  if (position < 0) {
    if (callback) {
      callback(static_cast<uint32_t>(LynxUIMethodResult::kParamInvalid), "");
    }
    return;
  }
  int align_to_int = 0;
  if (align_to == AlignTo::kStart) {
    align_to_int = 0;
  } else if (align_to == AlignTo::kMiddle) {
    align_to_int = 1;
  } else if (align_to == AlignTo::kEnd) {
    align_to_int = 2;
  }
  if (delegate_) {
    delegate_->OnScrollToPosition(position, offset, align_to_int, smooth);
  }
  if (callback) {
    callback(static_cast<uint32_t>(LynxUIMethodResult::kSuccess), "");
  }
}

void ListContainerView::UpdateContentOffsetForListContainer(
    float content_size, float target_content_offset_x,
    float target_content_offset_y) {
  SetMaxContent(content_size);
  should_block_did_scroll_ = true;
  if (GetScrollDirection() == ScrollDirection::kVertical) {
    ScrollWithDelta(false, target_content_offset_y);
  } else {
    ScrollWithDelta(false, target_content_offset_x);
  }
  should_block_did_scroll_ = false;
}

void ListContainerView::UpdateScrollInfo(bool smooth, float estimated_offset,
                                         bool scrolling) {
  scrolling_estimated_offset_ = estimated_offset;
  if (!scrolling) {
    ScrollTo(smooth, estimated_offset);
    if (smooth) {
      SetScrollStatus(Scrollable::ScrollStatus::kAnimating);
    }
  }
}

void ListContainerView::AddChild(BaseView* child, int position) {
  // The list container view only add child in `OnLayoutFinish` and
  // `InsertListItemPaintingNode`
  if (!update_sticky_for_diff_) {
    int index = GetIndexFromItemKey(child->ItemKey());
    UpdateStickyInfoForInsertedChild(child, sticky_top_items_,
                                     sticky_top_indexes_, index);
    UpdateStickyInfoForInsertedChild(child, sticky_bottom_items_,
                                     sticky_bottom_indexes_, index);
  }
}

void ListContainerView::UpdateStickyInfoForInsertedChild(
    BaseView* child, std::unordered_map<int, Component*>& sticky_items,
    std::vector<int>& sticky_indexes, int index) {
  if (!enable_list_sticky_ || !child->Is<Component>()) {
    return;
  }
  for (unsigned i = 0; i < sticky_indexes.size(); ++i) {
    if (index == sticky_indexes[i]) {
      sticky_items[index] = static_cast<Component*>(child);
    }
  }
}

void ListContainerView::UpdateStickyInfoForDeletedChild(
    BaseView* child, std::unordered_map<int, Component*>& sticky_items) {
  if (!enable_list_sticky_ || !child || !child->Is<Component>()) {
    return;
  }
  for (auto it = sticky_items.begin(); it != sticky_items.end(); ++it) {
    if (it->second == child) {
      if (enable_recycle_sticky_item_) {
        ResetStickyItem(static_cast<Component*>(child));
      }
      sticky_items.erase(it);
      break;
    }
  }
}

void ListContainerView::UpdateStickyInfoForUpdatedChild(
    Component* child, std::unordered_map<int, Component*> sticky_items,
    const std::vector<int>& sticky_indexes, int index) {
  if (!enable_list_sticky_ || !child || !child->Is<Component>()) {
    return;
  }
  for (unsigned i = 0; i < sticky_indexes.size(); ++i) {
    if (index == sticky_indexes[i]) {
      sticky_items[index] = static_cast<Component*>(child);
      break;
    }
  }
}

void ListContainerView::ResetStickyItem(Component* child) {
  if (child != nullptr) {
    child->SetTransform(TransformOperations(), FloatPoint());
  }
};

void ListContainerView::ApplyChildTranslateZ(Component* child) {
  if (child) {
    if (auto z = child->GetZIndex()) {
      ApplyChildTranslateZ(child, *z);
    }
  }
}

void ListContainerView::ApplyChildTranslateZ(Component* child,
                                             float translateZ) {
  if (child != nullptr) {
    TransformOperations ops;
    ops.AppendTranslate(0, 0, translateZ);
    child->SetTransform(ops, FloatPoint());
  }
}

void ListContainerView::OnNodeReady() {
  ScrollView::OnNodeReady();
  UpdateStickyStarts(scroll_offset_.width(), scroll_offset_.height());
  UpdateStickyEnds(scroll_offset_.width(), scroll_offset_.height());
}

void ListContainerView::OnComponentNodeReady(Component* component) {
  if (enable_list_sticky_ && update_sticky_for_diff_ && component != nullptr) {
    std::string item_key = component->ItemKey();
    if (!item_key.empty()) {
      if (sticky_top_item_key_set_.count(item_key)) {
        UpdateStickyItemMap(component, sticky_top_item_map_, true);
      } else if (sticky_bottom_item_key_set_.count(item_key)) {
        UpdateStickyItemMap(component, sticky_bottom_item_map_, true);
      } else {
        // Not sticky top or bottom list item, remove it from map.
        UpdateStickyItemMap(component, sticky_top_item_map_, false);
        UpdateStickyItemMap(component, sticky_bottom_item_map_, false);
      }
    }
  }
}

void ListContainerView::UpdateStickyEnds(float offset_x, float offset_y) {
  if (!enable_list_sticky_) {
    return;
  }

  bool is_vertical = GetScrollDirection() == ScrollDirection::kVertical;

  int offset = 0;
  if (is_vertical) {
    offset = offset_y + Height() - sticky_offset_;
  } else {
    offset = offset_x + Width() - sticky_offset_;
  }

  Component* sticky_end_item = nullptr;
  Component* next_sticky_end_item = nullptr;
  for (int end_index : sticky_bottom_indexes_) {
    Component* end_component = GetStickyItemWithIndex(end_index, false);
    if (!end_component) {
      continue;
    }
    auto cur_offset = is_vertical
                          ? (end_component->Top() + end_component->Height())
                          : (end_component->Left() + end_component->Width());
    if (cur_offset < offset) {
      next_sticky_end_item = end_component;
      ResetStickyItem(end_component);
    } else if (sticky_end_item != nullptr) {
      ResetStickyItem(end_component);
    } else {
      sticky_end_item = end_component;
    }
  }
  if (sticky_end_item != nullptr) {
    if (prev_sticky_bottom_item_ != sticky_end_item) {
      if (is_vertical) {
        page_view_->SendEvent(id(), event_attr::kEventListStickyBottom,
                              {"bottom"}, sticky_end_item->ItemKey());
      }
      page_view_->SendEvent(id(), event_attr::kEventListStickyEnd, {"end"},
                            sticky_end_item->ItemKey());
      prev_sticky_bottom_item_ = sticky_end_item;
    }
    int sticky_start_offset = offset - (is_vertical ? sticky_end_item->Height()
                                                    : sticky_end_item->Width());
    if (next_sticky_end_item != nullptr) {
      int next_sticky_end_item_distance_from_offset = 0;
      int squash_sticky_end_delta = 0;
      if (is_vertical) {
        next_sticky_end_item_distance_from_offset =
            offset -
            (next_sticky_end_item->Top() + next_sticky_end_item->Height());
        squash_sticky_end_delta = sticky_end_item->Height() -
                                  next_sticky_end_item_distance_from_offset;
      } else {
        next_sticky_end_item_distance_from_offset =
            offset -
            (next_sticky_end_item->Left() + next_sticky_end_item->Width());
        squash_sticky_end_delta = sticky_end_item->Width() -
                                  next_sticky_end_item_distance_from_offset;
      }

      if (squash_sticky_end_delta > 0) {
        sticky_start_offset += squash_sticky_end_delta;
      }
    }
    if (sticky_end_item != nullptr) {
      TransformOperations ops;
      if (is_vertical) {
        ops.AppendTranslate(0, sticky_start_offset - sticky_end_item->Top(),
                            std::numeric_limits<int>::max());
      } else {
        ops.AppendTranslate(sticky_start_offset - sticky_end_item->Left(), 0,
                            std::numeric_limits<int>::max());
      }

      sticky_end_item->SetTransform(ops, FloatPoint());
    }
  }
}

void ListContainerView::UpdateStickyItemMap(
    Component* component,
    std::unordered_map<std::string, Component*>& sticky_item_map,
    bool is_sticky_item) {
  if (component && !component->ItemKey().empty()) {
    if (is_sticky_item) {
      std::string item_key = component->ItemKey();
      auto it = sticky_item_map.find(item_key);
      if (it != sticky_item_map.end() && it->second == component) {
        return;
      }
      for (auto iter = sticky_item_map.begin(); iter != sticky_item_map.end();
           ++iter) {
        if (iter->second == component && iter->first != item_key) {
          sticky_item_map.erase(iter);
          sticky_item_map[item_key] = component;
          break;
        }
      }
    } else {
      // The component is not sticky top or bottom list item, remove it from
      // map.
      for (auto it = sticky_item_map.begin(); it != sticky_item_map.end();
           ++it) {
        if (it->second == component) {
          // Delete old <item-key, list-item> pair.
          sticky_item_map.erase(it);
          ResetStickyItem(component);
          break;
        }
      }
    }
  }
}

void ListContainerView::UpdateStickyStarts(float offset_x, float offset_y) {
  if (!enable_list_sticky_) {
    return;
  }

  bool is_vertical = GetScrollDirection() == ScrollDirection::kVertical;

  int offset = (is_vertical ? offset_y : offset_x) + sticky_offset_;
  Component* sticky_start_item = nullptr;
  Component* next_sticky_start_item = nullptr;
  for (auto it = sticky_top_indexes_.rbegin(); it != sticky_top_indexes_.rend();
       ++it) {
    int start_index = *it;
    Component* start_component = GetStickyItemWithIndex(start_index, true);
    if (start_component == nullptr) {
      continue;
    }
    auto cur_offset =
        is_vertical ? start_component->Top() : start_component->Left();
    if (cur_offset > offset) {
      next_sticky_start_item = start_component;
      ResetStickyItem(start_component);
    } else if (sticky_start_item != nullptr) {
      ResetStickyItem(start_component);
    } else {
      sticky_start_item = start_component;
    }
  }
  if (sticky_start_item != nullptr) {
    if (prev_sticky_top_item_ != sticky_start_item) {
      if (is_vertical) {
        page_view_->SendEvent(id(), event_attr::kEventListStickyTop, {"top"},
                              sticky_start_item->ItemKey());
      }

      page_view_->SendEvent(id(), event_attr::kEventListStickyStart, {"start"},
                            sticky_start_item->ItemKey());

      prev_sticky_top_item_ = sticky_start_item;
    }

    int sticky_start_offset = offset;
    if (next_sticky_start_item != nullptr) {
      int squash_sticky_top_delta = 0;
      if (is_vertical) {
        squash_sticky_top_delta = sticky_start_item->Height() -
                                  (next_sticky_start_item->Top() - offset);
      } else {
        squash_sticky_top_delta = sticky_start_item->Width() -
                                  (next_sticky_start_item->Left() - offset);
      }

      if (squash_sticky_top_delta > 0) {
        sticky_start_offset -= squash_sticky_top_delta;
      }
    }
    if (sticky_start_item != nullptr) {
      TransformOperations ops;
      if (is_vertical) {
        ops.AppendTranslate(0, sticky_start_offset - sticky_start_item->Top(),
                            std::numeric_limits<int>::max());
      } else {
        ops.AppendTranslate(sticky_start_offset - sticky_start_item->Left(), 0,
                            std::numeric_limits<int>::max());
      }

      sticky_start_item->SetTransform(ops, FloatPoint());
    }
  }
}

Component* ListContainerView::GetStickyItemWithIndex(int index,
                                                     bool is_sticky_top) {
  Component* component = nullptr;
  if (update_sticky_for_diff_) {
    auto& sticky_item_map =
        is_sticky_top ? sticky_top_item_map_ : sticky_bottom_item_map_;
    if (index >= 0 && index < static_cast<int>(item_keys_.size())) {
      const std::string& item_key = item_keys_[index];
      if (!item_key.empty()) {
        auto it = sticky_item_map.find(item_key);
        if (it != sticky_item_map.end()) {
          component = it->second;
        }
      }
    }
  } else {
    auto& sticky_items =
        is_sticky_top ? sticky_top_items_ : sticky_bottom_items_;
    auto it = sticky_items.find(index);
    if (it != sticky_items.end()) {
      component = it->second;
    }
  }
  return component;
}

void ListContainerView::GenerateStickyItemKeySet(
    std::unordered_set<std::string>& sticky_item_key_set,
    std::unordered_map<std::string, Component*>& sticky_item_map,
    const std::vector<int>& sticky_item_indexes) {
  sticky_item_key_set.clear();
  for (unsigned i = 0; i < sticky_item_indexes.size(); ++i) {
    int index = sticky_item_indexes[i];
    if (index >= 0 && index < static_cast<int>(item_keys_.size())) {
      sticky_item_key_set.insert(item_keys_[index]);
    }
  }
  // Remove item from sticky dict if not sticky.
  for (auto it = sticky_item_map.begin(); it != sticky_item_map.end();) {
    if (it->second &&
        sticky_item_key_set.find(it->first) == sticky_item_key_set.end()) {
      ResetStickyItem(it->second);
      it->second->SetNodeReadyListener(nullptr);
      it = sticky_item_map.erase(it);
    } else {
      ++it;
    }
  }
}

void ListContainerView::OnLayoutFinish(BaseView* view) {
  // In multi thread, the child component has been rendered when invoking
  // onLayoutFinish() but may not have any layout info, because we block the
  // child component layout info's flushing which triggered by starlight engine,
  // so we cannot add child view to the list until getting the real layout info
  // of the child component.
  if (!enable_batch_render_strategy_ &&
      !enable_insert_platform_view_operation_) {
    InsertListItemPaintingNode(view);
  }
}

void ListContainerView::CalculateOverFlow() {
  ScrollView::CalculateOverFlow();
  SetMaxContent(max_content_);
}

void ListContainerView::SetMaxContent(float value) {
  max_content_ = value;
  if (GetScrollDirection() == ScrollDirection::kVertical) {
    GetRenderScroll()->SetOverflowRect(
        FloatRect(0, 0, GetRenderScroll()->Width(), max_content_));
  } else {
    GetRenderScroll()->SetOverflowRect(
        FloatRect(0, 0, max_content_, GetRenderScroll()->Height()));
  }
}

void ListContainerView::OnScrollStatusChange(ScrollStatus old_status) {
  // NOTE: Here we skip the call of ScrollView::OnScrollStatusChange, because
  // the logic is useless for list container.
  NestedScrollable::OnScrollStatusChange(old_status);

  is_scroll_animating_ = status_ == Scrollable::ScrollStatus::kAnimating;

  switch (status_) {
    case Scrollable::ScrollStatus::kFling:
    case Scrollable::ScrollStatus::kBounce:
      SetScrollState(ScrollState::kSettling);
      break;
    case Scrollable::ScrollStatus::kDragging:
      SetScrollState(ScrollState::kDragging);
      break;
    case Scrollable::ScrollStatus::kIdle:
      SetScrollState(ScrollState::kIdle);
      break;
    default:
      break;
  }
}

void ListContainerView::SetScrollState(ScrollState state) {
  if (scroll_state_ == state) {
    return;
  }

  scroll_state_ = state;
  if (status_ == Scrollable::ScrollStatus::kIdle && delegate_) {
    delegate_->OnScrollStopped();
  }

  clay::Value::Map args;
  args["state"] = clay::Value(static_cast<int>(state));

  if (need_visible_item_info_) {
    auto cells_array = GetVisibleCells();
    args["attachedCells"] = clay::Value(std::move(cells_array));
  }

  page_view_->SendCustomEvent(id(), event_attr::kEventScrollStateChange,
                              std::move(args));
}

void ListContainerView::DidScroll() {
  ScrollView::DidScroll();
  if (!should_block_did_scroll_ && delegate_) {
    delegate_->OnScrollByListContainer(
        scroll_offset_.width(), scroll_offset_.height(), scroll_offset_.width(),
        scroll_offset_.height());
    UpdateStickyStarts(scroll_offset_.width(), scroll_offset_.height());
    UpdateStickyEnds(scroll_offset_.width(), scroll_offset_.height());
  }
}

void ListContainerView::OnScrollUpdate(float offset) {
  if (is_scroll_animating_) {
    if (initial_scrolling_estimated_offset_ != 0) {
      offset *=
          scrolling_estimated_offset_ / initial_scrolling_estimated_offset_;
    }
    if (scrolling_estimated_offset_ > 0) {
      if ((scroll_to_lower_ && offset > scrolling_estimated_offset_) ||
          (!scroll_to_lower_ && offset < scrolling_estimated_offset_)) {
        offset = scrolling_estimated_offset_;
      }
    }
  }
  ScrollView::OnScrollUpdate(offset);
}

void ListContainerView::OnScrollAnimationStart(float start, float delta,
                                               int duration) {
  ScrollView::OnScrollAnimationStart(start, delta, duration);
  scroll_to_lower_ = delta > 0;
  initial_scrolling_estimated_offset_ = start + delta;
}

void ListContainerView::DidUpdateAttributes() {
  ScrollView::DidUpdateAttributes();
  if (enable_list_sticky_ && update_sticky_for_diff_) {
    GenerateStickyItemKeySet(sticky_top_item_key_set_, sticky_top_item_map_,
                             sticky_top_indexes_);
    GenerateStickyItemKeySet(sticky_bottom_item_key_set_,
                             sticky_bottom_item_map_, sticky_bottom_indexes_);
  }
}

clay::Value::Array ListContainerView::GetVisibleCells() {
  std::vector<float> left_array, right_array, top_array, bottom_array;
  std::vector<int> position_array;
  std::vector<std::string> id_array;
  std::vector<std::string> item_key_array;

  size_t visible_count =
      GetVisibleItemsInfo(top_array, bottom_array, left_array, right_array,
                          position_array, id_array, item_key_array);

  clay::Value::Array cells_array(visible_count);
  for (size_t i = 0; i < visible_count; i++) {
    clay::Value::Map cell;
    cell["id"] = clay::Value(id_array[i]);
    cell["position"] = clay::Value(position_array[i]);
    cell["index"] = clay::Value(position_array[i]);
    cell["left"] = clay::Value(left_array[i]);
    cell["right"] = clay::Value(right_array[i]);
    cell["top"] = clay::Value(top_array[i]);
    cell["bottom"] = clay::Value(bottom_array[i]);
    cell["itemKey"] = clay::Value(item_key_array[i]);
    cells_array[i] = clay::Value(std::move(cell));
  }

  return cells_array;
}

size_t ListContainerView::GetVisibleItemsInfo(
    std::vector<float>& top_array, std::vector<float>& bottom_array,
    std::vector<float>& left_array, std::vector<float>& right_array,
    std::vector<int>& position, std::vector<std::string>& id_array,
    std::vector<std::string>& item_key_array) {
  std::vector<BaseView*> visible_children;
  for (BaseView* child : children_) {
    if (child && child->Width() != 0 && child->Height() != 0) {
      visible_children.emplace_back(child);
    }
  }
  if (!visible_children.empty()) {
    std::sort(visible_children.begin(), visible_children.end(),
              [this](BaseView* lhs, BaseView* rhs) {
                return GetIndexFromItemKey(lhs->ItemKey()) <
                       GetIndexFromItemKey(rhs->ItemKey());
              });
    for (size_t i = 0; i < visible_children.size(); ++i) {
      auto item = visible_children[i];
      position.push_back(GetIndexFromItemKey(item->ItemKey()));
      auto rect = page_view_->ConvertTo<kPixelTypeLogical>(
          item->BoundsRelativeTo(this));
      top_array.push_back(rect.y());
      bottom_array.push_back(rect.MaxY());
      left_array.push_back(rect.x());
      right_array.push_back(rect.MaxX());
      id_array.push_back(item->GetIdSelector());
      item_key_array.push_back(item->ItemKey());
    }
  }
  return top_array.size();
}

}  // namespace clay
