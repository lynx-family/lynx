// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ATTRIBUTE_HOLDER_H_
#define CORE_RENDERER_DOM_ATTRIBUTE_HOLDER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/style_node.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/events/events.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class Element;
class RadonNode;

template <class T>
void MapInsertOrAssign(T& map, const typename T::key_type& key,
                       const typename T::mapped_type& value) {
  if (auto result = map.emplace(key, value); !result.second) {
    result.first->second = value;
  }
}

template <class T>
void MapInsertOrAssign(T& map, const typename T::key_type& key,
                       typename T::mapped_type&& value) {
  if (auto result = map.try_emplace(key, std::move(value)); !result.second) {
    result.first->second = std::move(value);
  }
}

class AttributeHolder : public css::StyleNode {
 public:
  AttributeHolder(Element* element = nullptr)
      : pseudo_element_owner_(nullptr),
        element_(element),
        ssr_attribute_holder_{false} {}

  AttributeHolder(const AttributeHolder& holder)
      : classes_{holder.classes_},
        inline_styles_{holder.inline_styles_},
        attributes_{holder.attributes_},
        data_set_{holder.data_set_},
        id_selector_{holder.id_selector_},
        pseudo_state_{holder.pseudo_state_},
        pseudo_element_owner_{holder.pseudo_element_owner_},
        element_(holder.element_),
        radon_node_ptr_(holder.radon_node_ptr_),
        ssr_attribute_holder_{holder.ssr_attribute_holder_} {
    for (auto& static_event : holder.static_events_) {
      SetStaticEvent(static_event.second->type(), static_event.second->name(),
                     static_event.second->function());
    }
  }

  virtual ~AttributeHolder() = default;

  void OnStyleChange() override;

  void AddClass(const base::String& clazz) {
    classes_.push_back(clazz);
    OnStyleChange();
  }

  // Compatible code, using AttributeHolder::AddClass instead
  void SetClass(const base::String& clazz) { AddClass(clazz); }

  void SetClasses(ClassList&& classes) {
    classes_ = std::move(classes);
    OnStyleChange();
  }

  void RemoveAllClass() {
    classes_.clear();
    OnStyleChange();
  }

  void SetInlineStyles(StyleMap&& inline_styles) {
    inline_styles_ = std::move(inline_styles);
    OnStyleChange();
  }

  void SetInlineStyle(CSSPropertyID id, const base::String& value,
                      const CSSParserConfigs& configs) {
    UnitHandler::Process(id, lepus::Value(value), inline_styles_, configs);
    OnStyleChange();
  }

  void SetInlineStyle(CSSPropertyID id, base::String&& string_value,
                      const CSSParserConfigs& configs) {
    UnitHandler::Process(id, lepus::Value(std::move(string_value)),
                         inline_styles_, configs);
    OnStyleChange();
  }

  void SetInlineStyle(CSSPropertyID id, const tasm::CSSValue& value) {
    inline_styles_.insert_or_assign(id, value);
    OnStyleChange();
  }

  void SetInlineStyle(CSSPropertyID id, tasm::CSSValue&& value) {
    inline_styles_.insert_or_assign(id, std::move(value));
    OnStyleChange();
  }

  void ResetInlineStyle(CSSPropertyID id) {
    inline_styles_.erase(id);
    OnStyleChange();
  }

  void SetStaticClass(const base::String& clazz) {
    classes_.push_back(clazz);
    OnStyleChange();
  }

  void SetStaticAttribute(const base::String& key, const lepus::Value& value) {
    // Specific optimization
    // attributes_[key] = {value, false};
    if (auto result = attributes_.try_emplace(key, value); !result.second) {
      result.first->second = value;
    }
  }

  void SetStaticAttribute(const base::String& key, lepus::Value&& value) {
    // Specific optimization
    // attributes_[key] = {std::move(value), false};
    if (auto result = attributes_.try_emplace(key, std::move(value));
        !result.second) {
      result.first->second = std::move(value);
    }
  }

  void RemoveAttribute(const base::String& key) { attributes_.erase(key); }

  void SetDataSet(const base::String& key, const lepus::Value& value) {
    // Specific optimization
    // data_set_[key] = value;
    MapInsertOrAssign(data_set_, key, value);
  }

  void SetDataSet(const lepus::Value& data_set);

  // Update CSSVariable From Render.
  void UpdateCSSVariable(const base::String& key, const base::String& value,
                         CSSVariableMap* changed_css_vars = nullptr) {
    auto it = css_variables_.find(key);
    if (it == css_variables_.end() || !it->second.IsEqual(value)) {
      css_variables_.insert_or_assign(key, value);
      if (changed_css_vars != nullptr) {
        changed_css_vars->insert_or_assign(key, value);
      }
    }
  }

  // Update CSSVariable From JS SetProperty.
  void UpdateCSSVariableFromSetProperty(const base::String& key,
                                        const base::String& value);

  // For Element Api
  void MergeWithCSSVariables(lepus::Value& css_variable_updated);

