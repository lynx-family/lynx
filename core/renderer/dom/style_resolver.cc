// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/style_resolver.h"

#include <array>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/include/algorithm.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/dynamic_direction_styles_manager.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
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

void ApplyResolvedFontSize(Element* element, starlight::ComputedCSSStyle& style,
                           const CSSValue& value, bool mark_changed) {
  const auto current_font_size = style.GetFontSize();
  const auto root_font_size = style.GetRootFontSize();
  base::flex_optional<float> result;
  if (!value.IsEmpty()) {
    auto* em = element->element_manager();
    const auto& env_config = em->GetLynxEnvConfig();
    const auto unify_vw_vh_behavior =
        em->GetDynamicCSSConfigs().unify_vw_vh_behavior_;
    result = starlight::CSSStyleUtils::ResolveFontSize(
        value, env_config, unify_vw_vh_behavior, current_font_size,
        root_font_size, em->GetCSSParserConfigs());
  } else {
    result = current_font_size;
  }

  if (result.has_value()) {
    style.SetResolvedValue(kPropertyIDFontSize,
                           CSSValue(*result, CSSValuePattern::NUMBER));
    style.SetFontSize(*result, element->is_page() ? *result : root_font_size);
    if (!element->EnableLayoutInElementMode() ||
        element->IsShadowNodeCustom()) {
      style.SetValue(kPropertyIDFontSize,
                     CSSValue(*result, CSSValuePattern::NUMBER));
    }
    if (mark_changed) {
      style.MarkChanged(kPropertyIDFontSize);
    }
  } else {
    style.RemoveResolvedValue(kPropertyIDFontSize);
  }
}

void ReplayInheritedStyleSideEffects(Element* element,
                                     starlight::ComputedCSSStyle& style,
                                     const StyleMap& explicit_style_map) {
  if (!element->IsCSSInheritanceEnabled()) {
    return;
  }
  const bool is_first_render = element->IsNewlyCreated();
  const auto& inherited_resolved_values = style.GetResolvedValues();
  if (inherited_resolved_values.empty()) {
    return;
  }
  const auto explicit_style_ids = CSSIDBitset::FromKeys(explicit_style_map);

  for (const auto& [id, value] : inherited_resolved_values) {
    if (!element->IsInheritable(id) || explicit_style_ids.Has(id)) {
      continue;
    }

    if (id == kPropertyIDFontSize) {
      ApplyResolvedFontSize(element, style, value, is_first_render);
      continue;
    }

    if (!element->ShouldWritePropertyToComputedStyle(id)) {
      style.SetResolvedValue(id, value);
      continue;
    }

    style.SetValue(id, value);
    if (is_first_render) {
      style.MarkChanged(id);
    }
  }
}

bool HasPseudoRulesInStyleSheets(CSSFragment* fragment, ElementManager* em) {
  if (fragment->rule_set() && !fragment->rule_set()->pseudo_rules().empty()) {
    return true;
  }
  if (!em) {
    return false;
  }
  for (const auto& wrapper : em->GetAdoptedStyleSheets()) {
    if (wrapper && wrapper->fragment_ &&
        wrapper->fragment_->enable_css_selector() &&
        wrapper->fragment_->rule_set() &&
        !wrapper->fragment_->rule_set()->pseudo_rules().empty()) {
      return true;
    }
  }
  return false;
}

void ApplyComputedStyleValue(Element* element,
                             starlight::ComputedCSSStyle& style,
                             CSSPropertyID id, const CSSValue& value) {
  if (id == kPropertyIDFontSize) {
    ApplyResolvedFontSize(element, style, value, false);
    return;
  }

  if (!element->ShouldWritePropertyToComputedStyle(id)) {
    style.SetResolvedValue(id, value);
    return;
  }

  style.SetValue(id, value);
}

void NormalizeTextAlignForDirection(Element* element,
                                    starlight::ComputedCSSStyle& style) {
  if (element == nullptr) {
    return;
  }
  if (!element->is_text() && !element->NeedProcessDirection()) {
    return;
  }

  const auto direction = style.GetDirection();
  if (direction == starlight::DirectionType::kNormal) {
    return;
  }

  CSSValue text_align_value(starlight::TextAlignType::kStart);
  if (const auto& text_attributes = style.GetTextAttributes();
      text_attributes.has_value()) {
    text_align_value = CSSValue(text_attributes->text_align);
  }

  const auto resolved_text_align =
      ResolveTextAlign(kPropertyIDTextAlign, text_align_value, direction);
  ApplyComputedStyleValue(element, style, resolved_text_align.first,
                          resolved_text_align.second);
}

void InsertStyleWithLogicalPropertyResolved(Element* element,
                                            starlight::ComputedCSSStyle& style,
                                            CSSPropertyID key,
                                            const CSSValue& value,
                                            StyleMap& result) {
  auto direction_mapping = element->CheckDirectionMapping(key);
  bool is_direction_aware_property =
      direction_mapping.ltr_property_ != kPropertyStart ||
      direction_mapping.rtl_property_ != kPropertyStart;
  if (is_direction_aware_property) {
    auto current_direction = style.GetDirection();
    bool use_rtl_value =
        (IsRTL(current_direction) && direction_mapping.is_logic_) ||
        IsLynxRTL(current_direction);
    auto physical_id = use_rtl_value ? direction_mapping.rtl_property_
                                     : direction_mapping.ltr_property_;
    result.insert_or_assign(physical_id, value);
    return;
  }

  result.insert_or_assign(key, value);
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
    if (!HasPseudoRulesInStyleSheets(fragment, manager())) {
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

namespace {
struct PseudoElementDescriptor {
  PseudoState state;
  const char* selector;
  bool (*predicate)(FiberElement*);
};
}  // namespace

void StyleResolver::ResolvePseudoElementsForNewPipeline(CSSFragment* fragment) {
  Element* current_element = element();
  if (!fragment) {
    return;
  }
  if ((fragment->enable_css_selector() &&
       !HasPseudoRulesInStyleSheets(fragment, manager())) ||
      (!fragment->enable_css_selector() && fragment->pseudo_map().empty())) {
    return;
  }

  auto fiber_element = static_cast<FiberElement*>(current_element);

  static constexpr std::array<PseudoElementDescriptor, 2>
      kPseudoElementRegistry = {{
          {kPseudoStateSelection, kCSSSelectorSelection,
           [](FiberElement* fe) {
             return fe->HasTextSelection() && !fe->is_inline_element();
           }},
          {kPseudoStatePlaceHolder, kCSSSelectorPlaceholder,
           [](FiberElement* fe) { return fe->HasPlaceHolder(); }},
      }};

  for (const auto& descriptor : kPseudoElementRegistry) {
    if (descriptor.predicate && descriptor.predicate(fiber_element)) {
      ResolvePseudoElement(descriptor.state, fragment, fiber_element,
                           descriptor.selector);
    }
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
  const auto adopted_sheets =
      element_manager ? element_manager->GetAdoptedStyleSheets()
                      : std::vector<fml::RefPtr<SharedCSSFragmentWrapper>>{};
  const auto* adopted_sheets_ptr =
      adopted_sheets.empty() ? nullptr : &adopted_sheets;
  auto matched_rules = GetCSSMatchedRule(node, style_sheet, adopted_sheets_ptr);

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_RESOLVE_BASE_STYLE);
  auto* current_element = element();
  PrepareInitialComputedStyle(style, parent_style, previous_computed_style);

  CSSFragment* style_sheet = current_element->GetRelatedCSSFragment();
  CollectMatchedRules(style_sheet);

  StyleMap style_map;
  AnalyzeMatchedResult(style, style_map, current_element->CountInlineStyles());

  ApplyResolvedStyleMap(style, style_map);

  if (resolved_style_map != nullptr) {
    *resolved_style_map = std::move(style_map);
  }
}

std::unique_ptr<starlight::ComputedCSSStyle> StyleResolver::ResolveBaseStyle(
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_computed_style,
    StyleMap* resolved_style_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_RESOLVE_BASE_STYLE);
  auto* current_element = element();
  auto style =
      CreateInitialComputedStyle(parent_style, previous_computed_style);

  CSSFragment* style_sheet = current_element->GetRelatedCSSFragment();
  CollectMatchedRules(style_sheet);

  StyleMap style_map;
  AnalyzeMatchedResult(*style, style_map, current_element->CountInlineStyles());

  ApplyResolvedStyleMap(*style, style_map);

  if (resolved_style_map != nullptr) {
    *resolved_style_map = std::move(style_map);
  }
  return style;
}

std::unique_ptr<starlight::ComputedCSSStyle>
StyleResolver::RebuildFinalStyleFromParent(
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_style,
    const CustomPropertiesMap* sampled_custom_property_overrides,
    const StyleMap* sampled_property_overrides, StyleMap* resolved_style_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              STYLE_RESOLVER_REBUILD_FINAL_STYLE_FROM_PARENT);
  auto* current_element = element();
  auto style = CreateInitialComputedStyle(parent_style, previous_style);

  CSSFragment* style_sheet = current_element->GetRelatedCSSFragment();
  CollectMatchedRules(style_sheet);

  StyleMap style_map;
  AnalyzeMatchedResult(*style, style_map, current_element->CountInlineStyles(),
                       sampled_custom_property_overrides,
                       sampled_property_overrides);

  ApplyResolvedStyleMap(*style, style_map, sampled_custom_property_overrides,
                        sampled_property_overrides);

  if (resolved_style_map != nullptr) {
    *resolved_style_map = std::move(style_map);
  }

  return style;
}

