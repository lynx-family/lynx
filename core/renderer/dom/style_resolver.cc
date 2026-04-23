// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/style_resolver.h"

#include <utility>
#include <vector>

#include "base/include/algorithm.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/services/feature_count/global_feature_counter.h"

namespace lynx {
namespace tasm {

namespace {
inline std::string MergeCSSSelector(const std::string& lhs,
                                    const std::string& rhs) {
  return lhs + rhs;
}

inline std::string GetClassSelectorRule(const base::String& clazz) {
  return "." + clazz.str();
}

inline std::string GetClassSelectorRule(const std::string& clazz) {
  return "." + clazz;
}

inline std::string GetIDSelectorRule(const base::String& value) {
  return "#" + value.str();
}

inline std::string GetIDSelectorRule(const std::string& value) {
  return "#" + value;
}
}  // namespace

thread_local StyleResolver::MatchedVector<const StyleMap*>
    StyleResolver::matched_style_map;
thread_local StyleResolver::MatchedVector<const StyleMap*>
    StyleResolver::matched_important_style_map;
thread_local StyleResolver::MatchedVector<const CSSVariableMap*>
    StyleResolver::matched_variable_map;

Element* StyleResolver::element() const {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
  return reinterpret_cast<Element*>(
      reinterpret_cast<uintptr_t>(this) -
      offsetof(Element, style_resolver_));  // NOLINT
#pragma GCC diagnostic pop
}

/**
 * @brief Handle the case where an old style object is removed.
 *
 * @param old_ptr Pointer to the old style object array.
 */
static void HandleRemovedStyleObject(style::StyleObject**& old_ptr,
                                     style::SimpleStyleNode* element) {
  if (old_ptr && *old_ptr) {
    (*old_ptr)->ResetStylesInElement(element);
    ++old_ptr;
  }
}

/**
 * @brief Handle the case where a new style object is added.
 *
 * @param new_ptr Pointer to the new style object array.
 */
static void HandleAddedStyleObject(style::StyleObject**& new_ptr,
                                   style::SimpleStyleNode* element) {
  if (new_ptr && *new_ptr) {
    (*new_ptr)->FromBinary();
    (*new_ptr)->BindToElement(element);
    element->UpdateSimpleStyles((*new_ptr)->Properties());
    ++new_ptr;
  }
}

/**
 * @brief Check if a style object exists in the remaining part of the old array.
 *
 * @param old_ptr Pointer to the old style object array.
 * @param new_obj Pointer to the new style object.
 * @return true if the new object is found in the old array later, false
 * otherwise.
 */
static bool IsNewObjectInOldArrayLater(style::StyleObject** old_ptr,
                                       style::StyleObject* new_obj) {
  style::StyleObject** temp_old_ptr = old_ptr;
  while (temp_old_ptr && *temp_old_ptr) {
    if (*temp_old_ptr == new_obj) {
      return true;
    }
    ++temp_old_ptr;
  }
  return false;
}

static bool HasStyleObjects(style::StyleObject** style_objects) {
  return style_objects != nullptr && *style_objects != nullptr;
}

static bool HasStyleObject(style::StyleObject* style_object) {
  return style_object != nullptr;
}

static tasm::StyleMap ResolveStyleObjectProperties(
    style::StyleObject** style_objects) {
  tasm::StyleMap resolved_property;
  if (!HasStyleObjects(style_objects)) {
    return resolved_property;
  }

  for (auto** it = style_objects; *it; ++it) {
    (*it)->FromBinary();
    resolved_property.merge((*it)->Properties());
  }
  return resolved_property;
}

static tasm::StyleMap ResolveDynamicStyleObjectProperties(
    style::StyleObject* style_object) {
  tasm::StyleMap resolved_property;
  if (!HasStyleObject(style_object)) {
    return resolved_property;
  }

  style_object->FromBinary();
  for (const auto& [property_id, value] : style_object->Properties()) {
    if (!value.IsEmpty()) {
      resolved_property.insert_or_assign(property_id, value);
      continue;
    }

    // Non-empty shorthand values have already been expanded during parse. An
    // empty value reaches here only as a dynamic reset tombstone, so shorthand
    // ids must be expanded now to keep the resolved dynamic map canonical.
    size_t count = 0;
    if (const auto* property_ids =
            CSSProperty::GetExpandedLonghands(property_id, &count);
        property_ids != nullptr) {
      for (size_t index = 0; index < count; ++index) {
        resolved_property.insert_or_assign(property_ids[index], CSSValue());
      }
      continue;
    }

    resolved_property.insert_or_assign(property_id, CSSValue());
  }
  return resolved_property;
}

static bool ContainsResolvedProperty(const tasm::StyleMap& static_styles,
                                     const tasm::StyleMap& dynamic_styles,
                                     tasm::CSSPropertyID property_id) {
  return dynamic_styles.contains(property_id) ||
         static_styles.contains(property_id);
}

static void ResetRemovedEffectiveProperties(
    const tasm::StyleMap& old_static_styles,
    const tasm::StyleMap* old_dynamic_styles,
    const tasm::StyleMap& new_static_styles,
    const tasm::StyleMap& new_dynamic_styles, style::SimpleStyleNode* target) {
  // Handle static reset(to default value)
  for (const auto& [property_id, value] : old_static_styles) {
    if (!ContainsResolvedProperty(new_static_styles, new_dynamic_styles,
                                  property_id)) {
      target->ResetSimpleStyle(property_id);
    }
  }

  if (!old_dynamic_styles) {
    return;
  }
  // Handle dynamic reset(to default value)
  for (const auto& [property_id, value] : *old_dynamic_styles) {
    // Skip reset again because we have done this.
    if (old_static_styles.contains(property_id)) {
      continue;
    }
    if (!ContainsResolvedProperty(new_static_styles, new_dynamic_styles,
                                  property_id)) {
      target->ResetSimpleStyle(property_id);
    }
  }
}

static void ResetRemovedDynamicProperties(
    const tasm::StyleMap* old_dynamic_styles,
    const tasm::StyleMap& new_dynamic_styles,
    const tasm::StyleMap& base_static_styles, style::SimpleStyleNode* target) {
  if (!old_dynamic_styles) {
    return;
  }

  for (const auto& [property_id, value] : *old_dynamic_styles) {
    if (new_dynamic_styles.contains(property_id)) {
      continue;
    }

    // Check if we need to reset dynamic value to base value or default value.
    const auto it = base_static_styles.find(property_id);
    if (it != base_static_styles.end()) {
      target->ResetSimpleStyle(property_id, it->second);
    } else {
      target->ResetSimpleStyle(property_id);
    }
  }
}

void StyleResolver::ResolveStyleObjects(style::StyleObject** old_ptr,
                                        style::StyleObject** new_ptr,
                                        style::SimpleStyleNode* target) {
  // Continue as long as there are elements in either the old or new list
  while ((old_ptr && *old_ptr) || (new_ptr && *new_ptr)) {
    // Case 1: New list is exhausted, so remaining old styles are removed.
    if (!new_ptr || !(*new_ptr)) {
      HandleRemovedStyleObject(old_ptr, target);
      // Case 2: Old list is exhausted, so remaining new styles are added.
    } else if (!old_ptr || !(*old_ptr)) {
      HandleAddedStyleObject(new_ptr, target);
      // Case 3: Both lists have elements, and they are the same.
    } else if (*old_ptr == *new_ptr) {
      // Elements match, move both pointers
      ++old_ptr;
      ++new_ptr;
      // Case 4: Both lists have elements, but they are different.
    } else {
      // Check if the current new style object exists later in the old list.
      // If it does, it means the current old style object was removed.
      if (IsNewObjectInOldArrayLater(old_ptr, *new_ptr)) {
        HandleRemovedStyleObject(old_ptr, target);
        // Otherwise, the current new style object is a new addition.
      } else {
        while ((*old_ptr) != nullptr) {
          HandleRemovedStyleObject(old_ptr, target);
        }
        HandleAddedStyleObject(new_ptr, target);
      }
    }
  }
}

void StyleResolver::ResolveStyleObjectsBasedOnExistingMap(
    const tasm::StyleMap& old_dcl_style, style::StyleObject** new_ptr,
    style::SimpleStyleNode* target) {
  // Early return if no new style objects and no existing styles
  if (!new_ptr && old_dcl_style.empty()) {
    return;
  }

  // Reserve space to avoid reallocations - estimate based on old + new
  // properties
  tasm::StyleMap resolved_property;
  const size_t estimated_size = old_dcl_style.size() + (new_ptr ? 8 : 0);
  resolved_property.reserve(estimated_size);

  // Merge all properties from new style objects
  if (new_ptr) {
    for (auto** it = new_ptr; *it; ++it) {
      (*it)->FromBinary();
      resolved_property.merge((*it)->Properties());
    }
  }

  // Update target only if we have resolved properties
  if (!resolved_property.empty()) {
    // Update to new style object.
    // Reset any properties from old_dcl_style that don't exist in the new
    // styles
    for (const auto& [property_id, value] : old_dcl_style) {
      if (!resolved_property.contains(property_id)) {
        target->ResetSimpleStyle(property_id);
      }
    }

    target->UpdateSimpleStyles(std::move(resolved_property));

  } else {
    // Reset every styles, since the new styleObject array is empty.
    for (const auto& [property_id, value] : old_dcl_style) {
      target->ResetSimpleStyle(property_id);
    }
    // Keep this empty update so the reset-only path still goes through the
    // normal flush/prop-bundle tail in UpdateSimpleStyles(...).
    target->UpdateSimpleStyles(tasm::StyleMap{});
  }
}

void StyleResolver::ResolveStyleObjectsBasedOnExistingMap(
    const tasm::StyleMap& old_static_style, style::StyleObject** new_static_ptr,
    const tasm::StyleMap* old_dynamic_style,
    style::StyleObject* new_dynamic_obj, bool static_dirty, bool dynamic_dirty,
    style::SimpleStyleNode* target) {
  const bool has_dynamic_layer =
      (old_dynamic_style != nullptr && !old_dynamic_style->empty()) ||
      HasStyleObject(new_dynamic_obj);

  if (static_dirty && !dynamic_dirty && !has_dynamic_layer) {
    // Exact old fast path: no dynamic layer exists, so keep the historical
    // static-only behavior untouched.
    ResolveStyleObjectsBasedOnExistingMap(old_static_style, new_static_ptr,
                                          target);
    return;
  }

  if (static_dirty) {
    // Static changed => resolve both layers. Dynamic may be unchanged, but it
    // still needs to be re-applied on top of the new static/base result.
    tasm::StyleMap new_static_styles =
        ResolveStyleObjectProperties(new_static_ptr);
    tasm::StyleMap new_dynamic_styles =
        ResolveDynamicStyleObjectProperties(new_dynamic_obj);

    if (!dynamic_dirty && new_dynamic_styles.empty() && old_dynamic_style &&
        !old_dynamic_style->empty()) {
      // No new dynamic style change, old_dynamic_style == new_dynamic_styles
      new_dynamic_styles = *old_dynamic_style;
    }

    ResetRemovedEffectiveProperties(old_static_style, old_dynamic_style,
                                    new_static_styles, new_dynamic_styles,
                                    target);
    target->UpdateStaticAndDynamicSimpleStyles(std::move(new_static_styles),
                                               std::move(new_dynamic_styles));
    return;
  }

  if (!dynamic_dirty) {
    return;
  }

  // Static is unchanged here, so only resolve/diff the dynamic layer and let
  // removals fall back to the committed static/base map.
  tasm::StyleMap new_dynamic_styles =
      ResolveDynamicStyleObjectProperties(new_dynamic_obj);
  if ((old_dynamic_style == nullptr || old_dynamic_style->empty()) &&
      new_dynamic_styles.empty()) {
    return;
  }
  if (old_dynamic_style != nullptr &&
      *old_dynamic_style == new_dynamic_styles) {
    return;
  }
  ResetRemovedDynamicProperties(old_dynamic_style, new_dynamic_styles,
                                old_static_style, target);
  target->UpdateDynamicSimpleStyles(std::move(new_dynamic_styles));
}

ElementManager* StyleResolver::manager() const {
  return element()->element_manager();
}

void StyleResolver::ResolveStyle(StyleMap& result, CSSFragment* fragment,
                                 CSSVariableMap* changed_css_vars) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_PATCH_RESOLVE_STYLE);

