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
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_property.h"
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
using ParallelFlushReturn = base::closure;
using ParallelReduceTaskQueue =
    std::list<base::OnceTaskRefptr<ParallelFlushReturn>>;

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
  virtual fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const {
    // Because the performance of the copy constructor is better than the
    // combination of default construction and assignment operation, we choose
    // to use the copy constructor to copy the element here. To minimize the
    // impact caused by exposing the copy constructor, we have made it protected
    // and encapsulated it in CloneElement.
    return fml::AdoptRef<FiberElement>(
        new FiberElement(*this, clone_resolved_props));
  }

  void SetupFragmentBehavior(Fragment* fragment) override;

  ~FiberElement() override;

  void ReleaseSelf() const override { delete this; }

  struct InheritedProperty {
    // indicate it's children has been marked to propagate inherited properties.
    bool children_propagate_inherited_styles_flag_{false};

    const StyleMap* inherited_styles_{nullptr};
    const base::Vector<tasm::CSSPropertyID>* reset_inherited_ids_{nullptr};
    const CustomPropertiesMap* custom_properties_{nullptr};
  };

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

  virtual const InheritedProperty GetInheritedProperty();

  const InheritedProperty GetParentInheritedProperty();

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
   * Element API for inserting child
   * @param child refCounted child
   */
  virtual void InsertNode(const fml::RefPtr<Element>& child) override;

  /**
   * Element API for replacing elements
   * @param inserted inserted elements
   * @param removed removed elements
   */
  void ReplaceElements(const base::Vector<fml::RefPtr<FiberElement>>& inserted,
                       const base::Vector<fml::RefPtr<FiberElement>>& removed,
                       FiberElement* ref_node);

  /**
   * Element API for InsertingNodeBefore reference child
   * @param child the child Element need to be inserted
   * @param reference_child the reference child
   */
  void InsertNodeBefore(const fml::RefPtr<FiberElement>& child,
                        const fml::RefPtr<FiberElement>& reference_child);
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

  /**
   * Element API for updating css variables
   * @param variables the css variables to be updated from JS.
   */
  void UpdateCSSVariable(const lepus::Value& variables,
                         std::shared_ptr<PipelineOptions>& pipeline_option);

  /**
   * Element API for setNativeProps
   *  @param native_props the props that updated from js.
   */
  void SetNativeProps(
      const lepus::Value& native_props,
      std::shared_ptr<PipelineOptions>& pipeline_options) override;

  virtual StyleMap GetStylesForWorklet() override;

  /**
   * @brief Set the style objects for the current element.
   *
   * This method is used to assign a list of style objects to the element.
   * The object list is managed by a custom deleter, which will be called
   * when the unique pointer goes out of scope.
   * @note This function is not implemented yet.
   * @param object_list A unique pointer to an array of StyleObject pointers,
   *                    along with a custom deleter function for the array.
   */
  void SetStyleObjects(
      std::unique_ptr<style::StyleObject*, style::StyleObjectArrayDeleter>
          object_list) override final;

  /**
   * @brief Update the simple styles of the current element.
   *
   * This method is used to update the simple styles of the element based on
   * the provided style map. The style map contains key-value pairs representing
   * CSS properties and their values.
   *
   * @note This function is not implemented yet.
   *
   * @param style_map A constant reference to a tasm::StyleMap containing the
   *                  styles to be updated.
   */
  void UpdateSimpleStyles(const tasm::StyleMap& style_map) final;

  void UpdateSimpleStyles(tasm::StyleMap&& style_map) final;

  void UpdateStaticAndDynamicSimpleStyles(
      tasm::StyleMap&& style_map,
      tasm::StyleMap&& dynamic_style_map) override final;

  void UpdateDynamicSimpleStyles(tasm::StyleMap&& style_map) override final;

  /**
   * @brief Reset the simple style associated with the specified CSS property
   * ID.
   *
   * This method is intended to reset the simple style of the current element
   * corresponding to the given CSS property ID.
   *
   * @note This function is not implemented yet.
   *
   * @param id The CSS property ID of the style to be reset.
   */
  void ResetSimpleStyle(const tasm::CSSPropertyID id,
                        const tasm::CSSValue& value) override final;
  void ResetSimpleStyle(const tasm::CSSPropertyID id) override final;
  // Update the dynamic simple style source object. The resolved dynamic layer
  // will be applied during flush in ResolveSimpleStyles().
  void ReplaceDynamicSimpleStyles(
      style::DynamicStyleObjectRef new_style_object);
  void AddDynamicSimpleStyles(tasm::StyleMap&& new_styles);
  void RemoveDynamicSimpleStyleKV(tasm::CSSPropertyID id);
  void AddDynamicSimpleStyleKV(tasm::CSSPropertyID id, tasm::CSSValue&& value);
  void ResolveCSSStyles(StyleMap& parsed_styles,
                        base::InlineVector<CSSPropertyID, 16>& reset_style_ids,
                        bool& need_update,
                        bool& force_use_current_parsed_style_map);
  void ResolveSimpleStyles();

  void TraversalInsertFixedElementOfTree();

  template <typename F>
  void ApplyFunctionRecursive(F&& func) {
    func(this);
    for (const auto& child : scoped_children_) {
      static_cast<FiberElement*>(child.get())->ApplyFunctionRecursive(func);
    }
  }

  void MarkFontSizeInvalidateRecursively();

  // if child's related css variable is updated, invalidate child's style.
  void RecursivelyMarkChildrenCSSVariableDirty(
      const lepus::Value& css_variable_updated);

  void ConsumeStyle(const StyleMap& styles,
                    const StyleMap* inherit_styles) override;

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

  // TODO(linxs): to check if this APIs can be deleted
  void InsertNodeBeforeInternal(const fml::RefPtr<FiberElement>& child,
                                FiberElement* ref_node);
  void AddChildAt(fml::RefPtr<FiberElement> child, int index);

  /**
   * Special API for processing Font size
   * font size should be handled at the beginning
   */
  void SetFontSize(const tasm::CSSValue& value);

  void ResetFontSize();

  void UpdateFiberElement();

  virtual void MarkAsLayoutRoot() override;
  virtual void MarkLayoutDirty() override;
  virtual void AttachLayoutNode(const fml::RefPtr<PropBundle>& props) override;
  virtual void UpdateLayoutNodeProps(
      const fml::RefPtr<PropBundle>& props) override;
  virtual void UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                     const tasm::CSSValue& value) override;
  virtual void ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) override;
  virtual void UpdateLayoutNodeFontSize(double cur_node_font_size,
                                        double root_node_font_size) override;
  virtual void UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                         const lepus::Value& value) override;

  /**
   * Interface used to create/update LayoutNode for FiberElement.
   */
  void UpdateLayoutNodeByBundle();

  virtual void CheckHasInlineContainer(Element* parent) override;

  virtual void EnqueueLayoutTask(
      base::MoveOnlyClosure<void> operation) override;

  void HandleDelayTask(base::MoveOnlyClosure<void> operation) override;

  void HandleKeyframePropsChange();

  void RequestLayout() override;

  void RequestNextFrame() override;

  bool IsRelatedCSSVariableUpdated(AttributeHolder* holder,
                                   const lepus::Value changing_css_variables);

  void ResetSheetRecursively(
      const std::shared_ptr<CSSStyleSheetManager>& manager);

  virtual ParallelFlushReturn PrepareForCreateOrUpdate();

  void InsertLayoutNode(FiberElement* child, FiberElement* ref);
  void RemoveLayoutNode(FiberElement* child);

  void StoreLayoutNode(FiberElement* child, FiberElement* ref);
  void RestoreLayoutNode(FiberElement* child);

  // For snapshot test
  void DumpStyle(StyleMap& parsed_styles);

  void OnPseudoStatusChanged(PseudoState prev_status,
                             PseudoState current_status) override;

  bool RefreshStyle(StyleMap& parsed_styles,
                    base::Vector<CSSPropertyID>& reset_ids,
                    bool force_use_parsed_styles_map = false);

  void OnClassChanged(const ClassList& old_classes,
                      const ClassList& new_classes);

  void UpdateDynamicElementStyle(uint32_t style, bool force_update) override;

  void CheckDynamicUnit(CSSPropertyID id, const CSSValue& value,
                        bool reset) override;
  void WillResetCSSValue(CSSPropertyID& id) override;

  // FIXME(liujilong.me): unify trace relative macros.
