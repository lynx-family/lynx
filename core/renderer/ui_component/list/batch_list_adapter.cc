// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/batch_list_adapter.h"

#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

namespace list {
const uint32_t ItemStatus::kNeverBind = 0x01 << 1;
const uint32_t ItemStatus::kUpdated = 0x01 << 2;
const uint32_t ItemStatus::kRemoved = 0x01 << 3;
const uint32_t ItemStatus::kInBinding = 0x01 << 4;
const uint32_t ItemStatus::kFinishedBinding = 0x01 << 5;
const uint32_t ItemStatus::kRecycled = 0x01 << 6;
}  // namespace list

void BatchListAdapter::OnItemHolderInserted(ItemHolder* item_holder) {
  if (!item_holder->item_key().empty()) {
    const std::string& item_key = item_holder->item_key();
    if (item_status_map_->find(item_key) != item_status_map_->end()) {
      NLIST_LOGE("BatchListAdapter::OnItemHolderInserted: "
                 << "repeat insert item key: " << item_key);
    }
    (*item_status_map_)[item_key] = list::ItemStatus();
  }
}

void BatchListAdapter::OnItemHolderRemoved(ItemHolder* item_holder) {
  MarkItemStatus(item_holder->item_key(), list::ItemStatus::kRemoved);
}

void BatchListAdapter::OnItemHolderUpdateTo(ItemHolder* item_holder) {
  const std::string& item_key = item_holder->item_key();
  auto it = item_status_map_->find(item_key);
  if (it != item_status_map_->end() && !IsNeverBind(it->second)) {
    MarkItemStatus(item_key, list::ItemStatus::kUpdated);
  }
}

void BatchListAdapter::OnItemHolderReInsert(ItemHolder* item_holder) {
  MarkItemStatus(item_holder->item_key(), list::ItemStatus::kNeverBind);
}

void BatchListAdapter::OnDataSetChanged() {
  if (item_holder_map_) {
    for (const auto& pair : *item_holder_map_) {
      const std::string& item_key = pair.second->item_key();
      auto it = item_status_map_->find(item_key);
      if (it != item_status_map_->end() && !IsRemoved(it->second)) {
        MarkItemStatus(item_key, list::ItemStatus::kNeverBind);
      }
    }
  }
}

bool BatchListAdapter::BindItemHolder(ItemHolder* item_holder, int index,
                                      bool preload_section /* = false */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "BatchListAdapter::BindItemHolder", "index",
              index);
  if (!item_holder || index != item_holder->index() || preload_section) {
    // Note: not supports preload section when using component cache.
    return false;
  }
  ListNode* list_node = nullptr;
  if (!list_element_ || !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("BatchListAdapter::BindItemHolder: "
               << "null list element or list node");
    return false;
  }
  return BindItemHolderInternal(item_holder, index, list_node) !=
         list::kInvalidIndex;
}

void BatchListAdapter::BindItemHolders(const ItemHolderSet& item_holder_set) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "BatchListAdapter::BindItemHolders",
              "batch_item_number", item_holder_set.size());
  ListNode* list_node = nullptr;
  if (!list_element_ || !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("BatchListAdapter::BindItemHolders: "
               << "null list element or list node");
    return;
  }
  fml::RefPtr<lepus::CArray> index_array = lepus::CArray::Create();
  fml::RefPtr<lepus::CArray> operation_id_array = lepus::CArray::Create();
  for (ItemHolder* item_holder : item_holder_set) {
    if (item_holder) {
      const int index = item_holder->index();
      int64_t operation_id =
          BindItemHolderInternal(item_holder, index, list_node, false);
      if (operation_id != list::kInvalidIndex) {
        index_array->emplace_back(index);
        operation_id_array->emplace_back(operation_id);
      }
    }
  }
  list_node->ComponentAtIndexes(index_array, operation_id_array);
}

