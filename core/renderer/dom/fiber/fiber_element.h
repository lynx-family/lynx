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

  void ResolveCSSStyles(StyleMap& parsed_styles,
                        base::InlineVector<CSSPropertyID, 16>& reset_style_ids,
                        bool& need_update,
                        bool& force_use_current_parsed_style_map);
  void ResolveCSSStylesNewPipeline(bool& need_update);

  void TraversalInsertFixedElementOfTree();

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
  void InsertNodeBeforeInternal(const fml::RefPtr<FiberElement>& child,
                                FiberElement* ref_node,
                                bool update_logical_children);
  void AddChildAt(fml::RefPtr<FiberElement> child, int index);

  void UpdateFiberElement();

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

  animation::AnimationSampleForNewPipeline
  SampleAnimationOverridesForNewPipeline(
      starlight::ComputedCSSStyle& new_base_style, bool base_font_size_changed,
      bool base_root_font_size_changed,
      const StyleMap& new_underlying_layout_only_styles,
      const starlight::ComputedCSSStyle*& previous_final_style);
  bool HasAuthorAnimationDataChangedForNewPipeline(
      const starlight::ComputedCSSStyle& new_base_style,
      const starlight::ComputedCSSStyle* previous_base_style) const;
  void FlushImperativeAnimationCleanupForNewPipeline(
      starlight::ComputedCSSStyle& cleanup_style, bool& need_update,
      CSSIDBitset* replayed_ids, const CSSIDBitset* source_style_ids = nullptr);
  std::unique_ptr<starlight::ComputedCSSStyle>
  BuildFinalStyleFromAnimationSampleForNewPipeline(
      const starlight::ComputedCSSStyle& base_style,
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_final_style,
      const animation::AnimationSampleForNewPipeline& animation_sample,
      StyleMap& resolved_style_map, CSSIDBitset& variable_dependent_ids);
  animation::AnimationEventRecordsForNewPipeline
  TakeAnimationEventsForNewPipeline();
  bool NeedsAnimationFrameForNewPipeline() const;

  /**
   * @brief Resolves the base computed style by collecting matched rules,
   * inline styles, and attribute styles.
   * @param previous_final_style The previous final computed style.
   * @param old_font_size The previous font size.
   * @param old_root_font_size The previous root font size.
   * @return A NewPipelineStyleResolveResult containing base and final styles.
   */
  NewPipelineStyleResolveResult ResolveComputedStyles(
      const starlight::ComputedCSSStyle* previous_final_style,
      double old_font_size, double old_root_font_size);

  void ReplayMaterializedStyleSideEffects(
      const starlight::ComputedCSSStyle& computed_style,
      CSSIDBitset* replayed_ids = nullptr,
      const NewPipelineStyleMutationPlan* plan = nullptr);
  void ReplayDynamicResolvedStyleSideEffects(
      const StyleMap& resolved_style_map,
      DynamicCSSStylesManager::StyleUpdateFlags update_flags,
      const CSSIDBitset& replayed_ids,
      const CSSIDBitset* source_style_ids = nullptr,
      const CSSIDBitset* inherited_dynamic_ids = nullptr);
  NewPipelineStyleMutationPlan BuildNewPipelineStyleMutationPlan(
      const NewPipelineStyleResolveResult& resolved_styles,
      const NewPipelineDynamicStyleInputs& dynamic_inputs,
      DynamicCSSStylesManager::StyleUpdateFlags requested_dynamic_flags,
      bool first_render, double old_font_size, double old_root_font_size) const;
  bool MaterializeNewPipelineStyleMutationPlan(
      const NewPipelineStyleMutationPlan& plan,
      const starlight::ComputedCSSStyle& baseline_style,
      starlight::ComputedCSSStyle& final_style) const;
  bool HasInheritedPropertyMutation(
      const NewPipelineStyleMutationPlan& plan) const;
  void ReplayNewPipelineStyleMutationPlanSideEffects(
      const NewPipelineStyleMutationPlan& plan, CSSIDBitset* replayed_ids);
  NewPipelineResolveOutcome ResolveCSSStylesNewPipelineCore(
      const NewPipelineResolveRequest& request);

  void RequestLayout() override;

  void RequestNextFrame() override;

  virtual ParallelFlushReturn PrepareForCreateOrUpdate();

  void InsertLayoutNode(FiberElement* child, FiberElement* ref);
  void RemoveLayoutNode(FiberElement* child,
                        int layout_in_element_platform_index = -1);

  void StoreLayoutNode(FiberElement* child, FiberElement* ref);
  void RestoreLayoutNode(FiberElement* child);

  // For snapshot test
  void DumpStyle(StyleMap& parsed_styles);

  void OnPseudoStatusChanged(PseudoState prev_status,
                             PseudoState current_status) override;

  bool RefreshStyle(StyleMap& parsed_styles,
                    base::Vector<CSSPropertyID>& reset_ids,
                    bool force_use_parsed_styles_map = false);

  void UpdateDynamicElementStyle(uint32_t style, bool force_update) override;

  void CheckDynamicUnit(CSSPropertyID id, const CSSValue& value,
                        bool reset) override;
  void WillResetCSSValue(CSSPropertyID& id) override;

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

  bool MergeInlineStyles(StyleMap& new_styles,
                         StyleMap& important_styles) final;
  void PersistAnimationFillStyles(const StyleMap& styles) override;
  void ClearPersistedAnimationFillStyle(CSSPropertyID id) override;

  void CreateListItemScheduler(list::BatchRenderStrategy batch_render_strategy,
                               bool continuous_resolve_tree);

  ListItemSchedulerAdapter* GetSchedulerAdapter() {
    if (scheduler_adapter_) {
      return scheduler_adapter_.get();
    }
    return nullptr;
  }

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

  bool ShouldFallbackToSerialForNewStylingPipeline() const;

  void InvalidateChildrenIfNeeded();
  bool HasAdjacentSiblingRulesInStyleSheets();

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  void ConsumeStyleInternal(
      const StyleMap& styles, const StyleMap* inherit_styles,
      std::function<bool(CSSPropertyID, const tasm::CSSValue&)> should_skip)
      override;

  void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value) override;
  bool ResetCSSValue(CSSPropertyID id) override;

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
  virtual void OnNodeRemoved(FiberElement* child);

  virtual void SetAttributeInternal(const base::String& key,
                                    const lepus::Value& value);

  virtual CSSFragment* GetRelatedCSSFragment() override;

  virtual void MarkHasLayoutOnlyPropsIfNecessary(
      const base::String& attribute_key);

 private:
  friend class WrapperElement;
  friend class ComponentElement;
  friend class BlockElement;

  FiberElement* FindEnclosingNoneWrapper(FiberElement* parent,
                                         FiberElement* node);

  void HandleContainerInsertion(FiberElement* parent, FiberElement* child,
                                FiberElement* ref);
  void InsertLogicalChildBefore(const fml::RefPtr<FiberElement>& child,
                                FiberElement* ref_node);
  void RemoveLogicalChild(const fml::RefPtr<FiberElement>& child);
  void RemoveNodeInternal(const fml::RefPtr<FiberElement>& child, bool destroy,
                          bool update_logical_children);
  FiberElement* ReplaceTemplateChildIfNeeded(
      base::InlineVector<fml::RefPtr<Element>,
                         kChildrenInlineVectorSize>::iterator child_iter);

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

  void UpdateDynamicElementStyleRecursively(uint32_t style, bool force_update);
  void UpdateDynamicElementStyleForNewPipeline(uint32_t& style,
                                               bool& inner_force_update);

  void PrepareComponentExternalStyles(AttributeHolder* holder);
  void PrepareRootCSSVariables(AttributeHolder* holder);
  void ParseRawInlineStyles(CSSVariableMap* changed_css_vars);
  void DoFullCSSResolving();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