#if ENABLE_TRACE_PERFETTO
  virtual void UpdateTraceDebugInfo(TraceEvent* event);
#endif

  // The text element can call this function to convert child fiber elements
  // into inline elements. Currently, only view, text, image and wrapper
  // elements may be converted into inline elements.

  // current element is inserted to DOM tree
  virtual void InsertedInto(FiberElement* insertion_point);

  // current element is removed from DOM tree
  virtual void RemovedFrom(FiberElement* insertion_point);

  // The element object created using the clone interface of FiberElement is not
  // attached to the element manager. Use this function to attach it to the
  // element manager.
  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  int32_t GetCSSID() const override;

  bool MergeInlineStyles(StyleMap& new_styles) final;
  void PersistAnimationFillStyles(const StyleMap& styles) override;
  void ClearPersistedAnimationFillStyle(CSSPropertyID id) override;

  void PrepareOrUpdatePseudoElement(PseudoState state, StyleMap& style_map);

  void CreateListItemScheduler(list::BatchRenderStrategy batch_render_strategy,
                               ElementContextDelegate* parent_context,
                               bool continuous_resolve_tree);

  void RecursivelyMarkRenderRootElement(FiberElement* render_root);

  void UpdateRenderRootElementIfNecessary(FiberElement* child);

  ListItemSchedulerAdapter* GetSchedulerAdapter() {
    if (scheduler_adapter_) {
      return scheduler_adapter_.get();
    }
    return nullptr;
  }

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

  void SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func);

  bool CollectCustomProperties(AttributeHolder* holder);

  void PrepareSelfForThreadedElementResolution();

  void InvalidateChildrenIfNeeded();

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  void ConsumeStyleInternal(
      const StyleMap& styles, const StyleMap* inherit_styles,
      std::function<bool(CSSPropertyID, const tasm::CSSValue&)> should_skip)
      override;

  void ProcessFullRawInlineStyle(CSSVariableMap* changed_css_vars) override;

  bool ConsumeAllAttributes();

  void PerformElementContainerCreateOrUpdate(bool need_update, bool need_reset);

  ParallelFlushReturn CreateParallelTaskHandler();

  /**
   * This function will be called before add node.
   * @param child the added node
   */
  virtual void OnNodeAdded(FiberElement* child);

  // called when a child element is removed
  virtual void OnNodeRemoved(FiberElement* child) {}

  virtual void SetAttributeInternal(const base::String& key,
                                    const lepus::Value& value);

  virtual CSSFragment* GetRelatedCSSFragment() override;

  virtual void MarkHasLayoutOnlyPropsIfNecessary(
      const base::String& attribute_key);

  void UpdateLayoutInfoRecursively(PipelineOptions* options);

  void DispatchLayoutBeforeRecursively();

  void SetMeasureFunc(void* context, starlight::SLMeasureFunc measure_func);
  void SetAlignmentFunc(void* context,
                        starlight::SLAlignmentFunc alignment_func);

 private:
  friend class WrapperElement;
  friend class ComponentElement;

  FiberElement* FindEnclosingNoneWrapper(FiberElement* parent,
                                         FiberElement* node);

  void HandleContainerInsertion(FiberElement* parent, FiberElement* child,
                                FiberElement* ref);

  void ResetDirectionAwareProperty(const CSSPropertyID& id,
                                   const CSSValue& value);

  void TryDoDirectionRelatedCSSChange(CSSPropertyID id, const CSSValue& value,
                                      IsLogic is_logic_style);

  bool TryResolveLogicStyleAndSaveDirectionRelatedStyle(CSSPropertyID id,
                                                        const CSSValue& value);

  void HandleSelfFixedChange();
  void InsertFixedElement(FiberElement* child, FiberElement* ref_node);
  void RemoveFixedElement(FiberElement* child);

  void ResetTextAlign(StyleMap& update_map, bool direction_reset);

  bool CheckHasInvalidationForId(const std::string& old_id,
                                 const std::string& new_id) override;

  bool CheckHasInvalidationForClass(const ClassList& old_classes,
                                    const ClassList& new_classes);
  void InvalidateChildren(css::InvalidationSet* invalidation_set);
  void VisitChildren(const base::MoveOnlyClosure<void, FiberElement*>& visitor);

  PseudoElement* CreatePseudoElementIfNeed(PseudoState state);

  void SetFontSizeForAllElement(double cur_node_font_size,
                                double root_node_font_size);
  void UpdateLengthContextValueForAllElement(const LynxEnvConfig& env_config);

  void UpdateDynamicElementStyleRecursively(uint32_t style, bool force_update);

  void PrepareComponentExternalStyles(AttributeHolder* holder);
  void PrepareRootCSSVariables(AttributeHolder* holder);
  void ParseRawInlineStyles(CSSVariableMap* changed_css_vars);
  void DoFullCSSResolving();
  const tasm::CSSValue& ResolveCurrentStyleValue(
      const CSSPropertyID& key, const tasm::CSSValue& default_value);

  void UpdateLayoutInfo();

  void MarkLayoutDirtyLite() override;

  void EnsureSLNode();

  virtual void DispatchLayoutBefore();

  void ApplySimpleStyleWithoutTail(const tasm::CSSPropertyID id,
                                   const tasm::CSSValue& value);
  void ApplySimpleStylesWithoutTail(const tasm::StyleMap& style_map);
  void ApplyDynamicSimpleStylesWithoutTail(
      const tasm::StyleMap& dynamic_style_map,
      const tasm::StyleMap& base_style_map);
  void FinalizeSimpleStyleUpdate();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
