// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_batch_list_adapter.h"

#include "core/list/decoupled_list_container_impl.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

const uint32_t ItemStatus::kNeverBind = 0x01 << 1;
const uint32_t ItemStatus::kUpdated = 0x01 << 2;
const uint32_t ItemStatus::kRemoved = 0x01 << 3;
const uint32_t ItemStatus::kInBinding = 0x01 << 4;
const uint32_t ItemStatus::kFinishedBinding = 0x01 << 5;
const uint32_t ItemStatus::kRecycled = 0x01 << 6;

void BatchListAdapter::OnItemHolderInserted(ItemHolder* item_holder) {
  if (!item_holder->item_key().empty()) {
    const std::string& item_key = item_holder->item_key();
    if (item_status_map_->find(item_key) != item_status_map_->end()) {
      DLIST_LOGE("[" << list_container_
                     << "] BatchListAdapter::OnItemHolderInserted: "
                     << "repeat insert item key: " << item_key);
    }
    (*item_status_map_)[item_key] = ItemStatus();
  }
}

void BatchListAdapter::OnItemHolderRemoved(ItemHolder* item_holder) {
  MarkItemStatus(item_holder->item_key(), ItemStatus::kRemoved);
}

void BatchListAdapter::OnItemHolderUpdateTo(ItemHolder* item_holder,
                                            bool fiber_flush) {
  const std::string& item_key = item_holder->item_key();
  auto it = item_status_map_->find(item_key);
  if (it != item_status_map_->end() && !IsNeverBind(it->second)) {
    MarkItemStatus(item_key, ItemStatus::kUpdated);
    // TODO(dingwang.wxx): remove this logic.
    if (fiber_flush && GetItemElementDelegate(item_holder)) {
      fiber_flush_item_holder_set_.insert(item_holder);
    }
  }
}

void BatchListAdapter::OnItemHolderReInsert(ItemHolder* item_holder) {
  MarkItemStatus(item_holder->item_key(), ItemStatus::kNeverBind);
}

void BatchListAdapter::OnEnqueueElement(ItemHolder* item_holder) {
  if (item_holder) {
    // Note: This is only chance to erase list item element from map.
    list_item_delegate_map_->erase(item_holder->item_key());
  }
}

void BatchListAdapter::OnDataSetChanged() {
  if (item_holder_map_) {
    for (const auto& pair : *item_holder_map_) {
      const std::string& item_key = pair.second->item_key();
      auto it = item_status_map_->find(item_key);
      if (it != item_status_map_->end() && !IsRemoved(it->second)) {
        MarkItemStatus(item_key, ItemStatus::kNeverBind);
      }
    }
  }
}

bool BatchListAdapter::BindItemHolder(ItemHolder* item_holder, int index,
                                      bool preload_section /* = false */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_BIND_ITEM_HOLDER,
              [this, item_holder](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event(), item_holder);
              });
  if (!item_holder || index != item_holder->index() || preload_section) {
    // Note: not supports preload section when using component cache.
    return false;
  }
  return BindItemHolderInternal(item_holder, index) != kInvalidIndex;
}

void BatchListAdapter::BindItemHolders(const ItemHolderSet& item_holder_set) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_BIND_ITEM_HOLDERS,
              "batch_item_number", item_holder_set.size());
  const auto& value_factory = list_container_->value_factory();
  if (value_factory) {
    std::unique_ptr<pub::Value> index_array;
    std::unique_ptr<pub::Value> operation_id_array;
    if ((index_array = value_factory->CreateArray()) &&
        (operation_id_array = value_factory->CreateArray())) {
      for (ItemHolder* item_holder : item_holder_set) {
        if (item_holder) {
          const int index = item_holder->index();
          int64_t operation_id =
              BindItemHolderInternal(item_holder, index, false);
          if (operation_id != kInvalidIndex) {
            index_array->PushInt32ToArray(index);
            operation_id_array->PushInt64ToArray(operation_id);
          }
        }
      }
      list_container_->list_delegate()->ComponentAtIndexes(
          std::move(index_array), std::move(operation_id_array));
    }
  }
}