int64_t BatchListAdapter::BindItemHolderInternal(
    ItemHolder* item_holder, int index, ListNode* list_node,
    bool invoke_bind /* = true */) {
  const std::string& item_key = item_holder->item_key();
  auto it = item_status_map_->find(item_key);
  if (it != item_status_map_->end()) {
    const auto& item_status = it->second;
    if (IsDirty(item_status) || IsRecycled(item_status)) {
      // Generate binding key.
      int64_t operation_id = GenerateOperationId();
      // Check if the element is already in the component map. If it exists,
      // it needs to be recycled before invoking ComponentAtIndex(). This is
      // primarily for adapting to the Fiber architecture, where recycling
      // must occur before re-rendering.
      Element* list_item = GetListItemElement(item_key);
      if (list_element_->is_fiber_element() && list_item) {
        NLIST_LOGI("BatchListAdapter::BindItemHolderInternal: "
                   << "enqueue component before rendering with item_key = "
                   << item_key << ", index = " << index);
        RecycleItemHolder(item_holder);
      }
      // Mark status kInBinding.
      (it->second).status_ = list::ItemStatus::kInBinding;
      (it->second).operation_id_ = operation_id;
      // Note: Update binding key map and operation_id_.
      binding_key_map_->insert(std::make_pair(operation_id, item_key));
      if (invoke_bind) {
        NLIST_LOGI("BatchListAdapter::BindItemHolderInternal: "
                   << "with item_key = " << item_key << ", index = " << index
                   << ", operation_id = " << operation_id << ", "
                   << it->second.ToString());
        list_node->ComponentAtIndex(index, operation_id);
      }
      return operation_id;
    }
  }
  return list::kInvalidIndex;
}

void BatchListAdapter::OnFinishBindItemHolder(Element* list_item,
                                              const PipelineOptions& options) {
  ListNode* list_node = nullptr;
  if (!list_element_ || !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("BatchListAdapter::OnFinishBindItemHolder: "
               << "null list element or list node");
    return;
  }
  int valid_bind_index =
      OnFinishBindInternal(list_item, options.operation_id, list_node);
  if (valid_bind_index != list::kInvalidIndex) {
    // Note: Mark should_flush_finish_layout_ to determine whether needs to
    // invoke FinishLayoutOperation().
    list_container_->MarkShouldFlushFinishLayout(options.has_layout);
    if (list_container_->intercept_depth() == 0) {
      // Note: In MULTI_THREAD mode, if the list item has been rendered async,
      // we should invoke list OnLayoutChildren. But in ALL_ON_UI mode, we
      // should check intercept_depth_ value to make use that list will not
      // invoke new layout pass in one layout.
      list_container_->list_layout_manager()->OnLayoutChildren(
          true, valid_bind_index);
    }
  }
}

void BatchListAdapter::OnFinishBindItemHolders(
    const std::vector<Element*>& list_items, const PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "BatchListAdapter::OnFinishBindItemHolders",
              "batch_item_number", list_items.size());
  if (list_items.empty() || options.operation_ids_.empty() ||
      list_items.size() != options.operation_ids_.size()) {
    return;
  }
  ListNode* list_node = nullptr;
  if (!list_element_ || !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("BatchListAdapter::OnFinishBindItemHolders: "
               << "null list element or list node");
    return;
  }
  bool has_valid_bind = false;
  const size_t list_item_size = list_items.size();
  // Traverse list items and operation ids array.
  for (size_t i = 0; i < list_item_size; ++i) {
    int valid_bind_index = OnFinishBindInternal(
        list_items[i], options.operation_ids_[i], list_node);
    has_valid_bind |= valid_bind_index != list::kInvalidIndex;
  }
  if (has_valid_bind) {
    // Note: Mark should_flush_finish_layout_ to determine whether needs to
    // invoke FinishLayoutOperation().
    list_container_->MarkShouldFlushFinishLayout(options.has_layout);
    if (list_container_->intercept_depth() == 0) {
      // Note: In MULTI_THREAD mode, if the list items have been rendered async,
      // we should invoke list OnBatchLayoutChildren. But in ALL_ON_UI mode, we
      // should check intercept_depth_ value to make use that list will not
      // invoke new layout pass in one layout.
      list_container_->list_layout_manager()->OnBatchLayoutChildren();
    }
  }
}

