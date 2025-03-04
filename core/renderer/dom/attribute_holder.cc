// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/attribute_holder.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/selector/matching/attribute_selector_matching.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

void AttributeHolder::OnStyleChange() {
  if (radon_node_ptr_) {
    radon_node_ptr_->OnStyleChange();
  }
}

void AttributeHolder::OnPseudoStateChanged(PseudoState prev, PseudoState curr) {
  if (radon_node_ptr_) {
    radon_node_ptr_->OnPseudoStateChanged(prev, curr);
  }
}

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

// RadonNode override this method, so only work in fiber mode
// TODO(wangyifei.20010605): Use a delegate class rather than 'radon_node_ptr'.
css::StyleNode* AttributeHolder::HolderParent() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->HolderParent();
  } else {
    if (!element_ || !element_->parent()) {
      return nullptr;
    }
    return element_->parent()->data_model();
  }
}

css::StyleNode* AttributeHolder::NextSibling() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->NextSibling();
  }
  if (!element_) {
    return nullptr;
  }
  if (auto* sibling = element_->next_sibling()) {
    return sibling->data_model();
  }
  return nullptr;
}

css::StyleNode* AttributeHolder::PreviousSibling() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->PreviousSibling();
  }
  if (!element_) {
    return nullptr;
  }
  if (auto* sibling = element_->previous_sibling()) {
    return sibling->data_model();
  }
  return nullptr;
}

CSSFragment* AttributeHolder::ParentStyleSheet() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->ParentStyleSheet();
  }
  return nullptr;
}

bool AttributeHolder::GetRemoveCSSScopeEnabled() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->GetRemoveCSSScopeEnabled();
  }
  return false;
}
bool AttributeHolder::GetCascadePseudoEnabled() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->GetCascadePseudoEnabled();
  }
  return false;
}
bool AttributeHolder::GetRemoveDescendantSelectorScope() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->GetRemoveDescendantSelectorScope();
  }
  return true;
}

bool AttributeHolder::IsComponent() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->IsComponent();
  }
  return false;
}

size_t AttributeHolder::ChildCount() const {
  if (!element_) {
    return 0;
  }
  return element_->GetChildCount();
}

void AttributeHolder::CollectIdChangedInvalidation(
    CSSFragment* style_sheet, css::InvalidationLists& lists,
    const std::string& old_id, const std::string& new_id) {
  // We know the style_sheet is not empty
  if (!old_id.empty()) style_sheet->CollectInvalidationSetsForId(lists, old_id);
  if (!new_id.empty()) style_sheet->CollectInvalidationSetsForId(lists, new_id);
}

void AttributeHolder::CollectClassChangedInvalidation(
    CSSFragment* style_sheet, css::InvalidationLists& lists,
    const ClassList& old_classes, const ClassList& new_classes) {
  if (old_classes.empty()) {
    for (auto& class_name : new_classes) {
      style_sheet->CollectInvalidationSetsForClass(lists, class_name.str());
    }
  } else {
    base::InlineVector<bool, ClassList::kInlinedSize> remaining_class_bits(
        old_classes.size());
    for (auto& class_name : new_classes) {
      bool found = false;
      for (unsigned j = 0; j < old_classes.size(); ++j) {
        if (class_name == old_classes[j]) {
          // Mark each class that is still in the newClasses, so we can skip
          // doing a n^2 search below when looking for removals. We can't
          // break from this loop early since a class can appear more than
          // once.
          remaining_class_bits[j] = true;
          found = true;
        }
      }
      // Class was added.
      if (!found) {
        style_sheet->CollectInvalidationSetsForClass(lists, class_name.str());
      }
    }

    for (unsigned i = 0; i < old_classes.size(); ++i) {
      if (remaining_class_bits[i]) continue;
      // Class was removed.
      style_sheet->CollectInvalidationSetsForClass(lists, old_classes[i].str());
    }
  }
}

void AttributeHolder::CollectPseudoChangedInvalidation(
    CSSFragment* style_sheet, css::InvalidationLists& lists, PseudoState prev,
    PseudoState curr) {
  if ((prev ^ curr) & kPseudoStateFocus) {
    style_sheet->CollectInvalidationSetsForPseudoClass(
        lists, css::LynxCSSSelector::kPseudoFocus);
  }
  if ((prev ^ curr) & kPseudoStateActive) {
    style_sheet->CollectInvalidationSetsForPseudoClass(
        lists, css::LynxCSSSelector::kPseudoActive);
  }
  if ((prev ^ curr) & kPseudoStateHover) {
    style_sheet->CollectInvalidationSetsForPseudoClass(
        lists, css::LynxCSSSelector::kPseudoHover);
  }
}

}  // namespace tasm
}  // namespace lynx