std::unique_ptr<starlight::ComputedCSSStyle>
StyleResolver::BuildFinalStyleFromBaseFastPath(
    const starlight::ComputedCSSStyle& base_style,
    const StyleMap* sampled_property_overrides, StyleMap* resolved_style_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              STYLE_RESOLVER_BUILD_FINAL_STYLE_FROM_BASE_FAST_PATH);
  auto style = CreateInitialComputedStyle(nullptr, &base_style);
  style->CopyFrom(base_style);

  if (sampled_property_overrides == nullptr ||
      sampled_property_overrides->empty()) {
    return style;
  }

  if (resolved_style_map != nullptr) {
    resolved_style_map->reserve(resolved_style_map->size() +
                                sampled_property_overrides->size());
    for (const auto& [key, value] : *sampled_property_overrides) {
      resolved_style_map->insert_or_assign(key, value);
    }
  }

  ApplyHighPriorityProperties(*style, *sampled_property_overrides);
  ApplyStandardProperties(*style, *sampled_property_overrides);
  return style;
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_COLLECT_MATCHED_RULES);
  auto* current_element = element();
  // Reuse the legacy TLS vectors and keep them alive through the new-pipeline
  // analysis pass. They are cleared after cascading-affecting properties are
  // fully applied.
  matched_style_map.clear();
  matched_important_style_map.clear();
  matched_variable_map.clear();

  if (style_sheet) {
    if (style_sheet->enable_css_selector()) {
      GetCSSStyleNew(current_element->data_model(), style_sheet);
    } else {
      GetCSSStyleForFiber(static_cast<FiberElement*>(current_element),
                          style_sheet);
    }
  }
}

void StyleResolver::AnalyzeMatchedResult(
    starlight::ComputedCSSStyle& new_style, StyleMap& result,
    size_t base_reserving_size,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_ANALYZE_MATCHED_RESULT);
  result.clear();

  NewPipelineCollectedStyleInputs inputs;
  CollectStaticStyleInputs(new_style, inputs, base_reserving_size);
  FinalizeCustomProperties(new_style, inputs, custom_property_overrides,
                           property_overrides);
  ResolveCollectedStyleInputs(new_style, inputs, result, property_overrides);
}

void StyleResolver::CollectStaticStyleInputs(
    starlight::ComputedCSSStyle& new_style,
    NewPipelineCollectedStyleInputs& inputs, size_t base_reserving_size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_COLLECT_STATIC_STYLE_INPUTS);
  auto* current_element = element();
  current_element->ProcessFullRawInlineStyle(nullptr);
  CollectMatchedSpecifiedStyles(new_style, inputs.matched_styles,
                                base_reserving_size, matched_style_map);
  CollectMatchedSpecifiedStyles(new_style, inputs.matched_important_styles, 0,
                                matched_important_style_map);
  CollectMatchedCustomProperties(new_style);
  CollectInlineSpecifiedStyles(new_style, inputs.inline_styles, false);
  CollectInlineSpecifiedStyles(new_style, inputs.inline_important_styles, true);
  CollectAttributeSpecifiedStyles(new_style, inputs.attribute_styles);
  CollectInlineCustomProperties(new_style);
  CollectHolderCustomProperties(new_style);
}

void StyleResolver::FinalizeCustomProperties(
    starlight::ComputedCSSStyle& new_style,
    const NewPipelineCollectedStyleInputs& inputs,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_FINALIZE_CUSTOM_PROPERTIES);
  auto* current_element = element();
  auto* element_manager = manager();
  if (custom_property_overrides != nullptr) {
    for (const auto& [key, value] : *custom_property_overrides) {
      new_style.SetCustomProperty(key, value);
    }
  }
  new_style.FinalizeCustomProperties();

  CSSVariableHandler handler;
  if (current_element->IsCSSInlineVariablesEnabled() ||
      new_style.GetCustomProperties() ||
      handler.HasCSSVariableInAnyStyleMap(
          {&inputs.matched_styles, &inputs.inline_styles,
           &inputs.attribute_styles, &inputs.inline_important_styles,
           &inputs.matched_important_styles, property_overrides})) {
    element_manager->SetRequireCSSVariables(true);
  }
}

void StyleResolver::ResolveCollectedStyleInputs(
    const starlight::ComputedCSSStyle& new_style,
    NewPipelineCollectedStyleInputs& inputs, StyleMap& result,
    const StyleMap* property_overrides) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              STYLE_RESOLVER_RESOLVE_COLLECTED_STYLE_INPUTS);
  result.reserve(
      inputs.matched_styles.size() + inputs.inline_styles.size() +
      inputs.matched_important_styles.size() +
      inputs.inline_important_styles.size() + inputs.attribute_styles.size() +
      (property_overrides != nullptr ? property_overrides->size() : 0));
  ResolveSpecifiedStyleMap(new_style, inputs.matched_styles, result);
  ResolveSpecifiedStyleMap(new_style, inputs.inline_styles, result);
  ResolveSpecifiedStyleMap(new_style, inputs.matched_important_styles, result);
  ResolveSpecifiedStyleMap(new_style, inputs.inline_important_styles, result);
  ResolveSpecifiedStyleMap(new_style, inputs.attribute_styles, result);
  if (property_overrides != nullptr) {
    for (const auto& [key, value] : *property_overrides) {
      result.insert_or_assign(key, value);
    }
  }
}

