// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ELEMENT_CONTEXT_DELEGATE_H_
#define CORE_RENDERER_DOM_ELEMENT_CONTEXT_DELEGATE_H_
#include <atomic>
#include <memory>
#include <vector>

#include "base/include/closure.h"
#include "core/renderer/dom/element_context_task_queue.h"

namespace lynx {
namespace tasm {
class Element;

/// The object for scheduling tasks generated but not necessarily need to be
/// consumed in elemeent resolution. This class also serves as a tree node for
/// ElementContext tree, here ElementContext is defined as a scoped context for
/// independent pixeling pipeline.
class ElementContextDelegate {
 public:
  ElementContextDelegate(ElementContextDelegate* parent_element_context,
                         Element* context_root);
  virtual ~ElementContextDelegate() = default;

  void EnqueueTask(base::MoveOnlyClosure<void> operation);
  void FlushEnqueuedTasks();
  void OnChildElementContextAdded(
      std::shared_ptr<ElementContextDelegate>& child_element_context);
  /// Type specific check for list item element context.
  virtual bool IsListItemElementContext();
  Element* GetElementContextRoot() { return element_context_root_; }
  /// Remove self from ElementContext tree.
  void RemoveSelf();

 protected:
  /// The task queue for tasks generated but not necessarily need to be consumed
  /// in elemeent resolution, need to be initialzied in subclasses.
  std::unique_ptr<ElementContextTaskQueue> element_context_task_queue_{nullptr};

 private:
  void OnChildElementContextRemoved(
      ElementContextDelegate* child_element_context);

  std::vector<std::shared_ptr<ElementContextDelegate>>
      scoped_children_element_contexts_{};
  ElementContextDelegate* parent_element_context_{nullptr};
  Element* element_context_root_{nullptr};
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_ELEMENT_CONTEXT_DELEGATE_H_