  Element* element_ = element();

  if (element_->data_model() == nullptr) {
    LOGE("StyleResolver::ResolveStyle failed since data_model is null.");
    return;
  }

  if (!element_->WillResolveStyle(result, changed_css_vars)) {
    return;
  }

  StyleMap important_result;

  // Find the selectors that the current `Element` can match.
  if (fragment != nullptr) {
    if (fragment->enable_css_selector()) {
      GetCSSStyleNew(element_->data_model(), fragment);
    } else {
      GetCSSStyleForFiber(static_cast<FiberElement*>(element_), fragment);
    }
  }

  DidCollectMatchedRules(element_->data_model(), result, important_result,
                         changed_css_vars, element_->CountInlineStyles());

  HandleCSSVariables(result);
  HandleCSSVariables(important_result);

  // Inline styles may contain CSS variables that are not present in the other
  // styles. We need to re-resolve CSS variables if they are present in inline
  // styles.
  StyleMap important_inline_styles;
  if (element_->MergeInlineStyles(result, important_inline_styles)) {
    HandleCSSVariables(result);
    HandleCSSVariables(important_inline_styles);
  }
  if (!important_inline_styles.empty()) {
    important_result.merge(important_inline_styles);
  }
  if (!important_result.empty()) {
    result.merge(important_result);
  }
}

void StyleResolver::HandlePseudoElement(CSSFragment* fragment) {
  Element* element_ = element();
  if (!fragment) {
    return;
  }

  if (fragment->enable_css_selector()) {
    bool has_pseudo_rules =
        (fragment->rule_set() && !fragment->rule_set()->pseudo_rules().empty());
    if (!has_pseudo_rules) {
      ElementManager* em = manager();
      if (em) {
        for (const auto& wrapper : em->GetAdoptedStyleSheets()) {
          if (wrapper && wrapper->fragment_ &&
              wrapper->fragment_->enable_css_selector() &&
              wrapper->fragment_->rule_set() &&
              !wrapper->fragment_->rule_set()->pseudo_rules().empty()) {
            has_pseudo_rules = true;
            break;
          }
        }
      }
    }
    if (!has_pseudo_rules) {
      return;
    }
  } else if (fragment->pseudo_map().empty()) {
    return;
  }

  auto fiber_element = static_cast<FiberElement*>(element_);
  if (fiber_element->HasTextSelection() &&
      !fiber_element->is_inline_element()) {
    ResolvePseudoElement(kPseudoStateSelection, fragment, fiber_element,
                         kCSSSelectorSelection);
  }
  if (fiber_element->HasPlaceHolder()) {
    ResolvePseudoElement(kPseudoStatePlaceHolder, fragment, fiber_element,
                         kCSSSelectorPlaceholder);
  }
}