void StyleResolver::CollectMatchedSpecifiedStyles(
    starlight::ComputedCSSStyle& new_style, StyleMap& result,
    size_t base_reserving_size,
    const MatchedVector<const StyleMap*>& matched_style_maps) {
  auto* current_element = element();

  for (auto matched_style_ptr : matched_style_maps) {
    base_reserving_size += matched_style_ptr->size();
  }
  result.reserve(base_reserving_size);

  for (auto matched_style_ptr : matched_style_maps) {
    for (const auto& [key, value] : *matched_style_ptr) {
      InsertStyleWithLogicalPropertyResolved(current_element, new_style, key,
                                             value, result);
    }
  }
}

void StyleResolver::CollectInlineSpecifiedStyles(
    starlight::ComputedCSSStyle& new_style, StyleMap& result, bool important) {
  auto* current_element = element();
  const auto& raw_inline_styles =
      important ? current_element->GetCurrentRawImportantInlineStyles()
                : current_element->GetCurrentRawInlineStyles();
  if (!raw_inline_styles || raw_inline_styles->empty()) {
    return;
  }

  const auto& configs = manager()->GetCSSParserConfigs();
  const bool css_inline_enabled =
      current_element->IsCSSInlineVariablesEnabled();
  StyleMap inline_parsed_styles;
  inline_parsed_styles.reserve(raw_inline_styles->size());
  for (const auto& [key, value] : *raw_inline_styles) {
    if (css_inline_enabled) {
      base::String style_str = value.String();
      CSSStringParser parser{style_str.c_str(),
                             static_cast<uint32_t>(style_str.length()),
                             configs};
      CSSValue css_value = parser.ParseVariable();
      if (parser.HasMetVarToken()) {
        inline_parsed_styles.insert_or_assign(key, std::move(css_value));
        continue;
      }
    }

    UnitHandler::Process(key, value, inline_parsed_styles, configs);
  }

  for (const auto& [key, value] : inline_parsed_styles) {
    InsertStyleWithLogicalPropertyResolved(current_element, new_style, key,
                                           value, result);
  }
}

void StyleResolver::CollectAttributeSpecifiedStyles(
    starlight::ComputedCSSStyle& new_style, StyleMap& result) {
  auto* current_element = element();
  const auto* committed_attribute_styles =
      current_element->PeekCommittedStylesFromAttributes();
  if (committed_attribute_styles == nullptr) {
    return;
  }

  result.reserve(committed_attribute_styles->size());
  for (const auto& [key, value] : *committed_attribute_styles) {
    InsertStyleWithLogicalPropertyResolved(current_element, new_style, key,
                                           value, result);
  }
}

void StyleResolver::CollectMatchedCustomProperties(
    starlight::ComputedCSSStyle& new_style) {
  auto& tls_matched_variable_map = matched_variable_map;
  if (tls_matched_variable_map.empty()) {
    return;
  }

  const auto& configs = manager()->GetCSSParserConfigs();
  for (auto variable_ptr : tls_matched_variable_map) {
    for (const auto& [key, value] : *variable_ptr) {
      CSSStringParser parser{value.c_str(),
                             static_cast<uint32_t>(value.length()), configs};
      new_style.SetCustomProperty(key, parser.ParseVariable());
    }
  }
}

void StyleResolver::CollectInlineCustomProperties(
    starlight::ComputedCSSStyle& new_style) {
  auto* current_element = element();
  if (!current_element->IsCSSInlineVariablesEnabled()) {
    return;
  }

  const auto& inline_custom_properties =
      current_element->GetCurrentRawInlineCustomProperties();
  if (!inline_custom_properties.has_value() ||
      inline_custom_properties->empty()) {
    return;
  }

  const auto& configs = manager()->GetCSSParserConfigs();
  for (const auto& [key, value] : *inline_custom_properties) {
    CSSStringParser parser{value.c_str(), static_cast<uint32_t>(value.length()),
                           configs};
    new_style.SetCustomProperty(key, parser.ParseVariable());
  }
}

void StyleResolver::CollectHolderCustomProperties(
    starlight::ComputedCSSStyle& new_style) {
  auto* holder = element()->data_model();
  if (holder == nullptr) {
    return;
  }

  // TODO(zhouzhitao): Split holder-side persistent runtime custom
  // properties from selector-matched custom properties. The latter should come
  // from this resolve pass's matched_variable_map; reusing css_variables_map()
  // here can reapply stale selector variables when a rule no longer matches or
  // a declaration is removed.
  auto& matched_custom_properties = holder->css_variables_map();
  auto& inline_custom_properties = holder->GetCSSInlineVariables();
  if (matched_custom_properties.empty() && inline_custom_properties.empty()) {
    return;
  }

  const auto& configs = manager()->GetCSSParserConfigs();
  auto apply_custom_properties = [&new_style,
                                  &configs](const auto& properties) {
    for (const auto& [key, value] : properties) {
      CSSStringParser parser{value.c_str(),
                             static_cast<uint32_t>(value.length()), configs};
      new_style.SetCustomProperty(key, parser.ParseVariable());
    }
  };

  // Holder-backed matched variables include prepared :root declarations and
  // other selector-derived values that must participate in new-pipeline
  // inheritance before inline/runtime overrides are applied.
  apply_custom_properties(matched_custom_properties);
  apply_custom_properties(inline_custom_properties);
}

void StyleResolver::ResolveSpecifiedStyleMap(
    const starlight::ComputedCSSStyle& new_style, StyleMap& source,
    StyleMap& result) {
  static const CustomPropertiesMap empty_custom_properties;
  const auto* custom_properties = new_style.GetCustomProperties();
  if (custom_properties == nullptr) {
    custom_properties = &empty_custom_properties;
  }
  auto* current_element = element();
  const auto& configs = manager()->GetCSSParserConfigs();
  auto* holder = current_element->data_model();
  CSSVariableHandler handler(true);

  const auto handle_custom_property_func =
      [holder](const base::String& name, const base::String& resolved_value) {
        if (holder != nullptr) {
          holder->AddCSSVariableRelated(name, resolved_value);
        }
      };

  for (auto& [id, value] : source) {
    if (!value.IsVariable()) {
      result.insert_or_assign(id, std::move(value));
      continue;
    }

    handler.ResolveCSSVariables(id, value, result, custom_properties, configs,
                                handle_custom_property_func);
  }
}

void StyleResolver::ApplyCascadingAffectingProperties(
    starlight::ComputedCSSStyle& style, StyleMap& map,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  auto* current_element = element();
  const bool has_matched_rules = !matched_style_map.empty() ||
                                 !matched_variable_map.empty() ||
                                 !matched_important_style_map.empty();
  const auto& important_inline_styles =
      current_element->GetCurrentRawImportantInlineStyles();
  const bool has_reanalyzable_inputs =
      has_matched_rules || current_element->CountInlineStyles() > 0 ||
      (important_inline_styles.has_value() &&
       !important_inline_styles->empty());
  auto it = map.find(kPropertyIDDirection);
  if (it != map.end()) {
    auto direction =
        static_cast<starlight::DirectionType>(it->second.GetNumber());
    if (direction != style.GetDirection()) {
      // Keep inherited custom properties and inherited resolved values on
      // `style` because the second pass still needs them to resolve var()
      // references correctly.
      ApplyComputedStyleValue(current_element, style, kPropertyIDDirection,
                              it->second);
      if (has_reanalyzable_inputs) {
        AnalyzeMatchedResult(style, map, current_element->CountInlineStyles(),
                             custom_property_overrides, property_overrides);
      }
    }
  }

  // Clear matched rule caches only after cascading-affecting properties are
  // fully applied, because direction changes may retrigger
  // AnalyzeMatchedResult.
  if (has_matched_rules) {
    matched_style_map.clear();
    matched_important_style_map.clear();
    matched_variable_map.clear();
  }
}

