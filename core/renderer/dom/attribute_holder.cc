// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/attribute_holder.h"

#include <algorithm>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/selector/matching/attribute_selector_matching.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

void AttributeHolder::PresetInlineStyleMapCapacity(size_t count) {
  inline_styles_.reserve(count);
}

bool AttributeHolder::ContainsIdSelector(const std::string& selector) const {
  return idSelector().str() == selector;
}

bool AttributeHolder::ContainsClassSelector(const std::string& selector) const {
  for (const auto& c : classes()) {
    if (c.str() == selector) {
      return true;
    }
  }
  return false;
}

bool AttributeHolder::ContainsTagSelector(const std::string& selector) const {
  return tag().str() == selector;
}

// "[attribute]" or "[attribute=value]"
bool AttributeHolder::ContainsAttributeSelector(
    const std::string& selector) const {
  return AttributeSelectorMatching::Matches(selector, *this);
}

bool AttributeHolder::ContainsSelector(const std::string& selector) const {
  if (selector.empty()) {
    return false;
  }

  for (auto begin = selector.cbegin(); begin != selector.cend();) {
    char type = *begin;
    auto end = std::find_if(std::next(begin), selector.cend(), [type](char c) {
      if (type == '[') {
        return c == ']';
      } else {
        return c == '#' || c == '.' || c == '[';
      }
    });
    if (type == '[' && end != selector.cend()) {
      ++end;
    }
    bool skip_first_char = type == '#' || type == '.';
    std::string single_selector;
    std::copy(std::next(begin, skip_first_char), end,
              std::back_inserter(single_selector));

    bool result;
    switch (type) {
      case '#': {
        result = ContainsIdSelector(single_selector);
        break;
      }
      case '.': {
        result = ContainsClassSelector(single_selector);
        break;
      }
      case '[': {
        result = ContainsAttributeSelector(single_selector);
        break;
      }
      default: {
        result = ContainsTagSelector(single_selector);
        break;
      }
    }

    if (!result) {
      return false;
    }
    begin = end;
  }

  return true;
}

void AttributeHolder::UpdateCSSVariableFromSetProperty(
    const base::String& key, const base::String& value) {
  css_variables_from_js_[key] = value;
  OnStyleChange();
}

void AttributeHolder::MergeWithCSSVariables(
    lepus::Value& css_variable_updated) {
  if (css_variable_updated.IsTable()) {
    // in most cases, node does not has its own variables, clone only if needed.
    if (!(css_variables_from_js_.empty() && css_variables_.empty())) {
      css_variable_updated = lepus::Value::Clone(css_variable_updated);
    }
    auto& css_variable_updated_table = *css_variable_updated.Table();
    for (auto& pair : css_variable_updated_table) {
      auto it = css_variables_from_js_.find(pair.first);
      if (it != css_variables_from_js_.end()) {
        pair.second.SetString(it->second);
        continue;
      }

      it = css_variables_.find(pair.first);
      if (it != css_variables_.end()) {
        pair.second.SetString(it->second);
      }
    }
  }
};

base::String AttributeHolder::GetCSSVariableValue(
    const base::String& key) const {
  const AttributeHolder* base = this;
  while (base != nullptr) {
    base::String value;
    auto it = base->css_variables_from_js_.find(key);
    if (it != base->css_variables_from_js_.end()) {
      return value = it->second;
    }
    it = base->css_variables_.find(key);
    if (it != base->css_variables_.end()) {
      return value = it->second;
    }
    base = static_cast<AttributeHolder*>(base->HolderParent());
  }
  return base::String();
}

void AttributeHolder::Reset() {
  classes_.clear();
  inline_styles_.clear();
  attributes_.clear();
  data_set_.clear();
  static_events_.clear();
  lepus_events_.clear();
  id_selector_ = base::String();
  OnStyleChange();
}

void AttributeHolder::SetDataSet(const lepus::Value& data_set) {
  ForEachLepusValue(data_set,
                    [this](const lepus::Value& key, const lepus::Value& val) {
                      data_set_[key.String()] = val;
                    });
}

void AttributeHolder::RemoveEvent(const base::String& name,
                                  const base::String& type) {
  if (type == kGlobalBind) {
    global_bind_events_.erase(name);
  } else {
    static_events_.erase(name);
  }
}

void AttributeHolder::RemoveAllEvents() {
  static_events_.clear();
  lepus_events_.clear();
  global_bind_events_.clear();
}

css::StyleNode* AttributeHolder::SelectorMatchingParent() const {
  if (!element_ || (element_ && element_->IsRadonArch())) {  // Radon mode
    if (!GetRemoveDescendantSelectorScope() && IsComponent()) {
      // Descendant selector only works in current component scope
      return nullptr;
    }
    return HolderParent();
  }
  if (!element_->parent()) {
    return nullptr;
  }
  DCHECK(element_->is_fiber_element());
  // We know the element is fiber element,
  // descendant selector only works in current component scope
  if (static_cast<FiberElement*>(element_)->is_component() &&
      !element_->element_manager()->GetRemoveDescendantSelectorScope()) {
    return nullptr;
  }
  return element_->parent()->data_model();
}

css::StyleNode* AttributeHolder::HolderParent() const {
  if (!element_ || !element_->parent()) {
    return nullptr;
  }
  return element_->parent()->data_model();
}

css::StyleNode* AttributeHolder::NextSibling() const {
  if (!element_) {
    return nullptr;
  }
  if (auto* sibling = element_->next_sibling()) {
    return sibling->data_model();
  }
  return nullptr;
}

css::StyleNode* AttributeHolder::PreviousSibling() const {
  if (!element_) {
    return nullptr;
  }
  if (auto* sibling = element_->previous_sibling()) {
    return sibling->data_model();
  }
  return nullptr;
}

size_t AttributeHolder::ChildCount() const {
  if (!element_) {
    return 0;
  }
  return element_->GetChildCount();
}

}  // namespace tasm
}  // namespace lynx