  void SetStaticEvent(const base::String& type, const base::String& name,
                      const base::String& value) {
    if (type == kGlobalBind) {
      MapInsertOrAssign(global_bind_events_, name,
                        std::make_unique<EventHandler>(type, name, value));
    } else {
      MapInsertOrAssign(static_events_, name,
                        std::make_unique<EventHandler>(type, name, value));
    }
  }

  // set gesture detector to map
  void SetGestureDetector(const uint32_t key, const GestureDetector& detector) {
    MapInsertOrAssign(gesture_detectors_, key,
                      std::make_unique<GestureDetector>(detector));
  }

  // remove gesture detector from map
  void RemoveGestureDetector(const uint32_t key) {
    gesture_detectors_.erase(key);
  }

  const GestureMap& gesture_detectors() const { return gesture_detectors_; }

  // constructor for ssr server events
  void SetStaticEvent(
      const base::String& type, const base::String& name,
      const std::vector<std::pair<base::String, lepus::Value>>& vec) {
    std::vector<PiperEventContent> piper_event_vec;
    for (auto& iter : vec) {
      auto an_event = PiperEventContent(iter.first, iter.second);
      piper_event_vec.push_back(an_event);
    }
    if (type == kGlobalBind) {
      MapInsertOrAssign(
          global_bind_events_, name,
          std::make_unique<EventHandler>(type, name, piper_event_vec));
    } else {
      MapInsertOrAssign(
          static_events_, name,
          std::make_unique<EventHandler>(type, name, piper_event_vec));
    }
  }

  void SetLepusEvent(const base::String& type, const base::String& name,
                     const lepus::Value& script, const lepus::Value& func) {
    if (type == kGlobalBind) {
      MapInsertOrAssign(
          global_bind_events_, name,
          std::make_unique<EventHandler>(type, name, script, func));
    } else {
      MapInsertOrAssign(
          static_events_, name,
          std::make_unique<EventHandler>(type, name, script, func));
    }
  }

  void SetWorkletEvent(const base::String& type, const base::String& name,
                       const lepus::Value& worklet_info, lepus::Context* ctx) {
    // TODO(luochangan.adrian): Add UI Worklet Event
    if (type == kGlobalBind) {
      MapInsertOrAssign(
          global_bind_events_, name,
          std::make_unique<EventHandler>(type, name, worklet_info, ctx));
    } else {
      MapInsertOrAssign(
          lepus_events_, name,
          std::make_unique<EventHandler>(type, name, worklet_info, ctx));
    }
  }

  void RemoveEvent(const base::String& name, const base::String& type);
  void RemoveAllEvents();

  void SetIdSelector(const base::String& idSelector) {
    id_selector_ = idSelector;
    lepus::Value selector(idSelector);
    if (auto result = attributes_.try_emplace(
            BASE_STATIC_STRING(kIdSelectorAttrName), std::move(selector));
        !result.second) {
      result.first->second = std::move(selector);
    }
  }

  const base::String& idSelector() const override { return id_selector_; }

  const StyleMap& inline_styles() const { return inline_styles_; }

  StyleMap& MutableInlineStyles() { return inline_styles_; }

  const AttrMap& attributes() const { return attributes_; }

  AttrMap& attributes() { return attributes_; }

  const DataMap& dataset() const { return data_set_; }

  void set_css_variables_map(const CSSVariableMap& css_variables) {
    css_variables_ = css_variables;
  }

  void set_css_variables_map(CSSVariableMap&& css_variables) {
    css_variables_ = std::move(css_variables);
  }

  const CSSVariableMap& css_variables_map() const { return css_variables_; }

  const CSSVariableMap& css_variables_from_js() const {
    return css_variables_from_js_;
  }

  void AddCSSVariableRelated(const base::String& key,
                             const base::String& value) {
    css_variable_related_.insert_or_assign(key, value);
  }

  const CSSVariableMap& css_variable_related() { return css_variable_related_; }

  // GetCSSVariableValue.
  // variable_from_js first. css_variable_ from comes second.
  base::String GetCSSVariableValue(const base::String& key) const;

  ClassList ReleaseClasses() { return std::move(classes_); }

  StyleMap ReleaseInlineStyles() { return std::move(inline_styles_); }

  const ClassList& classes() const override { return classes_; }

  bool HasClass(const std::string& cls) const {
    return std::find_if(classes_.begin(), classes_.end(), [&cls](auto& s) {
             return s.str() == cls;
           }) != std::end(classes_);
  }

  const EventMap& static_events() const { return static_events_; }
  const EventMap& lepus_events() const { return lepus_events_; }
  const EventMap& global_bind_events() const { return global_bind_events_; }

  void PresetInlineStyleMapCapacity(size_t count);

  bool ContainsSelector(const std::string& selector) const;

  void Reset();

  void set_tag(const base::String& name) { tag_ = name; }

  virtual const base::String& tag() const override { return tag_; };