void StyleResolver::ApplyHighPriorityProperties(
    starlight::ComputedCSSStyle& style, const StyleMap& style_map) {
  auto it = style_map.find(kPropertyIDFontSize);
  if (it != style_map.end()) {
    ApplyResolvedFontSize(element(), style, it->second, false);
  }
}

void StyleResolver::ApplyStandardProperties(starlight::ComputedCSSStyle& style,
                                            const StyleMap& style_map) {
  for (const auto& [key, value] : style_map) {
    if (key == kPropertyIDDirection || key == kPropertyIDFontSize) {
      continue;
    }
    ApplyComputedStyleValue(element(), style, key, value);
  }

  NormalizeTextAlignForDirection(element(), style);
}

void StyleResolver::ApplyResolvedStyleMap(
    starlight::ComputedCSSStyle& style, StyleMap& style_map,
    const CustomPropertiesMap* custom_property_overrides,
    const StyleMap* property_overrides) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_APPLY_RESOLVED_STYLE_MAP);
  if (style_map.empty()) {
    return;
  }
  auto* current_element = element();
  ApplyCascadingAffectingProperties(style, style_map, custom_property_overrides,
                                    property_overrides);
  ReplayInheritedStyleSideEffects(current_element, style, style_map);
  ApplyHighPriorityProperties(style, style_map);
  ApplyStandardProperties(style, style_map);
}

