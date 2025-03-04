// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/default_list_adapter.h"

#include <string>

#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

void DefaultListAdapter::OnItemHolderRemoved(ItemHolder* item_holder) {
  if (item_holder) {
    item_holder->MarkDirty(true);
    item_holder->MarkRemoved(true);
  }
}

void DefaultListAdapter::OnItemHolderUpdateFrom(ItemHolder* item_holder) {
  if (item_holder) {
    item_holder->MarkDirty(true);
  }
}

void DefaultListAdapter::OnItemHolderUpdateTo(ItemHolder* item_holder) {
  if (item_holder) {
    item_holder->MarkDirty(true);
    item_holder->MarkDiffStatus(list::DiffStatus::kUpdateTo);
  }
}

void DefaultListAdapter::OnItemHolderMovedFrom(ItemHolder* item_holder) {
  if (item_holder) {
    item_holder->MarkDirty(true);
  }
}

void DefaultListAdapter::OnItemHolderMovedTo(ItemHolder* item_holder) {
  if (item_holder) {
    item_holder->MarkDirty(true);
  }
}

void DefaultListAdapter::OnItemHolderReInsert(ItemHolder* item_holder) {
  if (item_holder) {
    item_holder->MarkDirty(true);
    item_holder->MarkRemoved(false);
  }
}

void DefaultListAdapter::OnDataSetChanged() {
  if (item_holder_map_) {
    for (const auto& pair : *item_holder_map_) {
      if (pair.second && !(pair.second->removed())) {
        pair.second->MarkDirty(true);
      }
    }
  }
}

// Bind ItemHolder for the specified index. For each invoke
// ComponentAtIndex() to render a child element, a unique operation-id is
// generated and the pair <operation-id, ItemHolder> is added to a map.
bool DefaultListAdapter::BindItemHolder(ItemHolder* item_holder, int index,
                                        bool preload_section /* = false */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DefaultListAdapter::BindItemHolder",
              "index", index);
  if (!list_element_ || !item_holder || index != item_holder->index() ||
      (preload_section && item_holder->virtual_dom_preloaded())) {
    return false;
  }
  ListNode* list_node = nullptr;
  if (!list_element_ || !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("DefaultListAdapter::BindItemHolder: "
               << "null list element or list node");
    return false;
  }
  if (IsDirty(item_holder) || IsRecycled(item_holder)) {
    int64_t operation_id = GenerateOperationId();
    (*binding_item_holder_map_)[operation_id] = item_holder;
    // In ReactLynx 3.0, binding item_holder twice without enqueuing will result
    // in cloning of the old element. This MR aims to avoid this scenario by
    // mandating enqueuing before binding.
    if (list_element_->is_fiber_element() && item_holder->element()) {
      NLIST_LOGI("[" << list_container_
                     << "] DefaultListAdapter::BindItemHolder: enqueue "
                        "component before render with item_key = "
                     << item_holder->item_key() << ", index = " << index);
      RecycleItemHolder(item_holder);
    }
    item_holder->MarkDirty(false);
    item_holder->MarkDiffStatus(list::DiffStatus::kValid);
    item_holder->SetOperationId(operation_id);
    // item_holder has no element before calling the method of
    // "componentAtIndex()".if it has element, it means that the diffResult of
    // this index is "update".
    bool should_send = item_holder->element() == nullptr;
    NLIST_LOGI("[" << list_container_
                   << "] DefaultListAdapter::BindItemHolder: with index = "
                   << index << ", item_key = " << item_holder->item_key()
                   << ", operation_id = " << operation_id);
    list_node->ComponentAtIndex(
        index, operation_id, list_container_->should_request_state_restore());
    item_holder->MarkVirtualDomPreloaded(true);
    // TODO(dingwang.wxx): Move the events invocations in finishing bind.
    if (list_container_->should_request_state_restore() &&
        item_holder->element() != nullptr) {
      list_element_->painting_context()->ListCellWillAppear(
          item_holder->element()->impl_id(), item_holder->item_key());
    }
    if (list_container_->list_event_manager() && should_send) {
      list_container_->list_event_manager()->OnViewAttach(item_holder);
    }
    return true;
  }
  return false;
}