void StyleResolver::ResolvePseudoElement(PseudoState pseudo_state,
                                         CSSFragment* fragment,
                                         FiberElement* fiber_element,
                                         const char* pseudo_selector) {
  StyleMap result;
  StyleMap important_result;
  if (fragment->enable_css_selector()) {
    AttributeHolder attribute_holder;
    attribute_holder.AddPseudoState(pseudo_state);
    attribute_holder.SetPseudoElementOwner(fiber_element->data_model());
    GetCSSStyleNew(&attribute_holder, fragment);
    DidCollectMatchedRules(fiber_element->data_model(), result,
                           important_result);
  } else {
    ParsePseudoCSSTokensForFiber(fiber_element, fragment, pseudo_selector,
                                 result);
    // Note: old pseudo token path doesn't have important_attributes_ yet.
  }

  if (!result.empty()) {
    HandleCSSVariables(result);
  }
  if (!important_result.empty()) {
    HandleCSSVariables(important_result);
  }
  result.merge(important_result);
  fiber_element->PrepareOrUpdatePseudoElement(pseudo_state, result);
}

void StyleResolver::DidCollectMatchedRules(AttributeHolder* holder,
                                           StyleMap& result,
                                           StyleMap& important_result,
                                           CSSVariableMap* changed_css_vars,
                                           size_t base_reserving_size) {
  {
    auto& tls_matched_style_map = matched_style_map;
    auto& tls_matched_important_style_map = matched_important_style_map;

    size_t normal_reserve = base_reserving_size;
    size_t important_reserve = 0;
    for (auto matched_style_ptr : tls_matched_style_map) {
      normal_reserve += matched_style_ptr->size();
    }
    for (auto matched_style_ptr : tls_matched_important_style_map) {
      important_reserve += matched_style_ptr->size();
    }

    result.reserve(normal_reserve);
    important_result.reserve(important_reserve);

    for (auto matched_style_ptr : tls_matched_style_map) {
      result.merge(*matched_style_ptr);
    }
    tls_matched_style_map.clear();

    for (auto matched_style_ptr : tls_matched_important_style_map) {
      important_result.merge(*matched_style_ptr);
    }
    tls_matched_important_style_map.clear();
  }

  {
    auto& tls_matched_variable_map = matched_variable_map;
    // Early return if both matched_variable_map and holder's css_variable_map
    // are empty, no difference check is needed.
    if (tls_matched_variable_map.empty() &&
        holder->css_variables_map().empty()) {
      return;
    }

    // When CSSInlineVariables is enabled, use bulk update for better
    // performance and proper invalidation handling.
    if (element()->IsCSSInlineVariablesEnabled()) {
      // Merge all matched CSS variables into a single map
      // and let AttributeHolder handle the diff computation.
      size_t reserve_count = 0;
      for (auto variable_ptr : tls_matched_variable_map) {
        reserve_count += variable_ptr->size();
      }
      CSSVariableMap merged_vars;
      merged_vars.reserve(reserve_count);
      for (auto variable_ptr : tls_matched_variable_map) {
        for (const auto& [key, value] : *variable_ptr) {
          merged_vars.insert_or_assign(key, value);
        }
      }
      holder->UpdateCSSVariable(std::move(merged_vars), changed_css_vars);
    } else {
      // Legacy path: update variables one at a time
      // Note: Removal handling is only properly supported in the new bulk
      // update path with CSSInlineVariablesEnabled(). Legacy mode will be
      // deprecated.
      for (auto variable_ptr : tls_matched_variable_map) {
        for (const auto& [key, value] : *variable_ptr) {
          holder->UpdateCSSVariable(key, value, changed_css_vars);
        }
      }
    }
    tls_matched_variable_map.clear();
  }
}

void StyleResolver::HandleCSSVariables(StyleMap& styles) {
  Element* element_ = element();
  if (element_->data_model() == nullptr) {
    LOGE("StyleResolver::HandleCSSVariables failed since data_model is null.");
    return;
  }

  CSSVariableHandler handler(true);
  bool has_css_variable_in_style_map = false;
  bool is_css_inline_variables_enabled =
      element_->IsCSSInlineVariablesEnabled();
  if (element_->is_greedy_parallel_flush()) {
    has_css_variable_in_style_map = handler.HasCSSVariableInStyleMap(styles);
    if (has_css_variable_in_style_map ||
        (is_css_inline_variables_enabled &&
         handler.HasCSSVariableInHolder(element_->data_model()))) {
      // mark need refresh style in parallel flush with css variables in
      // StyleMap
      static_cast<FiberElement*>(element_)->MarkRefreshCSSStyles();
    }
  } else {
    if (is_css_inline_variables_enabled) {
      static_cast<FiberElement*>(element_)->CollectCustomProperties(
          element_->data_model());
    }

    has_css_variable_in_style_map = handler.HandleCSSVariables(
        styles, element_->data_model(), GetCSSParserConfigs());
  }

  if (has_css_variable_in_style_map || is_css_inline_variables_enabled) {
    element_->element_manager()->SetRequireCSSVariables(true);
  }
}

void StyleResolver::MergeHigherPriorityCSSStyle(const StyleMap& matched) {
  if (matched.empty()) {
    return;
  }
  matched_style_map.emplace_back(&matched);
}

void StyleResolver::MergeHigherPriorityImportantCSSStyle(
    const StyleMap& matched) {
  if (matched.empty()) {
    return;
  }
  matched_important_style_map.emplace_back(&matched);
}

void StyleResolver::MergeToken(CSSParseToken* token) {
  if (!token) {
    return;
  }
  MergeHigherPriorityCSSStyle(token->GetAttributes());
  MergeHigherPriorityImportantCSSStyle(token->GetImportantAttributes());
  SetCSSVariableToNode(token->GetStyleVariables());
}

void StyleResolver::SetCSSVariableToNode(const CSSVariableMap& matched) {
  if (matched.empty()) {
    return;
  }
  matched_variable_map.emplace_back(&matched);
}

static bool CompareRules(const css::MatchedRule& matched_rule1,
                         const css::MatchedRule& matched_rule2) {
  unsigned specificity1 = matched_rule1.Specificity();
  unsigned specificity2 = matched_rule2.Specificity();
  if (specificity1 != specificity2) return specificity1 < specificity2;

  return matched_rule1.Position() < matched_rule2.Position();
}

StyleResolver::MatchedVector<css::MatchedRule> StyleResolver::GetCSSMatchedRule(
    AttributeHolder* node, CSSFragment* style_sheet,
    const std::vector<fml::RefPtr<SharedCSSFragmentWrapper>>* adopted_sheets) {
  MatchedVector<css::MatchedRule> matched_rules;
  unsigned level = 0;
  if (style_sheet && style_sheet->rule_set()) {
    style_sheet->rule_set()->MatchStyles(node, level, matched_rules);
  }

  // Check for adopted stylesheets with higher cascade priority
  // The priority is ensured by the level. In the MatchStyles methods,
  // each rule set will internally increase it. Thus the later rule_set that
  // execute the match will have higher priority than the former ones.
  if (adopted_sheets) {
    for (const auto& wrapper : *adopted_sheets) {
      if (wrapper && wrapper->fragment_ &&
          wrapper->fragment_->enable_css_selector()) {
        auto* adopted_style_sheet = wrapper->fragment_.get();
        if (adopted_style_sheet->rule_set()) {
          adopted_style_sheet->rule_set()->MatchStyles(node, level,
                                                       matched_rules);
        }
      }
    }
  }

  base::InsertionSort(matched_rules.data(), matched_rules.size(), CompareRules);
  return matched_rules;
}

void StyleResolver::GetCSSStyleNew(AttributeHolder* node,
                                   CSSFragment* style_sheet) {
  // Then process regular styles
  ElementManager* element_manager = manager();
  const std::vector<fml::RefPtr<SharedCSSFragmentWrapper>>* adopted_sheets =
      element_manager ? &element_manager->GetAdoptedStyleSheets() : nullptr;
  auto matched_rules = GetCSSMatchedRule(node, style_sheet, adopted_sheets);

  for (const auto& matched : matched_rules) {
    if (matched.Data()->Rule()->Token() != nullptr) {
      auto* token = matched.Data()->Rule()->Token().get();
      MergeToken(token);
    }
  }
}