namespace {
template <typename OptionalT>
bool OptionalNotEqual(const OptionalT& a, const OptionalT& b) {
  if (a.has_value() != b.has_value()) {
    return true;
  }
  if (!a.has_value()) {
    return false;
  }
  return !(*a == *b);
}

template <typename ValueT>
const ValueT& SharedDefaultValue() {
  static const base::NoDestructor<ValueT> value;
  return *value;
}

template <typename ValueT>
struct SharedDefaultProvider {
  const ValueT& operator()() const { return SharedDefaultValue<ValueT>(); }
};

template <typename OptionalT, typename NewDefaultProvider,
          typename OldDefaultProvider, typename DiffFn, typename MaterializeFn>
bool DiffOptionalValueImpl(OptionalT* mutable_new_value,
                           const OptionalT& new_value,
                           const OptionalT& old_value,
                           NewDefaultProvider&& new_default_provider,
                           OldDefaultProvider&& old_default_provider,
                           DiffFn&& diff_fn,
                           MaterializeFn&& materialize_new_value) {
  const bool has_new_value = new_value.has_value();
  const bool has_old_value = old_value.has_value();
  if (!has_new_value && !has_old_value) {
    return false;
  }

  if (has_new_value && has_old_value) {
    if (*new_value == *old_value) {
      return false;
    }
    return diff_fn(*new_value, *old_value);
  }

  if (has_new_value) {
    const auto& effective_old = old_default_provider();
    if (*new_value == effective_old) {
      return false;
    }
    return diff_fn(*new_value, effective_old);
  }

  const auto& effective_new = new_default_provider();
  if (effective_new == *old_value) {
    return false;
  }
  if (mutable_new_value != nullptr) {
    materialize_new_value(*mutable_new_value, effective_new);
    return diff_fn(**mutable_new_value, *old_value);
  }
  return diff_fn(effective_new, *old_value);
}

template <typename OptionalT, typename NewDefaultProvider,
          typename OldDefaultProvider, typename DiffFn>
bool DiffOptionalValue(const OptionalT& new_value, const OptionalT& old_value,
                       NewDefaultProvider&& new_default_provider,
                       OldDefaultProvider&& old_default_provider,
                       DiffFn&& diff_fn) {
  return DiffOptionalValueImpl(
      static_cast<OptionalT*>(nullptr), new_value, old_value,
      std::forward<NewDefaultProvider>(new_default_provider),
      std::forward<OldDefaultProvider>(old_default_provider),
      std::forward<DiffFn>(diff_fn), [](auto&, const auto&) {});
}

template <typename OptionalT, typename NewDefaultProvider,
          typename OldDefaultProvider, typename DiffFn, typename MaterializeFn>
bool DiffOptionalValue(OptionalT& new_value, const OptionalT& old_value,
                       NewDefaultProvider&& new_default_provider,
                       OldDefaultProvider&& old_default_provider,
                       DiffFn&& diff_fn, MaterializeFn&& materialize_new) {
  return DiffOptionalValueImpl(
      &new_value, new_value, old_value,
      std::forward<NewDefaultProvider>(new_default_provider),
      std::forward<OldDefaultProvider>(old_default_provider),
      std::forward<DiffFn>(diff_fn),
      std::forward<MaterializeFn>(materialize_new));
}

inline bool MarkScalarIfChanged(starlight::ComputedCSSStyle& style,
                                const void* new_val, const void* old_val,
                                size_t size, CSSPropertyID id) {
  if (memcmp(new_val, old_val, size) != 0) {
    style.MarkChanged(id);
    return true;
  }
  return false;
}

template <typename T>
bool MarkIfChanged(starlight::ComputedCSSStyle& style, const T& new_val,
                   const T& old_val, CSSPropertyID id) {
  if constexpr (std::is_trivially_copyable<T>::value) {
    return MarkScalarIfChanged(style, &new_val, &old_val, sizeof(T), id);
  } else {
    if (new_val != old_val) {
      style.MarkChanged(id);
      return true;
    }
    return false;
  }
}

bool MarkChangedIf(starlight::ComputedCSSStyle& style, bool is_changed,
                   CSSPropertyID id) {
  if (is_changed) {
    style.MarkChanged(id);
    return true;
  }
  return false;
}

template <typename T>
bool PtrValuesDiffer(const T* a, const T* b) {
  if (a == b) return false;
  if (!a || !b) return true;
  return *a != *b;
}

template <typename T>
bool RefPtrValueEqual(const T& lhs, const T& rhs) {
  if (lhs == rhs) return true;
  return lhs && rhs && *lhs == *rhs;
}

template <typename T, typename MemberType>
bool MarkFieldIfChanged(starlight::ComputedCSSStyle& style, const T& n,
                        const T& o, MemberType T::*member, CSSPropertyID id) {
  return MarkIfChanged(style, n.*member, o.*member, id);
}

struct DiffAndMarkFn {
  starlight::ComputedCSSStyle& style;
  CSSPropertyID id;
  template <typename U>
  bool operator()(const U& nv, const U& ov) const {
    if (nv == ov) return false;
    style.MarkChanged(id);
    return true;
  }
};

template <typename T, typename NewDP, typename OldDP>
bool DiffOptionalAndMark(starlight::ComputedCSSStyle& style, const T& n,
                         const T& o, NewDP&& ndp, OldDP&& odp,
                         CSSPropertyID id) {
  return DiffOptionalValue(n, o, std::forward<NewDP>(ndp),
                           std::forward<OldDP>(odp), DiffAndMarkFn{style, id});
}

#define BOX_DIFF_FIELDS(V)                                 \
  V(starlight::BoxData, width_, kPropertyIDWidth)          \
  V(starlight::BoxData, height_, kPropertyIDHeight)        \
  V(starlight::BoxData, min_width_, kPropertyIDMinWidth)   \
  V(starlight::BoxData, max_width_, kPropertyIDMaxWidth)   \
  V(starlight::BoxData, min_height_, kPropertyIDMinHeight) \
  V(starlight::BoxData, max_height_, kPropertyIDMaxHeight) \
  V(starlight::BoxData, aspect_ratio_, kPropertyIDAspectRatio)

#define FLEX_DIFF_FIELDS(V)                                           \
  V(starlight::FlexData, flex_direction_, kPropertyIDFlexDirection)   \
  V(starlight::FlexData, flex_wrap_, kPropertyIDFlexWrap)             \
  V(starlight::FlexData, justify_content_, kPropertyIDJustifyContent) \
  V(starlight::FlexData, align_items_, kPropertyIDAlignItems)         \
  V(starlight::FlexData, align_self_, kPropertyIDAlignSelf)           \
  V(starlight::FlexData, align_content_, kPropertyIDAlignContent)     \
  V(starlight::FlexData, flex_grow_, kPropertyIDFlexGrow)             \
  V(starlight::FlexData, flex_shrink_, kPropertyIDFlexShrink)         \
  V(starlight::FlexData, flex_basis_, kPropertyIDFlexBasis)           \
  V(starlight::FlexData, order_, kPropertyIDOrder)

#define GRID_SINGLE_DIFF_FIELDS(V)                                       \
  V(starlight::GridData, grid_column_gap_, kPropertyIDColumnGap)         \
  V(starlight::GridData, grid_row_gap_, kPropertyIDRowGap)               \
  V(starlight::GridData, grid_auto_flow_, kPropertyIDGridAutoFlow)       \
  V(starlight::GridData, justify_self_, kPropertyIDJustifySelf)          \
  V(starlight::GridData, justify_items_, kPropertyIDJustifyItems)        \
  V(starlight::GridData, grid_column_start_, kPropertyIDGridColumnStart) \
  V(starlight::GridData, grid_column_end_, kPropertyIDGridColumnEnd)     \
  V(starlight::GridData, grid_row_start_, kPropertyIDGridRowStart)       \
  V(starlight::GridData, grid_row_end_, kPropertyIDGridRowEnd)           \
  V(starlight::GridData, grid_column_span_, kPropertyIDGridColumnSpan)   \
  V(starlight::GridData, grid_row_span_, kPropertyIDGridRowSpan)

#define LINEAR_DIFF_FIELDS(V)                                                 \
  V(starlight::LinearData, linear_orientation_, kPropertyIDLinearOrientation) \
  V(starlight::LinearData, linear_layout_gravity_,                            \
    kPropertyIDLinearLayoutGravity)                                           \
  V(starlight::LinearData, linear_gravity_, kPropertyIDLinearGravity)         \
  V(starlight::LinearData, linear_cross_gravity_,                             \
    kPropertyIDLinearCrossGravity)                                            \
  V(starlight::LinearData, linear_weight_sum_, kPropertyIDLinearWeightSum)    \
  V(starlight::LinearData, linear_weight_, kPropertyIDLinearWeight)           \
  V(starlight::LinearData, list_main_axis_gap_, kPropertyIDListMainAxisGap)   \
  V(starlight::LinearData, list_cross_axis_gap_, kPropertyIDListCrossAxisGap)

#define RELATIVE_DIFF_FIELDS(V)                                                \
  V(starlight::RelativeData, relative_id_, kPropertyIDRelativeId)              \
  V(starlight::RelativeData, relative_align_top_, kPropertyIDRelativeAlignTop) \
  V(starlight::RelativeData, relative_align_right_,                            \
    kPropertyIDRelativeAlignRight)                                             \
  V(starlight::RelativeData, relative_align_bottom_,                           \
    kPropertyIDRelativeAlignBottom)                                            \
  V(starlight::RelativeData, relative_align_left_,                             \
    kPropertyIDRelativeAlignLeft)                                              \
  V(starlight::RelativeData, relative_top_of_, kPropertyIDRelativeTopOf)       \
  V(starlight::RelativeData, relative_right_of_, kPropertyIDRelativeRightOf)   \
  V(starlight::RelativeData, relative_bottom_of_, kPropertyIDRelativeBottomOf) \
  V(starlight::RelativeData, relative_left_of_, kPropertyIDRelativeLeftOf)     \
  V(starlight::RelativeData, relative_layout_once_,                            \
    kPropertyIDRelativeLayoutOnce)                                             \
  V(starlight::RelativeData, relative_center_, kPropertyIDRelativeCenter)

#define SURROUND_DIFF_FIELDS(V)                                         \
  V(starlight::SurroundData, margin_left_, kPropertyIDMarginLeft)       \
  V(starlight::SurroundData, margin_right_, kPropertyIDMarginRight)     \
  V(starlight::SurroundData, margin_top_, kPropertyIDMarginTop)         \
  V(starlight::SurroundData, margin_bottom_, kPropertyIDMarginBottom)   \
  V(starlight::SurroundData, padding_left_, kPropertyIDPaddingLeft)     \
  V(starlight::SurroundData, padding_right_, kPropertyIDPaddingRight)   \
  V(starlight::SurroundData, padding_top_, kPropertyIDPaddingTop)       \
  V(starlight::SurroundData, padding_bottom_, kPropertyIDPaddingBottom) \
  V(starlight::SurroundData, left_, kPropertyIDLeft)                    \
  V(starlight::SurroundData, right_, kPropertyIDRight)                  \
  V(starlight::SurroundData, top_, kPropertyIDTop)                      \
  V(starlight::SurroundData, bottom_, kPropertyIDBottom)

#define BORDER_DIFF_FIELDS(V)                                           \
  V(starlight::BordersData, width_left, kPropertyIDBorderLeftWidth)     \
  V(starlight::BordersData, width_right, kPropertyIDBorderRightWidth)   \
  V(starlight::BordersData, width_top, kPropertyIDBorderTopWidth)       \
  V(starlight::BordersData, width_bottom, kPropertyIDBorderBottomWidth) \
  V(starlight::BordersData, color_left, kPropertyIDBorderLeftColor)     \
  V(starlight::BordersData, color_right, kPropertyIDBorderRightColor)   \
  V(starlight::BordersData, color_top, kPropertyIDBorderTopColor)       \
  V(starlight::BordersData, color_bottom, kPropertyIDBorderBottomColor) \
  V(starlight::BordersData, style_left, kPropertyIDBorderLeftStyle)     \
  V(starlight::BordersData, style_right, kPropertyIDBorderRightStyle)   \
  V(starlight::BordersData, style_top, kPropertyIDBorderTopStyle)       \
  V(starlight::BordersData, style_bottom, kPropertyIDBorderBottomStyle)

#define BACKGROUND_IMAGE_FIELDS(V) \
  V(position) V(repeat) V(size) V(origin) V(clip)

#define VISUAL_SCALAR_DIFF_FIELDS(V)             \
  V(clip_path_, kPropertyIDClipPath)             \
  V(offset_path_, kPropertyIDOffsetPath)         \
  V(z_index_, kPropertyIDZIndex)                 \
  V(opacity_, kPropertyIDOpacity)                \
  V(offset_distance_, kPropertyIDOffsetDistance) \
  V(offset_rotate_, kPropertyIDOffsetRotate)     \
  V(handle_color_, kPropertyIDXHandleColor)      \
  V(handle_size_, kPropertyIDXHandleSize)        \
  V(image_rendering_, kPropertyIDImageRendering) \
  V(app_region_, kPropertyIDXAppRegion)          \
  V(visibility_, kPropertyIDVisibility)          \
  V(pointer_events_, kPropertyIDPointerEvents)

#define TEXT_DIFF_FIELDS(V)                                              \
  V(starlight::TextAttributes, font_size, kPropertyIDFontSize)           \
  V(starlight::TextAttributes, font_weight, kPropertyIDFontWeight)       \
  V(starlight::TextAttributes, font_style, kPropertyIDFontStyle)         \
  V(starlight::TextAttributes, font_family, kPropertyIDFontFamily)       \
  V(starlight::TextAttributes, text_align, kPropertyIDTextAlign)         \
  V(starlight::TextAttributes, white_space, kPropertyIDWhiteSpace)       \
  V(starlight::TextAttributes, text_overflow, kPropertyIDTextOverflow)   \
  V(starlight::TextAttributes, word_break, kPropertyIDWordBreak)         \
  V(starlight::TextAttributes, line_spacing, kPropertyIDLineSpacing)     \
  V(starlight::TextAttributes, letter_spacing, kPropertyIDLetterSpacing) \
  V(starlight::TextAttributes, text_indent, kPropertyIDTextIndent)       \
  V(starlight::TextAttributes, hyphens, kPropertyIDHyphens)              \
  V(starlight::TextAttributes, font_optical_sizing,                      \
    kPropertyIDFontOpticalSizing)

#define PLACEHOLDER_TEXT_DIFF_FIELDS(V)                                        \
  V(starlight::TextAttributes, font_family, kPropertyIDXPlaceholderFontFamily) \
  V(starlight::TextAttributes, font_size, kPropertyIDXPlaceholderFontSize)     \
  V(starlight::TextAttributes, font_weight, kPropertyIDXPlaceholderFontWeight) \
  V(starlight::TextAttributes, font_style, kPropertyIDXPlaceholderFontStyle)

bool DiffTextAttributesColor(starlight::ComputedCSSStyle& style,
                             const starlight::TextAttributes& n,
                             const starlight::TextAttributes& o,
                             CSSPropertyID color_id) {
  if (n.color.value_or(starlight::DefaultColor::DEFAULT_TEXT_COLOR) !=
          o.color.value_or(starlight::DefaultColor::DEFAULT_TEXT_COLOR) ||
      n.text_gradient != o.text_gradient) {
    style.MarkChanged(color_id);
    return true;
  }
  return false;
}

struct BackgroundDiffIds {
  CSSPropertyID image;
  CSSPropertyID position;
  CSSPropertyID repeat;
  CSSPropertyID size;
  CSSPropertyID origin;
  CSSPropertyID clip;
};

starlight::OutLineData DefaultOutline(
    const starlight::ComputedCSSStyle& style) {
  starlight::OutLineData d;
  d.style = style.GetCssAlignLegacyWithW3c()
                ? starlight::DefaultLayoutStyle::W3C_DEFAULT_BORDER_STYLE
                : starlight::DefaultLayoutStyle::SL_DEFAULT_BORDER_STYLE;
  return d;
}

starlight::TextAttributes DefaultTextAttributes(
    const starlight::ComputedCSSStyle& style) {
  return starlight::TextAttributes(
      DEFAULT_FONT_SIZE_DP * style.GetMeasureContext().layouts_unit_per_px_);
}
}  // namespace