// When the rendering of the list's child node is complete, this method will
// be invoked.
void DefaultListAdapter::OnFinishBindItemHolder(Element* component,
                                                const PipelineOptions& option) {
  if (!component) {
    NLIST_LOGE(
        "DefaultListAdapter::OnFinishBindItemHolder: component is nullptr");
    return;
  }
  int64_t operation_id = option.operation_id;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DefaultListAdapter::OnFinishBindItemHolder",
              "operation_id", operation_id);
  list::BindingItemHolderMap::const_iterator it =
      binding_item_holder_map_->find(operation_id);
  ItemHolder* binding_item_holder = nullptr;
  // Find the corresponding ItemHolder based on the operation_id and bind the
  // ItemHolder to the child element.
  // (TODO)fangzhou.fz: if item_holder is not onscreen, it should be recycled
  // immediately and should not trigger OnLayoutChildren().
  // (TODO)fangzhou.fz: send viewAttach event here.
  if (it != binding_item_holder_map_->end() &&
      (binding_item_holder = it->second) &&
      binding_item_holder->operation_id() == operation_id) {
    int index = binding_item_holder->index();
    const std::string& item_key = binding_item_holder->item_key();
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "DefaultListAdapter::OnFinishBindItemHolder.finish", "index",
                index);
    NLIST_LOGI(
        "[" << list_container_
            << "] DefaultListAdapter::OnFinishBindItemHolder: with index = "
            << index << ", item_key = " << item_key
            << ", operation_id = " << operation_id);
    binding_item_holder->SetElement(component);
    binding_item_holder->UpdateLayoutFromElement();
    // Reset operation id.
    binding_item_holder->SetOperationId(0);
    binding_item_holder->SetOrientation(
        list_container_->list_layout_manager()->orientation());
    list_container_->CheckZIndex(component);
    // Add ItemHolder to attach_children_.
    list_container_->list_children_helper()->AttachChild(binding_item_holder,
                                                         component);
    binding_item_holder_map_->erase(it);
    // Note: Mark should_flush_finish_layout_ to determine whether needs to
    // invoke FinishLayoutOperation().
    list_container_->MarkShouldFlushFinishLayout(option.has_layout);
    if (list_container_->intercept_depth() == 0) {
      list_container_->list_layout_manager()->OnLayoutChildren(true, index);
    }
    list_container_->ReportListItemLifecycleStatistic(option, item_key);
  }
}

// Recycle ItemHolder. It will invoked list's EnqueueComponent() to recycle
// component bound with ItemHolder and remove platform view from parent.
void DefaultListAdapter::RecycleItemHolder(ItemHolder* item_holder) {
  if (!list_element_ || !list_container_ || !item_holder ||
      !item_holder->element()) {
    return;
  }
  ListNode* list_node = nullptr;
  if (!list_element_ || !(list_node = list_element_->GetListNode())) {
    NLIST_LOGE("DefaultListAdapter::RecycleItemHolder: "
               << "null list element or list node");
    return;
  }
  if (list_container_->list_event_manager()) {
    list_container_->list_event_manager()->OnViewDetach(item_holder);
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DefaultListAdapter::RecycleItemHolder",
              "index", item_holder->index());
  int32_t comp_id = item_holder->element()->impl_id();
  list_node->EnqueueComponent(comp_id);
  list_element_->element_manager()
      ->painting_context()
      ->RemoveListItemPaintingNode(list_element_->impl_id(), comp_id);
  if (list_container_->list_children_helper()) {
    list_container_->list_children_helper()->DetachChild(
        item_holder, item_holder->element());
  }
  item_holder->SetElement(nullptr);
  list_element_->painting_context()->FlushImmediately();
}

}  // namespace tasm
}  // namespace lynx
