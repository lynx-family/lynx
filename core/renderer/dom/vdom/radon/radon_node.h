// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_NODE_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_NODE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/dom/vdom/radon/radon_factory.h"
#include "core/renderer/dom/vdom/radon/radon_types.h"

namespace lynx {
namespace tasm {

class RadonComponent;
class RadonElement;
class PageProxy;

class RadonNode : public RadonBase {
 public:
  RadonNode(PageProxy* const page_proxy_, const base::String& tag_name,
            uint32_t node_index);
  RadonNode(const RadonNode& node, PtrLookupMap& map);
  virtual ~RadonNode() {
    if (attribute_holder_) {
      attribute_holder_->set_radon_node_ptr(nullptr);
    }
  }

  void OnStyleChange();

  void DispatchSelf(const DispatchOption&) override;

  virtual bool ShouldFlush(const std::unique_ptr<RadonBase>&,
                           const DispatchOption&);
  bool ShouldFlushAttr(const RadonNode* old_radon_node);
  bool ShouldFlushDataSet(const RadonNode* old_radon_node);
  bool ShouldFlushStyle(RadonNode* old_radon_node,
                        const DispatchOption& option);
  bool ShouldFlushGestureDetectors(const RadonNode* old_radon_node);

  // Collect descendant invalidation sets (e.g., if this element added or
  // removed the given class, what other types of elements need to change?).
  // Then apply style invalidation to children immediately.
  void CollectInvalidationSetsAndInvalidate(RadonNode* old_radon_node);
  void CollectInvalidationSetsForPseudoAndInvalidate(CSSFragment*, PseudoState,
                                                     PseudoState);
  // Optimized logic: use GetCachedStyleList to get new style only when
  // needed.
  // only can be used if forceCalcNewStyle is set to false by compilerOptions
  bool OptimizedShouldFlushStyle(RadonNode* old_radon_node,
                                 const DispatchOption& option);
  // When pseudo state changes, refresh css styles to apply new css.
  bool RefreshStyle();

  void OnPseudoStateChanged(PseudoState, PseudoState);

  void UpdateCSSVariable(const base::String& key, const base::String& value,
                         CSSVariableMap* changed_css_vars = nullptr) {
    attribute_holder_->UpdateCSSVariable(key, value, changed_css_vars);
  }
  void UpdateCSSVariableFromSetProperty(const base::String& key,
                                        const base::String& value) {
    attribute_holder_->UpdateCSSVariableFromSetProperty(key, value);
  }

  bool ContainsSelector(const std::string& selector) const {
    return attribute_holder_->ContainsSelector(selector);
  }

  void MarkAllDynamic() {
    has_dynamic_class_ = true;
    has_dynamic_attr_ = true;
    has_dynamic_inline_style_ = true;
  }

  void SetSSRAttrHolder(bool flag) {
    attribute_holder_->SetSSRAttrHolder(flag);
  }

  void UpdateIdSelector(const base::String& id_selector);

  const base::String& tag() const { return RadonBase::tag_name_; }

  virtual CSSFragment* ParentStyleSheet() const;

  CSSFragment* GetPageStyleSheet();

  bool GetRemoveCSSScopeEnabled() const;
  bool GetCascadePseudoEnabled() const;
  bool GetRemoveDescendantSelectorScope() const;
  bool IsComponent() const { return IsRadonComponent(); }

  AttributeHolder* HolderParent() const;

  AttributeHolder* NextSibling() const;

  AttributeHolder* PreviousSibling() const;

  virtual size_t ChildCount() const;

  virtual void RemoveElementFromParent() override;
  virtual void ResetElementRecursively() override;

  // node add/remove/move event
  virtual void OnElementRemoved(int idx) {}
  virtual void OnElementMoved(int fromIdx, int toIdx) {}

  PageProxy* const page_proxy_;

  virtual bool NeedsElement() const override { return true; }

  bool InComponent() const;
  int ParentComponentId() const;
  int ParentComponentElementId();

  bool IsRadonNode() const override { return true; }

  virtual Element* element() const override { return element_.get(); }

  virtual RadonElement* radon_element() const override {
    return static_cast<RadonElement*>(element_.get());
  }

  FiberElement* fiber_element() const {
    return static_cast<FiberElement*>(element_.get());
  }

  virtual const fml::RefPtr<Element>& GetElementRef() const override {
    return element_;
  }

  virtual int ImplId() const override;

  void ApplyDynamicCSSWhenParentIsReady(const Element* parent);

  bool GetDevToolFlag() override;

  /**
   * DevTool Related.
   */
  void NotifyElementNodeAdded() override;

  /**
   * DevTool Related.
   */
  void NotifyElementNodeRemoved();

  /**
   * DevTool Related.
   */
  void NotifyElementNodeSetted();

  /**
   * DevTool Related.
   */
  RadonPlug* GetRadonPlug() override;