/**
   Preset Global :not() Styles
 */
void StyleResolver::PreSetGlobalPseudoNotCSS(
    CSSSheet::SheetType type, const std::string& rule,
    const std::unordered_map<int, PseudoClassStyleMap>& pseudo_not_global_map,
    CSSFragment* style_sheet, AttributeHolder* node) {
  // Determine if global :not() styles exist
  if (pseudo_not_global_map.size() > 0) {
    PseudoClassStyleMap pseudo_not_global;

    auto it = pseudo_not_global_map.find(type);
    if (it != pseudo_not_global_map.end()) {
      pseudo_not_global = it->second;
    }

    const CSSParserTokenMap& pseudo = style_sheet->pseudo_map();
    for (auto& it : pseudo_not_global) {
      bool is_need_use_pseudo_not_style = false;

      if (type == CSSSheet::CLASS_SELECT) {
        const auto& class_vector = node->classes();

        if (class_vector.size() == 0) {
          // If an element has no class, directly preset the content of the
          // global :not() class selector
          is_need_use_pseudo_not_style = true;
        } else {
          // when the scope of global :not() is class, iterate through classes
          // to check for target class match
          bool is_match_class = false;
          for (const auto& cls : class_vector) {
            const auto class_name = GetClassSelectorRule(cls);
            if (class_name == it.second.scope) {
              is_match_class = true;
              break;
            }
          }
          is_need_use_pseudo_not_style = !is_match_class;
        }
      } else {
        // when the scope of global :not() is tag/id selector, determine whether
        // to apply global :not() styles
        if (it.second.scope != rule || rule.empty()) {
          is_need_use_pseudo_not_style = true;
        }
      }

      if (is_need_use_pseudo_not_style) {
        auto it_pseudo_not = pseudo.find(it.first);
        if (it_pseudo_not != pseudo.end()) {
          MergeToken(it_pseudo_not->second.get());
        }
      }
    }
  }
}

void StyleResolver::ApplyPseudoNotCSSStyle(
    AttributeHolder* node, const PseudoClassStyleMap& pseudo_not_map,
    CSSFragment* style_sheet, const std::string& selector_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_PATCH_APPLY_PSEUDO_NOT_STYLE);
  for (const auto& it : pseudo_not_map) {
    const auto& pseudo_key = it.second.selector_key;
    if (selector_key == pseudo_key ||
        GetClassSelectorRule(selector_key) == pseudo_key ||
        GetIDSelectorRule(selector_key) == pseudo_key) {
      bool is_need_use_pseudo_not_style = false;
      if (it.second.scope_type == CSSSheet::NAME_SELECT) {
        if (it.second.scope != node->tag().str()) {
          is_need_use_pseudo_not_style = true;
        }
      } else if (it.second.scope_type == CSSSheet::CLASS_SELECT) {
        const auto& class_vector = node->classes();
        if (class_vector.size() == 0) {
          // When a node has no class and the scope of :not() is a class
          // selector, styles need to be applied.
          is_need_use_pseudo_not_style = true;
        }
        // Handle the case of .class1:not(.class2)
        bool is_match_class = false;
        for (const auto& cls : class_vector) {
          const auto class_name = GetClassSelectorRule(cls);
          if (class_name == it.second.scope && class_name != pseudo_key) {
            is_match_class = true;
            break;
          }
        }

        is_need_use_pseudo_not_style = !is_match_class;
      } else if (it.second.scope_type == CSSSheet::ID_SELECT) {
        if (it.second.scope != GetIDSelectorRule(node->idSelector())) {
          is_need_use_pseudo_not_style = true;
        }
      }

      if (is_need_use_pseudo_not_style) {
        std::string full_pseudo_key = it.first;
        auto it_pseudo_not = style_sheet->pseudo_map().find(full_pseudo_key);
        if (it_pseudo_not != style_sheet->pseudo_map().end()) {
          MergeToken(it_pseudo_not->second.get());
        }
      }
    }
  }
}

void StyleResolver::ApplyPseudoClassChildSelectorStyle(
    Element* current_node, CSSFragment* style_sheet,
    const std::string& selector_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              CSS_PATCH_APPLY_PSEUDO_CLASS_CHILD_SELECTOR_STYLE);
  if (current_node->IsFiberArch()) {
    // child selector is only supported in RadonArch.
    return;
  }
  const CSSParserTokenMap& child_pseudo = style_sheet->child_pseudo_map();
  if (child_pseudo.empty()) {
    return;
  }
  Element* parent = nullptr;
  parent = current_node->parent();
  if (!parent) {
    return;
  }
  ElementManager* manager_ = manager();
  for (const auto& it : child_pseudo) {
    if (it.second && it.second->IsPseudoStyleToken() &&
        it.first.compare(0, selector_key.size(), selector_key) == 0) {
      if (it.first.find(kCSSSelectorFirstChild) != std::string::npos) {
        if (current_node == parent->first_child()) {
          report::GlobalFeatureCounter::Count(
              report::LynxFeature::CPP_ENABLE_PSEUDO_CHILD_CSS,
              manager_->GetInstanceId());
          MergeToken(it.second.get());
        }
      }
      if (it.first.find(kCSSSelectorLastChild) != std::string::npos) {
        if (current_node == parent->last_child()) {
          report::GlobalFeatureCounter::Count(
              report::LynxFeature::CPP_ENABLE_PSEUDO_CHILD_CSS,
              manager_->GetInstanceId());
          MergeToken(it.second.get());
        }
      }
    }
  }
}

/*
 * Matching Algorithm:
 *    1. Based on the key, determine if there are any CSS properties.
 *    2. Match the parent node of the node according to the CSS sheet from right
 *       to left, and check if the node's class meets the conditions. For
 *       example: .a .text_hello {"font-size":"10px"} the CSS style exists in
 *       the map as .text_hello. First, find text_hello, then check if the
 *       parent node is a. If it satisfies the condition, return the CSS style.
 *       The selectors are stored as a linked list in css_parse_token within
 *       sheets_.
 */
void StyleResolver::GetCSSByRule(CSSSheet::SheetType type,
                                 CSSFragment* style_sheet,
                                 AttributeHolder* node,
                                 const std::string& rule) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_PATCH_GET_CSS_BY_RULE,
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("rule", rule);
              });
  CSSParseToken* token;
  switch (type) {
    case CSSSheet::ID_SELECT:
      token = style_sheet->GetIdStyle(rule);
      break;
    case CSSSheet::NAME_SELECT:
      token = style_sheet->GetTagStyle(rule);
      break;
    case CSSSheet::ALL_SELECT:
      token = style_sheet->GetUniversalStyle(rule);
      break;
    case CSSSheet::PLACEHOLDER_SELECT:
    case CSSSheet::FIRST_CHILD_SELECT:
    case CSSSheet::LAST_CHILD_SELECT:
    case CSSSheet::PSEUDO_FOCUS_SELECT:
    case CSSSheet::SELECTION_SELECT:
    case CSSSheet::PSEUDO_ACTIVE_SELECT:
    case CSSSheet::PSEUDO_HOVER_SELECT:
      token = style_sheet->GetPseudoStyle(rule);
      break;
    default:
      token = style_sheet->GetCSSStyle(rule);
  }

  MergeToken(token);

  if ((type == CSSSheet::CLASS_SELECT || type == CSSSheet::ID_SELECT) &&
      style_sheet->HasCascadeStyle()) {
    ApplyCascadeStyles(style_sheet, node, rule);
  }
}

