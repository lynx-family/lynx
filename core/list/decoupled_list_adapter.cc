// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_adapter.h"

#include <unordered_set>

#include "core/list/decoupled_list_container_impl.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

ListAdapter::ListAdapter(ListContainerImpl* list_container_impl)
    : list_container_(list_container_impl),
      item_holder_map_(std::make_unique<ItemHolderMap>()),
      adapter_helper_(std::make_unique<AdapterHelper>()) {
  if (!list_container_) {
    DLIST_LOGE("ListAdapter::ListAdapter: list_container_ is nullptr");
  }
  adapter_helper_->SetDelegate(this);
}

void ListAdapter::OnErrorOccurred(lynx::base::LynxError error) {
  list_container_->list_delegate()->OnErrorOccurred(std::move(error));
}

// Update data source for radon diff arch.
std::pair<ListAdapterDiffResult, bool> ListAdapter::UpdateRadonDataSource(
    const pub::Value& radon_data_source) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_UPDATE_DATA_SOURCE);
  if (!radon_data_source.IsMap()) {
    return {ListAdapterDiffResult::kNone, false};
  }
  bool has_updated = false;
  // Parse diff result.
  radon_data_source.ForeachMap(
      [this, &has_updated](const pub::Value& key, const pub::Value& value) {
        if (key.IsString() && key.str() == kRadonDataDiffResult) {
          has_updated = adapter_helper_->UpdateRadonDiffResult(value);
        }
      });
  // TODO(@hujing.1):refactor the code according to the corresponding vector
  // if the diffResult:insert[0,1,2,3],update from:[0,1,2,3],update
  // To:[4,5,6,7].
  // Firstly: should handle the removed actions and inserted actions,to keep the
  // data size is right.
  for (const auto& cur : adapter_helper_->removals()) {
    ItemHolder* child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderRemoved(child_holder);
    }
  }
  // Secondly: according to the old data,to keep itemHolder status right.
  for (const auto& cur : adapter_helper_->move_from()) {
    ItemHolder* child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderMovedFrom(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->update_from()) {
    ItemHolder* child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderUpdateFrom(child_holder);
    }
  }
  // init list_container_info
  const auto& value_factory = list_container_->value_factory();
  std::unique_ptr<pub::Value> list_container_info;
  if (value_factory) {
    list_container_info = value_factory->CreateMap();
  }
  radon_data_source.ForeachMap(
      [this, &list_container_info](const pub::Value& key,
                                   const pub::Value& value) {
        if (key.IsString()) {
          const std::string& key_str = key.str();
          if (key_str == kRadonDataEstimatedHeightPx) {
            adapter_helper_->UpdateEstimatedHeightsPx(value);
          } else if (key_str == kRadonDataEstimatedMainAxisSizePx) {
            adapter_helper_->UpdateEstimatedSizesPx(value);
          } else if (key_str == kRadonDataFullSpan) {
            adapter_helper_->UpdateFullSpans(value);
          } else if (key_str == kRadonDataStickyTop) {
            adapter_helper_->UpdateStickyTops(value);
            if (list_container_info) {
              list_container_info->PushValueToMap(kListContainerInfoStickyStart,
                                                  value);
            }
          } else if (key_str == kRadonDataStickyBottom) {
            adapter_helper_->UpdateStickyBottoms(value);
            if (list_container_info) {
              list_container_info->PushValueToMap(kListContainerInfoStickyEnd,
                                                  value);
            }
          } else if (key_str == kRadonDataItemKeys) {
            adapter_helper_->UpdateItemKeys(value);
            if (list_container_info) {
              list_container_info->PushValueToMap(kListContainerInfoItemKeys,
                                                  value);
            }
          }
        }
      });
  // Thirdly: update the item holder status according to the new diffResult.
  for (const auto& cur : adapter_helper_->move_to()) {
    ItemHolder* child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderMovedTo(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->update_to()) {
    ItemHolder* child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderUpdateTo(child_holder, false);
    }
  }
  // Flush list-container-info
  if (list_container_info) {
    list_container_->list_delegate()->FlushListContainerInfo(
        kListContainerInfo, std::move(list_container_info), false);
  }
  // For output list diff info before clear
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_OUTPUT_DATA_SOURCE_DIFF_INFO,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto result = ListAdapterDiffResult::kNone;
  if (has_updated) {
    if (!adapter_helper_->removals().empty()) {
      result |= ListAdapterDiffResult::kRemove;
    }
    if (!adapter_helper_->insertions().empty()) {
      result |= ListAdapterDiffResult::kInsert;
    }
    if (!adapter_helper_->update_from().empty() &&
        !adapter_helper_->update_to().empty()) {
      result |= ListAdapterDiffResult::kUpdate;
    }
    if (!adapter_helper_->move_from().empty() &&
        !adapter_helper_->move_to().empty()) {
      result |= ListAdapterDiffResult::kMove;
    }
  }
  return {result, HasExpectedDiffAnimation()};
}

