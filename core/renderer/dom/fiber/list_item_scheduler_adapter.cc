// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"

#include <deque>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {

ListItemSchedulerAdapter::ListItemSchedulerAdapter(
    FiberElement* sub_root, list::BatchRenderStrategy batch_render_strategy,
    ElementContextDelegate* parent_context, bool continuous_resolve_tree)
    : ElementContextDelegate(parent_context, sub_root),
      render_root_(sub_root),
      batch_render_strategy_(batch_render_strategy),
      continuous_resolve_tree_(continuous_resolve_tree) {
  element_context_task_queue_ =
      std::make_unique<ElementContextTaskQueue>([this]() {
        return (render_root_ && render_root_->element_manager())
                   ? render_root_->element_manager()
                         ->GetParallelWithSyncLayout()
                   : false;
      });
}

void ListItemSchedulerAdapter::ResolveSubtreeProperty() {
  std::deque<FiberElement*> queue;
  queue.emplace_back(render_root_);
  while (!queue.empty()) {
    auto current = queue.front();
    {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  LIST_SCHEDULER_ADAPTER_RESOLVE_SUBTREE_PROP);
      current->ResolveParentComponentElement();
      if (current->parent()) {
        current->parent()->EnsureTagInfo();
      }
      current->PostResolveTaskToThreadPool(false, resolve_property_queue());
    }
    for (const auto& child : current->children()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  LIST_SCHEDULER_ADAPTER_SUBTREE_ASYNC_ENQUEUE);
      queue.emplace_back(static_cast<FiberElement*>(child.get()));
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
    // Execute resolve element tree.
    ResolveElementTree(
        render_root_->element_manager()->ParallelResolveTreeTasks());
  });
}

// TODO: Refactor as a general task-queue consumption static method later
void ListItemSchedulerAdapter::ConsumeResolvePropertyReduceTasks() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              LIST_SCHEDULER_ADAPTER_CONSUME_ITEM_REDUCE_TASKS, "list_item",
              std::to_string(render_root_->impl_id()));
  if (resolve_property_queue().empty()) {
    return;
  }
  while (!resolve_property_queue().empty()) {
    if (resolve_property_queue().front().get()->GetFuture().wait_for(
            std::chrono::seconds(0)) == std::future_status::ready) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  LIST_SCHEDULER_ADAPTER_CONSUME_REDUCE_TASKS);
      resolve_property_queue().front().get()->GetFuture().get()();
      resolve_property_queue().pop_front();
    } else if (resolve_property_queue().back().get()->Run()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  LIST_SCHEDULER_ADAPTER_RUN_AND_CONSUME_REDUCE_TASKS);
      resolve_property_queue().back().get()->GetFuture().get()();
      resolve_property_queue().pop_back();
    } else {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  LIST_SCHEDULER_ADAPTER_WAIT_AND_REDUCE_TASKS);
      ParallelFlushReturn task;
      task = resolve_property_queue().front().get()->GetFuture().get();
      task();
      resolve_property_queue().pop_front();
    }
  }
}

void ListItemSchedulerAdapter::ResolveElementTree(
    std::list<base::OnceTaskRefptr<base::closure>>&
        parallel_resolve_element_tree_queue) {
  if (batch_render_strategy_ ==
      list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_SCHEDULER_ADAPTER_POST_FLUSH_ACTIONS);

    std::promise<ParallelFlushReturn> promise;
    std::future<ParallelFlushReturn> future = promise.get_future();
    auto task_info_ptr =
        fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
            [this, promise = std::move(promise)]() mutable {
              TRACE_EVENT(
                  LYNX_TRACE_CATEGORY, LIST_SCHEDULER_ADAPTER_ASYNC_FLUSH,
                  [render_root = render_root_,
                   impl_id = render_root_->impl_id()](
                      lynx::perfetto::EventContext ctx) {
                    if (render_root->element_manager()) {
                      ctx.event()->add_debug_annotations(
                          INSTANCE_ID,
                          std::to_string(
                              render_root->element_manager()->GetInstanceId()));
                    }
                    ctx.event()->add_debug_annotations("list_item",
                                                       std::to_string(impl_id));
                  });
              batch_resolving_tree_ = true;
              render_root_->FlushActions();
              batch_resolving_tree_ = false;
              promise.set_value(
                  this->GenerateReduceTaskForResolveElementTree());
            },
            std::move(future));
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [task_info_ptr]() { task_info_ptr->Run(); },
        base::ConcurrentTaskType::HIGH_PRIORITY);
    parallel_resolve_element_tree_queue.emplace_back(std::move(task_info_ptr));
  } else {
    if (continuous_resolve_tree_ &&
        batch_render_strategy_ ==
            list::BatchRenderStrategy::kAsyncResolveProperty) {
      // Invoke resolve element tree directly after consuming resolve property
      // reduce tasks.
      render_root_->FlushActions();
    }
    if (render_root_->element_manager()
            ->GetEnableBatchLayoutTaskWithSyncLayout()) {
      FlushEnqueuedTasks();
    }
  }
}

base::closure
ListItemSchedulerAdapter::GenerateReduceTaskForResolveElementTree() {
  return ([this]() mutable {
    if (batch_render_strategy_ ==
        list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
      ConsumeResolveElementTreeReduceTasks();
    }

    if (render_root_->element_manager()
            ->GetEnableBatchLayoutTaskWithSyncLayout()) {
      FlushEnqueuedTasks();
    }
  });
}

void ListItemSchedulerAdapter::ConsumeResolveElementTreeReduceTasks() {
  if (batch_render_strategy_ ==
      list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                LIST_SCHEDULER_ADAPTER_CONSUME_ELEMENT_REDUCE_TASKS);
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