void StyleResolver::MergeHigherCascadeStyles(
    const std::string& current_selector, const std::string& parent_selector,
    AttributeHolder* node, CSSFragment* style_sheet) {
  std::string integrated_selector =
      MergeCSSSelector(current_selector, parent_selector);
  CSSParseToken* token_parent =
      style_sheet->GetCascadeStyle(integrated_selector);
  MergeToken(token_parent);
}

void StyleResolver::ApplyCascadeStyles(CSSFragment* style_sheet,
                                       AttributeHolder* node,
                                       const std::string& rule) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_PATCH_APPLY_CASCADE_STYLES);
  if (node == nullptr) {
    return;
  }
  const AttributeHolder* node_parent =
      static_cast<AttributeHolder*>(node->HolderParent());
  while (node_parent != nullptr) {
    for (const auto& cls : node_parent->classes()) {
      MergeHigherCascadeStyles(rule, GetClassSelectorRule(cls), node,
                               style_sheet);
      // Support for nested focus pseudo class. This is a naive implementation
      // and should be replaced in the future.
      if (node->GetCascadePseudoEnabled() &&
          node_parent->HasPseudoState(kPseudoStateFocus)) {
        MergeHigherCascadeStyles(rule, GetClassSelectorRule(cls) + ":focus",
                                 node, style_sheet);
      }
    }
    // Current is component and has scope, end the loop
    if (!node_parent->GetRemoveDescendantSelectorScope() &&
        node_parent->IsComponent()) {
      break;
    }
    node_parent = static_cast<AttributeHolder*>(node_parent->HolderParent());
  }

  node_parent = static_cast<AttributeHolder*>(node->HolderParent());
  while (node_parent != nullptr) {
    const base::String& id_node = node_parent->idSelector();
    if (!id_node.empty()) {
      const std::string rule_id_selector = GetIDSelectorRule(id_node);
      MergeHigherCascadeStyles(rule, rule_id_selector, node, style_sheet);
      if (node->GetCascadePseudoEnabled() &&
          node_parent->HasPseudoState(kPseudoStateFocus)) {
        MergeHigherCascadeStyles(rule, rule_id_selector + ":focus", node,
                                 style_sheet);
      }
    }
    // Current is component and has scope, end the loop
    if (!node_parent->GetRemoveDescendantSelectorScope() &&
        node_parent->IsComponent()) {
      break;
    }
    node_parent = static_cast<AttributeHolder*>(node_parent->HolderParent());
  }
}

void StyleResolver::GetPseudoClassStyle(PseudoClassType pseudo_type,
                                        CSSFragment* style_sheet,
                                        AttributeHolder* node) {
  std::string pseudo_class_name;
  switch (pseudo_type) {
    case PseudoClassType::kFocus:
      pseudo_class_name = ":focus";
      break;
    case PseudoClassType::kHover:
      pseudo_class_name = ":hover";
      break;
    case PseudoClassType::kActive:
      pseudo_class_name = ":active";
      break;
    default:
      return;
  }

  GetCSSByRule(CSSSheet::PSEUDO_FOCUS_SELECT, style_sheet, node,
               pseudo_class_name);

  GetCSSByRule(CSSSheet::PSEUDO_FOCUS_SELECT, style_sheet, node,
               std::string("*") + pseudo_class_name);

  const base::String& tag_node = node->tag();
  if (!tag_node.empty()) {
    GetCSSByRule(CSSSheet::PSEUDO_FOCUS_SELECT, style_sheet, node,
                 tag_node.str() + pseudo_class_name);
  }

  for (const auto& cls : node->classes()) {
    const auto rule_class_selector = GetClassSelectorRule(cls);
    GetCSSByRule(CSSSheet::PSEUDO_FOCUS_SELECT, style_sheet, node,
                 rule_class_selector + pseudo_class_name);
  }

  if (!node->idSelector().empty()) {
    auto rule_name = GetIDSelectorRule(node->idSelector()) + pseudo_class_name;
    GetCSSByRule(CSSSheet::PSEUDO_FOCUS_SELECT, style_sheet, node, rule_name);
  }
}

void StyleResolver::GetCSSStyleForFiber(FiberElement* node,
                                        CSSFragment* style_sheet) {
  ElementManager* manager_ = manager();
  style_sheet->InitPseudoNotStyle();
  // If has_pseudo_not_style means the pseudo_not_style is not empty
  const auto has_pseudo_not_style = style_sheet->HasPseudoNotStyle();
  auto* holder = node->data_model();
  if (style_sheet->HasCSSStyle()) {
    // process "*" first
    CSSParseToken* token = style_sheet->GetCSSStyle("*");
    if (token) {
      MergeToken(token);
    }

    // Start by processing the tag selectors first
    const base::String& tag_node = holder->tag();
    if (!tag_node.empty()) {
      const std::string& rule_tag_selector = tag_node.str();
      if (has_pseudo_not_style) {
        PreSetGlobalPseudoNotCSS(
            CSSSheet::NAME_SELECT, rule_tag_selector,
            style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
            holder);
      }
      token = style_sheet->GetCSSStyle(rule_tag_selector);
      if (token) {
        MergeToken(token);
      }
      if (has_pseudo_not_style) {
        report::GlobalFeatureCounter::Count(
            report::LynxFeature::CPP_ENABLE_PSEUDO_NOT_CSS,
            manager_->GetInstanceId());
        ApplyPseudoNotCSSStyle(
            holder, style_sheet->pseudo_not_style().pseudo_not_for_tag,
            style_sheet, rule_tag_selector);
      }
      ApplyPseudoClassChildSelectorStyle(node, style_sheet, rule_tag_selector);
    }

    // Class selectors
    if (has_pseudo_not_style) {
      PreSetGlobalPseudoNotCSS(
          CSSSheet::CLASS_SELECT, "",
          style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
          holder);
    }
    for (const auto& cls : holder->classes()) {
      const std::string rule_class_selector = GetClassSelectorRule(cls);
      token = style_sheet->GetCSSStyle(rule_class_selector);
      if (token) {
        MergeToken(token);
      }
      ApplyCascadeStylesForFiber(style_sheet, node, rule_class_selector);
      if (has_pseudo_not_style) {
        report::GlobalFeatureCounter::Count(
            report::LynxFeature::CPP_ENABLE_PSEUDO_NOT_CSS,
            manager_->GetInstanceId());
        ApplyPseudoNotCSSStyle(
            holder, style_sheet->pseudo_not_style().pseudo_not_for_class,
            style_sheet, rule_class_selector);
      }
      ApplyPseudoClassChildSelectorStyle(node, style_sheet,
                                         rule_class_selector);
    }

    // handle pseudo state
    if (holder->HasPseudoState(kPseudoStateFocus)) {
      GetPseudoClassStyle(PseudoClassType::kFocus, style_sheet, holder);
    }

    if (holder->HasPseudoState(kPseudoStateHover)) {
      GetPseudoClassStyle(PseudoClassType::kHover, style_sheet, holder);
    }

    if (holder->HasPseudoState(kPseudoStateActive)) {
      GetPseudoClassStyle(PseudoClassType::kActive, style_sheet, holder);
    }

    // ID selector
    const base::String& id_node = holder->idSelector();
    if (!id_node.empty()) {
      const std::string rule_id_selector = GetIDSelectorRule(id_node);
      if (has_pseudo_not_style) {
        PreSetGlobalPseudoNotCSS(
            CSSSheet::ID_SELECT, rule_id_selector,
            style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
            holder);
      }
      token = style_sheet->GetCSSStyle(rule_id_selector);
      if (token) {
        MergeToken(token);
      }
      ApplyCascadeStylesForFiber(style_sheet, node, rule_id_selector);
      if (has_pseudo_not_style) {
        report::GlobalFeatureCounter::Count(
            report::LynxFeature::CPP_ENABLE_PSEUDO_NOT_CSS,
            manager_->GetInstanceId());
        ApplyPseudoNotCSSStyle(
            holder, style_sheet->pseudo_not_style().pseudo_not_for_id,
            style_sheet, rule_id_selector);
      }
      ApplyPseudoClassChildSelectorStyle(node, style_sheet, rule_id_selector);
    } else if (has_pseudo_not_style) {
      // if the node doesn't contains the id selector, then try to apply the id
      // selector form global :not() selector
      PreSetGlobalPseudoNotCSS(
          CSSSheet::ID_SELECT, "",
          style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
          holder);
    }
  }
}