bool ListAdapter::HasExpectedDiffAnimation() const {
  return !(adapter_helper_->insertions().size() ==
               adapter_helper_->item_keys().size() &&
           adapter_helper_->update_from().empty() &&
           adapter_helper_->update_to().empty() &&
           adapter_helper_->move_from().empty() &&
           adapter_helper_->move_to().empty() &&
           adapter_helper_->removals().empty());
}

// Update data source for fiber arch.
std::pair<ListAdapterDiffResult, bool> ListAdapter::UpdateFiberDataSource(
    const pub::Value& fiber_data_source) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_UPDATE_FIVER_DATA_SOURCE);
  if (!fiber_data_source.IsMap()) {
    return {ListAdapterDiffResult::kNone, false};
  }
  std::unique_ptr<pub::Value> insert_action =
      fiber_data_source.GetValueForKey(kFiberDataInsertAction);
  std::unique_ptr<pub::Value> remove_action =
      fiber_data_source.GetValueForKey(kFiberDataRemoveAction);
  std::unique_ptr<pub::Value> update_action =
      fiber_data_source.GetValueForKey(kFiberDataUpdateAction);
  // Firstly only generate insert / remove / update arrays.
  adapter_helper_->UpdateFiberRemoveAction(remove_action);
  adapter_helper_->UpdateFiberInsertAction(insert_action);
  adapter_helper_->UpdateFiberUpdateAction(update_action);
  // Mark dirty based on index.
  MarkChildHolderDirty();
  // Parse extra info from insert / remove / update actions.
  adapter_helper_->UpdateFiberRemoveAction(remove_action, false);
  adapter_helper_->UpdateFiberInsertAction(insert_action, false);
  adapter_helper_->UpdateFiberUpdateAction(update_action, false);
  // Update extra info.
  adapter_helper_->UpdateFiberExtraInfo();
  // Generate and flush list-container-info for fiber.
  GenerateAndFlushListContainerInfo();
  // For output list diff info before clear
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              LIST_ADAPTER_OUTPUT_FIBER_DATA_SOURCE_DIFF_INFO,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (adapter_helper_->HasValidDiff()) {
    auto result = ListAdapterDiffResult::kNone;
    if (!adapter_helper_->removals().empty()) {
      result |= ListAdapterDiffResult::kRemove;
    }
    if (!adapter_helper_->insertions().empty()) {
      result |= ListAdapterDiffResult::kInsert;
    }
    if (!adapter_helper_->update_from().empty() &&
        !adapter_helper_->update_to().empty()) {
      result |= ListAdapterDiffResult::kUpdate;
    }
    if (!adapter_helper_->move_from().empty() &&
        !adapter_helper_->move_to().empty()) {
      result |= ListAdapterDiffResult::kMove;
    }
    return {result, HasExpectedDiffAnimation()};
  } else {
    return {ListAdapterDiffResult::kNone, false};
  }
}

void ListAdapter::GenerateAndFlushListContainerInfo() {
  const auto& value_factory = list_container_->value_factory();
  if (value_factory) {
    std::unique_ptr<pub::Value> list_container_info =
        value_factory->CreateMap();
    std::unique_ptr<pub::Value> sticky_start_indexes =
        value_factory->CreateArray();
    std::unique_ptr<pub::Value> sticky_end_indexes =
        value_factory->CreateArray();
    std::unique_ptr<pub::Value> item_keys = value_factory->CreateArray();
    if (list_container_info) {
      if (sticky_start_indexes) {
        for (const auto& index : adapter_helper_->sticky_tops()) {
          sticky_start_indexes->PushInt32ToArray(index);
        }
        list_container_info->PushValueToMap(kListContainerInfoStickyStart,
                                            *sticky_start_indexes);
      }
      if (sticky_end_indexes) {
        for (const auto& index : adapter_helper_->sticky_bottoms()) {
          sticky_end_indexes->PushInt32ToArray(index);
        }
        list_container_info->PushValueToMap(kListContainerInfoStickyEnd,
                                            *sticky_end_indexes);
      }
      if (item_keys) {
        for (const auto& item_key : adapter_helper_->item_keys()) {
          item_keys->PushStringToArray(item_key);
        }
        list_container_info->PushValueToMap(kListContainerInfoItemKeys,
                                            *item_keys);
      }
      list_container_->list_delegate()->FlushListContainerInfo(
          kListContainerInfo, std::move(list_container_info), true);
    }
  }
}

