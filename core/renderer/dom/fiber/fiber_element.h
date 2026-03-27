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
#include "core/renderer/dom/selector/selector_item.h"
#include "core/renderer/simple_styling/simple_style_node.h"
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

class FiberElement : public Element,
                     public SelectorItem,
                     public style::SimpleStyleNode {
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

  struct DirectionMapping {
    DirectionMapping()
        : is_logic_(false),
          ltr_property_(kPropertyStart),
          rtl_property_(kPropertyStart) {}
    DirectionMapping(bool is_logic, CSSPropertyID ltr_property,
                     CSSPropertyID rtl_property)
        : is_logic_(is_logic),
          ltr_property_(ltr_property),
          rtl_property_(rtl_property) {}
    bool is_logic_{false};
    CSSPropertyID ltr_property_{CSSPropertyID::kPropertyStart};
    CSSPropertyID rtl_property_{CSSPropertyID::kPropertyStart};
  };

  // for Fiber specific
  virtual bool is_component() const { return false; }
  virtual bool is_scroll_view() const { return false; }
  virtual bool is_raw_text() const { return false; }

  bool is_wrapper() const override { return false; }
  virtual bool is_none() const { return false; }

  virtual bool is_block() const { return false; }
  virtual bool is_if() const { return false; }
  virtual bool is_for() const { return false; }

  bool is_inline_element() const { return is_inline_element_; }

  bool is_list_item() const { return is_list_item_; }

  int32_t dirty() const { return dirty_; }

  virtual const InheritedProperty GetInheritedProperty();

  const InheritedProperty GetParentInheritedProperty();

  virtual bool NeedFullFlushPath(CSSPropertyID id,
                                 const CSSValue& value) override;

  const StyleMap& GetParsedStylesMap() const { return parsed_styles_map_; }

  /**
   * A key function to GetListNode
   */
  virtual ListNode* GetListNode() override { return nullptr; };

  /**
   * A key function to get parent component's element
   */
  virtual Element* GetParentComponentElement() const override;

  /**
   * A function to resolve parent component element CSSFragment
   */
  void ResolveParentComponentElement() const;

  void ResolveParentComponentElementImpl() const;

  /**
   * A key function to flush the tree with the current element as the root node.
   */
  virtual void FlushActionsAsRoot();

  // This interface is currently only used by the inspector. The inspector
  // determines whether an element is created by the itself by checking whether
  // element has a data model. Since the data model of a fiber element is not
  // empty by default, this interface is provided to the inspector to reset the
  // data model and mark the element as created by the inspector.
  void ResetDataModel() { data_model_ = nullptr; }

  virtual bool CanBeLayoutOnly() const override;

  void MarkCanBeLayoutOnly(bool flag) { can_be_layout_only_ = flag; }

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

  void UpdateResolveStatus(AsyncResolveStatus value) {
    resolve_status_ = value;
  }

  bool IsAsyncResolveInvoked() {
    return resolve_status_ != AsyncResolveStatus::kCreated &&
           resolve_status_ != AsyncResolveStatus::kUpdated;
  }

  bool IsAsyncResolveResolving() {
    return resolve_status_ == AsyncResolveStatus::kResolving ||
           resolve_status_ == AsyncResolveStatus::kResolved ||
           resolve_status_ == AsyncResolveStatus::kPreparing ||
           resolve_status_ == AsyncResolveStatus::kSyncResolving;
  }

  /**
   * A key function for generating children's actions.
   */
  void PrepareAndGenerateChildrenActions();

  virtual void HandleInsertChildAction(FiberElement* child, int index,
                                       FiberElement* ref_node);
  virtual void HandleRemoveChildAction(FiberElement* child);

  void HandleRemoveSelf(FiberElement* removal_point,
                        FiberElement* render_parent);

  int64_t GetParentComponentUniqueIdForFiber() {
    return parent_component_unique_id_;
  }

  void SetParentComponentUniqueIdForFiber(int64_t id) {
    if (id != parent_component_unique_id_) {
      parent_component_element_ = nullptr;
    }
    parent_component_unique_id_ = id;
  }

  void SetParentComponentUniqueIdRecursively(int64_t id) {
    if (is_page()) {
      SetParentComponentUniqueIdForFiber(impl_id());
    } else {
      SetParentComponentUniqueIdForFiber(id);
    }

    for (const auto& child : scoped_children_) {
      static_cast<FiberElement*>(child.get())
          ->SetParentComponentUniqueIdRecursively(
              is_page() || is_component() ? impl_id() : id);
    }
  }

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
   * Element API for setting class name to Element
   * @param clazz the name of class selector
   */
  void SetClass(const base::String& clazz);

  /**
   * Element API for setting class names to Element
   * @param classes the vector contains the name of class selector
   */
  void SetClasses(ClassList&& classes);

  /**
   * Element API for removing all classes of
   */
  LYNX_EXPORT_FOR_DEVTOOL void RemoveAllClass();

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
   * Element API for appending css style to element
   * @param id the css property id
   * @param value the css property lepus type vale
   */
  LYNX_EXPORT_FOR_DEVTOOL void SetStyle(CSSPropertyID id,
                                        const lepus::Value& value);

  /**
   * Element API for updating css variables
   * @param variables the css variables to be updated from JS.
   */
  void UpdateCSSVariable(const lepus::Value& variables,
                         std::shared_ptr<PipelineOptions>& pipeline_option);

  /**
   * Element API for removing all inline styles.
   */
  LYNX_EXPORT_FOR_DEVTOOL void RemoveAllInlineStyles();

  /**
   * Destroy the related platform node of this element
   */
  void DestroyPlatformNode();

  /**
   * Before SetAttribute(), reserve array size.
   */
  virtual void ReserveForAttribute(size_t count) override;

  /**
   * Element API for appending single attribute to element
   * @param key the attribute String type name
   * @param value the attribute value
   */
  virtual void SetAttribute(const base::String& key, const lepus::Value& value,
                            bool need_update_data_model = true) override;

  virtual void SetBuiltinAttribute(ElementBuiltInAttributeEnum key,
                                   const lepus::Value& value);
  /**
   * Element API for setting id for element
   * @param idSelector the id of the element
   */
  void SetIdSelector(const base::String& idSelector);

  /**
   * Element API for adding js event
   * @param name the binding event's name
   * @param type the binding event's type
   * @param callback the binding event's corresponding js function name
   */
  void SetJSEventHandler(const base::String& name, const base::String& type,
                         const base::String& callback);

  /**
   * Element API for setNativeProps
   *  @param native_props the props that updated from js.
   */
  virtual void SetNativeProps(
      const lepus::Value& native_props,
      std::shared_ptr<PipelineOptions>& pipeline_options) override;

  /**
   * Element API for adding lepus event
   * @param name the binding event's name
   * @param type the binding event's type
   * @param script the binding event's corresponding lepus script
   * @param callback the binding event's corresponding lepus function
   */
  void SetLepusEventHandler(const base::String& name, const base::String& type,
                            const lepus::Value& script,
                            const lepus::Value& callback);

  /**
   * Element API for adding worklet event
   * @param name the binding worklet event's name
   * @param type the binding worklet event's type
   * @param worklet_info the binding worklet info, passed to the front-end
   * @param ctx the context of Lepus / LepusNg
   * framework
   */
  void SetWorkletEventHandler(const base::String& name,
                              const base::String& type,
                              const lepus::Value& worklet_info,
                              lepus::Context* ctx);

  /**
   * Element API for removing specific event
   * @param name the removed event's name
   * @param type the removed event's type
   */
  void RemoveEvent(const base::String& name, const base::String& type);

  /**
   * Element API for removing all events
   */
  void RemoveAllEvents();

  /**
   * Element API for adding gesture detector
   */
  void SetGestureDetector(const uint32_t gesture_id,
                          GestureDetector gesture_detector);

  /**
   * Element API for removing specific gesture detector
   * @param gesture_id the removed gesture' id
   */
  void RemoveGestureDetector(const uint32_t gesture_id);

  /**
   * Element API for setting compile stage parsed style
   * @param parsed_styles the parsed styles
   * @param config parsed styles' config
   */
  void SetParsedStyles(const ParsedStyles& parsed_styles,
                       const lepus::Value& config);

  void SetParsedStyles(StyleMap&& parsed_styles, CSSVariableMap&& css_var);

  /**
   * Element API for adding config.
   * @param key the config key,
   * @param value the config value.
   */
  void AddConfig(const base::String& key, const lepus::Value& value);

  /**
   * Element API for setting config.
   * @param config the config will be setted,
   */
  void SetConfig(const lepus::Value& config);

  /**
   * A key function to get element's config.
   * The returned value is constant. You should not get Table() from
   * the value and change configs. Use AddConfig() instead which will
   * guarantee this element creates a writable config table.
   */
  const lepus::Value config() const {
    return lepus::Value(
        config_ ?: fml::RefPtr<lepus::Dictionary>(lepus::Value::DummyTable()));
  }

  virtual StyleMap GetStylesForWorklet() override;

  virtual const AttrMap& GetAttributesForWorklet() override;

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
  void ResetSimpleStyle(const tasm::CSSPropertyID id) override final;
  void ResolveCSSStyles(StyleMap& parsed_styles,
                        base::InlineVector<CSSPropertyID, 16>& reset_style_ids,
                        bool& need_update,
                        bool& force_use_current_parsed_style_map);
  void ResolveSimpleStyles();

  const base::String& GetRawInlineStyles();

  // Check has_value() before usage to avoid unintentional construction.
  const auto& GetCurrentRawInlineStyles() const {
    return current_raw_inline_styles_;
  }

  void SetRawInlineStyles(base::String value);

  void MarkDirty(const uint32_t flag) {
    dirty_ |= flag;
    RequireFlush();
  }

  virtual void MarkDirtyLite(const uint32_t flag) {
    dirty_ |= flag;
    MarkRequireFlush();
  }

  void ResetAllDirtyBits() { dirty_ = 0; }

  bool StyleDirty() const { return dirty_ & kDirtyStyle; }

  bool AttrDirty() const { return dirty_ & kDirtyAttr; }

  void MarkPropsDirty() { MarkDirty(kDirtyForceUpdate); }

  void TraversalInsertFixedElementOfTree();

  template <typename F>
  void ApplyFunctionRecursive(F&& func) {
    func(this);
    for (const auto& child : scoped_children_) {
      static_cast<FiberElement*>(child.get())->ApplyFunctionRecursive(func);
    }
  }

  void ResetStyleSheet() { style_sheet_ = nullptr; };

  void MarkStyleDirty(bool recursive = false);

  void MarkFontSizeInvalidateRecursively();

  // if child's related css variable is updated, invalidate child's style.
  void RecursivelyMarkChildrenCSSVariableDirty(
      const lepus::Value& css_variable_updated);

  void MarkRefreshCSSStyles() { MarkDirty(kDirtyRefreshCSSVariables); }

  void ConsumeStyle(const StyleMap& styles,
                    const StyleMap* inherit_styles) override;

  void AddDataset(const base::String& key, const lepus::Value& value);
  void SetDataset(const lepus::Value& data_set);

  bool NeedForceClassChangeTransmit() const {
    return enable_class_change_transmit_ && !(dirty_ & kDirtyCreated);
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

  bool InComponent() const override;

  std::string ParentComponentIdString() const override;
  const std::string& ParentComponentEntryName() const override;

  // TODO(linxs): to check if this APIs can be deleted
  void InsertNodeBeforeInternal(const fml::RefPtr<FiberElement>& child,
                                FiberElement* ref_node);
  void InsertNodeBeforeInternal(const fml::RefPtr<FiberElement>& child,
                                FiberElement* ref_node,
                                bool update_logical_children);
  void AddChildAt(fml::RefPtr<FiberElement> child, int index);

  virtual int32_t IndexOf(const Element* child) const override;

  Element* GetChildAt(size_t index) override;
  size_t GetChildCount() override { return scoped_children_.size(); }
  ElementChildrenArray GetChildren() override;

  /**
   * Special API for processing Font size
   * font size should be handled at the beginning
   */
  void SetFontSize(const tasm::CSSValue& value);

  void ResetFontSize();

  void UpdateFiberElement();

  inline void EnsureLayoutBundle();
  void InitLayoutBundle();
  void UpdateTagToLayoutBundle();
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

  void HandleBeforeFlushActionsTask(base::MoveOnlyClosure<void> operation,
                                    int32_t predicate_parallel_flush_bitset);

  void HandleKeyframePropsChange();

  void VerifyKeyframePropsChangedHandling();

  void RequestLayout() override;

  void RequestNextFrame() override;

  bool IsRelatedCSSVariableUpdated(AttributeHolder* holder,
                                   const lepus::Value changing_css_variables);

  bool HasElementContainer() { return element_container_ != nullptr; }

  void set_style_sheet_manager(
      const std::shared_ptr<CSSStyleSheetManager>& manager) {
    css_style_sheet_manager_ = manager;
  }

  const std::shared_ptr<CSSStyleSheetManager>& style_sheet_manager() {
    return css_style_sheet_manager_;
  }

  void ResetSheetRecursively(
      const std::shared_ptr<CSSStyleSheetManager>& manager);

  virtual void SetCSSID(int32_t id);

  bool IsInSameCSSScope(FiberElement* element) {
    return css_id_ == element->css_id_;
  }

  const auto& children() const { return scoped_children_; }
  const auto& logical_children() const { return logical_children_; }

  Element* Sibling(int offset) const override;
  Element* render_parent() override { return render_parent_; }
  Element* first_render_child() override { return first_render_child_; }
  Element* next_render_sibling() override { return next_render_sibling_; }
  virtual Element* first_child() const override {
    return scoped_children_.empty() ? nullptr : scoped_children_.front().get();
  };
  virtual Element* last_child() const override {
    return scoped_children_.empty() ? nullptr : scoped_children_.back().get();
  };

  // set/get virtual parent node in AirModeFiber
  void set_virtual_parent(FiberElement* virtual_parent) {
    virtual_parent_ = virtual_parent;
  }
  FiberElement* virtual_parent() {
    return static_cast<FiberElement*>(virtual_parent_);
  }
  FiberElement* root_virtual_parent();

  const ClassList& classes() { return data_model_->classes(); }

  ClassList ReleaseClasses() { return data_model_->ReleaseClasses(); }

  const base::String& GetIdSelector() { return data_model_->idSelector(); }

  const DataMap& dataset() { return data_model_->dataset(); }

  virtual ParallelFlushReturn PrepareForCreateOrUpdate();

  void set_attached_to_layout_parent(bool has) {
    attached_to_layout_parent_ = has;
  }
  bool attached_to_layout_parent() const { return attached_to_layout_parent_; }

  // Helpers for finding non-virtual / non-wrapper nodes in the render tree
  // starting from the current element.
  FiberElement* FindFirstNonVirtualRenderAncestor();
  FiberElement* FindFirstNonVirtualRenderSibling();
  FiberElement* FindFirstNonWrapperRenderAncestor();
  FiberElement* FindFirstNonWrapperChildOrSibling();

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

  void OnPatchFinish(std::shared_ptr<PipelineOptions>& option) override;

  void FlushAnimatedStyleInternal(tasm::CSSPropertyID,
                                  const tasm::CSSValue&) override;

  virtual void ConsumeTransitionStylesInAdvanceInternal(
      CSSPropertyID css_id, const tasm::CSSValue& value) override;

  virtual void ResetTransitionStylesInAdvanceInternal(
      CSSPropertyID css_id) override;

  virtual std::optional<CSSValue> GetElementStyle(
      tasm::CSSPropertyID css_id) override;

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
  virtual void ConvertToInlineElement();

  bool flush_required() { return flush_required_; }

  void MarkTemplateElement() { is_template_ = true; }

  bool IsTemplateElement() const { return is_template_; }

  void MarkPartElement(base::String&& part_id) {
    part_id_ = std::move(part_id);
  }

  bool IsPartElement() const { return !part_id_.empty(); }

  const base::String& GetPartID() const { return part_id_; }

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

  virtual size_t CountInlineStyles() override;
  bool MergeInlineStyles(StyleMap& new_styles) final;

  virtual bool WillResolveStyle(StyleMap& merged_styles,
                                CSSVariableMap* changed_css_vars) override;

  // Check has_value() before usage to avoid unintentional construction.
  const auto& builtin_attr_map() const { return builtin_attr_map_; }

  // Check has_value() before usage to avoid unintentional construction.
  const auto& updated_attr_map() const { return updated_attr_map_; }

  void PrepareOrUpdatePseudoElement(PseudoState state, StyleMap& style_map);

  void UpdateAttrMap(const base::String& key, const lepus::Value& value) {
    updated_attr_map_[key] = value;
  }

  void MarkAttrDirtyForPseudoElement() { dirty_ |= kDirtyAttr; }

  void CreateListItemScheduler(list::BatchRenderStrategy batch_render_strategy,
                               ElementContextDelegate* parent_context,
                               bool continuous_resolve_tree);

  void RecursivelyMarkRenderRootElement(FiberElement* render_root);

  void UpdateRenderRootElementIfNecessary(FiberElement* child);

  void ClearExtremeParsedStyles() {
    if (has_extreme_parsed_styles_) {
      extreme_parsed_styles_.reset();
      has_extreme_parsed_styles_ = false;
    }
  }

  // Exported for accessing private field from Element Manager to handle legacy
  // logic
  inline FiberElement* GetRenderRootElement() {
    return static_cast<FiberElement*>(render_root_element_);
  }
  ListItemSchedulerAdapter* GetSchedulerAdapter() {
    if (scheduler_adapter_) {
      return scheduler_adapter_.get();
    }
    return nullptr;
  }

  inline bool ShouldProcessParallelTasks() {
    return is_parallel_flush() ||
           resolve_status_ == AsyncResolveStatus::kSyncResolving;
  }

  inline bool ShouldResolveStyle() {
    return !IsAsyncResolveResolving() && ((dirty_ & ~kDirtyTree) != 0);
  }

  inline void EnqueueReduceTask(base::MoveOnlyClosure<void> operation) {
    parallel_reduce_tasks_->emplace_back(std::move(operation));
  }

  virtual int32_t GetMemoryUsage() const override { return sizeof(*this); }

  SLNode* GetLayoutObject() const override { return sl_node_.get(); }

  inline SLNode* slnode() const {
    if (sl_node_ != nullptr) {
      return sl_node_.get();
    }

    return nullptr;
  }
  inline bool IsAsyncFlushRoot() const { return is_async_flush_root_; }
  inline void MarkAsyncFlushRoot(bool value) { is_async_flush_root_ = value; }

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

  void SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func);

  lepus::Value GetComputedStyleByKey(const base::String& key);

  bool CollectCustomProperties(AttributeHolder* holder);
  void MarkCustomPropertiesDirty();

  void PrepareSelfForThreadedElementResolution();

  bool NeedPropagateInheritedDirtyFlag(bool force_propagate);

  void InvalidateChildrenIfNeeded();

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  void ConsumeStyleInternal(
      const StyleMap& styles, const StyleMap* inherit_styles,
      std::function<bool(CSSPropertyID, const tasm::CSSValue&)> should_skip);

  bool ConsumeAllAttributes();

  void PerformElementContainerCreateOrUpdate(bool need_update, bool need_reset);

  bool IsNewlyCreated() const { return dirty_ & kDirtyCreated; }

  ParallelFlushReturn CreateParallelTaskHandler();

  void CacheStyleFromAttributes(CSSPropertyID id, CSSValue&& value);
  void CacheStyleFromAttributes(CSSPropertyID id, const lepus::Value& value);
  void DidConsumeStyle();

  void MarkAsInline() {
    is_inline_element_ = true;
    has_layout_only_props_ = false;
  }

  void ProcessFullRawInlineStyle(CSSVariableMap* changed_css_vars);

  /**
   * This function will be called before add node.
   * @param child the added node
   */
  virtual void OnNodeAdded(FiberElement* child);

  // called when a child element is removed
  virtual void OnNodeRemoved(FiberElement* child) {}

  // handle default overflow logic
  void SetDefaultOverflow(bool visible);

  virtual void SetAttributeInternal(const base::String& key,
                                    const lepus::Value& value);

  void RequireFlush();

  // Mark flush_required without recursively mark parent element
  inline void MarkRequireFlush() { flush_required_ = true; }

  virtual CSSFragment* GetRelatedCSSFragment() override;

  virtual void MarkHasLayoutOnlyPropsIfNecessary(
      const base::String& attribute_key);

  bool ShouldDestroy() const;

  void UpdateLayoutInfoRecursively(PipelineOptions* options);

  void DispatchLayoutBeforeRecursively();

  void SetMeasureFunc(void* context, starlight::SLMeasureFunc measure_func);
  void SetAlignmentFunc(void* context,
                        starlight::SLAlignmentFunc alignment_func);

  virtual void OnLayoutObjectCreated();

 private:
  friend class WrapperElement;
  friend class ComponentElement;
  friend class BlockElement;

  inline void MarkPlatformNodeDestroyed();

  bool CheckHasIdMapInCSSFragment();

  FiberElement* FindEnclosingNoneWrapper(FiberElement* parent,
                                         FiberElement* node);

  void HandleContainerInsertion(FiberElement* parent, FiberElement* child,
                                FiberElement* ref);
  void InsertLogicalChildBefore(const fml::RefPtr<FiberElement>& child,
                                FiberElement* ref_node);
  void RemoveLogicalChild(const fml::RefPtr<FiberElement>& child);
  void RemoveNodeInternal(const fml::RefPtr<FiberElement>& child, bool destroy,
                          bool update_logical_children);

  bool IsInheritable(CSSPropertyID id) const;

  bool IsDirectionChangedEnabled() const;

  void ResetDirectionAwareProperty(const CSSPropertyID& id,
                                   const CSSValue& value);

  void TryDoDirectionRelatedCSSChange(CSSPropertyID id, const CSSValue& value,
                                      IsLogic is_logic_style);

  bool TryResolveLogicStyleAndSaveDirectionRelatedStyle(CSSPropertyID id,
                                                        const CSSValue& value);

  std::pair<bool, CSSPropertyID> ConvertRtlCSSPropertyID(CSSPropertyID id);

  void HandleSelfFixedChange();
  void InsertFixedElement(FiberElement* child, FiberElement* ref_node);
  void RemoveFixedElement(FiberElement* child);

  void ResetTextAlign(StyleMap& update_map, bool direction_reset);

  bool CheckHasInvalidationForId(const std::string& old_id,
                                 const std::string& new_id);

  bool CheckHasInvalidationForClass(const ClassList& old_classes,
                                    const ClassList& new_classes);
  void InvalidateChildren(css::InvalidationSet* invalidation_set);
  void VisitChildren(const base::MoveOnlyClosure<void, FiberElement*>& visitor);

  void LogNodeInfo();

  DirectionMapping CheckDirectionMapping(CSSPropertyID css_id);

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

  bool IfNeedsUpdateLayoutInfo();

  void EnsureSLNode();

  virtual void DispatchLayoutBefore();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
