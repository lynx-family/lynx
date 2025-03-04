// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_H_
#define CORE_RENDERER_DOM_ELEMENT_H_

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/no_destructor.h"
#include "base/include/vector.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/css_transition_manager.h"
#include "core/inspector/style_sheet.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_content_data.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/css_patching.h"
#include "core/renderer/dom/element_container.h"
#include "core/renderer/events/events.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/runtime/vm/lepus/ref_type.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

class AttributeHolder;
class ElementManager;
class HierarchyObserver;
class ListNode;

using ElementChildrenArray =
    base::InlineVector<Element*, kChildrenInlineVectorSize>;

enum ElementArchTypeEnum {
  FiberArch = 0,
  RadonArch,
};

class InspectorAttribute {
 public:
  BASE_EXPORT_FOR_DEVTOOL InspectorAttribute();
  BASE_EXPORT_FOR_DEVTOOL ~InspectorAttribute();

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

class Element : public lepus::RefCounted {
 public:
  Element(const base::String& tag, ElementManager* element_manager,
          uint32_t node_index = 0);

  Element& operator=(const Element&) = delete;

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kElement;
  };

  void SetAttributeHolder(const std::shared_ptr<AttributeHolder>& data_model) {
    data_model_ = data_model;
    data_model_->SetElement(this);
    data_model_->set_tag(tag_);
  }
  AttributeHolder* data_model() const { return data_model_.get(); };

  virtual bool is_radon_element() const { return false; }
  virtual bool is_fiber_element() const { return false; }

  bool is_fixed() { return is_fixed_; }
  bool is_parallel_flush() { return parallel_flush_; }

  int32_t impl_id() const { return id_; }

  void SetNodeIndex(uint32_t node_index) { node_index_ = node_index; }
  uint32_t NodeIndex() const { return node_index_; }

  std::vector<float> ScrollBy(float width, float height);
  std::vector<float> GetRectToLynxView();
  void Invoke(const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback);

  ElementManager* element_manager() const { return element_manager_; }
  Element* parent() const { return parent_; }
  Element* next_sibling() const { return Sibling(1); }
  Element* previous_sibling() const { return Sibling(-1); }
  virtual Element* Sibling(int offset) const = 0;

  // only for fiber arch, indicate current real render tree hierarchy
  virtual Element* render_parent() { return nullptr; }
  virtual Element* first_render_child() { return nullptr; }
  virtual Element* first_child() const { return nullptr; }
  virtual Element* last_child() const { return nullptr; }
  virtual Element* next_render_sibling() { return nullptr; }

  virtual ~Element() = default;

  // For style op
  BASE_EXPORT_FOR_DEVTOOL virtual void ConsumeStyle(
      const StyleMap& styles, StyleMap* inherit_styles = nullptr) = 0;

