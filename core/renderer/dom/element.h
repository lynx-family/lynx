// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_H_
#define CORE_RENDERER_DOM_ELEMENT_H_

#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/auto_create_optional.h"
#include "base/include/closure.h"
#include "base/include/flex_optional.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/no_destructor.h"
#include "base/include/value/ref_type.h"
#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/css_transition_manager.h"
#include "core/base/lynx_export.h"
#include "core/base/thread/once_task.h"
#include "core/event/event_target.h"
#include "core/inspector/style_sheet.h"
#include "core/public/common_constants.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_content_data.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/ng/invalidation/invalidation_set.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/base_element_container.h"
#include "core/renderer/dom/element_property.h"
#include "core/renderer/dom/imperative_animation_state.h"
#include "core/renderer/dom/selector/selector_item.h"
#include "core/renderer/dom/style_resolver.h"
#include "core/renderer/events/events.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/simple_styling/simple_style_node.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace runtime {
class MessageEvent;
}  // namespace runtime

namespace tasm {

class AttributeHolder;
class ElementManager;
class ElementContainer;
class HierarchyObserver;
class ListNode;
class LynxEnvConfig;
class Fragment;
class CSSFragmentDecorator;
class CSSParseToken;
struct LayoutBundle;
class PseudoElement;
class ListItemSchedulerAdapter;
class PlatformLayoutFunctionWrapper;
namespace list {
enum class BatchRenderStrategy;
}  // namespace list

using ElementChildrenArray =
    base::InlineVector<Element*, kChildrenInlineVectorSize>;
using ParallelFlushReturn = base::closure;
using ParallelReduceTaskQueue =
    std::list<base::OnceTaskRefptr<ParallelFlushReturn>>;

enum ElementArchTypeEnum : uint8_t {
  FiberArch = 0,
  RadonArch,
};

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

class InspectorAttribute {
 public:
  LYNX_EXPORT_FOR_DEVTOOL InspectorAttribute();
  LYNX_EXPORT_FOR_DEVTOOL ~InspectorAttribute();

 public:
  int node_type_;
  std::string local_name_;
  std::string node_name_;
  std::string node_value_;
  std::string selector_id_;
  std::vector<std::string> class_order_;
  lynx::devtool::InspectorElementType type_;

  // plug element's corresponding slot name
  std::string slot_name_;
  // element's parent component name, only plug element need set this attribute
  std::string parent_component_name_;

  std::vector<std::string> attr_order_;
  std::unordered_map<std::string, std::string> attr_map_;
  std::vector<std::string> data_order_;
  std::unordered_map<std::string, std::string> data_map_;
  std::vector<std::string> event_order_;
  std::unordered_map<std::string, std::string> event_map_;
  Element* style_root_;  // not owned
  lynx::devtool::InspectorStyleSheet inline_style_sheet_;
  std::vector<lynx::devtool::InspectorCSSRule> css_rules_;

  int start_line_;
  std::unordered_multimap<std::string, lynx::devtool::InspectorStyleSheet>
      style_sheet_map_;
  // DevTool selector edits need the exact source token. Keep the cache on the
  // style root so cached tokens cannot outlive the inspected DOM lifetime.
  std::unordered_map<std::string, fml::RefPtr<CSSParseToken>>
      style_sheet_source_token_map_;
  std::unordered_map<std::string, std::vector<lynx::devtool::InspectorKeyframe>>
      animation_map_;

  // for component remove view, erase component
  // id from node_manager in  element destructor
  bool needs_erase_id_ = false;

  bool enable_css_selector_ = false;

  bool wrapper_component_ = false;

  std::unique_ptr<Element> doc_;
  std::unique_ptr<Element> style_value_;
};

class Element : public lepus::RefCounted,
                public fml::EnableWeakFromThis<Element>,
                public event::EventTarget,
                public SelectorItem,
                public style::SimpleStyleNode {
 public:
  Element(ElementManager* manager, const base::String& tag);
  Element(ElementManager* manager, const base::String& tag, int32_t css_id);
  Element(const base::String& tag, ElementManager* element_manager,
          uint32_t node_index);
  void ReleaseSelf() const override { delete this; }

  Element& operator=(const Element&) = delete;

  // Element state, used to indicate whether the current Element is on the root
  // Dom tree.
  enum class State : uint8_t {
    // attached to root DOM tree
    kAttached,
    // removed from root DOM tree
    kDetached,
  };

  enum class Action : uint8_t {
    kCreateAct = 0,
    kDestroyAct,
    kInsertChildAct,
    kRemoveChildAct,
    kMoveAct,
    kUpdatePropsAct,
    kRemoveIntergenerationAct,
  };

  struct ActionParam {
    ActionParam(Action type, Element* parent, const fml::RefPtr<Element>& child,
                int from, Element* ref_node, bool is_fixed = false,
                bool has_z_index = false,
                fml::RefPtr<Element> source_parent = nullptr)
        : type_(type),
          parent_(parent),
          child_(child),
          index_(from),
          ref_node_(ref_node),
          is_fixed_(is_fixed),
          has_z_index_(has_z_index),
          source_parent_(std::move(source_parent)) {}
    Action type_;
    Element* parent_;
    fml::RefPtr<Element> child_;
    int index_;
    Element* ref_node_;
    bool is_fixed_;
    bool has_z_index_;
    fml::RefPtr<Element> source_parent_;
  };

  void AppendActionParam(ActionParam action_param) {
    action_param_list_.emplace_back(std::move(action_param));
  }

  // Direction mapping support for RTL/LTR layout
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

  struct InheritedProperty {
    // indicate it's children has been marked to propagate inherited properties.
    bool children_propagate_inherited_styles_flag_{false};

    const StyleMap* inherited_styles_{nullptr};
    const base::Vector<tasm::CSSPropertyID>* reset_inherited_ids_{nullptr};
    const CustomPropertiesMap* custom_properties_{nullptr};
  };

  static const uint32_t kDirtyCreated;
  static const uint32_t kDirtyTree;
  static const uint32_t kDirtyStyle;
  static const uint32_t kDirtyAttr;
  static const uint32_t kDirtyForceUpdate;
  static const uint32_t kDirtyEvent;
  static const uint32_t kDirtyReAttachContainer;
  static const uint32_t kDirtyPropagateInherited;
  static const uint32_t kDirtyDataset;
  static const uint32_t kDirtyGesture;
  static const uint32_t kDirtyFontSize;
  static const uint32_t kDirtyRefreshCSSVariables;
  static const uint32_t kDirtyStyleObjects;
  static constexpr uint32_t kDirtyCloned = 0x01 << 14;
  static constexpr uint32_t kDirtyDynamicStyleObjects = 0x01 << 15;
  static constexpr uint32_t kDirtyInvoke = 0x01 << 16;

  enum class AsyncResolveStatus : uint8_t {
    kCreated = 0,
    kPrepareRequested,
    kPrepareTriggered,
    kPreparing,
    kSyncResolving,
    kResolving,
    kResolved,
    kUpdated,
  };

  constexpr static const char* kFiberParallelPrepareMode = "ParallelPrepare";

  static const uint32_t kFlagGreedyParallel = 0x01 << 0;
  static const uint32_t kFlagLevelOrderParallel = 0x01 << 1;

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kElement;
  };

  void SetAttributeHolder(const fml::RefPtr<AttributeHolder>& data_model) {
    data_model_ = data_model;
    data_model_->SetElement(this);
    data_model_->set_tag(tag_);
  }
  AttributeHolder* data_model() const { return data_model_.get(); };

  bool is_fixed() { return is_fixed_; }
  bool fixed_changed() const { return fixed_changed_; }
  void set_fixed_changed(bool fixed_changed) { fixed_changed_ = fixed_changed; }
  void set_need_handle_fixed(bool need_handle_fixed) {
    need_handle_fixed_ = need_handle_fixed;
  }
  // TODO(ZHOUZHITAO): Move parallel_flush_ flag from element to
  // ParallelResolver
  bool is_parallel_flush() { return parallel_flush_ > 0; }
  bool is_greedy_parallel_flush() {
    return (parallel_flush_ & kFlagGreedyParallel) > 0;
  }
  void MarkParallelFlushFlag(uint32_t flag) { parallel_flush_ |= flag; }
  void ResetParallelFlushFlag() { parallel_flush_ = 0; }

  int32_t impl_id() const { return id_; }

  uint32_t GlobalInsertionOrder() const { return global_insertion_order_; }
  void UpdateGlobalInsertionOrder();
  void ResetGlobalInsertionOrder() {
    global_insertion_order_ = kInitialGlobalInsertionOrder;
  }

  void SetNodeIndex(uint32_t node_index) { node_index_ = node_index; }
  uint32_t NodeIndex() const { return node_index_; }

  std::vector<float> ScrollBy(float width, float height);
  std::vector<float> GetRectToLynxView();
  std::vector<float> GetRectToScreen();
  void Invoke(const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback);
  void AppendPendingInvokeTask(base::closure task);
  void EnqueueInvoke(
      const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>&
          callback);
  void FlushPendingInvokeTasks();

  SLNode* GetLayoutObject() const { return sl_node_.get(); }

  SLNode* slnode() const { return sl_node_.get(); }

  ElementManager* element_manager() const { return element_manager_; }
  Element* parent() const { return parent_; }
  Element* next_sibling() const { return Sibling(1); }
  Element* previous_sibling() const { return Sibling(-1); }
  virtual Element* Sibling(int offset) const;