int BatchListAdapter::OnFinishBindInternal(Element* list_item,
                                           int64_t operation_id,
                                           ListNode* list_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "BatchListAdapter::OnFinishBindInternal",
              "operation_id", operation_id);
  int valid_bind_index = list::kInvalidIndex;
  if (!list_item) {
    NLIST_LOGE("BatchListAdapter::OnFinishBindInternal: "
               << "null list item with operation_id = " << operation_id);
  } else {
    auto binding_key_it = binding_key_map_->find(operation_id);
    if (binding_key_it != binding_key_map_->end()) {
      const std::string& item_key = binding_key_it->second;
      // Note: The ItemStatus has the same lifecycle with ItemHolder, so it can
      // avoid the case that the ItemHolder is destroyed.
      auto it = item_status_map_->find(item_key);
      if (it != item_status_map_->end() &&
          it->second.operation_id_ == operation_id) {
        const auto& item_status = it->second;
        if (IsBinding(item_status) && !IsDirty(item_status)) {
          // This is a valid bind.
          valid_bind_index = OnFinishValidBind(item_key, list_item);
        } else {
          // Other status: The component can be recycled.
          list_node->EnqueueComponent(list_item->impl_id());
        }
        // Note: Need reset operation_id_.
        it->second.operation_id_ = 0;
      } else {
        // operation_id is not the latest one in item_status_map_, the component
        // can be recycled.
        list_node->EnqueueComponent(list_item->impl_id());
      }
      binding_key_map_->erase(binding_key_it);
    } else {
      NLIST_LOGE("BatchListAdapter::OnFinishBindInternal: "
                 << "not in binding_key_map_ with operation_id = "
                 << operation_id);
      list_node->EnqueueComponent(list_item->impl_id());
    }
  }
  return valid_bind_index;
}

int BatchListAdapter::OnFinishValidBind(const std::string& item_key,
                                        Element* list_item) {
  NLIST_LOGI("HandleValidBinding::HandleValidBinding: valid with item_key = "
             << item_key << ", list_item = " << list_item);
  // Note: This is only chance to insert list item element to element map.
  list_item_element_map_->insert(std::make_pair(item_key, list_item));
  MarkItemStatus(item_key, list::ItemStatus::kFinishedBinding);
  // Note: here using item key to find ItemHolder is just for the rationality of
  // code logic.
  auto it = item_holder_map_->find(item_key);
  if (it != item_holder_map_->end() && it->second) {
    const auto& item_holder = it->second;
    if (item_holder) {
      // Update layout info from component to ItemHolder.
      item_holder->UpdateLayoutFromElement(list_item);
      // Add item_holder to attach_children_set.
      list_container_->list_children_helper()->AttachChild(item_holder.get(),
                                                           list_item);
      return item_holder->index();
    }
  }
  return list::kInvalidIndex;
}

void BatchListAdapter::RecycleItemHolder(ItemHolder* item_holder) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "BatchListAdapter::RecycleItemHolder",
              "index", item_holder->index());
  ListNode* list_node = nullptr;
  if (!item_holder || !list_element_ ||
      !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("BatchListAdapter::RecycleItemHolder: "
               << "null item holder or list element or list node");
    return;
  }
  const std::string& item_key = item_holder->item_key();
  auto it = item_status_map_->find(item_key);
  if (it != item_status_map_->end()) {
    const auto& item_status = it->second;
    if (IsRemoved(item_status)) {
      // If the data is removed, we need erase it from item_status_map_.
      item_status_map_->erase(item_key);
    } else if (IsFinishedBinding(item_status)) {
      // If the data is finish binding, we set it to bound.
      MarkItemStatus(item_key, list::ItemStatus::kRecycled);
    }
    Element* list_item = GetListItemElement(item_key);
    if (list_item) {
      // Remove list item platform view and enqueue list item.
      list_element_->element_manager()
          ->painting_context()
          ->RemoveListItemPaintingNode(list_element_->impl_id(),
                                       list_item->impl_id());
      list_node->EnqueueComponent(list_item->impl_id());
      if (list_container_->list_children_helper()) {
        list_container_->list_children_helper()->DetachChild(item_holder,
                                                             list_item);
      }
      // Note: This is only chance to erase list item element from map.
      list_item_element_map_->erase(item_key);
    }
  }
}

bool BatchListAdapter::CheckItemStatus(const std::string& item_key,
                                       uint32_t item_status) const {
  auto it = item_status_map_->find(item_key);
  if (it == item_status_map_->end()) {
    NLIST_LOGE("BatchListAdapter::CheckItemStatus: "
               << "not found item_key = " << item_key);
    return false;
  }
  return it->second == item_status;
}

void BatchListAdapter::MarkItemStatus(const std::string& item_key,
                                      uint32_t item_status) {
  auto it = item_status_map_->end();
  if (item_key.empty() ||
      ((it = item_status_map_->find(item_key)) == item_status_map_->end())) {
    return;
  }
  (it->second).status_ = item_status;
}

}  // namespace tasm
}  // namespace lynx