// Update the latest data source to the ItemHolder and add updated ItemHolders
// to children_ set in ChildrenHelper. If has new insertions, create ItemHolders
// and add them to the ItemHolder map.
void ListAdapter::UpdateItemHolderToLatest(
    ListChildrenHelper* list_children_helper) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_UPDATE_ITEM_TO_LATEST);
  if (!list_children_helper) {
    return;
  }
  // Update anchor ref for removed on screen children.
  // Note: This logic should be invoked before latest diff info being updated to
  // all ItemHolders, so here UpdateAnchorRefItem() is invoked in the begin of
  // UpdateItemHolderToLatest()
  UpdateAnchorRefItem(list_children_helper);

  const auto& children = list_children_helper->children();
  const auto& attached_children = list_children_helper->attached_children();
  const auto& last_binding_children =
      list_children_helper->last_binding_children();
  std::unordered_set<ItemHolder*> attached_children_set(
      attached_children.begin(), attached_children.end());
  std::unordered_set<ItemHolder*> last_binding_children_set(
      last_binding_children.begin(), last_binding_children.end());
  list_children_helper->ClearChildren();
  // Note: If has diff info, the attached children set needs to be rebuilt to
  // delete removed item holders.
  list_children_helper->ClearAttachedChildren();
  list_children_helper->ClearLastBindingChildren();
  ItemHolder* item_holder = nullptr;
  auto it = item_holder_map_->end();
  for (const auto& pair : adapter_helper_->item_key_map()) {
    const auto& item_key = pair.first;
    int new_index = pair.second;
    it = item_holder_map_->find(item_key);
    if (item_holder_map_->end() != it) {
      item_holder = it->second.get();
      // One component with item-key == 'x' removed from 5 and inserted to 10,
      // here the item_holder is marked removed. We reuse this item_holder for
      // index 10, so we should clear removed flag and mark dirty.
      if (IsRemoved(item_holder)) {
        OnItemHolderReInsert(item_holder);
      }
    } else {
      ListAnimationManager* list_animation_manager =
          list_container_->list_animation_manager();
      (*item_holder_map_)[item_key] = std::make_unique<ItemHolder>(
          new_index, item_key, list_animation_manager);
      item_holder = (*item_holder_map_)[item_key].get();
      if (list_animation_manager->AnimationType() != ListAnimationType::kNone) {
        item_holder->MarkInsertOpacity();
      }
      OnItemHolderInserted(item_holder);
    }
    CheckSticky(item_holder, new_index);
    item_holder->SetIndex(new_index);
    item_holder->SetItemFullSpan(IsFullSpanAtIndex(new_index));
    item_holder->SetEstimatedSize(GetEstimatedSizeForIndex(new_index));
    item_holder->SetRecyclable(IsRecyclableAtIndex(new_index));
    list_children_helper->AddChild(children, item_holder);
    if (attached_children_set.find(item_holder) !=
        attached_children_set.end()) {
      // Add item holder to attached children.
      ItemElementDelegate* list_item_delegate =
          GetItemElementDelegate(item_holder);
      list_children_helper->AttachChild(item_holder, list_item_delegate);
    }
    if (last_binding_children_set.find(item_holder) !=
        last_binding_children_set.end()) {
      // Add item holder to last binding children.
      list_children_helper->AddChild(last_binding_children, item_holder);
    }
  }
}