int64_t BatchListAdapter::BindItemHolderInternal(
    ItemHolder* item_holder, int index, bool invoke_bind /* = true */) {
  const std::string& item_key = item_holder->item_key();
  auto it = item_status_map_->find(item_key);
  if (it != item_status_map_->end()) {
    const auto& item_status = it->second;
    if (IsDirty(item_status) || IsRecycled(item_status)) {
      ElementDelegate* list_delegate = list_container_->list_delegate();
      // Generate binding key.
      int64_t operation_id = GenerateOperationId();
      // Check if the element is already in the component map. If it exists,
      // it needs to be recycled before invoking ComponentAtIndex(). This is
      // primarily for adapting to the Fiber architecture, where recycling
      // must occur before re-rendering.
      if (list_delegate->IsFiberArch() && GetItemElementDelegate(item_holder)) {
        DLIST_LOGI("[" << list_container_
                       << "] BatchListAdapter::BindItemHolderInternal: enqueue "
                          "component before render with item_key = "
                       << item_holder->item_key() << ", index = " << index);
        RecycleItemHolder(item_holder);
        list_container_->list_children_helper()->EraseFromLastBindingChildren(
            item_holder);
      }
      // Mark status kInBinding.
      (it->second).status_ = ItemStatus::kInBinding;
      (it->second).operation_id_ = operation_id;
      // Note: Update binding key map and operation_id_.
      binding_key_map_->insert(std::make_pair(operation_id, item_key));
      if (invoke_bind) {
        DLIST_LOGI(
            "["
            << list_container_
            << "] BatchListAdapter::BindItemHolderInternal: with item_key = "
            << item_key << ", index = " << index << ", operation_id = "
            << operation_id << ", item_state = " << it->second.ToString());
        // TODO(dingwang.wxx): impl native state storage
        list_delegate->ComponentAtIndex(index, operation_id);
      }
      return operation_id;
    }
  }
  return kInvalidIndex;
}

void BatchListAdapter::OnFinishBindItemHolder(
    ItemElementDelegate* list_item_delegate,
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  int valid_bind_index =
      OnFinishBindInternal(list_item_delegate, options->operation_id);
  if (valid_bind_index != kInvalidIndex) {
    // Note: Mark should_flush_finish_layout_ to determine whether needs to
    // invoke FinishLayoutOperation().
    list_container_->MarkShouldFlushFinishLayout(options->has_layout);
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
    const std::vector<ItemElementDelegate*>& list_item_delegate_array,
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_FINISH_BIND_ITEM_HOLDERS,
              "batch_item_number", list_item_delegate_array.size());

  if (list_item_delegate_array.empty() || options->operation_ids_.empty() ||
      list_item_delegate_array.size() != options->operation_ids_.size()) {
    return;
  }
  bool has_valid_bind = false;
  const size_t list_item_size = list_item_delegate_array.size();
  // Traverse list items and operation ids array.
  for (size_t i = 0; i < list_item_size; ++i) {
    int valid_bind_index = OnFinishBindInternal(list_item_delegate_array[i],
                                                options->operation_ids_[i]);
    has_valid_bind |= valid_bind_index != kInvalidIndex;
  }
  if (has_valid_bind) {
    // Note: Mark should_flush_finish_layout_ to determine whether needs to
    // invoke FinishLayoutOperation().
    list_container_->MarkShouldFlushFinishLayout(options->has_layout);
    if (list_container_->intercept_depth() == 0) {
      // Note: In MULTI_THREAD mode, if the list items have been rendered async,
      // we should invoke list OnBatchLayoutChildren. But in ALL_ON_UI mode, we
      // should check intercept_depth_ value to make use that list will not
      // invoke new layout pass in one layout.
      list_container_->list_layout_manager()->OnBatchLayoutChildren();
    }
  }
}

