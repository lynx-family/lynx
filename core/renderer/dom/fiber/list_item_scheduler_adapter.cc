// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"

#include <deque>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

void ListItemSchedulerAdapter::ResolveSubtreeProperty() {
  std::deque<FiberElement*> queue;
  queue.emplace_back(render_root_);
  while (!queue.empty()) {
    auto current = queue.front();
    {
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY,
          "ListItemSchedulerAdapter::SubtreeNodePrepareAndPostResolveTask");
      current->ResolveParentComponentElement();
      if (current->parent()) {
        current->parent()->EnsureTagInfo();
      }
      current->PostResolveTaskToThreadPool(false, resolve_property_queue());
    }
    for (const auto& child : current->children()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "ListItemSchedulerAdapter::SubtreeAsyncResolveEnqueue");
      queue.emplace_back(child.get());
    }
    queue.pop_front();
  }
}

base::closure ListItemSchedulerAdapter::GenerateReduceTaskForResolveProperty() {
  return ([this]() mutable {
    if (batch_render_strategy_ ==
            list::BatchRenderStrategy::kAsyncResolveProperty ||
        batch_render_strategy_ ==
            list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
      ConsumeResolvePropertyReduceTasks();
    }

    // post flush the flushAction task in the thread pool and
    // collect the subtree adjustment tasks recursively.
    PostResolveElementTree(
        render_root_->element_manager()->ParallelResolveTreeTasks());
  });
}

// TODO: Refactor as a general task-queue consumption static method later
void ListItemSchedulerAdapter::ConsumeResolvePropertyReduceTasks() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListItemSchedulerAdapter::ConsumeListItemReduceTasks",
              "list_item", std::to_string(render_root_->impl_id()));
  if (resolve_property_queue().empty()) {
    return;
  }
  while (!resolve_property_queue().empty()) {
    if (resolve_property_queue().front().get()->GetFuture().wait_for(
            std::chrono::seconds(0)) == std::future_status::ready) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "ListItemSchedulerAdapter::ConsumeReduceTasks");
      resolve_property_queue().front().get()->GetFuture().get()();
      resolve_property_queue().pop_front();
    } else if (resolve_property_queue().back().get()->Run()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "ListItemSchedulerAdapter::RunAndConsumeReduceTasks");
      resolve_property_queue().back().get()->GetFuture().get()();
      resolve_property_queue().pop_back();
    } else {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "ListItemSchedulerAdapter:WaitAndReduceTasks");
      ParallelFlushReturn task;
      task = resolve_property_queue().front().get()->GetFuture().get();
      task();
      resolve_property_queue().pop_front();
    }
  }
}

void ListItemSchedulerAdapter::PostResolveElementTree(
    std::list<base::OnceTaskRefptr<base::closure>>&
        parallel_resolve_element_tree_queue) {
  if (batch_render_strategy_ ==
      list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "ListItemSchedulerAdapter::PostFlushListItemsActions");

    std::promise<ParallelFlushReturn> promise;
    std::future<ParallelFlushReturn> future = promise.get_future();
    auto task_info_ptr =
        fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
            [this, promise = std::move(promise)]() mutable {
              TRACE_EVENT(
                  LYNX_TRACE_CATEGORY,
                  "ListItemSchedulerAdapter::AsyncFlushActionWithListItem",
                  "list_item", std::to_string(render_root_->impl_id()));
              batch_rendering_ = true;
              render_root_->FlushActions();
              batch_rendering_ = false;
              promise.set_value(
                  this->GenerateReduceTaskForResolveElementTree());
            },
            std::move(future));
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [task_info_ptr]() { task_info_ptr->Run(); },
        base::ConcurrentTaskType::HIGH_PRIORITY);
    parallel_resolve_element_tree_queue.emplace_back(std::move(task_info_ptr));
  }
}

base::closure
ListItemSchedulerAdapter::GenerateReduceTaskForResolveElementTree() {
  return ([this]() mutable {
    if (batch_render_strategy_ ==
        list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
      ConsumeResolveElementTreeReduceTasks();
    }
  });
}

void ListItemSchedulerAdapter::ConsumeResolveElementTreeReduceTasks() {
  if (batch_render_strategy_ ==
      list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY,
        "ListItemSchedulerAdapter::ConsumeResolveElementTreeReduceTasks");
    while (!resolve_element_tree_queue().empty()) {
      // take out the task and execute it.
      resolve_element_tree_queue().front()();
      // delete the task
      resolve_element_tree_queue().pop_front();
    }
  }
}

}  // namespace tasm
}  // namespace lynx