bool StyleResolver::ComputeStyleDiff(
    starlight::ComputedCSSStyle& new_style,
    const starlight::ComputedCSSStyle& old_style) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, STYLE_RESOLVER_COMPUTE_STYLE_DIFF);
  bool changed = false;

#define MARK_PTR_FIELD(type, field, id) \
  changed |= MarkFieldIfChanged(new_style, *n, *o, &type::field, id);
  if (auto *n = new_style.layout_computed_style_.box_data_.Get(),
      *o = old_style.layout_computed_style_.box_data_.Get();
      n != o) {
    BOX_DIFF_FIELDS(MARK_PTR_FIELD)
  }
  if (auto *n = new_style.layout_computed_style_.flex_data_.Get(),
      *o = old_style.layout_computed_style_.flex_data_.Get();
      n != o) {
    FLEX_DIFF_FIELDS(MARK_PTR_FIELD)
  }
  if (auto *n = new_style.layout_computed_style_.grid_data_.Get(),
      *o = old_style.layout_computed_style_.grid_data_.Get();
      n != o) {
    changed |= MarkChangedIf(
        new_style,
        n->grid_template_columns_min_track_sizing_function_ !=
                o->grid_template_columns_min_track_sizing_function_ ||
            n->grid_template_columns_max_track_sizing_function_ !=
                o->grid_template_columns_max_track_sizing_function_,
        kPropertyIDGridTemplateColumns);
    changed |= MarkChangedIf(
        new_style,
        n->grid_template_rows_min_track_sizing_function_ !=
                o->grid_template_rows_min_track_sizing_function_ ||
            n->grid_template_rows_max_track_sizing_function_ !=
                o->grid_template_rows_max_track_sizing_function_,
        kPropertyIDGridTemplateRows);
    changed |=
        MarkChangedIf(new_style,
                      n->grid_auto_columns_min_track_sizing_function_ !=
                              o->grid_auto_columns_min_track_sizing_function_ ||
                          n->grid_auto_columns_max_track_sizing_function_ !=
                              o->grid_auto_columns_max_track_sizing_function_,
                      kPropertyIDGridAutoColumns);
    changed |=
        MarkChangedIf(new_style,
                      n->grid_auto_rows_min_track_sizing_function_ !=
                              o->grid_auto_rows_min_track_sizing_function_ ||
                          n->grid_auto_rows_max_track_sizing_function_ !=
                              o->grid_auto_rows_max_track_sizing_function_,
                      kPropertyIDGridAutoRows);
    GRID_SINGLE_DIFF_FIELDS(MARK_PTR_FIELD)
  }
  if (auto *n = new_style.layout_computed_style_.linear_data_.Get(),
      *o = old_style.layout_computed_style_.linear_data_.Get();
      n != o) {
    LINEAR_DIFF_FIELDS(MARK_PTR_FIELD)
  }
  if (auto *n = new_style.layout_computed_style_.relative_data_.Get(),
      *o = old_style.layout_computed_style_.relative_data_.Get();
      n != o) {
    RELATIVE_DIFF_FIELDS(MARK_PTR_FIELD)
  }
#undef MARK_PTR_FIELD