  virtual css::StyleNode* SelectorMatchingParent() const override;
  virtual css::StyleNode* HolderParent() const override;
  virtual css::StyleNode* NextSibling() const override;
  virtual css::StyleNode* PreviousSibling() const override;
  virtual css::StyleNode* PseudoElementOwner() const override {
    return pseudo_element_owner_;
  }

  virtual size_t ChildCount() const;
  virtual CSSFragment* ParentStyleSheet() const;

  void SetPseudoElementOwner(AttributeHolder* owner) {
    pseudo_element_owner_ = owner;
  }

  virtual CSSFragment* GetPageStyleSheet() { return nullptr; }

  bool GetRemoveCSSScopeEnabled() const;
  bool GetCascadePseudoEnabled() const;
  bool GetRemoveDescendantSelectorScope() const;
  bool IsComponent() const;

  void CloneAttributes(const AttributeHolder& src) {
    this->classes_ = src.classes_;
    this->inline_styles_ = src.inline_styles_;
    this->attributes_ = src.attributes_;
    this->data_set_ = src.data_set_;
    this->id_selector_ = src.id_selector_;
    this->css_variables_ = src.css_variables_;
  }

  static constexpr const char kIdSelectorAttrName[] = "idSelector";

  void OnPseudoStateChanged(PseudoState, PseudoState);

  void SetPseudoState(PseudoState state) {
    // If pseudo_state_ == state, which means the
    // PseudoState not change, return.
    if (pseudo_state_ == state) return;
    PseudoState old = pseudo_state_;
    pseudo_state_ = state;
    OnPseudoStateChanged(old, pseudo_state_);
  }

  void raw_set_pseudo_state(PseudoState state) { pseudo_state_ = state; }

  void AddPseudoState(PseudoState state) {
    PseudoState old = pseudo_state_;
    pseudo_state_ |= state;
    OnPseudoStateChanged(old, pseudo_state_);
  }

  void RemovePseudoState(PseudoState state) {
    PseudoState old = pseudo_state_;
    pseudo_state_ ^= state;
    OnPseudoStateChanged(old, pseudo_state_);
  }

  PseudoState GetPseudoState() const override { return pseudo_state_; }

  bool HasPseudoState(PseudoState type) const override {
    return pseudo_state_ & type;
  }

  bool HasID() const { return !id_selector_.empty(); };

  bool HasClass() const { return !classes_.empty(); }

  bool IsSSRAttrHolder() { return ssr_attribute_holder_; }

  void SetSSRAttrHolder(bool flag) { ssr_attribute_holder_ = flag; }

  static void CollectIdChangedInvalidation(CSSFragment*,
                                           css::InvalidationLists&,
                                           const std::string&,
                                           const std::string&);

  static void CollectClassChangedInvalidation(CSSFragment*,
                                              css::InvalidationLists&,
                                              const ClassList&,
                                              const ClassList&);

  static void CollectPseudoChangedInvalidation(CSSFragment*,
                                               css::InvalidationLists&,
                                               PseudoState, PseudoState);

  RadonNode* radon_node_ptr() { return radon_node_ptr_; }
  void set_radon_node_ptr(RadonNode* radon_node_ptr) {
    radon_node_ptr_ = radon_node_ptr;
  }

 protected:
  ClassList classes_;
  StyleMap inline_styles_{kCSSStyleMapFuzzyAllocationSize};
  AttrMap attributes_;
  DataMap data_set_;
  EventMap static_events_;
  EventMap lepus_events_;
  EventMap global_bind_events_;
  GestureMap gesture_detectors_;
  // Should be unique in component
  base::String id_selector_;

  // css variable definition on this node. such as:
  // `--bg-color: red`
  CSSVariableMap css_variables_;

  // css variable definition on this node that updated from JS. such as:
  // `background-color: var(--bg-color)`
  // this map will hold value like this:
  // `key: --bg-color value: red`
  CSSVariableMap css_variables_from_js_;

  // css variable related on this node, such as:
  // `background-color: var(--bg-color)`
  // this map will hold value like this:
  // `key: --bg-color value: red`
  CSSVariableMap css_variable_related_;

  // Record if is focused / hovered ...
  PseudoState pseudo_state_{kPseudoStateNone};
  base::String tag_;
  AttributeHolder* pseudo_element_owner_;
  // Reference the element for sibling and parent
  Element* element_;
  // Save path to trail Element to RadonNode.
  // TODO(wangyifei.20010605): Use a delegate class rather than
  // 'radon_node_ptr_'.
  RadonNode* radon_node_ptr_ = nullptr;

  bool ssr_attribute_holder_;

 public:
  bool ContainsIdSelector(const std::string& selector) const override;
  bool ContainsClassSelector(const std::string& selector) const override;
  bool ContainsTagSelector(const std::string& selector) const override;
  bool ContainsAttributeSelector(const std::string& selector) const;
  void SetElement(Element* element) { element_ = element; }
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ATTRIBUTE_HOLDER_H_
