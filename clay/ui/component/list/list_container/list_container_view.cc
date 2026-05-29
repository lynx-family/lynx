// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_container/list_container_view.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

#include "base/include/auto_reset.h"
#include "base/include/float_comparison.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view_callback/list_container_event_callback_manager.h"
#include "clay/ui/lynx_module/type_utils.h"

namespace clay {

namespace details {
static constexpr const char kDataSourceItemKeys[] = "itemkeys";
static constexpr const char kDataSourceStickyStart[] = "stickyStart";
static constexpr const char kDataSourceStickyEnd[] = "stickyEnd";
}  // namespace details

ListContainerView::ListContainerView(int32_t id, PageView* page_view,
                                     int32_t callback_id)
    : WithTypeInfo(id, callback_id, ScrollDirection::kVertical, page_view,
                   std::make_unique<ListContainerEventCallbackManager>(
                       this, callback_id, page_view),
                   std::make_unique<RenderScroll>()) {
  tag_ = "ListContainerView";
  AddEventCallback(event_attr::kEventScrollStateChange);
  // We skip correct scroll offset by default for ListContainerView. It's only
  // necessary when the max content size is updated.
  skip_correct_scroll_offset_ = true;
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
#ifdef ENABLE_ACCESSIBILITY
      MarkRebuildSemanticsTree();
#endif
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
  } else if (kw == KeywordID::kItemSnap) {
    ResolveItemSnapProp(value);
  } else if (kw == KeywordID::kEnableInsertPlatformViewOperation) {
    enable_insert_platform_view_operation_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kNeedVisibleItemInfo ||
             kw == KeywordID::kNeedsVisibleCells) {
    need_visible_item_info_ = attribute_utils::GetBool(value);
    GetEventCallbackManager()->SetNeedsVisibleCells(need_visible_item_info_);
  } else {
    ScrollView::SetAttribute(attr_c, value);
  }
}