#define MARK_REF_FIELD(type, field, id) \
  changed |= MarkFieldIfChanged(new_style, n, o, &type::field, id);
  {
    const auto& n = new_style.layout_computed_style_.surround_data_;
    const auto& o = old_style.layout_computed_style_.surround_data_;
    SURROUND_DIFF_FIELDS(MARK_REF_FIELD)
    const bool new_w3c =
        n.border_data_ ? n.border_data_->css_align_with_legacy_w3c_ : false;
    const bool old_w3c =
        o.border_data_ ? o.border_data_->css_align_with_legacy_w3c_ : false;
    const bool new_has = n.border_data_.has_value();
    const bool old_has = o.border_data_.has_value();
    changed |= DiffOptionalValue(
        n.border_data_, o.border_data_,
        [new_has, new_w3c, old_w3c]() {
          return starlight::BordersData(new_has ? new_w3c : old_w3c);
        },
        [old_has, old_w3c, new_w3c]() {
          return starlight::BordersData(old_has ? old_w3c : new_w3c);
        },
        [&](const auto& n, const auto& o) {
          bool changed = false;
          BORDER_DIFF_FIELDS(MARK_REF_FIELD)
          changed |=
              MarkChangedIf(new_style,
                            n.radius_x_top_left != o.radius_x_top_left ||
                                n.radius_y_top_left != o.radius_y_top_left,
                            kPropertyIDBorderTopLeftRadius);
          changed |=
              MarkChangedIf(new_style,
                            n.radius_x_top_right != o.radius_x_top_right ||
                                n.radius_y_top_right != o.radius_y_top_right,
                            kPropertyIDBorderTopRightRadius);
          changed |= MarkChangedIf(
              new_style,
              n.radius_x_bottom_right != o.radius_x_bottom_right ||
                  n.radius_y_bottom_right != o.radius_y_bottom_right,
              kPropertyIDBorderBottomRightRadius);
          changed |= MarkChangedIf(
              new_style,
              n.radius_x_bottom_left != o.radius_x_bottom_left ||
                  n.radius_y_bottom_left != o.radius_y_bottom_left,
              kPropertyIDBorderBottomLeftRadius);
          return changed;
        });
  }
#undef MARK_REF_FIELD

  const auto& new_layout = new_style.layout_computed_style_;
  const auto& old_layout = old_style.layout_computed_style_;
  changed |= MarkIfChanged(new_style, new_layout.position_,
                           old_layout.position_, kPropertyIDPosition);
  changed |= MarkIfChanged(new_style, new_layout.display_, old_layout.display_,
                           kPropertyIDDisplay);
  changed |= MarkIfChanged(new_style, new_layout.box_sizing_,
                           old_layout.box_sizing_, kPropertyIDBoxSizing);
  changed |= MarkIfChanged(new_style, new_layout.direction_,
                           old_layout.direction_, kPropertyIDDirection);

  auto diff_image = [&](const auto& n, const auto& o,
                        const BackgroundDiffIds& ids) {
    bool changed = MarkChangedIf(
        new_style, n.image_count != o.image_count || n.image != o.image,
        ids.image);
#define MARK_IMAGE_FIELD(field) \
  changed |= MarkIfChanged(new_style, n.field, o.field, ids.field);
    BACKGROUND_IMAGE_FIELDS(MARK_IMAGE_FIELD)
#undef MARK_IMAGE_FIELD
    return changed;
  };
  auto diff_background = [&](const auto& n, const auto& o,
                             const BackgroundDiffIds& ids, bool has_color) {
    bool changed = false;
    if (has_color) {
      changed |=
          MarkFieldIfChanged(new_style, n, o, &starlight::BackgroundData::color,
                             kPropertyIDBackgroundColor);
    }
    changed |= DiffOptionalValue(
        n.image_data, o.image_data,
        SharedDefaultProvider<starlight::BackgroundData::BackgroundImageData>{},
        SharedDefaultProvider<starlight::BackgroundData::BackgroundImageData>{},
        [&](const auto& ni, const auto& oi) {
          return diff_image(ni, oi, ids);
        });
    if (has_color && changed) new_style.MarkChanged(kPropertyIDBackground);
    return changed;
  };
  changed |= DiffOptionalValue(
      new_style.background_data_, old_style.background_data_,
      SharedDefaultProvider<starlight::BackgroundData>{},
      SharedDefaultProvider<starlight::BackgroundData>{},
      [&](const auto& n, const auto& o) {
        return diff_background(
            n, o,
            {kPropertyIDBackgroundImage, kPropertyIDBackgroundPosition,
             kPropertyIDBackgroundRepeat, kPropertyIDBackgroundSize,
             kPropertyIDBackgroundOrigin, kPropertyIDBackgroundClip},
            true);
      });
  changed |= DiffOptionalValue(
      new_style.mask_data_, old_style.mask_data_,
      SharedDefaultProvider<starlight::BackgroundData>{},
      SharedDefaultProvider<starlight::BackgroundData>{},
      [&](const auto& n, const auto& o) {
        return diff_background(n, o,
                               {kPropertyIDMaskImage, kPropertyIDMaskPosition,
                                kPropertyIDMaskRepeat, kPropertyIDMaskSize,
                                kPropertyIDMaskOrigin, kPropertyIDMaskClip},
                               false);
      });
  changed |= DiffOptionalValue(
      new_style.outline_, old_style.outline_,
      [&new_style]() { return DefaultOutline(new_style); },
      [&old_style]() { return DefaultOutline(old_style); },
      [&](const auto& n, const auto& o) {
        bool changed = false;
        changed |=
            MarkFieldIfChanged(new_style, n, o, &starlight::OutLineData::color,
                               kPropertyIDOutlineColor);
        changed |=
            MarkFieldIfChanged(new_style, n, o, &starlight::OutLineData::style,
                               kPropertyIDOutlineStyle);
        changed |=
            MarkFieldIfChanged(new_style, n, o, &starlight::OutLineData::width,
                               kPropertyIDOutlineWidth);
        return changed;
      },
      [](auto& outline, const auto& effective_new) {
        outline.emplace(effective_new);
      });
  changed |= DiffOptionalAndMark(
      new_style, new_style.box_shadow_, old_style.box_shadow_,
      SharedDefaultProvider<base::InlineVector<starlight::ShadowData, 1>>{},
      SharedDefaultProvider<base::InlineVector<starlight::ShadowData, 1>>{},
      kPropertyIDBoxShadow);
  changed |= DiffOptionalAndMark(
      new_style, new_style.transform_raw_, old_style.transform_raw_,
      SharedDefaultProvider<
          base::InlineVector<starlight::TransformRawData, 1>>{},
      SharedDefaultProvider<
          base::InlineVector<starlight::TransformRawData, 1>>{},
      kPropertyIDTransform);
  changed |= DiffOptionalAndMark(
      new_style, new_style.transform_origin_, old_style.transform_origin_,
      SharedDefaultProvider<starlight::TransformOriginData>{},
      SharedDefaultProvider<starlight::TransformOriginData>{},
      kPropertyIDTransformOrigin);
  changed |= DiffOptionalAndMark(
      new_style, new_style.perspective_data_, old_style.perspective_data_,
      SharedDefaultProvider<starlight::PerspectiveData>{},
      SharedDefaultProvider<starlight::PerspectiveData>{},
      kPropertyIDPerspective);
  changed |= DiffOptionalAndMark(
      new_style, new_style.filter_, old_style.filter_,
      SharedDefaultProvider<starlight::FilterData>{},
      SharedDefaultProvider<starlight::FilterData>{}, kPropertyIDFilter);
  if (OptionalNotEqual(new_style.cursor_, old_style.cursor_)) {
    new_style.MarkChanged(kPropertyIDCursor);
    changed = true;
  }