void ListAdapter::UpdateAnchorRefItem(
    ListChildrenHelper* list_children_helper) {
  const auto& children = list_children_helper->children();
  const auto& on_screen_children = list_children_helper->on_screen_children();
  bool has_valid_diff = list_adapter_helper()->HasValidDiff();
  bool should_search_ref_anchor = list_container_->ShouldSearchRefAnchor();
  SearchRefAnchorStrategy search_strategy =
      list_container_->search_ref_anchor_strategy();
  std::unordered_map<ItemHolder*, ItemHolder*> tmp_anchor_ref_map;
  if (has_valid_diff && should_search_ref_anchor &&
      !on_screen_children.empty() && !children.empty()) {
    list_children_helper->ForEachChild(
        on_screen_children,
        [this, list_children_helper, &children, &tmp_anchor_ref_map,
         search_strategy](ItemHolder* on_screen_child) {
          // We only need to update removed on screen child.
          if (IsRemoved(on_screen_child)) {
            auto weak_anchor_ref = on_screen_child->weak_anchor_ref();
            if (!weak_anchor_ref) {
              // (1) Removed child's weak_anchor_ref is never set.
              if (auto it = children.find(on_screen_child);
                  it != children.end()) {
                ItemHolder* anchor_ref_child =
                    list_children_helper->GetFirstChildFrom(
                        children, on_screen_child,
                        [this, on_screen_child](const ItemHolder* item_holder) {
                          return item_holder != on_screen_child &&
                                 !IsRemoved(item_holder);
                        },
                        search_strategy == SearchRefAnchorStrategy::kToStart);
                on_screen_child->SetWeakAnchorRef(anchor_ref_child);
              } else {
                // Unexpected case: child is not in children set.
                DLIST_LOGE("[" << list_container_
                               << "] ListAdapter::UpdateItemHolderToLatest: "
                                  "on_screen_child is not at children set and "
                                  "it's weak_anchor_ref is never set: index="
                               << on_screen_child->index()
                               << ", item-key=" << on_screen_child->item_key());
                on_screen_child->SetWeakAnchorRef(nullptr);
              }
            } else if (weak_anchor_ref && (*weak_anchor_ref)) {
              // (2) Update weak_anchor_ref if needed.
              ItemHolder* current_anchor_ref_child = (*weak_anchor_ref).get();
              if (IsRemoved(current_anchor_ref_child)) {
                // Current anchor ref child is removed, need to find a new
                // anchor ref.
                if (auto it = tmp_anchor_ref_map.find(current_anchor_ref_child);
                    it != tmp_anchor_ref_map.end()) {
                  // Fast find new ref anchor child from tmp_anchor_ref_map
                  on_screen_child->SetWeakAnchorRef(it->second);
                } else {
                  if (auto it = children.find(current_anchor_ref_child);
                      it != children.end()) {
                    ItemHolder* new_anchor_ref_child =
                        list_children_helper->GetFirstChildFrom(
                            children, current_anchor_ref_child,
                            [this, current_anchor_ref_child](
                                const ItemHolder* item_holder) {
                              return item_holder != current_anchor_ref_child &&
                                     !IsRemoved(item_holder);
                            },
                            search_strategy ==
                                SearchRefAnchorStrategy::kToStart);
                    tmp_anchor_ref_map[current_anchor_ref_child] =
                        new_anchor_ref_child;
                    on_screen_child->SetWeakAnchorRef(new_anchor_ref_child);
                  } else {
                    tmp_anchor_ref_map[current_anchor_ref_child] = nullptr;
                    on_screen_child->SetWeakAnchorRef(nullptr);
                  }
                }
              }
            }
          }
          return false;
        });
  }
}