  /**
   * DevTool Related.
   */
  void CheckAndProcessSlotForInspector(Element* element);

  /**
   * DevTool Related.
   */
  void CheckAndProcessComponentRemoveViewForInspector(Element* element);

  RadonNode* NodeParent();
  RadonNode* FirstNodeChild();
  RadonNode* LastNodeChild();

  const ClassList& classes() { return attribute_holder_->classes(); }
  void AddClass(const base::String& clazz) {
    has_dynamic_class_ = true;
    attribute_holder_->AddClass(clazz);
  }
  void SetClass(const base::String& clazz) {
    has_dynamic_class_ = true;
    attribute_holder_->SetClass(clazz);
  }
  void SetClasses(ClassList&& classes) {
    has_dynamic_class_ = true;
    attribute_holder_->SetClasses(std::move(classes));
  }
  void SetStaticClass(const base::String& clazz) {
    attribute_holder_->SetStaticClass(clazz);
  }
  void RemoveAllClass() { attribute_holder_->RemoveAllClass(); }
  bool HasClass(const std::string& cls) const {
    return attribute_holder_->HasClass(cls);
  }
  bool HasClass() const { return attribute_holder_->HasClass(); }

  const StyleMap& inline_styles() const {
    return attribute_holder_->inline_styles();
  }

  const AttrMap& attributes() const { return attribute_holder_->attributes(); }

  AttrMap& attributes() { return attribute_holder_->attributes(); }

  void SetStaticAttribute(const base::String& key, const lepus::Value& value) {
    attribute_holder_->SetStaticAttribute(key, value);
  }

  void SetStaticAttribute(const base::String& key, lepus::Value&& value) {
    attribute_holder_->SetStaticAttribute(key, std::move(value));
  }

  void SetDynamicAttribute(const base::String& key, const lepus::Value& value) {
    has_dynamic_attr_ = true;
    SetStaticAttribute(key, value);
  }

  void SetDynamicAttribute(const base::String& key, lepus::Value&& value) {
    has_dynamic_attr_ = true;
    SetStaticAttribute(key, std::move(value));
  }

  const DataMap& data_set() const { return attribute_holder_->dataset(); }
  void SetDataSet(const base::String& key, const lepus::Value& value) {
    attribute_holder_->SetDataSet(key, value);
  }
  void SetDataSet(const lepus::Value& data_set) {
    attribute_holder_->SetDataSet(data_set);
  }

  const EventMap& static_events() const {
    return attribute_holder_->static_events();
  }
  void SetStaticEvent(
      const base::String& type, const base::String& name,
      const std::vector<std::pair<base::String, lepus::Value>>& vec) {
    attribute_holder_->SetStaticEvent(type, name, vec);
  }
  void SetStaticEvent(const base::String& type, const base::String& name,
                      const base::String& value) {
    attribute_holder_->SetStaticEvent(type, name, value);
  }

  const EventMap& lepus_events() const {
    return attribute_holder_->lepus_events();
  }
  void SetLepusEvent(const base::String& type, const base::String& name,
                     const lepus::Value& script, const lepus::Value& func) {
    attribute_holder_->SetLepusEvent(type, name, script, func);
  }

  const EventMap& global_bind_events() const {
    return attribute_holder_->global_bind_events();
  }
  void SetWorkletEvent(const base::String& type, const base::String& name,
                       const lepus::Value& worklet_info, lepus::Context* ctx) {
    // TODO(luochangan.adrian): Add UI Worklet Event
    attribute_holder_->SetWorkletEvent(type, name, worklet_info, ctx);
  }

  const GestureMap& gesture_detectors() const {
    return attribute_holder_->gesture_detectors();
  }
  void SetGestureDetector(const uint32_t key, const GestureDetector& detector) {
    attribute_holder_->SetGestureDetector(key, detector);
  }
  void RemoveGestureDetector(const uint32_t key) {
    attribute_holder_->RemoveGestureDetector(key);
  }

  void SetInlineStyle(CSSPropertyID id, const base::String& string_value,
                      const CSSParserConfigs& configs) {
    has_dynamic_inline_style_ = true;
    SetStaticInlineStyle(id, string_value, configs);
  }
  void SetInlineStyle(CSSPropertyID id, base::String&& string_value,
                      const CSSParserConfigs& configs) {
    has_dynamic_inline_style_ = true;
    SetStaticInlineStyle(id, std::move(string_value), configs);
  }
  void SetInlineStyle(CSSPropertyID id, const tasm::CSSValue& value) {
    has_dynamic_inline_style_ = true;
    SetStaticInlineStyle(id, value);
  }
  void SetInlineStyle(CSSPropertyID id, tasm::CSSValue&& value) {
    has_dynamic_inline_style_ = true;
    SetStaticInlineStyle(id, std::move(value));
  }