#define MARK_VISUAL_SCALAR(field, id) \
  changed |= MarkIfChanged(new_style, new_style.field, old_style.field, id);
  VISUAL_SCALAR_DIFF_FIELDS(MARK_VISUAL_SCALAR)
#undef MARK_VISUAL_SCALAR

  changed |= MarkIfChanged(new_style, new_style.caret_color_,
                           old_style.caret_color_, kPropertyIDCaretColor);
  changed |= DiffOptionalValue(
      new_style.text_attributes_, old_style.text_attributes_,
      [&new_style]() { return DefaultTextAttributes(new_style); },
      [&old_style]() { return DefaultTextAttributes(old_style); },
      [&](const auto& n, const auto& o) {
        bool changed = false;
        changed |= DiffTextAttributesColor(new_style, n, o, kPropertyIDColor);
#define MARK_TEXT_FIELD(type, field, id) \
  changed |= MarkFieldIfChanged(new_style, n, o, &type::field, id);
        TEXT_DIFF_FIELDS(MARK_TEXT_FIELD)
#undef MARK_TEXT_FIELD
        changed |=
            MarkChangedIf(new_style,
                          n.computed_line_height != o.computed_line_height ||
                              n.line_height_factor != o.line_height_factor,
                          kPropertyIDLineHeight);
        if (n.underline_decoration != o.underline_decoration ||
            n.line_through_decoration != o.line_through_decoration ||
            n.text_decoration_style != o.text_decoration_style ||
            n.text_decoration_color.value_or(
                starlight::DefaultColor::DEFAULT_COLOR) !=
                o.text_decoration_color.value_or(
                    starlight::DefaultColor::DEFAULT_COLOR)) {
          new_style.MarkChanged(kPropertyIDTextDecoration);
          new_style.MarkChanged(kPropertyIDTextDecorationColor);
          changed = true;
        }
        if (n.decoration_color.value_or(
                starlight::DefaultColor::DEFAULT_COLOR) !=
            o.decoration_color.value_or(
                starlight::DefaultColor::DEFAULT_COLOR)) {
          new_style.MarkChanged(kPropertyIDTextDecorationColor);
          changed = true;
        }
        changed |= DiffOptionalAndMark(
            new_style, n.text_shadow, o.text_shadow,
            SharedDefaultProvider<
                base::InlineVector<starlight::ShadowData, 1>>{},
            SharedDefaultProvider<
                base::InlineVector<starlight::ShadowData, 1>>{},
            kPropertyIDTextShadow);
        if (n.vertical_align != o.vertical_align ||
            base::FloatsNotEqual(n.vertical_align_length,
                                 o.vertical_align_length)) {
          new_style.MarkChanged(kPropertyIDVerticalAlign);
          changed = true;
        }
        const bool stroke_width_changed =
            n.text_stroke_width != o.text_stroke_width;
        const bool stroke_color_changed =
            n.text_stroke_color.value_or(
                starlight::DefaultColor::DEFAULT_COLOR) !=
            o.text_stroke_color.value_or(
                starlight::DefaultColor::DEFAULT_COLOR);
        if (stroke_width_changed || stroke_color_changed) {
          new_style.MarkChanged(kPropertyIDTextStroke);
          changed = true;
        }
        if (stroke_width_changed) {
          new_style.MarkChanged(kPropertyIDTextStrokeWidth);
          changed = true;
        }
        if (stroke_color_changed) {
          new_style.MarkChanged(kPropertyIDTextStrokeColor);
          changed = true;
        }
        if (!RefPtrValueEqual(n.font_variation_settings,
                              o.font_variation_settings)) {
          new_style.MarkChanged(kPropertyIDFontVariationSettings);
          changed = true;
        }
        if (!RefPtrValueEqual(n.font_feature_settings,
                              o.font_feature_settings)) {
          new_style.MarkChanged(kPropertyIDFontFeatureSettings);
          changed = true;
        }
        if (n.is_auto_font_size != o.is_auto_font_size ||
            base::FloatsNotEqual(n.auto_font_size_min_size,
                                 o.auto_font_size_min_size) ||
            base::FloatsNotEqual(n.auto_font_size_max_size,
                                 o.auto_font_size_max_size) ||
            base::FloatsNotEqual(n.auto_font_size_step_granularity,
                                 o.auto_font_size_step_granularity)) {
          new_style.MarkChanged(kPropertyIDXAutoFontSize);
          changed = true;
        }
        changed |= DiffOptionalAndMark(
            new_style, n.auto_font_size_preset_sizes,
            o.auto_font_size_preset_sizes,
            SharedDefaultProvider<base::InlineVector<float, 6>>{},
            SharedDefaultProvider<base::InlineVector<float, 6>>{},
            kPropertyIDXAutoFontSizePresetSizes);
        changed |= DiffOptionalAndMark(
            new_style, n.auto_font_size_line_ranges,
            o.auto_font_size_line_ranges,
            SharedDefaultProvider<
                std::vector<starlight::AutoFontSizeLineRange>>{},
            SharedDefaultProvider<
                std::vector<starlight::AutoFontSizeLineRange>>{},
            kPropertyIDXAutoFontSizeLineRanges);
        return changed;
      });
  changed |= DiffOptionalValue(
      new_style.placeholder_text_attributes_,
      old_style.placeholder_text_attributes_,
      [&new_style]() { return DefaultTextAttributes(new_style); },
      [&old_style]() { return DefaultTextAttributes(old_style); },
      [&](const auto& n, const auto& o) {
        bool changed = false;
        changed |= DiffTextAttributesColor(new_style, n, o,
                                           kPropertyIDXPlaceholderColor);
#define MARK_PLACEHOLDER_FIELD(type, field, id) \
  changed |= MarkFieldIfChanged(new_style, n, o, &type::field, id);
        PLACEHOLDER_TEXT_DIFF_FIELDS(MARK_PLACEHOLDER_FIELD)
#undef MARK_PLACEHOLDER_FIELD
        return changed;
      });
  changed |= MarkIfChanged(new_style, new_style.overflow_, old_style.overflow_,
                           kPropertyIDOverflow);
  changed |= MarkIfChanged(new_style, new_style.overflow_x_,
                           old_style.overflow_x_, kPropertyIDOverflowX);
  changed |= MarkIfChanged(new_style, new_style.overflow_y_,
                           old_style.overflow_y_, kPropertyIDOverflowY);
  changed |= PtrValuesDiffer(new_style.GetRawCustomProperties(),
                             old_style.GetRawCustomProperties()) ||
             PtrValuesDiffer(new_style.GetCustomProperties(),
                             old_style.GetCustomProperties());
  return changed;
}
#undef BOX_DIFF_FIELDS
#undef FLEX_DIFF_FIELDS
#undef GRID_SINGLE_DIFF_FIELDS
#undef LINEAR_DIFF_FIELDS
#undef RELATIVE_DIFF_FIELDS
#undef SURROUND_DIFF_FIELDS
#undef BORDER_DIFF_FIELDS
#undef BACKGROUND_IMAGE_FIELDS
#undef VISUAL_SCALAR_DIFF_FIELDS
#undef TEXT_DIFF_FIELDS
#undef PLACEHOLDER_TEXT_DIFF_FIELDS
}  // namespace tasm
}  // namespace lynx