void StyleResolver::ApplyCascadeStylesForFiber(CSSFragment* style_sheet,
                                               FiberElement* node,
                                               const std::string& rule) {
  // for descendant selector, we just find the parent class in current
  // component scope!
  if (style_sheet->HasCascadeStyle()) {
    FiberElement* node_parent = static_cast<FiberElement*>(node->parent());
    while (node_parent) {
      // TTML: all the element in the same scope
      // React:  decided by react runtime
      if (node->IsInSameCSSScope(node_parent) ||
          node->element_manager()->GetRemoveDescendantSelectorScope()) {
        // class descendant selector
        for (const auto& clazz : node_parent->data_model()->classes()) {
          MergeHigherCascadeStylesForFiber(rule, GetClassSelectorRule(clazz),
                                           node->data_model(), style_sheet);

          // NOTE: Support for nested focus pseudo class. This is a naive
          // implementation and should be replaced in the future.
          if (node->element_manager()->GetEnableCascadePseudo() &&
              node_parent->data_model()->HasPseudoState(kPseudoStateFocus)) {
            MergeHigherCascadeStylesForFiber(
                rule, GetClassSelectorRule(clazz) + ":focus",
                node->data_model(), style_sheet);
          }
        }
        // id descendant selector
        const auto& id_selector = node_parent->data_model()->idSelector();
        if (!id_selector.empty()) {
          const std::string rule_id_selector = GetIDSelectorRule(id_selector);
          MergeHigherCascadeStylesForFiber(rule, rule_id_selector,
                                           node->data_model(), style_sheet);

          // NOTE: Support for nested focus pseudo class. This is a naive
          // implementation and should be replaced in the future.
          if (node->element_manager()->GetEnableCascadePseudo() &&
              node_parent->data_model()->HasPseudoState(kPseudoStateFocus)) {
            MergeHigherCascadeStylesForFiber(rule, rule_id_selector + ":focus",
                                             node->data_model(), style_sheet);
          }
        }
      }
      if (!node->element_manager()->GetRemoveDescendantSelectorScope() &&
          node_parent->is_component()) {
        // descendant selector only works in current component scope!
        break;
      }
      node_parent = static_cast<FiberElement*>(node_parent->parent());
    }
  }
}

void StyleResolver::MergeHigherCascadeStylesForFiber(
    const std::string& current_selector, const std::string& parent_selector,
    AttributeHolder* node, CSSFragment* style_sheet) {
  std::string integrated_selector =
      MergeCSSSelector(current_selector, parent_selector);
  CSSParseToken* token_parent =
      style_sheet->GetCascadeStyle(integrated_selector);
  MergeToken(token_parent);
}

const tasm::CSSParserConfigs& StyleResolver::GetCSSParserConfigs() {
  ElementManager* manager_ = manager();
  if (manager_) {
    return manager_->GetCSSParserConfigs();
  }
  static base::NoDestructor<tasm::CSSParserConfigs> kDefaultCSSConfigs;
  return *kDefaultCSSConfigs;
}

void StyleResolver::ParsePlaceHolderTokens(PseudoPlaceHolderStyles& result,
                                           const StyleMap& map) {
  for (const auto& i : map) {
    auto id = i.first;
    auto& value = i.second;
    if (id == kPropertyIDColor) {
      result.color_ = value;
    } else if (id == kPropertyIDFontSize) {
      result.font_size_ = value;
    } else if (id == kPropertyIDFontWeight) {
      result.font_weight_ = value;
    } else if (id == kPropertyIDFontFamily) {
      result.font_family_ = value;
    } else {
      UnitHandler::CSSWarning(false,
                              GetCSSParserConfigs().enable_css_strict_mode,
                              "placeholder only support color && font-size");
    }
  }
}

PseudoPlaceHolderStyles StyleResolver::ParsePlaceHolderTokens(
    const InlineTokenVector& tokens) {
  PseudoPlaceHolderStyles result;

  for (const auto& token : tokens) {
    auto& map = token->GetAttributes();
    ParsePlaceHolderTokens(result, map);
  }
  return result;
}

StyleResolver::InlineTokenVector StyleResolver::ParsePseudoCSSTokens(
    AttributeHolder* node, const char* selector) {
  InlineTokenVector tokens;

  CSSFragment* fragment = node->ParentStyleSheet();
  if (!fragment) return tokens;

  const base::String& tag_node = node->tag();
  // Global  ::xxx
  {
    auto token = fragment->GetPseudoStyle(selector);
    if (token) {
      tokens.emplace_back(token);
    }
  }

  // tag selector  tag::xxx
  if (!tag_node.empty()) {
    std::string rule = tag_node.str() + selector;
    auto token = fragment->GetPseudoStyle(rule);
    if (token) {
      tokens.emplace_back(token);
    }
  }

  // class selector  .class::xxx
  auto const& class_list = node->classes();
  for (auto const& clazz : class_list) {
    std::string rule = kCSSSelectorClass + clazz.str() + selector;

    auto token = fragment->GetPseudoStyle(rule);
    if (token) {
      tokens.emplace_back(token);
    }
  }

  // id selector #id::xxx
  auto const& id = node->idSelector();
  if (!id.empty()) {
    std::string rule = kCSSSelectorID + id.str() + selector;
    auto token = fragment->GetPseudoStyle(rule);
    if (token) {
      tokens.emplace_back(token);
    }
  }

  return tokens;
}

void StyleResolver::ParsePseudoCSSTokensForFiber(FiberElement* element,
                                                 CSSFragment* fragment,
                                                 const char* selector,
                                                 StyleMap& map) {
  if (!fragment) {
    return;
  }

  const base::String& tag_node = element->GetTag();
  // Global  ::xxx
  {
    auto token = fragment->GetPseudoStyle(selector);
    if (token) {
      map.merge(token->GetAttributes());
    }
  }

  // tag selector  tag::xxx
  if (!tag_node.empty()) {
    std::string rule = tag_node.str() + selector;
    auto token = fragment->GetPseudoStyle(rule);
    if (token) {
      map.merge(token->GetAttributes());
    }
  }

  // class selector  .class::xxx
  auto const& class_list = element->classes();
  for (auto const& clazz : class_list) {
    std::string rule = kCSSSelectorClass + clazz.str() + selector;
    auto token = fragment->GetPseudoStyle(rule);
    if (token) {
      map.merge(token->GetAttributes());
    }
  }

  // id selector #id::xxx
  auto const& id = element->GetIdSelector();
  if (!id.empty()) {
    std::string rule = kCSSSelectorID + id.str() + selector;
    auto token = fragment->GetPseudoStyle(rule);
    if (token) {
      map.merge(token->GetAttributes());
    }
  }
}

std::unique_ptr<starlight::ComputedCSSStyle>
StyleResolver::CreateInitialComputedStyle(
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_style) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              STYLE_RESOLVER_CREATE_INITIAL_COMPUTED_STYLE);
  auto* element_manager = manager();
  auto style = std::make_unique<starlight::ComputedCSSStyle>(
      *element_manager->platform_computed_css());
  PrepareInitialComputedStyle(*style, parent_style, previous_style);
  return style;
}