// Mark all child ItemHolders's diff status.
void ListAdapter::MarkChildHolderDirty() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_MARK_CHILD_DIRTY);
  if (!adapter_helper_) {
    return;
  }
  ItemHolder* child_holder = nullptr;
  for (const auto& cur : adapter_helper_->removals()) {
    if ((child_holder = GetItemHolderForIndex(cur))) {
      OnItemHolderRemoved(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->move_to()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderMovedTo(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->move_from()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderMovedFrom(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->update_to()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderUpdateTo(child_holder, true);
    }
  }
  for (const auto& cur : adapter_helper_->update_from()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderUpdateFrom(child_holder);
    }
  }
}

// Get the ItemHolder for the specified index.
ItemHolder* ListAdapter::GetItemHolderForIndex(int index) {
  if (adapter_helper_ && index >= 0 && index < GetDataCount()) {
    auto item_key = adapter_helper_->GetItemKeyForIndex(index);
    if (item_key &&
        item_holder_map_->end() != item_holder_map_->find(*item_key)) {
      return ((*item_holder_map_)[*item_key]).get();
    }
  }
  return nullptr;
}

// Get whether the ItemHolder is full span for the specified index.
bool ListAdapter::IsFullSpanAtIndex(int index) const {
  return adapter_helper_ ? adapter_helper_->full_spans().end() !=
                               adapter_helper_->full_spans().find(index)
                         : false;
}

std::unique_ptr<pub::Value> ListAdapter::GenerateDiffResult() const {
  std::unique_ptr<pub::Value> diff_info;
  const auto& value_factory = list_container_->value_factory();
  if (value_factory && (diff_info = value_factory->CreateMap())) {
    GenerateDiffArray(kDiffInfoInsertion, adapter_helper_->insertions(),
                      diff_info);
    GenerateDiffArray(kDiffInfoRemoval, adapter_helper_->removals(), diff_info);
    GenerateDiffArray(kDiffInfoUpdateFrom, adapter_helper_->update_from(),
                      diff_info);
    GenerateDiffArray(kDiffInfoUpdateTo, adapter_helper_->update_to(),
                      diff_info);
    GenerateDiffArray(kDiffInfoMoveFrom, adapter_helper_->move_from(),
                      diff_info);
    GenerateDiffArray(kDiffInfoMoveTo, adapter_helper_->move_to(), diff_info);
  }
  return diff_info;
}

void ListAdapter::ClearDiffResult() { adapter_helper_->ClearDiffResult(); }

void ListAdapter::GenerateDiffArray(
    const std::string& diff_key, const std::vector<int32_t>& diff_array,
    const std::unique_ptr<pub::Value>& diff_result) const {
  std::unique_ptr<pub::Value> array_value;
  const auto& value_factory = list_container_->value_factory();
  if (value_factory && diff_result &&
      (array_value = value_factory->CreateArray())) {
    for (auto index : diff_array) {
      array_value->PushInt32ToArray(index);
    }
    diff_result->PushValueToMap(diff_key, *array_value);
  }
}

// Get estimated height for the specified index.
float ListAdapter::GetEstimatedSizeForIndex(int index) {
  float estimated_size = kInvalidDimensionSize;
  const float layouts_unit_per_px =
      list_container_->list_delegate()->GetLayoutsUnitPerPx();
  // Note: Taking into account compatibility with lower versions, developers
  // might set both estimated-main-axis-size-px and estimated-height-px,
  // therefore, using estimated-main-axis-size-px in priority.
  if (adapter_helper_ && index >= 0 &&
      index < static_cast<int>(adapter_helper_->estimated_sizes_px().size())) {
    estimated_size =
        adapter_helper_->estimated_sizes_px()[index] * layouts_unit_per_px;
    if (estimated_size > 0.f) {
      return estimated_size;
    }
  }
  if (adapter_helper_ && index >= 0 &&
      index <
          static_cast<int>(adapter_helper_->estimated_heights_px().size())) {
    estimated_size =
        adapter_helper_->estimated_heights_px()[index] * layouts_unit_per_px;
    if (estimated_size > 0.f) {
      return estimated_size;
    }
  }
  return kInvalidDimensionSize;
}

// Get whether the ItemHolder is recyclable for the specified index.
bool ListAdapter::IsRecyclableAtIndex(int index) const {
  if (!adapter_helper_) {
    return true;
  }
  const auto& unrecyclable_set = adapter_helper_->unrecyclable();
  return unrecyclable_set.find(index) == unrecyclable_set.end();
}

// Check whether the ItemHolder is sticky item with the specified index.
void ListAdapter::CheckSticky(ItemHolder* item_holder, int32_t index) {
  if (!adapter_helper_) {
    return;
  }
  bool sticky_top = false;
  if (std::find(adapter_helper_->sticky_tops().begin(),
                adapter_helper_->sticky_tops().end(),
                (int32_t)index) != adapter_helper_->sticky_tops().end()) {
    sticky_top = true;
  }
  bool sticky_bottom = false;
  if (std::find(adapter_helper_->sticky_bottoms().begin(),
                adapter_helper_->sticky_bottoms().end(),
                (int32_t)index) != adapter_helper_->sticky_bottoms().end()) {
    sticky_bottom = true;
  }
  item_holder->SetSticky(sticky_top, sticky_bottom);
}

int64_t ListAdapter::GenerateOperationId() const {
  static int32_t base_operation_id = 0;
  return (static_cast<int64_t>(list_container_->list_delegate()->GetImplId())
          << 32) +
         base_operation_id++;
}

void ListAdapter::RecycleAllItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_RECYCLE_ALL_ITEM_HOLDER);
  for (auto it = item_holder_map_->begin(); it != item_holder_map_->end();) {
    const auto& item_holder = it->second;
    if (item_holder) {
      RecycleItemHolder(item_holder.get());
    }
    ++it;
  }
}

