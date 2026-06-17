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

  void FiberAddEvent(const base::String& type, const base::String& name,
                     const lepus::Value& callback,
                     const std::string& context_name);

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
  void ResolveCSSStylesNewPipeline(bool& need_update);
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
  void InvalidateChildrenFontSizeRecursively();
  void InvalidateChildrenInheritedStylesRecursively();

  // if child's related css variable is updated, invalidate child's style.
  void RecursivelyMarkChildrenCSSVariableDirty(
      const lepus::Value& css_variable_updated);
  void MarkDirectChildrenStyleDirtyForInheritedPropertyMutation();

  /**
   * @brief Recursively marks all scoped children as style-dirty when custom
   * properties change on this element.
   */
  void RecursivelyMarkCustomPropertiesDirty();

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

  /**
   * Special API for processing Font size
   * font size should be handled at the beginning
   */
  void SetFontSize(const tasm::CSSValue& value);

  /**
   * @brief Sets font-size on a specific target ComputedCSSStyle rather than
   * the element's own platform style.
   * @param value The font-size CSS value.
   * @param target_style The ComputedCSSStyle to update.
   */
  void SetFontSize(const tasm::CSSValue& value,
                   starlight::ComputedCSSStyle* target_style);

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

  enum class StyleSideEffectReplayMode {
    kNormal,
    kPreserveLayoutOnly,
  };

  /**
   * @brief Replays the side effects of a single changed style property.
   * @param id The CSS property that changed.
   * @param value The new computed value.
   */
  void ReplayChangedStyleSideEffect(
      CSSPropertyID id, const CSSValue& value,
      StyleSideEffectReplayMode mode = StyleSideEffectReplayMode::kNormal);
  void ReplayResetStyleSideEffect(
      CSSPropertyID id,
      StyleSideEffectReplayMode mode = StyleSideEffectReplayMode::kNormal);

  /**
   * @brief Commits font-size and root-font-size changes after style resolution.
   * @param computed_style The final computed style.
   * @param old_font_size The previous font size.
   * @param old_root_font_size The previous root font size.
   */
  void CommitFontContext(const starlight::ComputedCSSStyle& computed_style,
                         double old_font_size, double old_root_font_size);
  void FinalizeAnimationPropsChange(bool& need_update);
  struct AnimationPropertyChangeAnalysisForLegacyAnimator {
    bool has_transition_props_changed{false};
    bool has_keyframe_props_changed{false};
  };
  AnimationPropertyChangeAnalysisForLegacyAnimator
  AnalyzeAnimationPropChangesForLegacyAnimator(
      const starlight::ComputedCSSStyle& final_style,
      const starlight::ComputedCSSStyle* previous_final_style,
      const StyleMap& resolved_style_map) const;
  struct AnimationSampleAnalysisForNewPipeline {
    bool has_style_effects{false};
    bool has_animated_font_size{false};
    bool has_custom_property_effects{false};
    bool changes_resolve_context{false};
  };

  // Inputs that tell the new-pipeline resolve pass why it is being run and
  // which external style contexts must be refreshed.
  struct NewPipelineResolveRequest {
    // Run the resolve path even when the element has no style dirty bit.
    bool force_resolve{false};
    // Force platform side-effect replay after resolve, even if no dynamic
    // dependency flag matches the element's resolved styles.
    bool force_platform_update{false};
    // Dynamic style contexts that changed in this flush, such as viewport,
    // screen metrics, rem, or em.
    DynamicCSSStylesManager::StyleUpdateFlags dynamic_update_flags{0};
  };

  // Return summary from ResolveCSSStylesNewPipelineCore() to the outer flush
  // flow. Unlike NewPipelineStyleResolveResult, this does not carry resolved
  // style snapshots or ownership. It only reports what the caller should do
  // after the element has resolved and optionally committed its style.
  struct NewPipelineResolveOutcome {
    // Whether this element needs a platform node update/layout request.
    bool need_update{false};
    // Whether descendants must be resolved because this element changed a
    // context they depend on, such as inherited styles, variables, or font
    // units.
    bool force_children{false};
    // Dynamic dependency flags found in this element's resolved styles.
    DynamicCSSStylesManager::StyleUpdateFlags dynamic_style_flags{0};
    // Dynamic dependency flags that descendants should refresh because of this
    // element's context change.
    DynamicCSSStylesManager::StyleUpdateFlags child_update_flags{0};
  };

  // Dynamic-style replay inputs collected from the resolved final style.
  // They let dynamic context updates replay affected properties without
  // rebuilding the full cascade for every element.
  struct NewPipelineDynamicStyleInputs {
    // Properties that should participate in dynamic-unit replay. Starts with
    // explicit resolved styles from this pass and may be extended with
    // inherited dynamic-unit values from the final ComputedCSSStyle.
    StyleMap resolved_style_map;
    // Subset of resolved_style_map that came only from inheritance, not from
    // explicit matched/inline/attribute/animation sources on this element.
    CSSIDBitset inherited_dynamic_ids;
    // Union of dynamic dependency flags for inherited_dynamic_ids.
    DynamicCSSStylesManager::StyleUpdateFlags inherited_dynamic_flags{0};
  };

  // Diff plan for committing and replaying one new-pipeline style resolve
  // result. It is built from the old final style, the new final style, and the
  // explicit resolved source map.
  struct NewPipelineStyleMutationPlan {
    // New resolved values for properties that changed or need dynamic replay.
    StyleMap update_values;
    // Property ids present in update_values.
    CSSIDBitset update_ids;
    // Properties that existed in the previous final style but disappeared from
    // the new final style.
    CSSIDBitset reset_ids;
    // Properties explicitly produced by this resolve pass. Replay uses this to
    // distinguish explicit styles from inherited platform values, especially
    // for layout-only inherited-property preservation.
    CSSIDBitset source_style_ids;
    // Whether the plan was built for the element's first render.
    bool first_render{false};
    // True when AddUpdate/AddReset recorded a normal resolved-value diff.
    bool source_changed{false};
    // True when the raw/resolved custom-property maps changed.
    bool custom_properties_changed{false};
    // True when this element's font-size context changed.
    bool font_size_context_changed{false};
    // True when this element's root-font-size context changed.
    bool root_font_size_context_changed{false};

    void AddUpdate(CSSPropertyID id, const CSSValue& value);
    void AddReset(CSSPropertyID id);
    bool HasOperations() const;
    bool NeedsSemanticCommit() const;
  };

  // Detailed internal result from ResolveComputedStyles(). It carries the
  // resolved base/final style views, source maps, variable dependency data, and
  // transient owned snapshots needed by ResolveCSSStylesNewPipelineCore() to
  // build mutation plans, commit style, and replay side effects. The outer
  // flush flow receives only NewPipelineResolveOutcome.
  struct NewPipelineStyleResolveResult {
    // Explicit resolved source properties from this resolve pass.
    StyleMap resolved_style_map;
    // TODO(zhouzhitao): get rid of underlying_layout_only_styles if
    // layout_in_element is fully rolled out

    // Layout-only source properties needed by transition sampling while
    // layout-in-element is not universally enabled.
    StyleMap underlying_layout_only_styles;
    // Properties in resolved_style_map whose values depend on var().
    CSSIDBitset variable_dependent_ids;
    // Animation overrides/resets sampled against the newly resolved base style.
    animation::AnimationSampleForNewPipeline animation_sample;
    // Parent style used for inheritance and animation-triggered rebuilds.
    const starlight::ComputedCSSStyle* parent_inheritance_style{nullptr};
    // Final style committed by the previous resolve, used as the diff baseline.
    const starlight::ComputedCSSStyle* previous_final_style{nullptr};
    // Semantic style after animation effects. Downstream logic should read this
    // after ResolveComputedStyles() returns. It may alias owned_final_style,
    // owned_base_style, or the element's platform_css_style_.
    starlight::ComputedCSSStyle* final_style{nullptr};
    // Semantic style before animation effects. It may alias owned_base_style or
    // final_style when no separate base snapshot is needed.
    starlight::ComputedCSSStyle* base_style{nullptr};
    // owned_* carry the transient storage backing those semantic views until
    // the caller decides whether this resolution pass actually commits. They
    // cannot be replaced by final_style/base_style because commit-time code
    // needs to move ownership into platform_css_style_ / base_css_style_, while
    // final_style/base_style may also alias existing external storage.
    // Owns the resolved unanimated base snapshot when it cannot stay in
    // base_css_style_ / platform_css_style_ directly during resolution.
    std::unique_ptr<starlight::ComputedCSSStyle> owned_base_style;
    // Owns the resolved animated final snapshot before it is committed into
    // platform_css_style_.
    std::unique_ptr<starlight::ComputedCSSStyle> owned_final_style;

    // Publishes the semantic final/base style views after ownership has been
    // decided. Callers should read final_style/base_style and ignore owned_*.
    void BindResolvedStyles(starlight::ComputedCSSStyle* platform_style) {
      DCHECK(platform_style != nullptr);
      final_style = owned_final_style != nullptr  ? owned_final_style.get()
                    : owned_base_style != nullptr ? owned_base_style.get()
                                                  : platform_style;
      base_style =
          owned_base_style != nullptr ? owned_base_style.get() : final_style;
      DCHECK(final_style != nullptr);
      DCHECK(base_style != nullptr);
    }

    // Commits the resolved final style into platform_css_style_ when a platform
    // update is required. This moves whichever owned snapshot currently backs
    // final_style and leaves platform_css_style_ unchanged when final_style
    // already aliases the existing platform slot.
    void CommitPlatformStyleIfNeeded(
        std::unique_ptr<starlight::ComputedCSSStyle>& platform_css_style,
        bool style_changed) {
      if (!style_changed) {
        return;
      }
      if (final_style == owned_final_style.get()) {
        platform_css_style = std::move(owned_final_style);
      } else if (final_style == owned_base_style.get()) {
        platform_css_style = std::move(owned_base_style);
      }
    }

    // Persists the unanimated base snapshot into base_css_style_ after the
    // final style has been committed. If final_style reuses owned_base_style,
    // the base slot is only kept when no platform update happened.
    void PersistBaseStyle(
        std::unique_ptr<starlight::ComputedCSSStyle>& base_css_style,
        bool style_changed) {
      if (owned_base_style == nullptr) {
        base_css_style.reset();
        return;
      }
      if (final_style == owned_base_style.get()) {
        if (style_changed) {
          base_css_style.reset();
        } else {
          base_css_style = std::move(owned_base_style);
        }
        return;
      }
      base_css_style = std::move(owned_base_style);
    }
  };

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
  static AnimationSampleAnalysisForNewPipeline
  AnalyzeAnimationSampleForNewPipeline(
      const animation::AnimationSampleForNewPipeline& animation_sample);
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
  DynamicCSSStylesManager::StyleUpdateFlags CollectDynamicFlagsForNewPipeline(
      const StyleMap& resolved_style_map) const;
  NewPipelineStyleMutationPlan BuildNewPipelineStyleMutationPlan(
      const NewPipelineStyleResolveResult& resolved_styles,
      const NewPipelineDynamicStyleInputs& dynamic_inputs,
      DynamicCSSStylesManager::StyleUpdateFlags requested_dynamic_flags,
      bool first_render, double old_font_size, double old_root_font_size) const;
  bool MaterializeNewPipelineStyleMutationPlan(
      const NewPipelineStyleMutationPlan& plan,
      const starlight::ComputedCSSStyle& baseline_style,
      starlight::ComputedCSSStyle& final_style) const;
  bool HasMaterializedInheritedPropertyMutation(
      const starlight::ComputedCSSStyle& style) const;
  void ReplayNewPipelineStyleMutationPlanSideEffects(
      const NewPipelineStyleMutationPlan& plan, CSSIDBitset* replayed_ids);
  NewPipelineResolveOutcome ResolveCSSStylesNewPipelineCore(
      const NewPipelineResolveRequest& request);

  void RequestLayout() override;

  void RequestNextFrame() override;

  bool IsRelatedCSSVariableUpdated(AttributeHolder* holder,
                                   const lepus::Value changing_css_variables);

  void ResetSheetRecursively(
      const std::shared_ptr<CSSStyleSheetManager>& manager);

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

  bool MergeInlineStyles(StyleMap& new_styles,
                         StyleMap& important_styles) final;
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
  bool ShouldFallbackToSerialForNewStylingPipeline() const;

  void InvalidateChildrenIfNeeded();
  bool HasAdjacentSiblingRulesInStyleSheets();

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  // Hook for subclasses to replay element-specific derived style state.
  // Callers should go through ReplayChangedStyleSideEffect() or
  // ReplayResetStyleSideEffect() so FiberElement preserves replay bookkeeping.
  virtual void ReplayElementSpecificStyleSideEffect(CSSPropertyID id) {}

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
  virtual void OnNodeRemoved(FiberElement* child);

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
  friend class BlockElement;

  bool ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
      CSSPropertyID id, const CSSIDBitset& source_style_ids);

  static event::EventListener::Options GetEventListenerOptions(
      const base::String& type);

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
  void UpdateDynamicElementStyleForNewPipeline(uint32_t& style,
                                               bool& inner_force_update);
  void UpdateDynamicChildrenStyleRecursively(uint32_t style, bool force_update);

  void PrepareComponentExternalStyles(AttributeHolder* holder);
  void PrepareRootCSSVariables(AttributeHolder* holder);
  void ParseRawInlineStyles(CSSVariableMap* changed_css_vars);
  void DoFullCSSResolving();
  const tasm::CSSValue& ResolveCurrentStyleValue(
      const CSSPropertyID& key, const tasm::CSSValue& default_value);

  void UpdateLayoutInfo();

  void MarkLayoutDirtyLite() override;

  void EnsureSLNode();
  bool HasLayoutInElementPlatformNode();
  int GetLayoutInElementPlatformChildIndex(FiberElement* child);

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