void StyleResolver::PrepareInitialComputedStyle(
    starlight::ComputedCSSStyle& style,
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_style) {
  if (previous_style != &style) {
    InitializeStyleShell(style, previous_style);
    InheritParentStyle(style, parent_style);
    if (previous_style != nullptr) {
      style.default_overflow_visible_ =
          previous_style->default_overflow_visible_;
    }
    style.ResetOverflow();
  } else {
    // This path is only used for the first screen. The caller passes in a
    // brand new shell, so we only need to inherit parent style.
    InheritParentStyle(style, parent_style);
  }

  // Sync root font size from live page root for non-page elements.
  // Descendants must resolve rem against the live page root font-size,
  // not against a potentially stale parent snapshot.
  if (!element()->is_page()) {
    style.SetFontSize(style.GetFontSize(), element()->GetCurrentRootFontSize());
  }
}

void StyleResolver::ResolveBaseStyleInPlace(
    starlight::ComputedCSSStyle& style,
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_computed_style,
    StyleMap* resolved_style_map) {
  // TODO(zhouzhitao): STUB. Must fully resolve base style and optionally
  // populate `resolved_style_map` before enabling
  // `ElementManager::EnableNewStylingPipeline()`.
  // Resolves the base (static) ComputedCSSStyle for an element in the new
  // pipeline, mutating the provided style in-place. Prepares the initial
  // style, collects matched CSS rules, analyzes the matched result into a
  // resolved StyleMap (including custom properties and logical properties),
  // and applies the resolved map. Optionally outputs the resolved_style_map
  // for diffing or caching.
}

std::unique_ptr<starlight::ComputedCSSStyle> StyleResolver::ResolveBaseStyle(
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_computed_style,
    StyleMap* resolved_style_map) {
  // TODO(zhouzhitao): STUB. Must return a fully resolved base style before
  // enabling `ElementManager::EnableNewStylingPipeline()`.
  // Resolves the base (static) ComputedCSSStyle for an element in the new
  // pipeline, returning a newly allocated style. Creates the initial shell,
  // collects matched rules, analyzes and resolves all inputs (matched,
  // inline, attribute styles plus custom properties), and applies them.
  // Optionally outputs the resolved_style_map.
  return nullptr;
}

std::unique_ptr<starlight::ComputedCSSStyle>
StyleResolver::RebuildFinalStyleFromParent(
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_style,
    const CustomPropertiesMap* sampled_custom_property_overrides,
    const StyleMap* sampled_property_overrides, StyleMap* resolved_style_map) {
  // TODO(zhouzhitao): STUB. Must rebuild final style with animation/transition
  // sampling applied before enabling
  // `ElementManager::EnableNewStylingPipeline()`. Rebuilds the final
  // ComputedCSSStyle from a parent style in the new pipeline, incorporating
  // sampled animation / transition overrides. Creates an initial style
  // inherited from the parent, re-collects and re-analyzes matched rules (with
  // overrides taken into account), applies the resolved map including the
  // sampled overrides, and optionally returns the resolved style map. This is
  // used when dynamic style changes require a full rebuild.
  return nullptr;
}

std::unique_ptr<starlight::ComputedCSSStyle>
StyleResolver::BuildFinalStyleFromBaseFastPath(
    const starlight::ComputedCSSStyle& base_style,
    const StyleMap* sampled_property_overrides, StyleMap* resolved_style_map) {
  // TODO(zhouzhitao): STUB. Must build final style on top of base style before
  // enabling `ElementManager::EnableNewStylingPipeline()`.
  // Fast-path final style construction in the new pipeline when the base style
  // is already known and only sampled normal-property overrides need to be
  // applied. Copies the base style into a new shell, merges the override map
  // into the resolved_style_map, and applies high-priority and standard
  // properties from the overrides. Avoids re-collecting matched rules.
  return nullptr;
}

void StyleResolver::InitializeStyleShell(
    starlight::ComputedCSSStyle& shell_style,
    const starlight::ComputedCSSStyle* previous_style) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_INITIALIZE_STYLE_SHELL);
  auto* element_manager = manager();
  const auto& env_config = element_manager->GetLynxEnvConfig();
  shell_style.SetEnableZIndex(element_manager->GetEnableZIndex());
  shell_style.SetScreenWidth(env_config.ScreenWidth());
  shell_style.SetViewportHeight(env_config.ViewportHeight());
  shell_style.SetViewportWidth(env_config.ViewportWidth());
  shell_style.SetCssAlignLegacyWithW3c(
      element_manager->GetLayoutConfigs().css_align_with_legacy_w3c_);
  shell_style.SetFontScaleOnlyEffectiveOnSp(env_config.FontScaleSpOnly());
  shell_style.SetFontScale(env_config.FontScale());
  shell_style.SetFontSize(env_config.PageDefaultFontSize(),
                          env_config.PageDefaultFontSize());
  shell_style.SetLayoutUnit(env_config.PhysicalPixelsPerLayoutUnit(),
                            env_config.LayoutsUnitPerPx());
  shell_style.SetCSSParserConfigs(element_manager->GetCSSParserConfigs());
}

void StyleResolver::InheritParentStyle(
    starlight::ComputedCSSStyle& computed_style,
    const starlight::ComputedCSSStyle* parent_style) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_INHERIT_PARENT_STYLE);
  if (parent_style) {
    const auto inherited_font_size =
        element()->IsCSSInheritanceEnabled()
            ? parent_style->GetFontSize()
            : manager()->GetLynxEnvConfig().PageDefaultFontSize();
    computed_style.SetFontSize(inherited_font_size,
                               parent_style->GetRootFontSize());
    computed_style.InheritCustomPropertiesFrom(*parent_style);

    if (!element()->IsCSSInheritanceEnabled()) {
      return;
    }

    const auto& configs = manager()->GetDynamicCSSConfigs();
    const auto* inheritable_props =
        configs.custom_inherit_list_.empty()
            ? &DynamicCSSStylesManager::GetInheritableProps()
            : &configs.custom_inherit_list_;
    computed_style.InheritNormalPropertiesFrom(*parent_style,
                                               *inheritable_props);
    computed_style.InheritResolvedValuesFrom(*parent_style, *inheritable_props);
  }
}

void StyleResolver::CollectMatchedRules(CSSFragment* style_sheet) {
  // TODO(zhouzhitao): STUB. Must collect matched rules into TLS vectors before
  // enabling `ElementManager::EnableNewStylingPipeline()`.
  // Collects matched CSS rules for the current element in the new pipeline.
  // Clears the thread-local matched_style_map and matched_variable_map,
  // then invokes the legacy CSS matching path (selector-based or fiber-based)
  // to populate them. These TLS vectors are consumed later by
  // AnalyzeMatchedResult and cleared after cascading-affecting properties are
  // fully applied.
}

void StyleResolver::AnalyzeMatchedResult(
    starlight::ComputedCSSStyle& new_style, StyleMap& result,
    size_t base_reserving_size,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  // TODO(zhouzhitao): STUB. Must resolve specified styles and custom
  // properties (including var() resolution) before enabling
  // `ElementManager::EnableNewStylingPipeline()`.
  // Analyzes the collected matched CSS rules and produces a fully resolved
  // StyleMap for the new pipeline. Orchestrates three phases: collecting
  // static style inputs (matched, inline, attribute styles and custom
  // properties), finalizing custom properties (including overrides), and
  // resolving all collected inputs into the result map. The result is then
  // ready for ApplyResolvedStyleMap.
}

void StyleResolver::CollectStaticStyleInputs(
    starlight::ComputedCSSStyle& new_style,
    NewPipelineCollectedStyleInputs& inputs, size_t base_reserving_size) {
  // TODO(zhouzhitao): STUB. Must collect matched/inline/attribute specified
  // styles and custom properties before enabling
  // `ElementManager::EnableNewStylingPipeline()`.
  // Collects all static (non-animated) style inputs for the new pipeline.
  // Processes raw inline styles, then gathers matched specified styles,
  // matched custom properties, inline specified styles, attribute specified
  // styles, inline custom properties, and holder custom properties into the
  // inputs structure. These inputs are later resolved and applied.
}

