// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "base/include/auto_create_optional.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/base/thread/once_task.h"
#include "core/event/event_listener.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"
#include "core/renderer/dom/layout_bundle.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/base/element_template_info.h"

namespace lynx {
namespace tasm {
class NodeManager;
class PlatformLayoutFunctionWrapper;

enum NodeInfoBits : int32_t {
  // Mask for layout node type, using lower 16 bits.
  kLayoutNodeTypeMask = 0x0000FFFF,
  // Mask for async creation flag.
  kCreateAsyncMask = 0x00010000,
};

constexpr const int32_t kCommonBuiltInNodeInfo =
    (static_cast<int32_t>(LayoutNodeType::COMMON) &
     NodeInfoBits::kLayoutNodeTypeMask) |
    NodeInfoBits::kCreateAsyncMask;
constexpr const int32_t kVirtualBuiltInNodeInfo =
    (static_cast<int32_t>(LayoutNodeType::VIRTUAL) &
     NodeInfoBits::kLayoutNodeTypeMask);
constexpr const int32_t kCustomBuiltInNodeInfo =
    (static_cast<int32_t>(LayoutNodeType::CUSTOM) &
     NodeInfoBits::kLayoutNodeTypeMask) |
    NodeInfoBits::kCreateAsyncMask;

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
    return fml::AdoptRef<Element>(
        new FiberElement(*this, clone_resolved_props));
  }

  void SetupFragmentBehavior(Fragment* fragment) override;

  ~FiberElement() override;

  void ReleaseSelf() const override { delete this; }

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

  // for Fiber specific

  /**
   * A key function to GetListNode
   */
  /**
   * A key function to flush the tree with the current element as the root node.
   */
  void FlushActionsAsRoot() override;

  /**
   * A key function for flush all pending actions for current Element
   */
  void FlushActions() override;

  void FlushSelf() override;

  void PrepareChildren() override;

  void PrepareChildForInsertion(Element* child);

  virtual void ParallelFlushAsRoot();

  void DidParallelFlushAsRoot(PerfStatistic& stats);

  void OnParallelFlushAsRoot(PerfStatistic& stats);

  void ParallelFlushRecursively() override;

  virtual void PostResolveTaskToThreadPool(
      bool is_engine_thread, ParallelReduceTaskQueue& task_queue) override;

  /**
   * A key function for generating children's actions.
   */
  void PrepareAndGenerateChildrenActions();

  /**
   * Element API for inserting child
   * @param child refCounted child
   */
  virtual void InsertNode(const fml::RefPtr<Element>& child) override;

  /**
   * Element API for replacing elements
   * @param inserted inserted elements
   * @param removed removed elements
   */
  void ReplaceElements(const base::Vector<fml::RefPtr<Element>>& inserted,
                       const base::Vector<fml::RefPtr<Element>>& removed,
                       Element* ref_node) override;

  /**
   * Element API for InsertingNodeBefore reference child
   * @param child the child Element need to be inserted
   * @param reference_child the reference child
   */
  void InsertNodeBefore(const fml::RefPtr<Element>& child,
                        const fml::RefPtr<Element>& reference_child) override;
  /**
   * Element API for removing the specific child Element
   * @param child the Element to be removed
   */
  virtual void RemoveNode(const fml::RefPtr<Element>& child,
                          bool destroy = true) override;

  /**
   * Deprecated: Inset child Element to the specific index
   * @param child the Element to be inserted
   * @param index the index where the child Element to be inserted
   */
  virtual void InsertNode(const fml::RefPtr<Element>& child,
                          int32_t index) override;

  void ResolveCSSStyles(StyleMap& parsed_styles,
                        base::InlineVector<CSSPropertyID, 16>& reset_style_ids,
                        bool& need_update,
                        bool& force_use_current_parsed_style_map);

  void TraversalInsertFixedElementOfTree() override;

  void ConsumeStyle(const StyleMap& styles,
                    const StyleMap* inherit_styles) override;

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

  // TODO(linxs): to check if this APIs can be deleted
  void InsertNodeBeforeInternal(const fml::RefPtr<Element>& child,
                                Element* ref_node) override;
  void InsertNodeBeforeInternal(const fml::RefPtr<Element>& child,
                                Element* ref_node,
                                bool update_logical_children) override;
  void AddChildAt(fml::RefPtr<Element> child, int index) override;
  void RemoveNodeInternal(const fml::RefPtr<Element>& child, bool destroy,
                          bool update_logical_children) override;

  virtual void CheckHasInlineContainer(Element* parent) override;

  virtual void EnqueueLayoutTask(
      base::MoveOnlyClosure<void> operation) override;

  ParallelFlushReturn PrepareForCreateOrUpdate() override;

  void OnPseudoStatusChanged(PseudoState prev_status,
                             PseudoState current_status) override;

  // The text element can call this function to convert child fiber elements
  // into inline elements. Currently, only view, text, image and wrapper
  // elements may be converted into inline elements.

  // current element is inserted to DOM tree
  void InsertedInto(Element* insertion_point) override;

  // current element is removed from DOM tree
  void RemovedFrom(Element* insertion_point) override;

  // The element object created using the clone interface of FiberElement is not
  // attached to the element manager. Use this function to attach it to the
  // element manager.
  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

  bool ShouldFallbackToSerialForNewStylingPipeline() const;

  bool HasAdjacentSiblingRulesInStyleSheets();

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  void ConsumeStyleInternal(
      const StyleMap& styles, const StyleMap* inherit_styles,
      std::function<bool(CSSPropertyID, const tasm::CSSValue&)> should_skip)
      override;

  void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value) override;
  bool ResetCSSValue(CSSPropertyID id) override;

  bool ConsumeAllAttributes();

  ParallelFlushReturn CreateParallelTaskHandler();

  /**
   * This function will be called before add node.
   * @param child the added node
   */
  virtual void OnNodeAdded(Element* child);

  // called when a child element is removed
  virtual void OnNodeRemoved(Element* child);

  virtual void SetAttributeInternal(const base::String& key,
                                    const lepus::Value& value);

  virtual void MarkHasLayoutOnlyPropsIfNecessary(
      const base::String& attribute_key);

 private:
  friend class WrapperElement;
  friend class ComponentElement;
  friend class BlockElement;

  void InsertLogicalChildBefore(const fml::RefPtr<Element>& child,
                                Element* ref_node);
  Element* ReplaceTemplateChildIfNeeded(
      base::InlineVector<fml::RefPtr<Element>,
                         kChildrenInlineVectorSize>::iterator child_iter);

  void ResetDirectionAwareProperty(const CSSPropertyID& id,
                                   const CSSValue& value);

  void TryDoDirectionRelatedCSSChange(CSSPropertyID id, const CSSValue& value,
                                      IsLogic is_logic_style);

  bool TryResolveLogicStyleAndSaveDirectionRelatedStyle(CSSPropertyID id,
                                                        const CSSValue& value);

  void ResetTextAlign(StyleMap& update_map, bool direction_reset);

  void PrepareComponentExternalStyles(AttributeHolder* holder);
  void PrepareRootCSSVariables(AttributeHolder* holder);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