  void SetStaticInlineStyle(CSSPropertyID id, const base::String& value,
                            const CSSParserConfigs& configs);

  void SetStaticInlineStyle(CSSPropertyID id, base::String&& string_value,
                            const CSSParserConfigs& configs);

  void SetStaticInlineStyle(CSSPropertyID id, const tasm::CSSValue& value);

  void SetStaticInlineStyle(CSSPropertyID id, tasm::CSSValue&& value);

  base::String id_selector() { return attribute_holder_->idSelector(); }
  void SetIdSelector(const base::String& idSelector) {
    attribute_holder_->SetIdSelector(idSelector);
  }

  // css_variables_map_
  const CSSVariableMap& css_variables_map() const {
    return attribute_holder_->css_variables_map();
  }
  void set_css_variables_map(const CSSVariableMap& css_variables) {
    attribute_holder_->set_css_variables_map(css_variables);
  }
  void set_css_variables_map(CSSVariableMap&& css_variables) {
    attribute_holder_->set_css_variables_map(std::move(css_variables));
  }

  const CSSVariableMap& css_variables_from_js() const {
    return attribute_holder_->css_variables_from_js();
  }

  const CSSVariableMap& css_variable_related() {
    return attribute_holder_->css_variable_related();
  }

  PseudoState pseudo_state() const {
    return attribute_holder_->GetPseudoState();
  }

  void SetPseudoState(PseudoState state) {
    attribute_holder_->SetPseudoState(state);
  }

  bool IsSSRAttrHolder() { return attribute_holder_->IsSSRAttrHolder(); }

  std::shared_ptr<AttributeHolder> attribute_holder() {
    return attribute_holder_;
  }

  void PresetInlineStyleMapCapacity(size_t count) {
    attribute_holder_->PresetInlineStyleMapCapacity(count);
  }

  void SetRawInlineStyle(CSSPropertyID id, const lepus::Value& value) {
    raw_inline_styles_.insert_or_assign(id, value);
  }

  void SetRawInlineStyle(CSSPropertyID id, lepus::Value&& value) {
    raw_inline_styles_.insert_or_assign(id, std::move(value));
  }

  const RawLepusStyleMap& raw_inline_styles() const {
    return raw_inline_styles_;
  }

  void SwapElement(const std::unique_ptr<RadonBase>&,
                   const DispatchOption&) override;

  void InsertElementIntoParent(Element* parent);

  Element* GetParentWithFixed(Element* parent_element);

  void MarkChildStyleDirtyRecursively(bool is_root) override;

#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event) override {
    RadonBase::UpdateTraceDebugInfo(event);
    if (!id_selector().empty()) {
      auto* idInfo = event->add_debug_annotations();
      idInfo->set_name("idSelector");
      idInfo->set_string_value(id_selector().str());
    }
    if (!classes().empty()) {
      std::string class_str = "";
      for (auto& aClass : classes()) {
        class_str = class_str + " " + aClass.str();
      }
      if (!class_str.empty()) {
        auto* classInfo = event->add_debug_annotations();
        classInfo->set_name("class");
        classInfo->set_string_value(class_str);
      }
    }
  }
#endif

 protected:
  bool CreateElementIfNeeded();
  virtual fml::RefPtr<Element> CreateFiberElement();
  virtual void DispatchFirstTime();
  virtual void OnDataSetChanged(){};
  virtual void OnSelectorChanged(){};

  RadonNodeIndexType GetOriginalNodeIndex();

  // TODO(wangyifei.20010605): rename it to data_model_;
  std::shared_ptr<AttributeHolder> attribute_holder_;
  fml::RefPtr<Element> element_;

 private:
  bool has_dynamic_class_{false};
  bool has_dynamic_inline_style_{false};
  bool has_dynamic_attr_{false};
  RawLepusStyleMap raw_inline_styles_{kCSSStyleMapFuzzyAllocationSize};
  friend class RadonElement;

  RadonNode* Sibling(int offset) const;
  bool DiffRawStyleForFiber(const RawLepusStyleMap& old_map,
                            const RawLepusStyleMap& new_map);
  bool DiffAttrMapForFiber(const AttrMap& old_map, const AttrMap& new_map);
  bool DiffStyleImpl(const StyleMap& old_map, const StyleMap& new_map,
                     bool check_remove);
  void UpdateInlineStylesFromOldModel(AttributeHolder* const old_data_model);

  bool HydrateNode(const DispatchOption& option);

  StyleMap cached_styles_;

  bool has_external_class_{false};

  bool id_dirty_{false};
  bool class_dirty_{false};

  bool need_transmit_class_dirty_{false};
  bool css_variables_changed_{false};
  bool force_calc_new_style_{true};
  // Used for CSS invalidation
  bool style_invalidated_ = false;
  ClassTransmitOption class_transmit_option_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_NODE_H_