  virtual void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value,
                                bool force_update = false);
  void SetPlaceHolderStyles(const PseudoPlaceHolderStyles& styles);
  BASE_EXPORT_FOR_DEVTOOL virtual void ResetStyle(
      const base::Vector<CSSPropertyID>& style_names);

  // For attr op
  BASE_EXPORT_FOR_DEVTOOL virtual void SetAttribute(
      const base::String& key, const lepus::Value& value,
      bool need_update_data_model = true) = 0;
  virtual void ResetAttribute(const base::String& key);
  void WillConsumeAttribute(const base::String& key, const lepus::Value& value);

  // For dataset op
  void SetDataSet(const tasm::DataMap& data);

  // For event handler
  virtual void SetEventHandler(const base::String& name, EventHandler* handler);
  virtual void ResetEventHandlers();

  // For gesture handler
  void SetGestureDetectorState(int32_t gesture_id, int32_t state);
  void SetGestureDetector(const uint32_t key, GestureDetector* detector);
  void ConsumeGesture(int32_t gesture_id, const lepus::Value& params);

  // For prop op
  void SetProp(const char* key, const lepus::Value& value);
  void ResetProp(const char* key);

  // For keyframe op
  // The first parameter names can be string type or array type of lepus value
  void SetKeyframesByNames(const lepus::Value& names,
                           const CSSKeyframesTokenMap&, bool force_flush);

  virtual void SetKeyframesByNamesInner(std::unique_ptr<PropBundle> bundle);

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
  void Animate(const lepus::Value& args);

  // For JS API setNativeProps
  virtual void SetNativeProps(const lepus::Value& args,
                              PipelineOptions& pipeline_options) = 0;

  // Get List Node
  virtual ListNode* GetListNode() = 0;

  // Get Parent Component's Element
  BASE_EXPORT_FOR_DEVTOOL virtual Element* GetParentComponentElement()
      const = 0;

  Catalyzer* GetCaCatalyzer() { return catalyzer_; }

  virtual const EventMap& event_map() const;
  virtual const EventMap& lepus_event_map();
  virtual const EventMap& global_bind_event_map();
  // GestureMap key - gesture id / value - GestureDetector
  virtual const GestureMap& gesture_map();

  virtual bool InComponent() const { return false; }
  virtual int ParentComponentId() const { return 0; }
  virtual std::string ParentComponentIdString() const = 0;
  virtual const std::string& ParentComponentEntryName() const = 0;

  inline bool IsLayoutOnly() { return is_layout_only_; }
  inline bool IsNewFixed() const { return is_fixed_ && enable_new_fixed_; }
  inline bool GetEnableFixedNew() const { return enable_new_fixed_; }
  inline bool is_virtual() { return is_virtual_; }
  virtual bool is_fixed_new() { return false; }
  BASE_EXPORT_FOR_DEVTOOL virtual bool GetPageElementEnabled() { return false; }
  BASE_EXPORT_FOR_DEVTOOL virtual bool GetRemoveCSSScopeEnabled() {
    return false;
  }

  void SetArchType(ElementArchTypeEnum arch_type) { arch_type_ = arch_type; }

  bool GetArchType() { return arch_type_; }
  bool IsRadonArch() const { return arch_type_ == RadonArch; }
  bool IsFiberArch() const { return arch_type_ == FiberArch; }

  void UpdateLayout(float left, float top, float width, float height,
                    const std::array<float, 4>& paddings,
                    const std::array<float, 4>& margins,
                    const std::array<float, 4>& borders,
                    const std::array<float, 4>* sticky_positions,
                    float max_height);
  // Used to update child element's left and top value from list element. The
  // another overloaded function is used to update layout info from starlight,
  // but if the element is list's child, the left and top's value are always 0.
  void UpdateLayout(float left, float top);

  virtual Element* GetChildAt(size_t index) { return nullptr; }

  virtual size_t GetChildCount() { return 0; }

  virtual ElementChildrenArray GetChildren() { return {}; }

  virtual size_t GetUIIndexForChild(Element* child) { return 0; }

  virtual int32_t IndexOf(const Element* child) const = 0;

  virtual void InsertNode(const fml::RefPtr<Element>& child) = 0;
  virtual void InsertNode(const fml::RefPtr<Element>& child, int32_t index) = 0;
  virtual void RemoveNode(const fml::RefPtr<Element>& child,
                          bool destroy = true) = 0;

  inline bool CanHasLayoutOnlyChildren() {
    return can_has_layout_only_children_;
  };

  void onNodeReload();

  /*
   * return the font size from platform_css_style_.
   */
  BASE_EXPORT_FOR_DEVTOOL virtual double GetFontSize();

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

  void SetFontSize(const tasm::CSSValue* value);

  starlight::DirectionType Direction() { return direction_; }

  virtual void OnPseudoStatusChanged(PseudoState prev_status,
                                     PseudoState current_status) {}

  ContentData* content_data() const { return content_data_.get(); }
  void SetContentData(ContentData* data) { content_data_.reset(data); }

  virtual void UpdateDynamicElementStyle(uint32_t style, bool force_update) = 0;

  bool HasPlaceHolder() { return has_placeholder_; }
  bool HasTextSelection() { return has_text_selection_; }

  PaintingContext* painting_context();

  // Declared platform node tag
  const base::String& GetTag() const { return tag_; }

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

  ElementContainer* element_container() { return element_container_.get(); }
  void CreateElementContainer(bool platform_is_flatten);

  virtual void HandleLayoutTask(base::MoveOnlyClosure<void> operation) {
    operation();
  }

  virtual void HandleDelayTask(base::MoveOnlyClosure<void> operation) {
    operation();
  }

  DynamicCSSStylesManager& StylesManager() { return styles_manager_; }
  const DynamicCSSStylesManager& StylesManager() const {
    return styles_manager_;
  }

  void set_parent(Element* parent) { parent_ = parent; }
  bool EnableTriggerGlobalEvent() const { return trigger_global_event_; }

  void PreparePropBundleIfNeed();

  bool GetEnableZIndex();

  virtual void MarkLayoutDirty();

  // In RadonDiff Mode, worklets require the following two APIs. In RL3.0 or
  // TTML NoDiff, the implementation of worklets no longer relies on these
  // capabilities. After the 2.0 worklet services are phased out, the following
  // two APIs will also be removed.
  virtual StyleMap GetStylesForWorklet() = 0;
  virtual const AttrMap& GetAttributesForWorklet() = 0;

  inline const std::set<std::string>& GlobalBindTarget() {
    return global_bind_target_set_;
  }
  virtual bool CanBeLayoutOnly() const = 0;

  virtual void CheckHasInlineContainer(Element* parent);

  bool FlushAnimatedStyle();
  virtual void FlushAnimatedStyleInternal(tasm::CSSPropertyID,
                                          const tasm::CSSValue&) = 0;

  float width() { return width_; }
  float height() { return height_; }
  float top() { return top_; }
  float left() { return left_; }

  bool enable_new_animator() { return enable_new_animator_; }

  PropertiesResolvingStatus GenerateRootPropertyStatus() const;

  void SetDirection(const tasm::CSSValue& value) {
    styles_manager_.UpdateDirectionStyle(value);
  }

  void SetComputedFontSize(const tasm::CSSValue& value, double font_size,
                           double root_font_size, bool force_update = false);
  void SetPlaceHolderStylesInternal(const PseudoPlaceHolderStyles& styles);

  void ResetStyleInternal(CSSPropertyID id);
  void SetDirectionInternal(const tasm::CSSValue& value) {
    direction_ = value.GetEnum<starlight::DirectionType>();
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
  inline const std::array<float, 4>& sticky_positions() {
    return sticky_positions_;
  }

  inline tasm::CSSKeyframesTokenMap& keyframes_map() { return keyframes_map_; }

  bool ShouldAvoidFlattenForView();

  bool TendToFlatten();

  bool NeedCreateNodeAsync() { return create_node_async_; }

  static constexpr short OVERFLOW_HIDDEN = 0x00;
  static constexpr short OVERFLOW_X = 0x01;
  static constexpr short OVERFLOW_Y = 0x02;
  static constexpr short OVERFLOW_XY = (OVERFLOW_X | OVERFLOW_Y);
  short overflow() { return overflow_; }

  void CheckOverflow(CSSPropertyID id, const tasm::CSSValue& value);

  bool HasPaintingNode() { return has_painting_node_; }
  void ResetPropBundle();
  void ResetFontSize();

  void PreparePropsBundleForDynamicCSS();
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
  void CheckHasOpacityProps(CSSPropertyID id, bool reset);
  // return true indicates current style is transtion related
  bool CheckTransitionProps(CSSPropertyID id);
  // return true indicates current style is keyframe related
  bool CheckKeyframeProps(CSSPropertyID id);

  void CheckHasNonFlattenCSSProps(CSSPropertyID id);
  void CheckFixedSticky(CSSPropertyID id, const tasm::CSSValue& value);
  // return true indicate that current css is z-index
  bool CheckZIndexProps(CSSPropertyID id, bool reset);
  void CheckBoxShadowOrOutline(CSSPropertyID id);
  bool DisableFlattenWithOpacity();

  inline starlight::ComputedCSSStyle* computed_css_style() {
    return platform_css_style_.get();
  }
  inline const starlight::ComputedCSSStyle* computed_css_style() const {
    return platform_css_style_.get();
  }

  starlight::ComputedCSSStyle* GetParentComputedCSSStyle();

  void SetDataToNativeKeyframeAnimator(bool from_resume = false);
  void SetDataToNativeTransitionAnimator();

  bool ShouldConsumeTransitionStylesInAdvance();
  bool ConsumeTransitionStylesInAdvance(const StyleMap& styles,
                                        bool force_reset = false);

  virtual void MarkAsLayoutRoot() = 0;
  virtual void AttachLayoutNode(const std::shared_ptr<PropBundle>& props) = 0;
  virtual void UpdateLayoutNodeProps(
      const std::shared_ptr<PropBundle>& props) = 0;
  virtual void UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                     const tasm::CSSValue& value) = 0;
  virtual void ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) = 0;
  virtual void UpdateLayoutNodeFontSize(double cur_node_font_size,
                                        double root_node_font_size) = 0;
  virtual void UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                         const lepus::Value& value) = 0;

  virtual void ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value,
                                 bool force_update) {}
  virtual void CheckDynamicUnit(CSSPropertyID id, const CSSValue& value,
                                bool reset) {
    // currently, radon element do no need to such kind of check
  }

  virtual void WillResetCSSValue(CSSPropertyID& id) {}

  virtual void ResetCSSValue(CSSPropertyID id);
  virtual void ConsumeTransitionStylesInAdvanceInternal(
      CSSPropertyID css_id, const tasm::CSSValue& value) = 0;
  bool ResetTransitionStylesInAdvance(
      const base::Vector<CSSPropertyID>& css_names);
  virtual void ResetTransitionStylesInAdvanceInternal(CSSPropertyID css_id) = 0;

  void ResolveAndFlushKeyframes();

  void RecordElementPreviousStyle(CSSPropertyID css_id,
                                  const tasm::CSSValue& value);
  void ResetElementPreviousStyle(CSSPropertyID css_id);

  std::optional<CSSValue> GetElementPreviousStyle(tasm::CSSPropertyID css_id);

  virtual std::optional<CSSValue> GetElementStyle(
      tasm::CSSPropertyID css_id) = 0;

  CSSKeyframesToken* GetCSSKeyframesToken(const std::string& animation_name);

  virtual CSSFragment* GetRelatedCSSFragment() = 0;

  virtual void FlushProps() = 0;

  void set_will_destroy(bool destroy) { will_destroy_ = destroy; }

  bool will_destroy() { return will_destroy_; }

  virtual void TickElement(fml::TimePoint& time){};

  bool TickAllAnimation(fml::TimePoint& time, PipelineOptions& options);

  virtual void RequestLayout() = 0;

  virtual void RequestNextFrame() = 0;

  void UpdateFinalStyleMap(const StyleMap& styles);

  virtual void OnPatchFinish(PipelineOptions& option) = 0;

  virtual int32_t GetCSSID() const = 0;

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
  virtual void OnListElementUpdated(const PipelineOptions& options) {}

  // When the rendering of the list's child node is complete, this method will
  // be invoked. In this method, we can accurately obtain the layout
  // information of the child node.
  virtual void OnComponentFinished(Element* component,
                                   const PipelineOptions& option) {}

  // This method is override by ListElement or RadonListElement, which used to
  // notify list that the list item's layout info has been updated.
  virtual void OnListItemLayoutUpdated(Element* component) {}

  // When the batch rendering of the list's child nodes are complete, this
  // method will be invoked. In this method, we can accurately obtain the layout
  // information of the child nodes
  virtual void OnListItemBatchFinished(const PipelineOptions& options) {}

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

  virtual bool NeedFastFlushPath(
      const std::pair<CSSPropertyID, tasm::CSSValue>& style) = 0;

  virtual bool is_view() const { return false; }

  virtual bool is_image() const { return false; }

  virtual bool is_text() const { return false; }

  virtual bool is_list() const { return false; }

  virtual bool is_wrapper() const { return false; }

  virtual void MarkAsListItem() { is_list_item_ = true; }

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

  // When list component finishes all props update
  virtual void PropsUpdateFinish() {}
  bool HasPropsToBeFlush() const { return prop_bundle_ != nullptr; }

  void PushToBundle(CSSPropertyID id);

  bool has_z_props() const { return has_z_props_; }

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

  void HandleCSSVariables(StyleMap& styles);

  void ResolvePseudoSelectors();

  void ResolvePlaceHolder();

  // Callback before style resolving. Return false to skip style resolving.
  virtual bool WillResolveStyle(StyleMap& merged_styles) { return true; }
  virtual void DidResolveStyle(StyleMap& merged_styles) {}

  // Style resolver will firstly call `CountInlineStyles` to get count
  // of inline styles to be merged. Then it will merge styles from CSS
  // selectors and finally calls `MergeInlineStyles` to merge the inline
  // styles.
  virtual size_t CountInlineStyles() = 0;
  virtual void MergeInlineStyles(StyleMap& merged_styles) = 0;

 protected:
  Element(const Element&, bool clone_resolved_props);

  // The element object created using the clone interface of FiberElement is
  // not attached to the element manager. Use this function to attach it to
  // the element manager.
  virtual void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id);

  std::unique_ptr<ElementContainer> element_container_;

  std::shared_ptr<AttributeHolder> data_model_{nullptr};

  ElementArchTypeEnum arch_type_ = ElementArchTypeEnum::FiberArch;
  bool is_fixed_{false};
  bool is_sticky_{false};
  // indicate the element's position:fixed style has changed
  bool fixed_changed_{false};
  // Indicate element is flush in parallel
  bool parallel_flush_{false};

  bool has_event_listener_{false};

  bool has_transition_props_changed_{false};
  bool has_keyframe_props_changed_{false};
  bool has_non_flatten_attrs_{false};

  bool has_opacity_{false};
  // relevant to z-index
  bool has_z_props_{false};

  // Should be set to false if children's layout parameter will be used on
  // platform layer. (e.g. scroll-view will use children's margin value on
  // both android and iOS)
  bool can_has_layout_only_children_{true};

  bool enable_class_change_transmit_{false};

  // relevant to layout only
  bool is_virtual_{false};

  base::String tag_;

  // indicate has platform UI(view)
  bool has_painting_node_{false};

  Catalyzer* catalyzer_;

  // TODO(songshourui.null): rename css_patching_ to style_resolver_;
  CSSPatching css_patching_;

  // config settings for enableLayoutOnly
  bool config_enable_layout_only_{true};
  bool has_layout_only_props_{true};

  // enable_component_layout_only_ & enable_extended_layout_only_opt_
  // optimization is enabled by default in Fiber Arch.
  // But in Radon Arch, this switch should be get from page_config.
  bool enable_extended_layout_only_opt_{true};
  bool enable_component_layout_only_{true};

  bool enable_new_fixed_{false};

  std::shared_ptr<PropBundle> prop_bundle_{nullptr};
  // just for unit test now.
  std::shared_ptr<PropBundle> pre_prop_bundle_{nullptr};

  // relevant to layout and frame
  float width_{0};
  float height_{0};
  float top_{0};
  float left_{0};
  // left, right, top, bottom -> starlight::Direction
  std::array<float, 4> borders_{};
  std::array<float, 4> margins_{};
  std::array<float, 4> paddings_{};
  std::array<float, 4> sticky_positions_{};
  float max_height_{starlight::DefaultLayoutStyle::kDefaultMaxSize};
  bool subtree_need_update_{false};
  bool frame_changed_{false};
  // Determine by Catalyzer
  bool is_layout_only_{false};

  // the child of text will be inline,
  // inline views work differently in many props such as layout_only/flatten,

  bool is_text_{false};

  // indicate if its an inline element,such as inline-text, inline-image,etc.
  bool is_inline_element_{false};

  // indicate this element is a list's sub element.
  bool is_list_item_{false};

  bool allow_layoutnode_inline_{false};

  std::unique_ptr<ContentData> content_data_{nullptr};
  bool config_flatten_{true};

  // relevant to hierarchy
  Element* parent_{nullptr};
  ElementChildrenArray children_;

  bool is_pseudo_{false};
  int pseudo_type_{0};

  starlight::DirectionType direction_ =
      starlight::DefaultLayoutStyle::SL_DEFAULT_DIRECTION;

  short overflow_{0};

  bool has_placeholder_{false};
  bool has_text_selection_{false};
  bool trigger_global_event_{false};

  DynamicCSSStylesManager styles_manager_;

  int32_t id_;
  uint32_t node_index_{0};

  constexpr const static int32_t kLayoutNodeTypeNotInit = -1;
  // Used to record the LayoutNodeType corresponding to the current tag, init
  // with LayoutNodeType::UNKNOWN.
  int32_t layout_node_type_{kLayoutNodeTypeNotInit};

  bool create_node_async_{false};

  std::unique_ptr<starlight::ComputedCSSStyle> platform_css_style_;

  bool will_destroy_{false};
  ElementManager* element_manager_{nullptr};

  // for animation
  bool enable_new_animator_;
  std::unique_ptr<animation::CSSKeyframeManager> css_keyframe_manager_;
  std::unique_ptr<animation::CSSTransitionManager> css_transition_manager_;
  // Saves the css style that the all animation applied to the element.
  StyleMap final_animator_map_;
  // Save the keyframes of the Animate API.
  tasm::CSSKeyframesTokenMap keyframes_map_;
  // Save increase key of the Animate API.
  std::string will_removed_keyframe_name_;
  // for global-bind event
  std::set<std::string> global_bind_target_set_;

  // Using to record some previous element styles which New Animator needs.
  std::unordered_map<tasm::CSSPropertyID, CSSValue>
      animation_previous_styles_{};

  // Used to record all layout-related styles of the element only when we
  // enable dump element tree. In the copied element, we will use these styles
  // to initialize the layout node.
  std::unordered_map<tasm::CSSPropertyID, CSSValue> layout_styles_{};

  // for devtool
  std::unique_ptr<InspectorAttribute> inspector_attribute_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_H_
