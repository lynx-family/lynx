// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_LIST_ITEM_SCHEDULER_ADAPTER_H_
#define CORE_RENDERER_DOM_FIBER_LIST_ITEM_SCHEDULER_ADAPTER_H_

#include <list>
#include <memory>

#include "base/include/closure.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {
class FiberElement;

class ListItemSchedulerAdapter {
 public:
  ListItemSchedulerAdapter(FiberElement* sub_root,
                           list::BatchRenderStrategy batch_render_strategy,
                           bool continuous_resolve_tree);

  std::list<base::OnceTaskRefptr<base::closure>>& resolve_property_queue() {
    return resolve_property_queue_;
  }

  std::list<base::closure>& resolve_element_tree_queue() {
    return resolve_element_tree_queue_;
  }

  void ResolveSubtreeProperty();

  base::closure GenerateReduceTaskForResolveProperty();

  void ConsumeResolvePropertyReduceTasks();

  base::closure GenerateReduceTaskForResolveElementTree();

  void ConsumeResolveElementTreeReduceTasks();

  void ResolveElementTree(std::list<base::OnceTaskRefptr<base::closure>>&
                              parallel_resolve_element_tree_queue);

  bool IsBatchResolvingTree() { return batch_resolving_tree_; }

 private:
  FiberElement* render_root_;
  list::BatchRenderStrategy batch_render_strategy_{
      list::BatchRenderStrategy::kDefault};
  bool continuous_resolve_tree_{false};

  std::list<base::OnceTaskRefptr<base::closure>> resolve_property_queue_{};

  std::list<base::closure> resolve_element_tree_queue_{};
  bool batch_resolving_tree_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_LIST_ITEM_SCHEDULER_ADAPTER_H_
