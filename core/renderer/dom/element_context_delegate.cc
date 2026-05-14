// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/element_context_delegate.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace tasm {
ElementContextDelegate::ElementContextDelegate(
    ElementContextDelegate* parent_element_context, Element* context_root)
    : parent_element_context_(parent_element_context),
      element_context_root_(context_root) {}

void ElementContextDelegate::EnqueueTask(
    base::MoveOnlyClosure<void> operation) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONTEXT_DELEGATE_ENQUEUE_TASK);
  element_context_task_queue_->EnqueueTask(std::move(operation));
}

void ElementContextDelegate::FlushEnqueuedTasks() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              ELEMENT_CONTEXT_DELEGATE_FLUSH_ENQUEUED_TASKS);

  for (auto& child_context : scoped_children_element_contexts_) {
    child_context->FlushEnqueuedTasks();
  }
  element_context_task_queue_->FlushEnqueuedTasks();
}

void ElementContextDelegate::OnChildElementContextAdded(
    std::shared_ptr<ElementContextDelegate>& child_element_context) {
  child_element_context->parent_element_context_ = this;
  scoped_children_element_contexts_.emplace_back(
      std::move(child_element_context));
}

void ElementContextDelegate::OnChildElementContextRemoved(
    ElementContextDelegate* child_element_context) {
  for (auto it = scoped_children_element_contexts_.begin();
       it != scoped_children_element_contexts_.end(); ++it) {
    if (it->get() == child_element_context) {
      scoped_children_element_contexts_.erase(it);
      break;
    }
  }
}

bool ElementContextDelegate::IsListItemElementContext() { return false; }

void ElementContextDelegate::RemoveSelf() {
  if (parent_element_context_) {
    for (auto& child_context_ptr : scoped_children_element_contexts_) {
      parent_element_context_->OnChildElementContextAdded(child_context_ptr);
    }
    // Remove self from parent need to be called after moving children to
    // parent, for removing self from parent will free the memory of self.
    parent_element_context_->OnChildElementContextRemoved(this);
  }
}
}  // namespace tasm
}  // namespace lynx