  // only for fiber arch, indicate current real render tree hierarchy
  Element* render_parent() { return render_parent_; }
  Element* first_render_child() { return first_render_child_; }
  virtual Element* first_child() const;
  virtual Element* last_child() const;
  Element* next_render_sibling() { return next_render_sibling_; }

  const auto& children() const { return scoped_children_; }
  const auto& logical_children() const { return logical_children_; }

  // Helpers for finding non-virtual / non-wrapper nodes in the render tree
  // starting from the current element.
  Element* FindFirstNonVirtualRenderAncestor();
  Element* FindFirstNonVirtualRenderSibling();
  Element* FindFirstNonWrapperRenderAncestor();
  Element* FindFirstNonWrapperChildOrSibling();

  DirectionMapping CheckDirectionMapping(CSSPropertyID css_id);

  // CSS inheritance and direction-related helpers
  bool IsInheritable(CSSPropertyID id) const;
  bool IsDirectionChangedEnabled() const;
  std::pair<bool, CSSPropertyID> ConvertRtlCSSPropertyID(CSSPropertyID id);

  virtual ~Element();

  // For style op
  LYNX_EXPORT_FOR_DEVTOOL virtual void ConsumeStyle(
      const StyleMap& styles, const StyleMap* inherit_styles = nullptr);

  void CacheStyleFromAttributes(CSSPropertyID id, CSSValue&& value);
  void CacheStyleFromAttributes(CSSPropertyID id, const lepus::Value& value);
  void RemoveStyleFromAttributes(CSSPropertyID id) {
    if (styles_from_attributes_.has_value()) {
      styles_from_attributes_->erase(id);
      if (styles_from_attributes_->empty()) {
        styles_from_attributes_.reset();
      }
    }
    RemoveCommittedStyleFromAttributes(id);
  }
  const StyleMap* PeekCachedStylesFromAttributes() const {
    if (!styles_from_attributes_.has_value() ||
        styles_from_attributes_->empty()) {
      return nullptr;
    }
    return &*styles_from_attributes_;
  }
  virtual const StyleMap* PeekCommittedStylesFromAttributes() const {
    return nullptr;
  }
  void ClearCachedStylesFromAttributes() { styles_from_attributes_.reset(); }
  void DidConsumeStyle();

  void ParseRawInlineStyles(CSSVariableMap* changed_css_vars);
  LYNX_EXPORT_FOR_DEVTOOL void ProcessFullRawInlineStyle(
      CSSVariableMap* changed_css_vars);
  virtual void ConsumeStyleInternal(
      const StyleMap& styles, const StyleMap* inherit_styles,
      std::function<bool(CSSPropertyID, const tasm::CSSValue&)> should_skip);