void StyleResolver::FinalizeCustomProperties(
    starlight::ComputedCSSStyle& new_style,
    const NewPipelineCollectedStyleInputs& inputs,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  // TODO(zhouzhitao): STUB. Must apply overrides and resolve var() references
  // before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Finalizes custom properties for the new pipeline. Applies any sampled
  // custom property overrides onto the style, calls FinalizeCustomProperties
  // to resolve var() references, and sets the element-manager-level flag
  // indicating whether CSS variables are required based on the presence of
  // variable tokens in matched, inline, attribute, or override styles.
}

void StyleResolver::ResolveCollectedStyleInputs(
    const starlight::ComputedCSSStyle& new_style,
    NewPipelineCollectedStyleInputs& inputs, StyleMap& result,
    const StyleMap* property_overrides) {
  // TODO(zhouzhitao): STUB. Must merge and resolve all inputs into `result`
  // before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Resolves all collected static style inputs into a single StyleMap for the
  // new pipeline. Merges matched, inline, and attribute specified styles
  // (with CSS variable substitution) into the result, then overlays any
  // sampled property overrides. This produces the definitive map that
  // ApplyResolvedStyleMap consumes.
}

void StyleResolver::CollectMatchedSpecifiedStyles(
    starlight::ComputedCSSStyle& new_style, StyleMap& result,
    size_t base_reserving_size) {
  // TODO(zhouzhitao): STUB. Must collect matched specified styles and resolve
  // logical properties before enabling
  // `ElementManager::EnableNewStylingPipeline()`. Collects specified styles
  // from the matched CSS rules (thread-local matched_style_map) for the new
  // pipeline. Reserves capacity based on the matched map sizes plus the inline
  // style count, then iterates the matched styles and inserts each entry into
  // the result with logical properties resolved to physical ones.
}

void StyleResolver::CollectInlineSpecifiedStyles(
    starlight::ComputedCSSStyle& new_style, StyleMap& result) {
  // TODO(zhouzhitao): STUB. Must parse inline styles (including var() tokens
  // when enabled) before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Collects inline specified styles from the element’s current raw inline
  // styles for the new pipeline. Parses variable tokens when CSS inline
  // variables are enabled, otherwise processes values through UnitHandler.
  // Each parsed entry is inserted into the result with logical properties
  // resolved to physical ones.
}

void StyleResolver::CollectAttributeSpecifiedStyles(
    starlight::ComputedCSSStyle& new_style, StyleMap& result) {
  // TODO(zhouzhitao): STUB. Must collect attribute-backed specified styles
  // before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Collects specified styles committed from element attributes for the new
  // pipeline. Reads the committed attribute styles (e.g., from JSX style
  // attributes) and inserts each entry into the result with logical
  // properties resolved to physical ones.
}

void StyleResolver::CollectMatchedCustomProperties(
    starlight::ComputedCSSStyle& new_style) {
  // TODO(zhouzhitao): STUB. Must collect matched custom properties into
  // ComputedCSSStyle before enabling
  // `ElementManager::EnableNewStylingPipeline()`. Collects custom property
  // declarations from the matched CSS rules for the new pipeline. Iterates the
  // thread-local matched_variable_map, parses each variable value with
  // CSSStringParser, and stores the result on the ComputedCSSStyle via
  // SetCustomProperty.
}

void StyleResolver::CollectInlineCustomProperties(
    starlight::ComputedCSSStyle& new_style) {
  // TODO(zhouzhitao): STUB. Must collect inline custom properties before
  // enabling `ElementManager::EnableNewStylingPipeline()`.
  // Collects inline custom property declarations from the element for the new
  // pipeline. When CSS inline variables are enabled, reads the current raw
  // inline custom properties, parses each with CSSStringParser, and stores
  // them on the ComputedCSSStyle via SetCustomProperty.
}

void StyleResolver::CollectHolderCustomProperties(
    starlight::ComputedCSSStyle& new_style) {
  // TODO(zhouzhitao): STUB. Must collect AttributeHolder-level custom
  // properties before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Collects custom property declarations from the element’s AttributeHolder
  // data model for the new pipeline. Reads the holder-level CSS inline
  // variables, parses each with CSSStringParser, and stores them on the
  // ComputedCSSStyle via SetCustomProperty.
}

void StyleResolver::ResolveSpecifiedStyleMap(
    const starlight::ComputedCSSStyle& new_style, StyleMap& source,
    StyleMap& result) {
  // TODO(zhouzhitao): STUB. Must resolve var() references and re-parse values
  // into `result` before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Resolves a source StyleMap into the result StyleMap for the new pipeline.
  // For each entry, if the value is a CSS variable reference (var()),
  // substitutes it using the ComputedCSSStyle’s finalized custom properties
  // and re-parses the resolved value through UnitHandler. Non-variable values
  // are moved directly into the result.
}

void StyleResolver::ApplyCascadingAffectingProperties(
    starlight::ComputedCSSStyle& style, StyleMap& map,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  // TODO(zhouzhitao): STUB. Must handle direction and cascading-affecting
  // re-resolution before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Applies cascading-affecting properties (e.g., direction) in the new
  // pipeline. If the direction property changes, updates the style and
  // re-triggers AnalyzeMatchedResult so that direction-dependent selectors
  // and logical properties are re-evaluated. Clears the matched rule caches
  // only after all cascading-affecting properties are stable.
}

void StyleResolver::ApplyHighPriorityProperties(
    starlight::ComputedCSSStyle& style, const StyleMap& style_map) {
  // TODO(zhouzhitao): STUB. Must apply high-priority properties (e.g.
  // font-size) before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Applies high-priority properties (e.g., font-size) in the new pipeline.
  // These properties are resolved before standard properties because they
  // establish context (such as the em unit baseline) needed by downstream
  // property resolution.
}

void StyleResolver::ApplyStandardProperties(starlight::ComputedCSSStyle& style,
                                            const StyleMap& style_map) {
  // TODO(zhouzhitao): STUB. Must apply standard properties and normalize
  // text-align before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Applies all standard (non-cascading, non-high-priority) properties in the
  // new pipeline. Iterates the resolved style map, skipping direction and
  // font-size (already handled), and writes each property into the
  // ComputedCSSStyle. Finally normalizes text-align for the current direction.
}

void StyleResolver::ApplyResolvedStyleMap(
    starlight::ComputedCSSStyle& style, StyleMap& style_map,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  // TODO(zhouzhitao): STUB. Must apply resolved style map end-to-end before
  // enabling `ElementManager::EnableNewStylingPipeline()`.
  // Applies a fully resolved style map to a ComputedCSSStyle in the new
  // pipeline. Orchestrates four sub-phases: cascading-affecting properties
  // (e.g., direction), inherited side-effects replay, high-priority
  // properties (e.g., font-size), and standard properties. This is the
  // final step of base-style resolution before animation sampling.
}

bool StyleResolver::ComputeStyleDiff(
    starlight::ComputedCSSStyle& new_style,
    const starlight::ComputedCSSStyle& old_style) {
  // TODO(zhouzhitao): STUB. Must compute accurate diffs and mark changed/reset
  // properties before enabling `ElementManager::EnableNewStylingPipeline()`.
  // Computes the property-level diff between a newly resolved style and the
  // old style in the new pipeline. Walks all layout, visual, text, animation,
  // and misc data structures, calling MarkChanged for every property that
  // differs. Returns true if any property changed. This diff drives
  // incremental layout and paint invalidation.
  return false;
}
}  // namespace tasm
}  // namespace lynx
