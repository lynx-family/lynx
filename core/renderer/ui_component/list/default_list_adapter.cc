// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/default_list_adapter.h"

#include <memory>
#include <string>

#include "core/renderer/trace/renderer_trace_event_def.h"
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

void DefaultListAdapter::OnItemHolderUpdateTo(ItemHolder* item_holder,
                                              bool fiber_flush) {
  if (item_holder) {
    item_holder->MarkDirty(true);
    item_holder->MarkDiffStatus(list::DiffStatus::kUpdateTo);
    if (fiber_flush && GetListItemElement(item_holder)) {
      fiber_flush_item_holder_set_.insert(item_holder);
    }
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

void DefaultListAdapter::OnEnqueueElement(ItemHolder* item_holder) {
  if (item_holder) {
    if (list_container_->list_event_manager()) {
      list_container_->list_event_manager()->OnViewDetach(item_holder);
    }
    item_holder->SetElement(nullptr);
  }
}

// Bind ItemHolder for the specified index. For each invoke
// ComponentAtIndex() to render a child element, a unique operation-id is
// generated and the pair <operation-id, ItemHolder> is added to a map.
bool DefaultListAdapter::BindItemHolder(ItemHolder* item_holder, int index,
                                        bool preload_section /* = false */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_BIND_ITEM_HOLDER,
              [this, item_holder](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event(), item_holder);
              });
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
    (*binding_item_holder_weak_map_)[operation_id] =
        item_holder->WeakFromThis();
    // In ReactLynx 3.0, binding item_holder twice without enqueuing will result
    // in cloning of the old element. This MR aims to avoid this scenario by
    // mandating enqueuing before binding.
    if (list_element_->IsFiberArch() && item_holder->element()) {
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
      item_holder->element()->element_container()->ListCellWillAppear(
          item_holder->item_key());
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
void DefaultListAdapter::OnFinishBindItemHolder(
    Element* component, const std::shared_ptr<PipelineOptions>& option) {
  if (!component) {
    NLIST_LOGE(
        "DefaultListAdapter::OnFinishBindItemHolder: component is nullptr");
    return;
  }
  int64_t operation_id = option->operation_id;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DEFAULT_LIST_ADAPTER_FINISH_BIND_ITEM_HOLDER,
              "operation_id", operation_id);
  // Find the corresponding ItemHolder based on the operation_id and bind the
  // ItemHolder to the child element.
  if (auto it = binding_item_holder_weak_map_->find(operation_id);
      it != binding_item_holder_weak_map_->end()) {
    ItemHolder* binding_item_holder = nullptr;
    if ((binding_item_holder = it->second.get()) &&
        binding_item_holder->operation_id() == operation_id) {
      binding_item_holder_weak_map_->erase(operation_id);
      int index = binding_item_holder->index();
      const std::string& item_key = binding_item_holder->item_key();
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  DEFAULT_LIST_ADAPTER_FINISH_BIND_ITEM_HOLDER_FINISH, "index",
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
      // Note: Mark should_flush_finish_layout_ to determine whether needs to
      // invoke FinishLayoutOperation().
      list_container_->MarkShouldFlushFinishLayout(option->has_layout);
      if (list_container_->intercept_depth() == 0) {
        list_container_->list_layout_manager()->OnLayoutChildren(true, index);
      }
      list_container_->ReportListItemLifecycleStatistic(option, item_key);
    } else {
      NLIST_LOGI("[" << list_container_
                     << "] DefaultListAdapter::OnFinishBindItemHolder: "
                        "ItemHolder is destroy with operation_id = "
                     << operation_id);
      binding_item_holder_weak_map_->erase(operation_id);
      list_element_->GetListNode()->EnqueueComponent(component->impl_id());
    }
  } else {
    NLIST_LOGE(
        "[" << list_container_
            << "] DefaultListAdapter::OnFinishBindItemHolder: "
            << "not in binding_item_holder_weak_map_ with operation_id = "
            << operation_id);
    const auto& attached_element_item_holder_map =
        list_container_->list_children_helper()
            ->attached_element_item_holder_map();
    if (attached_element_item_holder_map.find(component) !=
        attached_element_item_holder_map.end()) {
      // The component is not attached to any item holder, so we can enqueue
      // this component directly.
      list_element_->GetListNode()->EnqueueComponent(component->impl_id());
    }
  }
}

// Recycle ItemHolder.
void DefaultListAdapter::RecycleItemHolder(ItemHolder* item_holder) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ADAPTER_RECYCLE_ITEM_HOLDER,
              [this, item_holder](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event(), item_holder);
              });
  if (item_holder) {
    EnqueueElement(item_holder);
    list_element_->element_container()->FlushImmediately();
  }
}

#if ENABLE_TRACE_PERFETTO
void DefaultListAdapter::UpdateTraceDebugInfo(TraceEvent* event,
                                              ItemHolder* item_holder) const {
  ListAdapter::UpdateTraceDebugInfo(event, item_holder);
  auto* adapter_type_info = event->add_debug_annotations();
  adapter_type_info->set_name("adapter_type");
  adapter_type_info->set_string_value("default");
}
#endif

}  // namespace tasm
}  // namespace lynx