  virtual void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value);

  void SetStyleObjects(
      std::unique_ptr<style::StyleObject*, style::StyleObjectArrayDeleter>
          object_list) override;

  void UpdateSimpleStyles(const tasm::StyleMap& style_map) override final;
  void UpdateSimpleStyles(tasm::StyleMap&& style_map) override final;
  void UpdateStaticAndDynamicSimpleStyles(
      tasm::StyleMap&& style_map,
      tasm::StyleMap&& dynamic_style_map) override final;
  void UpdateDynamicSimpleStyles(tasm::StyleMap&& style_map) override final;
  void ResetSimpleStyle(const tasm::CSSPropertyID id,
                        const tasm::CSSValue& value) override final;
  void ResetSimpleStyle(const tasm::CSSPropertyID id) override final;

  void ReplaceDynamicSimpleStyles(
      style::DynamicStyleObjectRef new_style_object);
  void AddDynamicSimpleStyles(tasm::StyleMap&& new_styles);
  void RemoveDynamicSimpleStyleKV(tasm::CSSPropertyID id);
  void AddDynamicSimpleStyleKV(tasm::CSSPropertyID id, tasm::CSSValue&& value);

  LYNX_EXPORT_FOR_DEVTOOL virtual void ResetStyle(
      const base::Vector<CSSPropertyID>& style_names);

  /**
   * Before SetAttribute(), reserve array size.
   */
  virtual void ReserveForAttribute(size_t count) {
    updated_attr_map_.reserve(count);
  }

  /**
   * Element API for appending single attribute to element
   * @param key the attribute String type name
   * @param value the attribute value
   */
  LYNX_EXPORT_FOR_DEVTOOL virtual void SetAttribute(
      const base::String& key, const lepus::Value& value,
      bool need_update_data_model = true);
  // Tracks attributes written by the Modifier receiver separately so its next
  // clear does not remove attributes owned by other Element APIs.
  void SetModifierAttribute(const base::String& key, const lepus::Value& value);
  void RemoveAllModifierAttributes();
  virtual void ResetAttribute(const base::String& key);
  void WillConsumeAttribute(const base::String& key, const lepus::Value& value);

  /**
   * Element API for setting class name to Element
   * @param clazz the name of class selector
   */
  void SetClass(const base::String& clazz);
  void OnClassChanged(const ClassList& old_classes,
                      const ClassList& new_classes);

  /**
   * Element API for setting class names to Element
   * @param classes the vector contains the name of class selector
   */
  LYNX_EXPORT_FOR_DEVTOOL void SetClasses(ClassList&& classes);

  /**
   * Element API for removing all classes of
   */
  LYNX_EXPORT_FOR_DEVTOOL void RemoveAllClass();

  virtual void SetBuiltinAttribute(ElementBuiltInAttributeEnum key,
                                   const lepus::Value& value);

  /**
   * Element API for setting id for element
   * @param idSelector the id of the element
   */
  LYNX_EXPORT_FOR_DEVTOOL void SetIdSelector(const base::String& idSelector);

  // For dataset op
  void SetDataSet(const tasm::DataMap& data);
  void AddDataset(const base::String& key, const lepus::Value& value);
  void RemoveDataset(const base::String& key);
  void SetDataset(const lepus::Value& data_set);
  // For event handler
  virtual void SetEventHandler(const base::String& name, EventHandler* handler);
  virtual void ResetEventHandlers();
  void ResetGestureDetectors();

  /**
   * Element API for adding js event
   * @param name the binding event's name
   * @param type the binding event's type
   * @param callback the binding event's corresponding js function name
   */
  void SetJSEventHandler(const base::String& name, const base::String& type,
                         const base::String& callback);

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
                              runtime::MTSRuntime* ctx);
  void SetWorkletEventHandler(const base::String& name,
                              const base::String& type,
                              const lepus::Value& worklet_info,
                              const std::string& context_name);

  event::DispatchEventResult DispatchMessageEvent(
      fml::RefPtr<runtime::MessageEvent> event);

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

  void FiberAddEvent(const base::String& type, const base::String& name,
                     const lepus::Value& callback,
                     const std::string& context_name,
                     runtime::MTSRuntime* lepus_context = nullptr,
                     const lepus::Value& lepus_script = lepus::Value());
  void FiberAddPiperEvent(
      const base::String& type, const base::String& name,
      std::vector<std::pair<base::String, lepus::Value>> piper_event_content);

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
  const lepus::Value config() const;

  // For gesture handler
  void SetGestureDetectorState(int32_t gesture_id, int32_t state);
  void SetGestureDetector(const uint32_t key, GestureDetectorImpl* detector);
  void ConsumeGesture(int32_t gesture_id, const lepus::Value& params);

  // For prop op
  void SetProp(const char* key, const lepus::Value& value);

  // For keyframe op
  // The first parameter names can be string type or array type of lepus value
  void SetKeyframesByNames(const lepus::Value& names,
                           const CSSKeyframesTokenMap&, bool force_flush);

  //  The first parameter names can be string type or array type of lepus value
  lepus::Value ResolveCSSKeyframesByNames(
      const lepus::Value& names, const tasm::CSSKeyframesTokenMap& frames,
      const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs, bool force_flush);

  // For font face
  void SetFontFaces(const CSSFontFaceRuleMap&);

  // For pseudo
  void ResetPseudoType(int pseudo_type) { pseudo_type_ = pseudo_type; }

  // For Animation API
  void Animate(const lepus::Value& args,
               std::shared_ptr<PipelineOptions>& pipeline_option);

  // For Animation API
  void AnimateV2(const lepus::Value& args,
                 std::shared_ptr<PipelineOptions>& pipeline_option);

  // For JS API setNativeProps
  virtual void SetNativeProps(
      const lepus::Value& args,
      std::shared_ptr<PipelineOptions>& pipeline_options);

  // Get List Node
  virtual ListNode* GetListNode() { return nullptr; }

  /**
   * A key function to get parent component's element
   */
  LYNX_EXPORT_FOR_DEVTOOL virtual Element* GetParentComponentElement() const;

  Catalyzer* GetCaCatalyzer() { return catalyzer_; }

  virtual const EventMap& event_map() const;
  virtual const EventMap& lepus_event_map();
  virtual const EventMap& global_bind_event_map();
  // GestureMap key - gesture id / value - GestureDetector
  virtual const GestureMap& gesture_map();

  virtual bool InComponent() const;
  virtual int ParentComponentId() const { return 0; }
  virtual std::string ParentComponentIdString() const;
  virtual const std::string& ParentComponentEntryName() const;
  const base::String& entry_name() const { return element_entry_name_; }
  void set_entry_name(const base::String& entry_name) {
    element_entry_name_ = entry_name;
  }

  inline bool IsLayoutOnly() { return is_layout_only_; }
  // Check if this element is a fixed element using the new fixed positioning
  // system
  bool IsNewFixed() const;
  // Get whether the new fixed positioning system is enabled
  bool GetEnableFixedNew() const;
  // Check if this element is a fixed element using the unified fixed behavior
  bool IsFixedUnified() const;
  // Get whether the unified fixed behavior is enabled, to be removed when
  // enable_unify_fixed_behavior is default enabled
  bool IsFixedUnifiedEnabled() const;
  // Check if either new fixed or unified fixed behavior is enabled, to be
  // removed when enable_unify_fixed_behavior is default enabled
  bool IsFixedNewOrUnifiedEnabled() const;
  // Check if this element uses either new fixed positioning or
  // unified fixed
  bool IsFixedNewOrUnified() const;
  // Check if this element uses unified fixed behavior but not new fixed
  // positioning
  bool IsFixedUnifiedOnly() const;

  inline bool is_virtual() { return is_virtual_; }
  LYNX_EXPORT_FOR_DEVTOOL virtual bool GetPageElementEnabled() { return false; }
  LYNX_EXPORT_FOR_DEVTOOL virtual bool GetRemoveCSSScopeEnabled() {
    return false;
  }

  bool IsRadonArch() const { return arch_type_ == RadonArch; }
  bool IsFiberArch() const { return arch_type_ == FiberArch; }

  // Element type checking methods
  virtual bool is_none() const { return false; }
  virtual bool is_block() const { return false; }
  virtual bool is_if() const { return false; }
  virtual bool is_for() const { return false; }
  bool is_inline_element() const { return is_inline_element_; }

  // Virtual parent node access methods (for AirModeFiber)
  void set_virtual_parent(Element* virtual_parent) {
    virtual_parent_ = virtual_parent;
  }
  Element* virtual_parent() { return virtual_parent_; }
  Element* root_virtual_parent();
  void AddScopedVirtualChild(const fml::RefPtr<Element>& child);
  void RemoveScopedVirtualChild(const fml::RefPtr<Element>& child);

  // Parent component unique ID access methods
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
      child->SetParentComponentUniqueIdRecursively(
          is_page() || is_component() ? impl_id() : id);
    }
  }
  /**
   * A function to resolve parent component element CSSFragment
   */
  void ResolveParentComponentElement() const;
  void ResolveParentComponentElementImpl() const;

  void ClearExtremeParsedStyles() {
    if (has_extreme_parsed_styles_) {
      extreme_parsed_styles_.reset();
      has_extreme_parsed_styles_ = false;
    }
  }

  // Exported for accessing private field from Element Manager to handle legacy
  // logic
  inline Element* GetRenderRootElement() { return render_root_element_; }
  void RecursivelyMarkRenderRootElement(Element* render_root);
  void UpdateRenderRootElementIfNecessary(Element* child);

  bool IsInSameCSSScope(Element* element) {
    return css_id_ == element->css_id_;
  }

  // This interface is currently only used by the inspector. The inspector
  // determines whether an element is created by the itself by checking whether
  // element has a data model. Since the data model of a fiber element is not
  // empty by default, this interface is provided to the inspector to reset the
  // data model and mark the element as created by the inspector.
  void ResetDataModel() { data_model_ = nullptr; }

  void MarkCanBeLayoutOnly(bool flag) { can_be_layout_only_ = flag; }

  // Async resolve status query methods
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

  bool flush_required() { return flush_required_; }

  inline bool ShouldProcessParallelTasks() {
    return is_parallel_flush() ||
           resolve_status_ == AsyncResolveStatus::kSyncResolving;
  }

  inline bool ShouldResolveStyle() {
    return !IsAsyncResolveResolving() &&
           ((dirty_ & ~(kDirtyTree | kDirtyReAttachContainer)) != 0);
  }

  inline void EnqueueReduceTask(base::MoveOnlyClosure<void> operation) {
    parallel_reduce_tasks_->emplace_back(std::move(operation));
  }

  inline bool IsAsyncFlushRoot() const { return is_async_flush_root_; }
  inline void MarkAsyncFlushRoot(bool value) { is_async_flush_root_ = value; }
  void AsyncResolveSubtreeProperty();

  // Data model accessor methods
  const ClassList& classes() { return data_model_->classes(); }

  ClassList ReleaseClasses() { return data_model_->ReleaseClasses(); }

  const base::String& GetIdSelector() { return data_model_->idSelector(); }

  const DataMap& dataset() { return data_model_->dataset(); }

  // Check has_value() before usage to avoid unintentional construction.
  const auto& builtin_attr_map() const { return builtin_attr_map_; }

  // Check has_value() before usage to avoid unintentional construction.
  const auto& updated_attr_map() const { return updated_attr_map_; }

  void set_style_sheet_manager(
      const std::shared_ptr<CSSStyleSheetManager>& manager) {
    css_style_sheet_manager_ = manager;
  }

  const std::shared_ptr<CSSStyleSheetManager>& style_sheet_manager() {
    return css_style_sheet_manager_;
  }

  void ResetStyleSheet();

  void ResetSheetRecursively(
      const std::shared_ptr<CSSStyleSheetManager>& manager);

  void set_attached_to_layout_parent(bool has) {
    attached_to_layout_parent_ = has;
  }
  bool attached_to_layout_parent() const { return attached_to_layout_parent_; }

  void UpdateAttrMap(const base::String& key, const lepus::Value& value) {
    updated_attr_map_[key] = value;
  }

  void MarkAttrDirtyForPseudoElement() { dirty_ |= kDirtyAttr; }

  void UpdateLayout(float left, float top, float width, float height,
                    const std::array<float, 4>& paddings,
                    const std::array<float, 4>& margins,
                    const std::array<float, 4>& borders,
                    const std::array<float, 4>* sticky_positions,
                    float max_height, bool display_none = false);
  // Used to update child element's left and top value from list element. The
  // another overloaded function is used to update layout info from starlight,
  // but if the element is list's child, the left and top's value are always 0.
  void UpdateLayout(float left, float top);

  virtual Element* GetChildAt(size_t index);

  virtual size_t GetChildCount();

  virtual ElementChildrenArray GetChildren();

  Element* enclosing_none_wrapper() const { return enclosing_none_wrapper_; }
  void set_enclosing_none_wrapper(Element* wrapper) {
    enclosing_none_wrapper_ = wrapper;
  }
  uint32_t wrapper_element_count() const { return wrapper_element_count_; }
  void IncrementWrapperElementCount() { wrapper_element_count_++; }
  void DecrementWrapperElementCount() { wrapper_element_count_--; }

  virtual size_t GetUIIndexForChild(Element* child) { return 0; }

  virtual int32_t IndexOf(const Element* child) const;

  virtual void InsertNode(const fml::RefPtr<Element>& child);
  virtual void InsertNode(const fml::RefPtr<Element>& child, int32_t index);
  virtual void InsertNodeBefore(const fml::RefPtr<Element>& child,
                                const fml::RefPtr<Element>& reference_child);
  virtual void InsertNodeBeforeInternal(const fml::RefPtr<Element>& child,
                                        Element* ref_node);
  virtual void InsertNodeBeforeInternal(const fml::RefPtr<Element>& child,
                                        Element* ref_node,
                                        bool update_logical_children);
  // Reparents a rendered child from a distinct parent to this Element. The
  // caller guarantees that child and index belong to the same valid scoped
  // tree. The target retains the render source until the deferred platform
  // remove-and-insert operations have been generated.
  void MoveNodeToIndex(const fml::RefPtr<Element>& child, int32_t index);
  virtual void ReplaceElements(
      const base::Vector<fml::RefPtr<Element>>& inserted,
      const base::Vector<fml::RefPtr<Element>>& removed,
      Element* ref_node = nullptr);
  virtual void RemoveNode(const fml::RefPtr<Element>& child,
                          bool destroy = true);
  void RemoveLogicalChild(const fml::RefPtr<Element>& child);
  virtual void RemoveNodeInternal(const fml::RefPtr<Element>& child,
                                  bool destroy, bool update_logical_children);
  virtual void InsertedInto(Element* insertion_point);
  virtual void RemovedFrom(Element* insertion_point);
  void HandleInsertChildAction(Element* child, int index, Element* ref_node);
  void HandleRemoveChildAction(Element* child);
  void HandleRemoveSelf(Element* removal_point, Element* render_parent);
  void InsertFixedElement(Element* child, Element* ref_node);
  void RemoveFixedElement(Element* child);
  virtual void AddChildAt(fml::RefPtr<Element> child, int index);
  void StoreLayoutNode(Element* child, Element* ref);
  void RestoreLayoutNode(Element* child);
  void InsertLayoutNode(Element* child, Element* ref);
  void RemoveLayoutNode(Element* child,
                        int layout_in_element_platform_index = -1);
  bool HasLayoutInElementPlatformNode();

  inline bool CanHasLayoutOnlyChildren() {
    return can_has_layout_only_children_;
  };

  void OnNodeReady();

  void onNodeReload();

  /*
   * return the font size from platform_css_style_.
   */
  LYNX_EXPORT_FOR_DEVTOOL virtual double GetFontSize();

  /**
   * Special API for processing font-size.
   * Font-size should be handled at the beginning of style application.
   */
  void SetFontSize(const tasm::CSSValue& value);

  /**
   * Sets font-size on a specific target ComputedCSSStyle rather than the
   * element's own platform style.
   */
  void SetFontSize(const tasm::CSSValue& value,
                   starlight::ComputedCSSStyle* target_style);

  void ResetFontSize();

  /*
   * return the font size of parent.
   */
  double GetParentFontSize();

  /*
   * return the root font size from platform_css_style_.
   */
  double GetRecordedRootFontSize();

  /*
   * return the value of the root element's GetFontSize() function.
   */
  double GetCurrentRootFontSize();

  virtual void OnPseudoStatusChanged(PseudoState prev_status,
                                     PseudoState current_status);

  ContentData* content_data() const { return content_data_.get(); }

  virtual void UpdateDynamicElementStyle(uint32_t style, bool force_update);

  bool HasPlaceHolder() { return has_placeholder_; }
  bool HasTextSelection() { return has_text_selection_; }

  PaintingContext* painting_context();

  // Declared platform node tag
  const base::String& GetTag() const { return tag_; }
  bool IsOverlay() const { return is_overlay_; }

  // The actual platform node tag, which is typically the same as the declared
  // platform node tag, except in one case:
  // - In a list, the actual platform node tag can be specified using the
  // custom-list-name attribute.
  virtual const base::String& GetPlatformNodeTag() const { return tag_; }

  void UpdateElement();

  int ZIndex() {
    return GetEnableZIndex() ? computed_css_style()->GetZIndex() : 0;
  }

  bool HasElementContainer() { return element_container_ != nullptr; }

  bool IsStackingContextNode();

  bool IsCSSInheritanceEnabled() const;

  bool IsCSSInlineVariablesEnabled() const;
  bool IsRelatedCSSVariableUpdated(AttributeHolder* holder,
                                   const lepus::Value changing_css_variables);

  BaseElementContainer* element_container() const {
    return element_container_.get();
  }

  ElementContainer* element_container_impl();
  Fragment* fragment_impl();

  void CreateElementContainer(bool platform_is_flatten);

  virtual void EnqueueLayoutTask(base::MoveOnlyClosure<void> operation);

  virtual void HandleDelayTask(base::MoveOnlyClosure<void> operation);
  void set_parent(Element* parent) { parent_ = parent; }
  bool EnableTriggerGlobalEvent() const { return trigger_global_event_; }

  void PreparePropBundleIfNeed();
  fml::RefPtr<PropBundle> GetPropBundleForRecording();

  bool GetEnableZIndex();

  virtual void MarkLayoutDirty();

  virtual void MarkLayoutDirtyLite();

  // Dirty flag primitives
  int32_t dirty() const { return dirty_; }

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

  void MarkRefreshCSSStyles() { MarkDirty(kDirtyRefreshCSSVariables); }

  bool CollectCustomProperties(AttributeHolder* holder);

  // In RadonDiff Mode, worklets require the following two APIs. In RL3.0 or
  // TTML NoDiff, the implementation of worklets no longer relies on these
  // capabilities. After the 2.0 worklet services are phased out, the following
  // two APIs will also be removed.
  virtual StyleMap GetStylesForWorklet();
  virtual const AttrMap& GetAttributesForWorklet();

  inline const auto& GlobalBindTarget() { return global_bind_target_set_; }
  virtual bool CanBeLayoutOnly() const;

  LYNX_EXPORT_FOR_DEVTOOL bool HasUIPrimitive() const;

  virtual void CheckHasInlineContainer(Element* parent);

  //{need_request_layout,has_pending_bundle}
  std::tuple<bool, bool> FlushAnimatedStyle();
  void FlushAnimatedStyle(tasm::CSSPropertyID id, tasm::CSSValue value);
  virtual void FlushAnimatedStyleInternal(tasm::CSSPropertyID id,
                                          const tasm::CSSValue& value);

  starlight::LayoutResultForRendering layout_result();

  float width() { return width_; }
  float height() { return height_; }
  float top() { return top_; }
  float left() { return left_; }
  bool display_none() { return display_none_; }

  bool enable_new_animator() { return enable_new_animator_; }
  bool supports_platform_animation_routing() const;
  bool use_cpp_animation_builder() const;

  PropertiesResolvingStatus GenerateRootPropertyStatus() const;

  void SetComputedFontSize(double font_size, double root_font_size);

  void SetPlaceHolderStylesInternal(const PseudoPlaceHolderStyles& styles);

  void ResetStyleInternal(CSSPropertyID id);
  void SetDirectionInternal(const tasm::CSSValue& value) {
    SetStyleInternal(kPropertyIDDirection, value);
  }

  inline const std::array<float, 4>& borders() { return borders_; }
  inline const std::array<float, 4>& paddings() { return paddings_; }
  inline const std::array<float, 4>& margins() { return margins_; }
  inline float max_height() { return max_height_; }
  inline bool need_update() { return subtree_need_update_; }
  inline bool frame_changed() { return frame_changed_; }
  inline void MarkUpdated() {
    subtree_need_update_ = false;
    frame_changed_ = false;
  }
  inline void MarkFrameChanged() { frame_changed_ = true; }

  inline void set_config_flatten(bool value) { config_flatten_ = value; }

  void MarkSubtreeNeedUpdate();
  std::pair<CSSValuePattern, CSSValuePattern>
  ConvertDynamicStyleFlagToCSSValuePattern(uint32_t style);
  void NotifyElementSizeUpdated();
  void NotifyUnitValuesUpdatedToAnimation(uint32_t style);

  inline void set_is_layout_only(bool is_layout_only) {
    is_layout_only_ = is_layout_only;
  }

  // APIs related to Sticky
  inline bool is_sticky() { return is_sticky_; }
  inline void set_is_sticky(bool is_sticky) { is_sticky_ = is_sticky; }

  inline const base::auto_create_optional<std::array<float, 10>>&
  sticky_positions() const {
    return sticky_positions_;
  }

  // Check has_value() before usage to avoid unintentional construction.
  inline const auto& keyframes_map() { return keyframes_map_; }

  bool ShouldAvoidFlattenForView();

  bool TendToFlatten();

  bool NeedCreateNodeAsync() { return create_node_async_; }

  bool HasPaintingNode() { return has_painting_node_; }

  // Platform animations need a concrete platform rendering target. This is
  // separate from HasPaintingNode(), which is also true for layout-only nodes
  // whose ElementContainer intentionally skipped native node creation.
  bool PrepareForPlatformAnimation();

  bool IsNewlyCreated() const { return dirty_ & kDirtyCreated; }

  void MarkAsInline() {
    is_inline_element_ = true;
    has_layout_only_props_ = false;
  }

  // The text element can call this function to convert child fiber elements
  // into inline elements. Currently, only view, text, image and wrapper
  // elements may be converted into inline elements.
  virtual void ConvertToInlineElement();

  void ResetPropBundle();

  void CheckFlattenRelatedProp(const base::String& key,
                               const lepus::Value& value = lepus::Value(true));

  void CheckHasPlaceholder(const base::String& key,
                           const lepus::Value& value = lepus::Value(true));
  void CheckHasTextSelection(const base::String& key,
                             const lepus::Value& value = lepus::Value(true));

  void CheckTriggerGlobalEvent(const lynx::base::String& key,
                               const lynx::lepus::Value& value);
  void CheckClassChangeTransmitAttribute(const base::String& key,
                                         const lepus::Value& value);
  void CheckGlobalBindTarget(
      const lynx::base::String& key,
      const lynx::lepus::Value& value = lepus::Value(base::String()));
  void CheckTimingAttribute(const lynx::base::String& key,
                            const lynx::lepus::Value& value);
  void CheckNewAnimatorAttr(const base::String& key, const lepus::Value& value);

  // return true indicates current style is transtion related
  bool CheckTransitionProps(CSSPropertyID id);
  // return true indicates current style is keyframe related
  bool CheckKeyframeProps(CSSPropertyID id);

  void CheckHasNonFlattenCSSProps(CSSPropertyID id);
  void CheckFixedSticky(CSSPropertyID id, const tasm::CSSValue& value);

  bool DisableFlattenWithOpacity();

  inline starlight::ComputedCSSStyle* computed_css_style() {
    return platform_css_style_.get();
  }
  inline const starlight::ComputedCSSStyle* computed_css_style() const {
    return platform_css_style_.get();
  }

  /**
   * @brief Returns the base computed CSS style (before animation sampling).
   */
  inline starlight::ComputedCSSStyle* base_css_style() {
    return base_css_style_.get();
  }
  inline const starlight::ComputedCSSStyle* base_css_style() const {
    return base_css_style_.get();
  }

  starlight::ComputedCSSStyle* GetParentComputedCSSStyle();

  /**
   * @brief Returns the parent's base computed CSS style, skipping wrapper
   * nodes.
   */
  starlight::ComputedCSSStyle* GetParentBaseComputedCSSStyle();

  void SetDataToNativeKeyframeAnimator(bool from_resume = false);
  void SetDataToNativeTransitionAnimator();
  bool ShouldUseLegacyTransitionInterception() const;

  bool ShouldConsumeTransitionStylesInAdvance();
  bool ConsumeTransitionStylesInAdvance(const StyleMap& styles,
                                        bool force_reset = false);

  virtual void MarkAsLayoutRoot();
  virtual void AttachLayoutNode(const fml::RefPtr<PropBundle>& props);
  virtual void UpdateLayoutNodeProps(const fml::RefPtr<PropBundle>& props);
  virtual void UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                     const tasm::CSSValue& value);
  virtual void ResetLayoutNodeStyle(tasm::CSSPropertyID css_id);
  virtual void UpdateLayoutNodeFontSize(double cur_node_font_size,
                                        double root_node_font_size);
  virtual void UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                         const lepus::Value& value);

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

  virtual bool ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value);

  virtual void CheckDynamicUnit(CSSPropertyID id, const CSSValue& value,
                                bool reset);

  bool EnableLayoutInElementMode() const;

  // Returns true if the property should be written to ComputedCSSStyle's
  // value map (not just resolved values). Layout-only properties are skipped
  // unless the element runs in layout-in-element mode.
  bool ShouldWritePropertyToComputedStyle(CSSPropertyID id) const {
    return !LayoutProperty::IsLayoutOnly(id) || EnableLayoutInElementMode();
  }

  void EnsureLayoutBundle();

  void InitLayoutBundle();

  void UpdateTagToLayoutBundle();

  void UpdateLayoutNodeByBundle();

  void MarkCustomPropertiesDirty() { custom_properties_ = nullptr; }

  const StyleMap& GetParsedStylesMap() const { return parsed_styles_map_; }

  bool UsingTextService() const;

  bool EnableFragmentLayerRender() const;

  virtual void WillResetCSSValue(CSSPropertyID& id);

  virtual bool ResetCSSValue(CSSPropertyID id);
  virtual void ConsumeTransitionStylesInAdvanceInternal(
      CSSPropertyID css_id, const tasm::CSSValue& value) {
    SetStyleInternal(css_id, value);
  }
  bool ResetTransitionStylesInAdvance(
      const base::Vector<CSSPropertyID>& css_names);
  virtual void ResetTransitionStylesInAdvanceInternal(CSSPropertyID css_id) {
    ResetStyleInternal(css_id);
  }

  void ResolveAndFlushKeyframes();

  void RecordElementPreviousStyle(CSSPropertyID css_id,
                                  const tasm::CSSValue& value);
  void ResetElementPreviousStyle(CSSPropertyID css_id);

  std::optional<CSSValue> GetElementPreviousStyle(tasm::CSSPropertyID css_id);

  virtual std::optional<CSSValue> GetElementStyle(tasm::CSSPropertyID css_id);

  CSSKeyframesToken* GetCSSKeyframesToken(const base::String& animation_name);

  virtual CSSFragment* GetRelatedCSSFragment();

  virtual void FlushProps();

  void DispatchLayoutBeforeRecursively();
  virtual void DispatchLayoutBefore();

  virtual void set_will_destroy(bool destroy);

  bool will_destroy() { return will_destroy_; }

  bool ShouldDestroy() const { return !will_destroy_ && element_manager(); }

  virtual void TickElement(fml::TimePoint& time) {}

  bool TickAllAnimation(fml::TimePoint& time,
                        std::shared_ptr<PipelineOptions>& options);

  void ClearTransitionPreviousEndValue(const base::String&);

  virtual void RequestLayout();

  virtual void RequestNextFrame();

  void SetAnimationSampleTimeForNewPipeline(const fml::TimePoint& sample_time);
  base::flex_optional<fml::TimePoint> TakeAnimationSampleTimeForNewPipeline();
  void QueuePlatformAnimationCommand(gfx::PlatformAnimationCommand command);
  bool HasPendingPlatformAnimationCommands() const {
    return platform_animation_commands_ != nullptr &&
           !platform_animation_commands_->empty();
  }
  void DispatchAnimationEventsForNewPipeline(
      const animation::AnimationEventRecordsForNewPipeline& event_records);
  void UpdateFinalStyleMap(const StyleMap& styles);
  virtual void MarkLayoutInElementTextMeasurerPropertyIfNeeded(
      CSSPropertyID id) {}
  // Hook for subclasses to replay element-specific derived style state.
  // Callers should go through ReplayChangedStyleSideEffect() or
  // ReplayResetStyleSideEffect() so Element preserves replay bookkeeping.
  virtual void ReplayElementSpecificStyleSideEffect(CSSPropertyID id) {}

  virtual void OnPatchFinish(std::shared_ptr<PipelineOptions>& option);

  virtual int32_t GetCSSID() const;

  virtual void SetCSSID(int32_t id);

  bool IsExtendedLayoutOnlyProps(CSSPropertyID css_id);

  // This method should be overridden by the subclass to establish whether the
  // attribute needs updating at the platform. If update is unnecessary, will
  // return false.
  virtual bool OnAttributeSet(const base::String& key,
                              const lepus::Value& value) {
    return true;
  }

  // TODO(dingwang.wxx): Interfaces in Element about list should be moved to
  // ListElement after unifying the RadonElement and FiberElement.
  // When the list element changes, this method will be invoked. For example, if
  // the list's width or height changes, or if the List itself has new diff
  // information.
  virtual void OnListElementUpdated(
      const std::shared_ptr<PipelineOptions>& options) {}

  // Called when the layout object is created
  virtual void OnLayoutObjectCreated() {}

  // When the rendering of the list's child node is complete, this method will
  // be invoked. In this method, we can accurately obtain the layout
  // information of the child node.
  virtual void OnComponentFinished(
      Element* component, const std::shared_ptr<PipelineOptions>& option) {}

  // This method is override by ListElement or RadonListElement, which used to
  // notify list that the list item's layout info has been updated.
  virtual void OnListItemLayoutUpdated(Element* component) {}

  // When the batch rendering of the list's child nodes are complete, this
  // method will be invoked. In this method, we can accurately obtain the layout
  // information of the child nodes
  virtual void OnListItemBatchFinished(
      const std::shared_ptr<PipelineOptions>& options) {}

  // Send drag distance to list element.
  virtual void ScrollByListContainer(float content_offset_x,
                                     float content_offset_y, float original_x,
                                     float original_y) {}

  // Implement list's ScrollToPosition ui method.
  virtual void ScrollToPosition(int index, float offset, int align,
                                bool smooth) {}

  // Finish ScrollToPosition.
  virtual void ScrollStopped() {}

  // Whether list uses platform component.
  virtual bool DisableListPlatformImplementation() const { return false; }

  virtual const InheritedProperty GetInheritedProperty();
  const InheritedProperty GetParentInheritedProperty();

  virtual bool NeedFullFlushPath(CSSPropertyID id, const CSSValue& value);

  virtual bool is_view() const { return false; }

  virtual bool is_image() const { return false; }

  virtual bool is_text() const { return false; }

  virtual bool is_list() const { return false; }

  virtual bool is_template() const { return false; }

  virtual bool is_wrapper() const { return false; }

  virtual bool is_modifier() const { return false; }

  virtual bool is_component() const { return false; }

  virtual bool is_scroll_view() const { return false; }

  virtual bool is_raw_text() const { return false; }

  virtual void MarkAsListItem() { is_list_item_ = true; }

  void CreateListItemScheduler(list::BatchRenderStrategy batch_render_strategy,
                               bool continuous_resolve_tree);
  ListItemSchedulerAdapter* GetSchedulerAdapter() {
    if (scheduler_adapter_) {
      return scheduler_adapter_.get();
    }
    return nullptr;
  }

  void MarkAsDirectChildOfCompatibleComponent(bool flag) {
    is_direct_child_of_compatible_component_ = flag;
  }

  bool is_direct_child_of_compatible_component() const {
    return is_direct_child_of_compatible_component_;
  }

  virtual int32_t GetBuiltInNodeInfo() const { return 0; }

  bool is_list_item() const { return is_list_item_; }

  void EnsureTagInfo();

  bool IsShadowNodeVirtual() {
    EnsureTagInfo();
    return layout_node_type_ & LayoutNodeType::VIRTUAL;
  }

  bool IsShadowNodeCustom() {
    EnsureTagInfo();
    return layout_node_type_ & LayoutNodeType::CUSTOM;
  }

  // If element not CanBeLayoutOnly, call this function to create LynxUI.
  void TransitionToNativeView();
  void SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func);
  void SetMeasureFunc(void* context, starlight::SLMeasureFunc measure_func);
  void SetAlignmentFunc(void* context,
                        starlight::SLAlignmentFunc alignment_func);

  // When list component finishes all props update
  virtual void PropsUpdateFinish() {}

  void PushToBundle(CSSPropertyID id);

  bool has_z_props() const { return computed_css_style()->HasZIndex(); }

  // For devtool
  ALLOW_UNUSED_TYPE void set_inspector_attribute(
      std::unique_ptr<InspectorAttribute> ptr) {
    inspector_attribute_ = std::move(ptr);
  }

  ALLOW_UNUSED_TYPE InspectorAttribute* inspector_attribute() {
    return inspector_attribute_.get();
  }

  void ResolveStyle(StyleMap& new_styles,
                    CSSVariableMap* changed_css_vars = nullptr);

  void HandlePseudoElement();
  void PrepareOrUpdatePseudoElement(PseudoState state, StyleMap& style_map);

  void HandleCSSVariables(StyleMap& styles);

  // Callback before style resolving. Return false to skip style resolving.
  virtual bool WillResolveStyle(StyleMap& merged_styles,
                                CSSVariableMap* changed_css_vars) {
    ProcessFullRawInlineStyle(changed_css_vars);
    return true;
  }

  // Style resolver will firstly call `CountInlineStyles` to get count
  // of inline styles to be merged. Then it will merge styles from CSS
  // selectors and finally calls `MergeInlineStyles` to merge the inline
  // styles.
  size_t CountInlineStyles() {
    return current_raw_inline_styles_.has_value()
               ? CSSProperty::GetTotalParsedStyleCountFromMap(
                     *current_raw_inline_styles_)
               : 0;
  }

  // Check has_value() before usage to avoid unintentional construction.
  const auto& GetCurrentRawInlineStyles() const {
    return current_raw_inline_styles_;
  }

  // Check has_value() before usage to avoid unintentional construction.
  const auto& GetCurrentRawImportantInlineStyles() const {
    return current_raw_important_inline_styles_;
  }

  /**
   * @brief Returns the current raw inline custom properties (CSS variables).
   */
  const auto& GetCurrentRawInlineCustomProperties() const {
    return current_raw_inline_custom_properties_;
  }

  LYNX_EXPORT_FOR_DEVTOOL const base::String& GetRawInlineStyles();
  LYNX_EXPORT_FOR_DEVTOOL void SetRawInlineStyles(base::String value);

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
  // The caller batches the pipeline flush after updating all matched elements.
  LYNX_EXPORT_FOR_DEVTOOL void UpdateMatchedCSSVariablesForDevTool(
      CSSVariableMap variables);

  /**
   * Element API for removing all inline styles.
   */
  LYNX_EXPORT_FOR_DEVTOOL void RemoveAllInlineStyles();

  /**
   * Element API for removing all important inline styles.
   */
  LYNX_EXPORT_FOR_DEVTOOL void RemoveAllImportantInlineStyles();

  /**
   * Element API for setting compile stage parsed style
   * @param parsed_styles the parsed styles
   * @param config parsed styles' config
   */
  void SetParsedStyles(const ParsedStyles& parsed_styles,
                       const lepus::Value& config);

  void SetParsedStyles(StyleMap&& parsed_styles, CSSVariableMap&& css_var);

  /**
   * Element API for adding gesture detector
   */
  void SetGestureDetector(const uint32_t gesture_id,
                          GestureDetectorImpl gesture_detector);

  /**
   * Element API for removing specific gesture detector
   * @param gesture_id the removed gesture' id
   */
  void RemoveGestureDetector(const uint32_t gesture_id);

  // Returns true if CSS variables were merged and need to be resolved.
  bool MergeInlineStyles(StyleMap& merged_styles, StyleMap& important_styles);
  void PersistAnimationFillStyles(const StyleMap& styles);
  void ClearPersistedAnimationFillStyle(CSSPropertyID id);
  virtual int32_t GetMemoryUsage() const { return sizeof(*this); }

  virtual bool is_page() const { return false; }

  virtual bool NeedProcessDirection() {
    EnsureTagInfo();
    return need_process_direction_;
  }

  EventTarget* GetParentTarget() override { return parent_; }

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

  bool IsEventPathSkip(event::EventTarget* target,
                       event::Event* event) override;

  virtual void HandleGlobalEvent(fml::RefPtr<event::Event> event) override;

  virtual lepus::Value GetEventTargetInfo(bool is_core_event = false) override;

  virtual lepus::Value GetEventControlInfo(bool is_core_event = false) override;

  virtual bool GetEnableMultiTouchParamsCompatible() override;

  virtual float GetLayoutsUnitPerPx() override;

  // Updates the latest platform scroll position in layout units. The value is
  // consumed on the engine thread by Element point conversion.
  void UpdateScrollOffset(float x, float y);
  float scroll_offset_x() const { return scroll_offset_x_; }
  float scroll_offset_y() const { return scroll_offset_y_; }

  // Updates the latest platform-applied sticky translation in layout units.
  // The value is consumed on the engine thread by Element point conversion.
  void UpdateStickyTranslation(float x, float y);
  float sticky_translation_x() const { return sticky_translation_x_; }
  float sticky_translation_y() const { return sticky_translation_y_; }

  /**
   * Get computed style value by property key.
   * @param key the CSS property name
   * @return the computed style value as lepus::Value
   */
  lepus::Value GetComputedStyleByKey(const base::String& key);

  fml::WeakPtr<EventTarget> GetWeakTarget() override { return WeakFromThis(); }

  virtual std::string GetUniqueID() override {
    return std::to_string(impl_id());
  }

  virtual void MarkAttached() { state_ = State::kAttached; }
  virtual bool IsAttached() const { return state_ == State::kAttached; }

  virtual void MarkDetached() { state_ = State::kDetached; }
  virtual bool IsDetached() const { return state_ == State::kDetached; }
  virtual void SetupFragmentBehavior(Fragment* fragment);

  void SetDefaultOverflow(bool visible);

  /**
   * Destroy the related platform node of this element
   */
  void DestroyPlatformNode();
  virtual void MarkPlatformNodeDestroyed();

  // Mark style dirty, optionally recursively for children
  LYNX_EXPORT_FOR_DEVTOOL void MarkStyleDirty(bool recursive = false);
  void RecursivelyMarkChildrenCSSVariableDirty(
      const lepus::Value& css_variable_updated);