void ListAdapter::RecycleRemovedItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_RECYCLE_REMOVED_ITEM_HOLDER);
  ListAnimationManager* list_animation_manager =
      list_container_->list_animation_manager();
  for (auto it = item_holder_map_->begin(); it != item_holder_map_->end();) {
    const auto& item_holder = it->second.get();
    if (item_holder && IsRemoved(item_holder)) {
      RecycleItemHolder(item_holder);
      if (list_animation_manager->UpdateAnimation() &&
          list_animation_manager->AnimationType() != ListAnimationType::kNone) {
        ++it;
      } else {
        it = item_holder_map_->erase(it);
      }
    } else {
      ++it;
    }
  }
}

void ListAdapter::UpdateLayoutInfoToItemHolder(
    ItemElementDelegate* list_item_delegate, ItemHolder* item_holder) {
  if (list_item_delegate && item_holder && IsFinishedBinding(item_holder) &&
      GetItemElementDelegate(item_holder) == list_item_delegate) {
    item_holder->UpdateLayoutFromItemDelegate(list_item_delegate);
  }
}

void ListAdapter::EnqueueElementsIfNeeded() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_ENQUEUE_ELEMENTS);
  bool need_flush = !fiber_flush_item_holder_set_.empty();
  if (!fiber_flush_item_holder_set_.empty()) {
    list_container_->list_children_helper()->ForEachChild(
        fiber_flush_item_holder_set_, [this](ItemHolder* item_holder) {
          EnqueueElement(item_holder);
          return false;
        });
    fiber_flush_item_holder_set_.clear();
  }
  for (auto& it : *item_holder_map_) {
    const auto& item_holder = it.second;
    // Note: If the ItemHolder is removed, we only try to enqueue element, not
    // erase it from item_holder_map_.
    if (item_holder && IsRemoved(item_holder.get())) {
      EnqueueElement(item_holder.get());
      need_flush = true;
    }
  }
  if (need_flush) {
    list_container_->list_delegate()->FlushImmediately();
  }
}

// It will invoked list's EnqueueComponent() to recycle component
// bound with ItemHolder and remove platform view from parent.
void ListAdapter::EnqueueElement(ItemHolder* item_holder) {
  if (!item_holder) {
    DLIST_LOGE("[" << list_container_
                   << "] ListAdapter::EnqueueElement: null item holder");
    return;
  }
  if (auto type = list_container_->list_animation_manager()->AnimationType();
      type != ListAnimationType::kNone) {
    if (type == ListAnimationType::kRemove) {
      item_holder->RecycleAfterAnimation(ItemHolderAnimationType::kOpacity);
    } else if (type == ListAnimationType::kInsert) {
      // The insert animation in a list may push a child off the screen, but at
      // that moment we still need a transform animation, so deferred destroy is
      // still necessary.
      item_holder->RecycleAfterAnimation(ItemHolderAnimationType::kTransform);
    }
    return;
  }
  ItemElementDelegate* list_item_delegate = GetItemElementDelegate(item_holder);
  if (list_item_delegate) {
    int32_t list_item_id = list_item_delegate->GetImplId();
    // Remove platform view and enqueue list item.
    list_container_->list_delegate()->RemoveListItemPaintingNode(list_item_id);
    list_container_->list_delegate()->EnqueueComponent(list_item_id);
    // Detach child.
    if (list_container_->list_children_helper()) {
      list_container_->list_children_helper()->DetachChild(item_holder,
                                                           list_item_delegate);
    }
    OnEnqueueElement(item_holder);
  }
}