int ListContainerView::GetIndexFromItemKey(std::string item_key) const {
  if (item_key.empty()) {
    return -1;
  }
  auto it = item_key_map_.find(item_key);
  if (it == item_key_map_.end()) {
    return -1;
  }
  return it->second;
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

void ListContainerView::ResolveItemSnapProp(const clay::Value& value) {
  ResetItemSnapProp();

  if (!value.IsMap()) {
    return;
  }

  const auto& map = attribute_utils::GetMap(value);

  auto iter = map.find("factor");
  if (iter != map.end() && iter->second.IsNumber()) {
    snap_factor_ =
        std::clamp(attribute_utils::GetDouble(iter->second), 0.0, 1.0);
  }

  iter = map.find("offset");
  if (iter != map.end() && iter->second.IsNumber()) {
    snap_offset_ = attribute_utils::GetDouble(iter->second);
  }
}

void ListContainerView::ResetItemSnapProp() {
  snap_factor_ = -1;
  snap_offset_ = 0;
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
  if (!view) {
    return;
  }
  InsertListItemPaintingNodeInternal(view);
  if (auto* render_object = view->render_object()) {
    render_object->SetRepaintBoundary(true);
  }
  if (view->Is<Component>()) {
    auto component = static_cast<Component*>(view);
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
#ifdef ENABLE_ACCESSIBILITY
  MarkRebuildSemanticsTree();
#endif
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
  lynx::base::AutoReset<bool> resetter(&should_block_did_scroll_, true);
  SetMaxContent(content_size);
  if (CanScrollY()) {
    OnScrollUpdate(scroll_offset_.y() + target_content_offset_y);
  } else {
    OnScrollUpdate(scroll_offset_.x() + target_content_offset_x);
  }
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

#ifdef ENABLE_ACCESSIBILITY
void ListContainerView::PrepareSemantics(
    fml::RefPtr<SemanticsNode> parent_node,
    std::vector<fml::RefPtr<SemanticsNode>>& result,
    const std::vector<std::string>& ancestor_a11y_elements) {
  FML_DCHECK(GetSemanticsOwner() &&
             GetSemanticsOwner()->NeedRebuildSemanticsTree());
  FML_DCHECK(Parent());

  std::vector<BaseView*> ordered_children = children_;
  std::stable_sort(
      ordered_children.begin(), ordered_children.end(),
      [this](BaseView* lhs, BaseView* rhs) {
        const int lhs_index = lhs ? GetIndexFromItemKey(lhs->ItemKey()) : -1;
        const int rhs_index = rhs ? GetIndexFromItemKey(rhs->ItemKey()) : -1;
        const bool lhs_has_index = lhs_index >= 0;
        const bool rhs_has_index = rhs_index >= 0;
        if (lhs_has_index && rhs_has_index && lhs_index != rhs_index) {
          return lhs_index < rhs_index;
        }
        if (lhs_has_index != rhs_has_index) {
          return lhs_has_index;
        }
        return false;
      });

  PrepareSemanticsWithChildren(ordered_children, parent_node, result,
                               ancestor_a11y_elements);
}
#endif

bool ListContainerView::OnScrollToMiddle(BaseView* target_view) {
  if (!target_view) {
    FML_DLOG(ERROR) << "OnScrollToMiddle but view is nullptr";
    return false;
  }

  BaseView* item_view = nullptr;
  for (BaseView* current = target_view; current && current != this;
       current = current->Parent()) {
    if (!current->ItemKey().empty()) {
      item_view = current;
    }
    if (current->Parent() == this) {
      if (!item_view) {
        item_view = current;
      }
      break;
    }
  }

  if (!item_view) {
    return ScrollView::OnScrollToMiddle(target_view);
  }

  int index = GetIndexFromItemKey(item_view->ItemKey());
  if (index < 0) {
    return ScrollView::OnScrollToMiddle(target_view);
  }

  ScrollToPosition(false, index, 0.f, AlignTo::kMiddle,
                   target_view->GetIdSelector(), nullptr);
  return true;
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
    Component* child, std::unordered_map<int, Component*>& sticky_items,
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

void ListContainerView::OnNodeReady() {
  ScrollView::OnNodeReady();
  UpdateStickyStarts(scroll_offset_.x(), scroll_offset_.y());
  UpdateStickyEnds(scroll_offset_.x(), scroll_offset_.y());
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

  bool is_vertical = CanScrollY();

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
        page_view_->SendEvent(GetCallbackId(), event_attr::kEventStickyBottom,
                              {"bottom"}, sticky_end_item->ItemKey());
      }
      page_view_->SendEvent(GetCallbackId(), event_attr::kEventStickyEnd,
                            {"end"}, sticky_end_item->ItemKey());
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
      for (auto iter = sticky_item_map.begin();
           iter != sticky_item_map.end();) {
        if (iter->second == component && iter->first != item_key) {
          iter = sticky_item_map.erase(iter);
        } else {
          ++iter;
        }
      }
      sticky_item_map[item_key] = component;
      component->SetNodeReadyListener(this);
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

  bool is_vertical = CanScrollY();

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
        page_view_->SendEvent(GetCallbackId(), event_attr::kEventStickyTop,
                              {"top"}, sticky_start_item->ItemKey());
      }

      page_view_->SendEvent(GetCallbackId(), event_attr::kEventStickyStart,
                            {"start"}, sticky_start_item->ItemKey());

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
  for (auto* child : children_) {
    if (!child || !child->Is<Component>()) {
      continue;
    }
    auto* component = static_cast<Component*>(child);
    const auto& item_key = component->ItemKey();
    if (!item_key.empty() && sticky_item_key_set.count(item_key)) {
      sticky_item_map[item_key] = component;
      component->SetNodeReadyListener(this);
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
  auto* scroll = GetRenderScroll();
  float old_max_content = max_content_;
  max_content_ = value;
  if (CanScrollY()) {
    scroll->SetOverflowRect(FloatRect(0, 0, scroll->Width(), max_content_));
  } else {
    scroll->SetOverflowRect(FloatRect(0, 0, max_content_, scroll->Height()));
  }
  if (old_max_content != max_content_) {
    // When max content changes, we need to correct scroll offset if necessary.
    lynx::base::AutoReset<bool> resetter(&skip_correct_scroll_offset_, false);
    CorrectScrollOffset();
    // Stop bounce animation if necessary.
    Scrollable::StopAnimation();
    // Clear overscroll state if necessary.
    ClearOverscrollState();
  }
}

void ListContainerView::HandleEvent(const PointerEvent& event) {
  if (snap_factor_ >= 0 &&
      (event.device == PointerEvent::DeviceType::kTouch ||
       event.device == PointerEvent::DeviceType::kMouse ||
       event.device == PointerEvent::DeviceType::kTrackpad)) {
    DetectSnapScroll(event.type);
  }
  ScrollView::HandleEvent(event);
}

void ListContainerView::OnScrollStatusChange(ScrollStatus old_status) {
  // NOTE: Here we skip the call of ScrollView::OnScrollStatusChange, because
  // the logic is useless for list container.
  NestedScrollable::OnScrollStatusChange(old_status);

  is_scroll_animating_ = status_ == Scrollable::ScrollStatus::kAnimating;

  switch (status_) {
    case Scrollable::ScrollStatus::kFling:
      SetScrollState(ListScrollState::kSettling);
      break;
    case Scrollable::ScrollStatus::kBounce:
      SetScrollState(ListScrollState::kSettling);
      break;
    case Scrollable::ScrollStatus::kDragging:
      last_scroll_offset_for_snap_ = scroll_offset_;
      SetScrollState(ListScrollState::kDragging);
      break;
    case Scrollable::ScrollStatus::kIdle:
      SetScrollState(ListScrollState::kIdle);
      break;
    default:
      break;
  }
}

void ListContainerView::SetScrollState(ListScrollState state) {
  if (scroll_state_ == state) {
    return;
  }

  scroll_state_ = state;
  if (state == ListScrollState::kIdle && delegate_) {
    delegate_->OnScrollStopped();
  }

  clay::Value::Map args;
  args["state"] = clay::Value(static_cast<int>(state));

  if (need_visible_item_info_) {
    auto cells_array = GetVisibleCells();
    args["attachedCells"] = clay::Value(std::move(cells_array));
  }

  page_view_->SendCustomEvent(
      GetCallbackId(), event_attr::kEventScrollStateChange, std::move(args));
}

void ListContainerView::DidScroll() {
  ScrollView::DidScroll();
  if (!should_block_did_scroll_ && delegate_) {
    delegate_->OnScrollByListContainer(scroll_offset_.x(), scroll_offset_.y(),
                                       scroll_offset_.x(), scroll_offset_.y());
    UpdateStickyStarts(scroll_offset_.x(), scroll_offset_.y());
    UpdateStickyEnds(scroll_offset_.x(), scroll_offset_.y());
  }
}

void ListContainerView::OnScrollUpdate(float offset) {
  if (status_ == Scrollable::ScrollStatus::kDragging) {
    last_scroll_offset_for_snap_ = scroll_offset_;
  }
  if (is_scroll_animating_ && !should_block_did_scroll_) {
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

void ListContainerView::DetectSnapScroll(PointerEvent::EventType type) {
  if (snap_factor_ >= 0) {
    switch (type) {
      case PointerEvent::EventType::kDownEvent:
      case PointerEvent::EventType::kMoveEvent:
      case PointerEvent::EventType::kPanZoomUpdateEvent:
        last_scroll_offset_for_snap_ = scroll_offset_;
        break;
      case PointerEvent::EventType::kUpEvent:
      case PointerEvent::EventType::kPanZoomEndEvent: {
        bool is_vertical = CanScrollY();
        bool forward = false;
        if (is_vertical) {
          if (last_scroll_offset_for_snap_.y() == scroll_offset_.y() &&
              scroll_offset_.y() == 0) {
            forward = false;
          } else {
            forward = scroll_offset_.y() >= last_scroll_offset_for_snap_.y();
          }
        } else {
          if (last_scroll_offset_for_snap_.x() == scroll_offset_.x() &&
              scroll_offset_.x() == 0) {
            forward = false;
          } else {
            forward = scroll_offset_.x() >= last_scroll_offset_for_snap_.x();
          }
        }

        bool has_velocity =
            is_vertical
                ? last_scroll_offset_for_snap_.y() != scroll_offset_.y()
                : last_scroll_offset_for_snap_.x() != scroll_offset_.x();

        auto scroll_target = CalcSnapScroll(forward, has_velocity);
        int32_t scroll_position = std::get<0>(scroll_target);
        if (scroll_position != -1) {
          float target_x = std::get<1>(scroll_target);
          float target_y = std::get<2>(scroll_target);

          float target_offset = is_vertical ? target_y : target_x;

          // ScrollTo expects an absolute content offset.
          // However, we must ensure we don't scroll beyond bounds (though
          // ScrollTo handles some clamping, explicit safety is better).
          // And importantly, ensure we are not already there to avoid
          // unnecessary calls/animations.
          ScrollTo(true, target_offset);
        }

        if (page_view_) {
          clay::Value::Map dict;
          dict["position"] = clay::Value(scroll_position);
          dict["currentScrollLeft"] = clay::Value(scroll_offset_.x());
          dict["currentScrollTop"] = clay::Value(scroll_offset_.y());
          dict["targetScrollLeft"] = clay::Value(std::get<1>(scroll_target));
          dict["targetScrollTop"] = clay::Value(std::get<2>(scroll_target));
          page_view_->SendCustomEvent(GetCallbackId(), "snap", std::move(dict));
        }
      } break;
      case PointerEvent::EventType::kCancel:
        break;
      default:
        break;
    }
  }
}

std::tuple<int32_t, float, float> ListContainerView::CalcSnapScroll(
    bool forward, bool has_velocity) const {
  bool is_vertical = CanScrollY();
  float content_offset = is_vertical ? scroll_offset_.y() : scroll_offset_.x();

  Component* closest_item_before = nullptr;
  Component* closest_item_after = nullptr;

  float distance_before = std::numeric_limits<float>::lowest();
  float distance_after = std::numeric_limits<float>::max();
  float max_scroll_range = GetScrollRange();
  float min_scroll_range = 0.f;

  float viewport_size = is_vertical ? Height() : Width();
  float viewport_start = content_offset;
  float viewport_end = content_offset + viewport_size;
  float check_start = viewport_start - viewport_size;
  float check_end = viewport_end + viewport_size;

  for (BaseView* child : children_) {
    if (child && child->Is<Component>()) {
      auto* list_item = static_cast<Component*>(child);
      // Only consider visible items or items close to viewport
      // Simple visibility check:
      float item_start = is_vertical ? list_item->Top() : list_item->Left();
      float item_end = is_vertical ? (list_item->Top() + list_item->Height())
                                   : (list_item->Left() + list_item->Width());

      if (item_end >= check_start && item_start <= check_end) {
        float item_snap_offset = GetListItemSnapScrollOffset(list_item);
        float clamped_snap_offset =
            std::clamp(item_snap_offset, min_scroll_range, max_scroll_range);

        float distance = clamped_snap_offset - content_offset;

        if (lynx::base::FloatsLargerOrEqual(0, distance) &&
            distance > distance_before) {
          distance_before = distance;
          closest_item_before = list_item;
        }
        if (lynx::base::FloatsLargerOrEqual(distance, 0) &&
            distance < distance_after) {
          distance_after = distance;
          closest_item_after = list_item;
        }
      }
    }
  }

  int32_t target_position = -1;
  Component* target_item = nullptr;

  if (!has_velocity) {
    if (closest_item_after && closest_item_before) {
      if (distance_after < std::abs(distance_before)) {
        target_item = closest_item_after;
      } else {
        target_item = closest_item_before;
      }
    } else if (closest_item_after) {
      target_item = closest_item_after;
    } else if (closest_item_before) {
      target_item = closest_item_before;
    }
  } else {
    if (forward && closest_item_after) {
      target_item = closest_item_after;
    } else if (!forward && closest_item_before) {
      target_item = closest_item_before;
    }
  }

  if (target_item) {
    target_position = GetIndexFromItemKey(target_item->ItemKey());
    auto offsets = CalculateOffsets(target_item);
    return {target_position, offsets.first, offsets.second};
  }

  // Fallback: extrapolate if no item found (edge case)
  Component* visible_item = forward ? closest_item_before : closest_item_after;
  if (!visible_item) {
    visible_item = forward ? closest_item_after : closest_item_before;
  }
  if (!visible_item) return {-1, scroll_offset_.x(), scroll_offset_.y()};

  target_position =
      GetIndexFromItemKey(visible_item->ItemKey()) + (forward ? 1 : -1);
  if (target_position < 0) target_position = 0;

  // Try to find item by index
  // Since children are not strictly ordered by index in children_ vector
  // (z-index affects order), and we might not have all items loaded, this is
  // tricky. For now, return current offset if target item not found.
  for (BaseView* child : children_) {
    if (child && child->Is<Component>()) {
      auto* list_item = static_cast<Component*>(child);
      if (GetIndexFromItemKey(list_item->ItemKey()) == target_position) {
        auto offsets = CalculateOffsets(list_item);
        return {target_position, offsets.first, offsets.second};
      }
    }
  }

  return {-1, scroll_offset_.x(), scroll_offset_.y()};
}

float ListContainerView::GetListItemSnapScrollOffset(
    Component* list_item) const {
  if (CanScrollX()) {
    return list_item->Left() - (Width() - list_item->Width()) * snap_factor_ +
           snap_offset_;
  } else {
    return list_item->Top() - (Height() - list_item->Height()) * snap_factor_ +
           snap_offset_;
  }
}

std::pair<float, float> ListContainerView::CalculateOffsets(
    Component* item) const {
  bool is_vertical = CanScrollY();
  float range = GetScrollRange();
  float snap_offset = GetListItemSnapScrollOffset(item);
  float clamped_offset = std::clamp(snap_offset, 0.f, range);

  if (is_vertical) {
    return {0.f, clamped_offset};
  } else {
    return {clamped_offset, 0.f};
  }
}

float ListContainerView::GetScrollRange() const {
  bool is_horizontal = CanScrollX();
  float content_size = is_horizontal
                           ? GetRenderScroll()->OverflowRect().width()
                           : GetRenderScroll()->OverflowRect().height();
  float view_size = is_horizontal ? Width() : Height();
  return std::max(0.f, content_size - view_size);
}

size_t ListContainerView::GetVisibleItemsInfo(
    std::vector<float>& top_array, std::vector<float>& bottom_array,
    std::vector<float>& left_array, std::vector<float>& right_array,
    std::vector<int>& position, std::vector<std::string>& id_array,
    std::vector<std::string>& item_key_array) {
  std::vector<BaseView*> visible_children;
  FloatRect viewport(0, 0, Width(), Height());
  for (BaseView* child : children_) {
    if (!child) {
      continue;
    }
    auto item_rect = child->BoundsRelativeTo(this);
    if (item_rect.Intersects(viewport)) {
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
      auto rect = item->BoundsRelativeTo(this);
      auto logical_rect = page_view_->ConvertTo<kPixelTypeLogical>(rect);
      top_array.push_back(logical_rect.y());
      bottom_array.push_back(logical_rect.MaxY());
      left_array.push_back(logical_rect.x());
      right_array.push_back(logical_rect.MaxX());
      id_array.push_back(item->GetIdSelector());
      item_key_array.push_back(item->ItemKey());
    }
  }
  return top_array.size();
}

void ListContainerView::OnOverscroll(FloatPoint prev_overscroll_offset) {
  lynx::base::AutoReset<bool> resetter(&should_block_did_scroll_, true);
  ScrollView::OnOverscroll(prev_overscroll_offset);
}

}  // namespace clay