#if ENABLE_TRACE_PERFETTO
  virtual void UpdateTraceDebugInfo(TraceEvent* event);
#endif

  template <typename F>
  void ApplyFunctionRecursive(F&& func) {
    func(this);
    for (const auto& child : scoped_children_) {
      child->ApplyFunctionRecursive(func);
    }
  }

  void MarkTemplateElement() { is_template_ = true; }

  void MarkPartElement(base::String&& part_id) {
    part_id_ = std::move(part_id);
  }

  bool IsTemplateElement() const { return is_template_; }

  void SetTemplateAttributes(
      const SharedTemplateAttributes& template_attributes) {
    template_attributes_ = template_attributes;
  }
  const SharedTemplateAttributes& template_attributes() const {
    return template_attributes_;
  }
  bool HasTemplateAttributes() const { return template_attributes_ != nullptr; }

  bool IsPartElement() const { return !part_id_.empty(); }

  const base::String& GetPartID() const { return part_id_; }

  void LogNodeInfo();

  /**
   * A key function to flush the tree with the current element as the root node.
   */
  virtual void FlushActionsAsRoot();

  /**
   * A key function for flush all pending actions for current Element.
   */
  virtual void FlushActions();
  virtual void FlushSelf();

  /**
   * Prepare or update the platform representation and return deferred work.
   */
  virtual ParallelFlushReturn PrepareForCreateOrUpdate();

  virtual void PrepareChildren();

  /**
   * Recursively schedule or perform parallel flush work for this subtree.
   */
  virtual void ParallelFlushRecursively();

  /**
   * Recursively insert pending fixed elements for this subtree.
   */
  virtual void TraversalInsertFixedElementOfTree();

  /**
   * Check if this element needs to propagate inherited dirty flag to children.
   * @param force_propagate whether to force propagation
   * @return true if propagation is needed
   */
  bool NeedPropagateInheritedDirtyFlag(bool force_propagate);

  void InvalidateChildrenIfNeeded();

  /**
   * Check if the CSS fragment has id selector map.
   * @return true if id selector map exists
   */
  bool CheckHasIdMapInCSSFragment();

  /**
   * Handle task that needs to be executed before flush actions.
   * @param operation the operation to execute
   * @param predicate_parallel_flush_flag flag to check for parallel flush
   */
  void HandleBeforeFlushActionsTask(base::MoveOnlyClosure<void> operation,
                                    int32_t predicate_parallel_flush_flag);

  /**
   * Verify that keyframe property changes have been properly handled.
   * Throws DCHECK in debug mode if keyframe_props_changed_ is still set.
   */
  void VerifyKeyframePropsChangedHandling();

  /**
   * Check if this element needs to update layout info.
   * @return true if layout info needs update
   */
  bool IfNeedsUpdateLayoutInfo();
  void UpdateLayoutInfoRecursively(PipelineOptions* options);
  void UpdateLayoutInfo();

  virtual fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const {
    return fml::AdoptRef<Element>(new Element(*this, clone_resolved_props));
  }

  // The element object created using clone interfaces is not attached to the
  // element manager. Use this function to attach it to the element manager.
  virtual void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id);

  void AsyncResolveProperty();
  void DispatchAsyncResolveSubtreeProperty();
  void DispatchAsyncResolveProperty();
  void AsyncPostResolveTaskToThreadPool();
  virtual void PostResolveTaskToThreadPool(bool is_engine_thread,
                                           ParallelReduceTaskQueue& task_queue);

  void PrepareSelfForThreadedElementResolution();

  struct PerfStatistic {
    PerfStatistic(uint32_t total_task_count)
        : total_task_count_(total_task_count) {}

    bool enable_report_stats_{false};
    uint32_t engine_thread_task_count_{0};
    uint32_t total_task_count_{0};
    uint64_t total_processing_start_{0};
    uint64_t total_waiting_time_{0};
  };

  void PrepareChildForInsertion(Element* child);
  virtual void ParallelFlushAsRoot();
  void DidParallelFlushAsRoot(PerfStatistic& stats);
  void OnParallelFlushAsRoot(PerfStatistic& stats);
  void PrepareAndGenerateChildrenActions();
  void ResolveCSSStyles(StyleMap& parsed_styles,
                        base::InlineVector<CSSPropertyID, 16>& reset_style_ids,
                        bool& need_update,
                        bool& force_use_current_parsed_style_map);
  bool ShouldFallbackToSerialForNewStylingPipeline() const;
  bool HasAdjacentSiblingRulesInStyleSheets();

  // For snapshot test
  void DumpStyle(StyleMap& parsed_styles);

 protected:
  Element(const Element&, bool clone_resolved_props);

  virtual void PushStyleToBundle();
  void PushCurrentPropsToBundleForRecording(PropBundle* bundle);

  void PerformElementContainerCreateOrUpdate(bool need_update, bool need_reset);
  void UpdateFiberElement();

  void HandleSelfFixedChange();
  virtual void OnNodeAdded(Element* child);
  virtual void OnNodeRemoved(Element* child);

  void UpdateNodeInfo(int32_t node_info);
  // Returns whether consuming the attribute should update the element.
  virtual bool SetAttributeInternal(const base::String& key,
                                    const lepus::Value& value);
  virtual void MarkHasLayoutOnlyPropsIfNecessary(
      const base::String& attribute_key);
  bool ConsumeAllAttributes();
  ParallelFlushReturn CreateParallelTaskHandler();

  Element* FindEnclosingNoneWrapper(Element* parent, Element* node);
  void HandleContainerInsertion(Element* parent, Element* child, Element* ref);
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

  void RequireFlush();

  const tasm::CSSValue& ResolveCurrentStyleValue(
      const CSSPropertyID& key, const tasm::CSSValue& default_value);

  void ApplySimpleStyleWithoutTail(const tasm::CSSPropertyID id,
                                   const tasm::CSSValue& value);
  void ApplySimpleStylesWithoutTail(const tasm::StyleMap& style_map);
  void ApplyDynamicSimpleStylesWithoutTail(
      const tasm::StyleMap& dynamic_style_map,
      const tasm::StyleMap& base_style_map);
  void MarkSimpleStyleDirty(uint32_t dirty_bits);

  void HandleKeyframePropsChange();
  void FinalizeSimpleStyleUpdate();
  void ResolveSimpleStyles();
  void DoFullCSSResolving();
  bool RefreshStyle(StyleMap& parsed_styles,
                    base::Vector<CSSPropertyID>& reset_ids,
                    bool force_use_parsed_styles_map = false);
  DynamicCSSStylesManager::StyleUpdateFlags CollectDynamicFlagsForNewPipeline(
      const StyleMap& resolved_style_map) const;
  bool ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
      CSSPropertyID id, const CSSIDBitset& source_style_ids);
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

  struct AnimationSampleAnalysisForNewPipeline {
    bool has_style_effects{false};
    bool has_animated_font_size{false};
    bool has_custom_property_effects{false};
    bool changes_resolve_context{false};
  };
  static AnimationSampleAnalysisForNewPipeline
  AnalyzeAnimationSampleForNewPipeline(
      const animation::AnimationSampleForNewPipeline& animation_sample);

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
  NewPipelineDynamicStyleInputs BuildDynamicStyleInputsForNewPipeline(
      const starlight::ComputedCSSStyle& final_style,
      const StyleMap& explicit_resolved_style_map) const;

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
  void ReplayMaterializedStyleSideEffects(
      const starlight::ComputedCSSStyle& computed_style,
      CSSIDBitset* replayed_ids = nullptr,
      const NewPipelineStyleMutationPlan* plan = nullptr);
  void ReplayNewPipelineStyleMutationPlanSideEffects(
      const NewPipelineStyleMutationPlan& plan, CSSIDBitset* replayed_ids);
  void ReplayDynamicResolvedStyleSideEffects(
      const StyleMap& resolved_style_map,
      DynamicCSSStylesManager::StyleUpdateFlags update_flags,
      const CSSIDBitset& replayed_ids,
      const CSSIDBitset* source_style_ids = nullptr,
      const CSSIDBitset* inherited_dynamic_ids = nullptr);
  std::unique_ptr<starlight::ComputedCSSStyle>
  BuildFinalStyleFromAnimationSampleForNewPipeline(
      const starlight::ComputedCSSStyle& base_style,
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_final_style,
      const animation::AnimationSampleForNewPipeline& animation_sample,
      StyleMap& resolved_style_map, CSSIDBitset& variable_dependent_ids);
  animation::AnimationSampleForNewPipeline
  SampleAnimationOverridesForNewPipeline(
      starlight::ComputedCSSStyle& new_base_style, bool base_font_size_changed,
      bool base_root_font_size_changed,
      const StyleMap& new_underlying_layout_only_styles,
      const starlight::ComputedCSSStyle*& previous_final_style);
  animation::AnimationEventRecordsForNewPipeline
  TakeAnimationEventsForNewPipeline();
  bool NeedsAnimationFrameForNewPipeline() const;
  bool HasAuthorAnimationDataChangedForNewPipeline(
      const starlight::ComputedCSSStyle& new_base_style,
      const starlight::ComputedCSSStyle* previous_base_style) const;
  void FlushImperativeAnimationCleanupForNewPipeline(
      starlight::ComputedCSSStyle& cleanup_style, bool& need_update,
      CSSIDBitset* replayed_ids, const CSSIDBitset* source_style_ids = nullptr);
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
  NewPipelineResolveOutcome ResolveCSSStylesNewPipelineCore(
      const NewPipelineResolveRequest& request);
  void ResolveCSSStylesNewPipeline(bool& need_update);

  // Mark flush_required without recursively mark parent element
  inline void MarkRequireFlush() { flush_required_ = true; }

  bool ShouldTrackImperativeAnimationsForNewPipeline() const;
  void RecordImperativeAnimationStart(ImperativeAnimationState::Source source,
                                      const base::String& js_name,
                                      const base::String& animation_name,
                                      bool owns_generated_keyframe,
                                      const StyleMap& timing_styles);
  void UpdateImperativeAnimationPlayState(
      ImperativeAnimationState::Source source, const base::String& name,
      const StyleMap& timing_styles, bool paused);
  void CancelImperativeAnimation(ImperativeAnimationState::Source source,
                                 const base::String& name);
  void FinishImperativeAnimation(ImperativeAnimationState::Source source,
                                 const base::String& name);
  void ClearImperativeAnimationsForStyleAnimationUpdate();
  void ReplayImperativeAnimationsToStyle(
      starlight::ComputedCSSStyle& computed_style) const;
  CSSIDBitset TakePendingImperativeAnimationCleanupProperties();
  bool HasPendingImperativeAnimationCleanupProperties() const;
  bool HasImperativeAnimations() const;
  void ApplyImperativeAnimationMutation(
      const ImperativeAnimationState::Mutation& mutation);
  void RemoveOwnedImperativeAnimationKeyframe(
      const base::String& animation_name);
  void ClearImperativeAnimationState();

  virtual void CacheCommittedStyleFromAttributes(CSSPropertyID id,
                                                 const CSSValue& value) {}
  virtual void CacheCommittedStyleFromAttributes(CSSPropertyID id,
                                                 const lepus::Value& value) {}
  virtual void RemoveCommittedStyleFromAttributes(CSSPropertyID id) {}

  // Check if class change should be transmitted
  bool NeedForceClassChangeTransmit() const {
    return enable_class_change_transmit_ && !(dirty_ & kDirtyCreated);
  }

  // Check if there's invalidation for id selector change.
  bool CheckHasInvalidationForId(const std::string& old_id,
                                 const std::string& new_id);
  bool CheckHasInvalidationForClass(const ClassList& old_classes,
                                    const ClassList& new_classes);
  void MarkFontSizeInvalidateRecursively();
  void InvalidateChildrenFontSizeRecursively();
  void InvalidateChildrenInheritedStylesRecursively();
  void MarkDirectChildrenStyleDirtyForInheritedPropertyMutation();
  void RecursivelyMarkCustomPropertiesDirty();
  void InvalidateChildren(css::InvalidationSet* invalidation_set);
  void VisitChildren(const base::MoveOnlyClosure<void, Element*>& visitor);
  void SetFontSizeForAllElement(double cur_node_font_size,
                                double root_node_font_size);
  void UpdateLengthContextValueForAllElement(const LynxEnvConfig& env_config);
  void UpdateDynamicChildrenStyleRecursively(uint32_t style, bool force_update);
  void UpdateDynamicElementStyleRecursively(uint32_t style, bool force_update);
  void UpdateDynamicElementStyleForNewPipeline(uint32_t& style,
                                               bool& inner_force_update);
  void EnsureSLNode();
  int GetLayoutInElementPlatformChildIndex(Element* child);
  void UpdateFixedNodeSet();
  void UpdateFixedNodeSetRecursively(bool is_insert);

  base::String tag_;
  bool is_overlay_{false};

  int32_t id_;
  /**
   * A globally unique sequential identifier representing
   * the chronological position at which this element was
   * inserted into the DOM tree relative to all other elements.
   */
  uint32_t global_insertion_order_{kInitialGlobalInsertionOrder};
  uint32_t node_index_{kInvalidNodeIndex};

  constexpr const static int32_t kLayoutNodeTypeNotInit = -1;
  // Used to record the LayoutNodeType corresponding to the current tag, init
  // with LayoutNodeType::UNKNOWN.
  int32_t layout_node_type_{kLayoutNodeTypeNotInit};
  // Indicate element is flush in parallel
  int32_t parallel_flush_{0};

  int pseudo_type_{0};

  ElementArchTypeEnum arch_type_;

  bool will_destroy_{false};

  bool is_pseudo_{false};

  bool is_fixed_{false};
  bool is_sticky_{false};
  // indicate the element's position:fixed style has changed
  bool fixed_changed_{false};

  bool has_event_listener_{false};

  bool has_transition_props_changed_{false};
  bool has_keyframe_props_changed_{false};
  bool is_applying_animation_style_{false};
  bool has_non_flatten_attrs_{false};

  bool enable_class_change_transmit_{false};

  // relevant to layout only
  bool is_virtual_{false};

  // indicate has platform UI(view)
  bool has_painting_node_{false};

  bool subtree_need_update_{false};
  bool frame_changed_{false};
  bool display_none_{false};
  // Determine by Catalyzer
  bool is_layout_only_{false};

  // the child of text will be inline,
  // inline views work differently in many props such as layout_only/flatten,

  bool is_text_{false};

  // indicate if its an inline element,such as inline-text, inline-image,etc.
  bool is_inline_element_{false};

  // indicate this element is a list's sub element.
  bool is_list_item_{false};

  bool is_direct_child_of_compatible_component_{false};

  bool allow_layoutnode_inline_{false};

  bool has_placeholder_{false};
  bool has_text_selection_{false};
  bool trigger_global_event_{false};

  bool enable_new_animator_;

  std::shared_ptr<gfx::PlatformAnimationCommandBatch>
      platform_animation_commands_;

  bool create_node_async_{false};

  bool config_flatten_{true};

  bool has_layout_only_props_{true};

  // Should be set to false if children's layout parameter will be used on
  // platform layer. (e.g. scroll-view will use children's margin value on
  // both android and iOS)
  bool can_has_layout_only_children_{true};

  // Should be set to true if related platform node need process direction to
  // work properly in RTL mode
  bool need_process_direction_{false};

  // enable_component_layout_only_ & enable_extended_layout_only_opt_
  // optimization is enabled by default in Fiber Arch.
  // But in Radon Arch, this switch should be get from page_config.
  bool enable_extended_layout_only_opt_{true};
  bool enable_component_layout_only_{true};

  /**
   StyleResolver has no member variables and its size is 1 byte. Put it here
   along with booleans to minimize size of Element.
   */
  friend class StyleResolver;
  StyleResolver style_resolver_;

  // relevant to layout and frame
  float width_{0};
  float height_{0};
  float top_{0};
  float left_{0};
  float scroll_offset_x_{0};
  float scroll_offset_y_{0};
  float sticky_translation_x_{0};
  float sticky_translation_y_{0};
  std::array<float, 4> borders_{};
  std::array<float, 4> margins_{};
  std::array<float, 4> paddings_{};
  // Sticky payload:
  // info[0-3]: left, top, right, bottom
  // info[4-5]: parent width, parent height
  // info[6-7]: self left/top relative to scroller
  // info[8-9]: parent left/top relative to scroller.
  base::auto_create_optional<std::array<float, 10>> sticky_positions_;
  float max_height_{starlight::DefaultLayoutStyle::kDefaultMaxSize};

  float record_parent_font_size_ = -1;

  std::unique_ptr<ContentData> content_data_;

  std::unique_ptr<BaseElementContainer> element_container_;

  fml::RefPtr<AttributeHolder> data_model_;

  ElementManager* element_manager_{nullptr};

  Catalyzer* catalyzer_;

  fml::RefPtr<PropBundle> prop_bundle_;

  // Stores the previous PropBundle for unit test verification after a reset.
  fml::RefPtr<PropBundle> pre_prop_bundle_;

  // relevant to hierarchy
  Element* parent_{nullptr};

  std::unique_ptr<starlight::ComputedCSSStyle> base_css_style_;
  std::unique_ptr<starlight::ComputedCSSStyle> platform_css_style_;

  // for animation
  std::unique_ptr<animation::CSSKeyframeManager> css_keyframe_manager_;
  std::unique_ptr<animation::CSSTransitionManager> css_transition_manager_;
  base::flex_optional<fml::TimePoint> animation_sample_time_for_new_pipeline_;
  // Saves the css style that the all animation applied to the element.
  base::auto_create_optional<StyleMap> final_animator_map_;
  // Save the keyframes of the Animate API.
  base::auto_create_optional<CSSKeyframesTokenMap> keyframes_map_;
  // Save increase key of the Animate API.
  base::String will_removed_keyframe_name_;
  ImperativeAnimationState imperative_animation_state_;
  // for global-bind event
  base::auto_create_optional<base::LinearFlatSet<std::string>>
      global_bind_target_set_;

  // Using to record some previous element styles which New Animator needs.
  base::LinearFlatMap<tasm::CSSPropertyID, CSSValue> animation_previous_styles_;

  // Tracks the previous underlying target values for transition-relevant
  // layout-only properties in the new styling pipeline.
  base::auto_create_optional<StyleMap>
      committed_underlying_layout_only_styles_for_new_pipeline_;
  // Used to record all layout-related styles of the element only when we
  // enable dump element tree. In the copied element, we will use these styles
  // to initialize the layout node.
  base::auto_create_optional<base::LinearFlatMap<tasm::CSSPropertyID, CSSValue>>
      layout_styles_;

  // for devtool
  std::unique_ptr<InspectorAttribute> inspector_attribute_;

  std::unique_ptr<PlatformLayoutFunctionWrapper> customized_layout_node_;

  base::InlineVector<fml::RefPtr<Element>, kChildrenInlineVectorSize>
      scoped_children_;
  base::InlineVector<fml::RefPtr<Element>, kChildrenInlineVectorSize>
      logical_children_;
  base::auto_create_optional<base::InlineVector<fml::RefPtr<Element>, 2>>
      scoped_virtual_children_;
  Element* virtual_parent_{nullptr};

  Element* render_parent_{nullptr};
  Element* last_render_child_{nullptr};
  Element* first_render_child_{nullptr};
  Element* previous_render_sibling_{nullptr};
  Element* next_render_sibling_{nullptr};
  css::InvalidationLists invalidation_lists_;

  int64_t parent_component_unique_id_{-1};
  mutable Element* parent_component_element_{nullptr};
  mutable Element* render_root_element_{nullptr};
  Element* enclosing_none_wrapper_{nullptr};

  std::shared_ptr<CSSStyleSheetManager> css_style_sheet_manager_;
  std::unique_ptr<CSSFragmentDecorator> style_sheet_;

  uint32_t dirty_{0};
  uint32_t wrapper_element_count_{0};
  base::Vector<base::closure> pending_invoke_tasks_;

  int32_t css_id_{kInvalidCssId};
  base::String element_entry_name_;

  DynamicCSSStylesManager::StyleUpdateFlags dynamic_style_flags_{0};

  AsyncResolveStatus resolve_status_{AsyncResolveStatus::kCreated};

  bool need_handle_fixed_{false};
  bool has_extreme_parsed_styles_{false};
  bool only_selector_extreme_parsed_styles_{false};
  bool children_propagate_inherited_styles_flag_{false};
  bool attached_to_layout_parent_{false};
  bool can_be_layout_only_{false};
  bool has_to_store_insert_remove_actions_{false};
  bool has_font_size_{false};
  bool is_template_{false};
  bool has_transition_props_{false};

  SharedTemplateAttributes template_attributes_{};

  bool flush_required_{true};
  bool is_first_created_{true};
  bool is_async_flush_root_{false};
  base::String full_raw_inline_style_;
  StyleMap parsed_styles_map_;
  base::auto_create_optional<StyleMap> parsed_dynamic_styles_map_;
  base::auto_create_optional<StyleMap> styles_from_attributes_;
  base::auto_create_optional<RawLepusStyleMap> current_raw_inline_styles_;
  base::auto_create_optional<CSSVariableMap>
      current_raw_inline_custom_properties_;
  base::auto_create_optional<RawLepusStyleMap>
      current_raw_important_inline_styles_;
  base::auto_create_optional<StyleMap> extreme_parsed_styles_;
  base::auto_create_optional<StyleMap> inherited_styles_;
  base::auto_create_optional<StyleMap> updated_inherited_styles_;
  base::auto_create_optional<StyleMap> animation_override_styles_map_;
  base::auto_create_optional<base::Vector<tasm::CSSPropertyID>>
      reset_inherited_ids_;

  CustomPropertiesMapRef custom_properties_;

  base::auto_create_optional<
      base::LinearFlatMap<tasm::CSSPropertyID, std::pair<CSSValue, IsLogic>>>
      pending_updated_direction_related_styles_;

  base::Vector<ActionParam> action_param_list_;

  AttrUMap updated_attr_map_;
  base::auto_create_optional<BuiltinAttrMap> builtin_attr_map_;
  base::auto_create_optional<base::Vector<base::String>> reset_attr_vec_;
  base::auto_create_optional<std::unordered_set<base::String>>
      modifier_attribute_names_;

  fml::RefPtr<lepus::Dictionary> config_;

  base::auto_create_optional<std::list<base::closure>> parallel_reduce_tasks_;
  base::auto_create_optional<std::list<base::closure>>
      parallel_before_flush_action_tasks_;

  std::unique_ptr<LayoutBundle> layout_bundle_;

  base::String part_id_;

  base::auto_create_optional<
      base::LinearFlatMap<PseudoState, std::unique_ptr<PseudoElement>>>
      pseudo_elements_;

  std::unique_ptr<ListItemSchedulerAdapter> scheduler_adapter_;

  std::unique_ptr<style::StyleObject*, style::StyleObjectArrayDeleter>
      style_objects_{nullptr};
  std::unique_ptr<style::StyleObject*, style::StyleObjectArrayDeleter>
      last_style_objects_{nullptr};
  style::DynamicStyleObjectRef dynamic_simple_object_{nullptr};

  std::unique_ptr<SLNode> sl_node_{nullptr};

 private:
  // Updates logical and scoped ownership without enqueueing platform actions.
  void AttachNodeToElementTree(const fml::RefPtr<Element>& child,
                               Element* ref_node, int32_t index,
                               bool update_logical_children);
  void DetachNodeFromElementTree(const fml::RefPtr<Element>& child,
                                 int32_t index, bool update_logical_children);
  // Returns the original render source, consuming a pending move when repeated
  // moves are coalesced before a flush.
  fml::RefPtr<Element> TakeMoveSourceParent(const fml::RefPtr<Element>& child);

  void AttachToElementManagerInner(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id);

  // Element state, used to identify whether the current Element is on the root
  // Dom tree. When an Element is constructed, it is definitely not on the root
  // Dom tree, so state_ is initialized as State::kDetached.
  State state_{State::kDetached};

  PseudoElement* CreatePseudoElementIfNeed(PseudoState state);

  bool WriteRenderStyleToBundle(tasm::CSSPropertyID id,
                                const tasm::CSSValue& value);
  void DispatchBundleToPaintingNode(fml::RefPtr<PropBundle> bundle);

  void UpdateStickyPosition(const std::array<float, 4>* sticky_positions);

  CSSKeyframesToken* GetSimpleStyleKeyframesToken(
      const base::String& animation_name);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_H_