#if ENABLE_TRACE_PERFETTO
void ListAdapter::UpdateTraceDebugInfo(TraceEvent* event) const {
  // item-key
  auto* item_keys = event->add_debug_annotations();
  item_keys->set_name("item-keys");
  std::string item_keys_str = "";
  int index = 0;
  for (const auto& item_key : adapter_helper_->item_keys()) {
    item_keys_str += "(" + std::to_string(index++) + ") " + item_key + "\n";
  }
  item_keys->set_string_value(item_keys_str);

  bool has_update = false;
  // update-from
  auto* update_from = event->add_debug_annotations();
  update_from->set_name("update-from");
  std::string update_from_str = "";
  for (const auto& index : adapter_helper_->update_from()) {
    has_update = true;
    update_from_str += std::to_string(index) + "\n";
  }
  update_from->set_string_value(update_from_str);

  // update-to
  auto* update_to = event->add_debug_annotations();
  update_to->set_name("update-to");
  std::string update_to_str = "";
  for (const auto& index : adapter_helper_->update_to()) {
    has_update = true;
    update_to_str += std::to_string(index) + "\n";
  }
  update_to->set_string_value(update_to_str);

  // insert
  auto* insert = event->add_debug_annotations();
  insert->set_name("insert");
  std::string insert_str = "";
  for (const auto& index : adapter_helper_->insertions()) {
    has_update = true;
    insert_str += std::to_string(index) + "\n";
  }
  insert->set_string_value(insert_str);

  // remove
  auto* remove = event->add_debug_annotations();
  remove->set_name("remove");
  std::string remove_str = "";
  for (const auto& index : adapter_helper_->removals()) {
    has_update = true;
    remove_str += std::to_string(index) + "\n";
  }
  remove->set_string_value(remove_str);

  // has update
  auto* has_update_annotations = event->add_debug_annotations();
  has_update_annotations->set_name("has_update");
  has_update_annotations->set_string_value(std::to_string(has_update));

  // sticky top
  auto* sticky_top = event->add_debug_annotations();
  sticky_top->set_name("sticky-top");
  std::string sticky_top_str = "";
  for (const auto& index : adapter_helper_->sticky_tops()) {
    sticky_top_str += std::to_string(index) + "\n";
  }
  sticky_top->set_string_value(sticky_top_str);

  // sticky bottom
  auto* sticky_bottom = event->add_debug_annotations();
  sticky_bottom->set_name("sticky-bottom");
  std::string sticky_bottom_str = "";
  for (const auto& index : adapter_helper_->sticky_bottoms()) {
    sticky_bottom_str += std::to_string(index) + "\n";
  }
  sticky_bottom->set_string_value(sticky_bottom_str);

  // full span
  auto* full_span = event->add_debug_annotations();
  full_span->set_name("full-span");
  std::string full_span_str = "";
  for (const auto& index : adapter_helper_->full_spans()) {
    full_span_str += std::to_string(index) + "\n";
  }
  full_span->set_string_value(full_span_str);

  // estimated_heights_px
  auto* estimated_heights_px = event->add_debug_annotations();
  estimated_heights_px->set_name("estimated-heights-px");
  std::string estimated_heights_px_str = "";
  for (const auto& value : adapter_helper_->estimated_heights_px()) {
    estimated_heights_px_str += std::to_string(value) + "\n";
  }
  estimated_heights_px->set_string_value(estimated_heights_px_str);

  // estimated_sizes_px
  auto* estimated_sizes_px = event->add_debug_annotations();
  estimated_sizes_px->set_name("estimated-sizes-px");
  std::string estimated_sizes_px_str = "";
  for (const auto& value : adapter_helper_->estimated_sizes_px()) {
    estimated_sizes_px_str += std::to_string(value) + "\n";
  }
  estimated_sizes_px->set_string_value(estimated_sizes_px_str);

  // recyclable
  auto* unrecyclable = event->add_debug_annotations();
  unrecyclable->set_name("unrecyclable");
  std::string unrecyclable_info_str = "";
  for (const auto& index : adapter_helper_->unrecyclable()) {
    unrecyclable_info_str += std::to_string(index) + "\n";
  }
  unrecyclable->set_string_value(unrecyclable_info_str);
}

void ListAdapter::UpdateTraceDebugInfo(TraceEvent* event,
                                       ItemHolder* item_holder) const {
  if (item_holder) {
    auto* index_info = event->add_debug_annotations();
    index_info->set_name("index");
    index_info->set_string_value(std::to_string(item_holder->index()));

    auto* item_key_info = event->add_debug_annotations();
    item_key_info->set_name("item-key");
    item_key_info->set_string_value(item_holder->item_key());
  }
}
#endif

}  // namespace list
}  // namespace lynx
