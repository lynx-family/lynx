// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_

#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "base/include/auto_create_optional.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "core/event/event_listener.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_context_delegate.h"
#include "core/renderer/dom/element_context_task_queue.h"
#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"
#include "core/renderer/dom/fiber/pseudo_element.h"
#include "core/renderer/dom/layout_bundle.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/base/element_template_info.h"

namespace lynx {
namespace tasm {
class NodeManager;
class PlatformLayoutFunctionWrapper;

class FiberElement : public Element {
 public:
  using Action = Element::Action;
  using ActionParam = Element::ActionParam;
  using AsyncResolveStatus = Element::AsyncResolveStatus;

  FiberElement(ElementManager* manager, const base::String& tag);
  FiberElement(ElementManager* manager, const base::String& tag,
               int32_t css_id);

  // This function will clone an incomplete fiber element that is not attached
  // to the element manager. Before using this fiber element, it needs to be
  // attached to the element manager first.
  fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const override {
    // Because the performance of the copy constructor is better than the
    // combination of default construction and assignment operation, we choose
    // to use the copy constructor to copy the element here. To minimize the
    // impact caused by exposing the copy constructor, we have made it protected
    // and encapsulated it in CloneElement.
    return fml::AdoptRef<FiberElement>(
        new FiberElement(*this, clone_resolved_props));
  }

  ~FiberElement() override = default;

  struct PerfStatistic {
    PerfStatistic(uint32_t total_task_count)
        : total_task_count_(total_task_count) {}

    // true if enable reporting stats
    bool enable_report_stats_{false};

    // count of tasks executing on engine thread
    uint32_t engine_thread_task_count_{0};
    uint32_t total_task_count_{0};

    uint64_t total_processing_start_{0};
    uint64_t total_waiting_time_{0};
  };

  /**
   * A key function to GetListNode
   */
  virtual ListNode* GetListNode() override { return nullptr; };

  /**
   * A key function to flush the tree with the current element as the root node.
   */
  virtual void FlushActionsAsRoot();

  virtual bool CanBeLayoutOnly() const override;

  /**
   * A key function for flush all pending actions for current Element
   */
  void FlushActions();

  void FlushSelf();

  void PrepareChildren();

  void PrepareChildForInsertion(FiberElement* child);

  virtual void ParallelFlushAsRoot();

  void DidParallelFlushAsRoot(PerfStatistic& stats);

  void OnParallelFlushAsRoot(PerfStatistic& stats);

  void ParallelFlushRecursively();

  void AsyncResolveProperty();

  virtual void PostResolveTaskToThreadPool(bool is_engine_thread,
                                           ParallelReduceTaskQueue& task_queue);

  void AsyncResolveSubtreeProperty();

  void DispatchAsyncResolveSubtreeProperty();

  void DispatchAsyncResolveProperty();

  void AsyncPostResolveTaskToThreadPool();

  /**
   * A key function for generating children's actions.
   */
  void PrepareAndGenerateChildrenActions();

  virtual void HandleInsertChildAction(FiberElement* child, int index,
                                       FiberElement* ref_node);
  virtual void HandleRemoveChildAction(FiberElement* child);

  void HandleRemoveSelf(FiberElement* removal_point,
                        FiberElement* render_parent);

  /**
   * Element API for replacing elements
   * @param inserted inserted elements
   * @param removed removed elements
   */
  void ReplaceElements(const base::Vector<fml::RefPtr<FiberElement>>& inserted,
                       const base::Vector<fml::RefPtr<FiberElement>>& removed,
                       FiberElement* ref_node);

  void TraversalInsertFixedElementOfTree();

  template <typename F>
  void ApplyFunctionRecursive(F&& func) {
    func(this);
    for (const auto& child : scoped_children_) {
      static_cast<FiberElement*>(child.get())->ApplyFunctionRecursive(func);
    }
  }

  // Flush style and attribute to platform shadow node, platform painting node
  // will be created if has not been created,
  void FlushProps() override;
  const EventMap& event_map() const override {
    if (data_model_) {
      return data_model_->static_events();
    }
    return AttributeHolder::EventBundle::DefaultEmptyEventMap();
  }
  const EventMap& lepus_event_map() override {
    if (data_model_) {
      return data_model_->lepus_events();
    }
    return AttributeHolder::EventBundle::DefaultEmptyEventMap();
  }

  virtual void EnqueueLayoutTask(
      base::MoveOnlyClosure<void> operation) override;

  virtual ParallelFlushReturn PrepareForCreateOrUpdate();

  void InsertLayoutNode(FiberElement* child, FiberElement* ref);
  void RemoveLayoutNode(FiberElement* child);

  void StoreLayoutNode(FiberElement* child, FiberElement* ref);
  void RestoreLayoutNode(FiberElement* child);

  // The text element can call this function to convert child fiber elements
  // into inline elements. Currently, only view, text, image and wrapper
  // elements may be converted into inline elements.

  // The element object created using the clone interface of FiberElement is not
  // attached to the element manager. Use this function to attach it to the
  // element manager.
  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

  using Element::SetMeasureFunc;

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  ParallelFlushReturn CreateParallelTaskHandler();

  void UpdateLayoutInfoRecursively(PipelineOptions* options);

  void SetMeasureFunc(void* context, starlight::SLMeasureFunc measure_func);

 private:
  FiberElement* ReplaceTemplateChildIfNeeded(
      base::InlineVector<fml::RefPtr<Element>,
                         kChildrenInlineVectorSize>::iterator child_iter);

  void HandleSelfFixedChange();
  void InsertFixedElement(FiberElement* child, FiberElement* ref_node);
  void RemoveFixedElement(FiberElement* child);

  void PrepareComponentExternalStyles(AttributeHolder* holder);
  void PrepareRootCSSVariables(AttributeHolder* holder);

  void MarkLayoutDirtyLite() override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