int BatchListAdapter::OnFinishBindInternal(
    ItemElementDelegate* list_item_delegate, int64_t operation_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_FINISH_BIND_INTERNAL,
              "operation_id", operation_id);
  int valid_bind_index = kInvalidIndex;
  if (!list_item_delegate) {
    DLIST_LOGE("[" << list_container_
                   << "] BatchListAdapter::OnFinishBindInternal: "
                   << "null list item with operation_id = " << operation_id);
  } else {
    ElementDelegate* list_delegate = list_container_->list_delegate();
    auto binding_key_it = binding_key_map_->find(operation_id);
    if (binding_key_it != binding_key_map_->end()) {
      // Get item_key according to operation_id from binding_key_map_.
      const std::string& item_key = binding_key_it->second;
      // Note: The ItemStatus has the same lifecycle with ItemHolder, so it can
      // avoid the case that the ItemHolder is destroyed.
      auto it = item_status_map_->find(item_key);
      if (it != item_status_map_->end() &&
          it->second.operation_id_ == operation_id) {
        // case 1. Found item_status and check operation_id is equal.
        const auto& item_status = it->second;
        if (IsBinding(item_status) && !IsDirty(item_status)) {
          // case 1.1. This is a valid bind.
          valid_bind_index = OnFinishValidBind(item_key, list_item_delegate);
        } else {
          // case 1.2. Other status, for example, if the item holder is marked
          // dirty again, we can enqueue this component directly.
          DLIST_LOGI("[" << list_container_
                         << "] BatchListAdapter::OnFinishBindInternal: other "
                            "state with item_key = "
                         << item_key
                         << ", item_status = " << item_status.ToString());
          list_delegate->EnqueueComponent(list_item_delegate->GetImplId());
        }
        // Note: Need reset operation_id_.
        it->second.operation_id_ = 0;
      } else if (it != item_status_map_->end()) {
        // case 2. The Operation_id is not the latest one in item_status_map_,
        // the component can be recycled.
        DLIST_LOGI("[" << list_container_
                       << "] BatchListAdapter::OnFinishBindInternal: not "
                          "latest binding with operation_id = "
                       << operation_id
                       << ", item_status = " << it->second.ToString());
        list_delegate->EnqueueComponent(list_item_delegate->GetImplId());
      } else {
        // case 3. Not found item_status.
        DLIST_LOGI("[" << list_container_
                       << "] BatchListAdapter::OnFinishBindInternal: not found "
                          "item_status with operation_id = "
                       << operation_id);
        list_delegate->EnqueueComponent(list_item_delegate->GetImplId());
      }
      binding_key_map_->erase(binding_key_it);
    } else {
      DLIST_LOGE("[" << list_container_
                     << "] BatchListAdapter::OnFinishBindInternal: "
                     << "not in binding_key_map_ with operation_id = "
                     << operation_id);
      list_delegate->EnqueueComponent(list_item_delegate->GetImplId());
    }
  }
  return valid_bind_index;
}

int BatchListAdapter::OnFinishValidBind(
    const std::string& item_key, ItemElementDelegate* list_item_delegate) {
  DLIST_LOGI(
      "[" << list_container_
          << "] BatchListAdapter::HandleValidBinding: valid with item_key = "
          << item_key << ", impl_id = " << list_item_delegate->GetImplId());
  // Note: This is only chance to insert list item element to element map.
  list_item_delegate_map_->insert(std::make_pair(item_key, list_item_delegate));
  MarkItemStatus(item_key, ItemStatus::kFinishedBinding);
  // Note: here using item key to find ItemHolder is just for the rationality of
  // code logic.
  auto it = item_holder_map_->find(item_key);
  if (it != item_holder_map_->end() && it->second) {
    const auto& item_holder = it->second;
    if (item_holder) {
      // Update layout info from component to ItemHolder.
      item_holder->UpdateLayoutFromItemDelegate(list_item_delegate);
      // Add item_holder to attach_children_set.
      list_container_->list_children_helper()->AttachChild(item_holder.get(),
                                                           list_item_delegate);
      return item_holder->index();
    }
  }
  return kInvalidIndex;
}

void BatchListAdapter::RecycleItemHolder(ItemHolder* item_holder) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_BIND_ITEM_HOLDER,
              [this, item_holder](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event(), item_holder);
              });
  if (item_holder) {
    const std::string& item_key = item_holder->item_key();
    auto it = item_status_map_->find(item_key);
    if (it != item_status_map_->end()) {
      const auto& item_status = it->second;
      if (IsRemoved(item_status)) {
        // If the data is removed, we need erase it from item_status_map_.
        item_status_map_->erase(item_key);
      } else if (IsFinishedBinding(item_status)) {
        // If the data is finish binding, we set it to bound.
        MarkItemStatus(item_key, ItemStatus::kRecycled);
      }
      EnqueueElement(item_holder);
    }
  }
}

bool BatchListAdapter::CheckItemStatus(const std::string& item_key,
                                       uint32_t item_status) const {
  auto it = item_status_map_->find(item_key);
  if (it == item_status_map_->end()) {
    DLIST_LOGE("[" << list_container_ << "] BatchListAdapter::CheckItemStatus: "
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

#if ENABLE_TRACE_PERFETTO
void BatchListAdapter::UpdateTraceDebugInfo(TraceEvent* event,
                                            ItemHolder* item_holder) const {
  ListAdapter::UpdateTraceDebugInfo(event, item_holder);
  auto* adapter_type_info = event->add_debug_annotations();
  adapter_type_info->set_name("adapter_type");
  adapter_type_info->set_string_value("batch");
}
#endif

}  // namespace list
}  // namespace lynx
