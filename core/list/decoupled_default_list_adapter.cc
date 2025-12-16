// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_default_list_adapter.h"

#include <memory>
#include <string>

#include "core/list/decoupled_list_container_impl.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

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
    item_holder->MarkDiffStatus(DiffStatus::kUpdateTo);
    // TODO(dingwang.wxx): remove this logic
    if (fiber_flush && GetItemElementDelegate(item_holder)) {
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
      list_container_->list_event_manager()->SendExposureEvent(
          kEventNodeDisappear, item_holder);
    }
    item_holder->SetItemDelegate(nullptr);
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
  if (!item_holder || index != item_holder->index() ||
      (preload_section && item_holder->virtual_dom_preloaded())) {
    return false;
  }
  if (IsDirty(item_holder) || IsRecycled(item_holder)) {
    ElementDelegate* list_delegate = list_container_->list_delegate();
    int64_t operation_id = GenerateOperationId();
    (*binding_item_holder_weak_map_)[operation_id] =
        item_holder->WeakFromThis();
    // In ReactLynx 3.0, binding item_holder twice without enqueuing will result
    // in cloning of the old element. This MR aims to avoid this scenario by
    // mandating enqueuing before binding.
    if (list_delegate->IsFiberArch() && GetItemElementDelegate(item_holder)) {
      DLIST_LOGI("[" << list_container_
                     << "] DefaultListAdapter::BindItemHolder: enqueue "
                        "component before render with item_key = "
                     << item_holder->item_key() << ", index = " << index);
      RecycleItemHolder(item_holder);
    }
    item_holder->MarkDirty(false);
    item_holder->MarkDiffStatus(DiffStatus::kValid);
    item_holder->SetOperationId(operation_id);
    // Note: before invoking ComponentAtIndex(), we use GetItemElementDelegate()
    // to get latest item delegate from ItemHolder, and if got nullptr, we
    // should send exposure event after invoking ComponentAtIndex().
    ItemElementDelegate* list_item_delegate =
        GetItemElementDelegate(item_holder);
    bool should_send_exposure_event = list_item_delegate == nullptr;
    DLIST_LOGI("[" << list_container_
                   << "] DefaultListAdapter::BindItemHolder: with index = "
                   << index << ", item_key = " << item_holder->item_key()
                   << ", operation_id = " << operation_id);
    bool should_request_state_restore =
        list_container_->should_request_state_restore();
    if (list_delegate->ComponentAtIndex(index, operation_id,
                                        should_request_state_restore)) {
      item_holder->MarkVirtualDomPreloaded(true);
      // TODO(dingwang.wxx): Move the events invocations in finishing bind.
      list_item_delegate = GetItemElementDelegate(item_holder);
      if (should_send_exposure_event && list_item_delegate) {
        list_item_delegate->OnListItemWillAppear(item_holder->item_key());
      }
      if (should_send_exposure_event) {
        list_container_->list_event_manager()->SendExposureEvent(
            kEventNodeAppear, item_holder);
      }
      return true;
    }
  }
  return false;
}

// When the rendering of the list's child node is complete, this method will
// be invoked.
void DefaultListAdapter::OnFinishBindItemHolder(
    ItemElementDelegate* list_item_delegate,
    const std::shared_ptr<tasm::PipelineOptions>& option) {
  if (!list_item_delegate) {
    DLIST_LOGE("[" << list_container_
                   << "] DefaultListAdapter::OnFinishBindItemHolder: "
                   << "list_item_delegate is nullptr");
    return;
  }
  int64_t operation_id = option->operation_id;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DEFAULT_LIST_ADAPTER_FINISH_BIND_ITEM_HOLDER,
              "operation_id", operation_id);
  ElementDelegate* list_delegate = list_container_->list_delegate();
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
      DLIST_LOGI(
          "[" << list_container_
              << "] DefaultListAdapter::OnFinishBindItemHolder: with index = "
              << index << ", item_key = " << item_key << ", operation_id = "
              << operation_id << ", list item element impl_id = "
              << list_item_delegate->GetImplId());
      // Update item holder's info with list_item_delegate.
      binding_item_holder->SetItemDelegate(list_item_delegate);
      binding_item_holder->UpdateLayoutFromItemDelegate();
      binding_item_holder->SetOrientation(
          list_container_->list_layout_manager()->orientation());
      list_delegate->CheckZIndex(list_item_delegate);
      // Reset operation id.
      binding_item_holder->SetOperationId(0);

      // Add ItemHolder to attach_children_.
      list_container_->list_children_helper()->AttachChild(binding_item_holder,
                                                           list_item_delegate);
      // Note: Mark should_flush_finish_layout_ to determine whether needs to
      // invoke FinishLayoutOperation().
      list_container_->MarkShouldFlushFinishLayout(option->has_layout);
      if (list_container_->intercept_depth() == 0) {
        list_container_->list_layout_manager()->OnLayoutChildren(true, index);
      }
      list_delegate->ReportListItemLifecycleStatistic(option, item_key);
    } else {
      DLIST_LOGI("[" << list_container_
                     << "] DefaultListAdapter::OnFinishBindItemHolder: "
                        "ItemHolder is destroy with operation_id = "
                     << operation_id);
      binding_item_holder_weak_map_->erase(operation_id);
      list_delegate->EnqueueComponent(list_item_delegate->GetImplId());
    }
  } else {
    DLIST_LOGE(
        "[" << list_container_
            << "] DefaultListAdapter::OnFinishBindItemHolder: "
            << "not in binding_item_holder_weak_map_ with operation_id = "
            << operation_id);
    const auto& attached_delegate_item_holder_map =
        list_container_->list_children_helper()
            ->attached_delegate_item_holder_map();
    if (attached_delegate_item_holder_map.find(list_item_delegate) !=
        attached_delegate_item_holder_map.end()) {
      // The component is not attached to any item holder, so we can enqueue
      // this component directly.
      list_delegate->EnqueueComponent(list_item_delegate->GetImplId());
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
    list_container_->list_delegate()->FlushImmediately();
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

}  // namespace list
}  // namespace lynx
