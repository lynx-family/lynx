// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/css_patching.h"

#include <algorithm>
#include <utility>

#include "base/include/algorithm.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/vdom/radon/radon_element.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
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

thread_local CSSPatching::MatchedVector<const StyleMap*>
    CSSPatching::matched_style_map;
thread_local CSSPatching::MatchedVector<const CSSVariableMap*>
    CSSPatching::matched_variable_map;

CSSPatching::CSSPatching(Element* element, ElementManager* manager)
    : element_(element), manager_(manager) {}

void CSSPatching::SetEnableFiberArch(bool enable) {
  css_var_handler_.SetEnableFiberArch(enable);
}

void CSSPatching::ResolveStyle(StyleMap& result, CSSFragment* fragment,
                               CSSVariableMap* changed_css_vars) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSPatching::ResolveStyle");

  if (element_ == nullptr || element_->data_model() == nullptr) {
    LOGE(
        "CSSPatching::ResolveStyle failed since element or data_model is "
        "nullptr.");
    return;
  }

  if (!element_->WillResolveStyle(result)) {
    return;
  }

  // Find the selectors that the current `Element` can match.
  if (fragment != nullptr) {
    if (fragment->enable_css_selector()) {
      GetCSSStyleNew(element_->data_model(), fragment);
    } else if (element_->is_fiber_element()) {
      GetCSSStyleForFiber(static_cast<FiberElement*>(element_), fragment);
    } else {
      /**
       CSS Selector Specificity Rules
       ID Selector > Class Selector > Tag Selector
       The specificity of the :not() selector is related to the content inside
       the parentheses, refer to the W3C specification
       https://www.w3.org/TR/selectors/#specificity-rules
       */
      GetCSSStyleCompatible(element_, fragment);
    }
  }

  DidCollectMatchedRules(element_->data_model(), result, changed_css_vars,
                         element_->CountInlineStyles());

  element_->MergeInlineStyles(result);

  HandleCSSVariables(result);

  element_->DidResolveStyle(result);
}

void CSSPatching::HandlePseudoElement(CSSFragment* fragment) {
  if (!fragment || !element_ || !element_->is_fiber_element()) {
    return;
  }
  // Determine whether there are pseudo-elements according to the rule set when
  // enable_css_selector is true, according to pseudo map when
  // enable_css_selector is false.
  if ((fragment->enable_css_selector() &&
       (!fragment->rule_set() ||
        fragment->rule_set()->pseudo_rules().empty())) ||
      (!fragment->enable_css_selector() && fragment->pseudo_map().empty())) {
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

void CSSPatching::ResolvePseudoElement(PseudoState pseudo_state,
                                       CSSFragment* fragment,
                                       FiberElement* fiber_element,
                                       const char* pseudo_selector) {
  StyleMap result;
  if (fragment->enable_css_selector()) {
    AttributeHolder attribute_holder;
    attribute_holder.AddPseudoState(pseudo_state);
    attribute_holder.SetPseudoElementOwner(fiber_element->data_model());
    GetCSSStyleNew(&attribute_holder, fragment);
    DidCollectMatchedRules(fiber_element->data_model(), result, nullptr);
  } else {
    ParsePseudoCSSTokensForFiber(fiber_element, fragment, pseudo_selector,
                                 result);
  }

  if (!result.empty()) {
    HandleCSSVariables(result);
  }
  fiber_element->PrepareOrUpdatePseudoElement(pseudo_state, result);
}

void CSSPatching::DidCollectMatchedRules(AttributeHolder* holder,
                                         StyleMap& result,
                                         CSSVariableMap* changed_css_vars,
                                         size_t base_reserving_size) {
  {
    auto& tls_matched_style_map = matched_style_map;

    // Precalculate reserve count of result map from matched maps.
    for (auto matched_style_ptr : tls_matched_style_map) {
      base_reserving_size += matched_style_ptr->size();
    }
    result.reserve(base_reserving_size);

    for (auto matched_style_ptr : tls_matched_style_map) {
      result.merge(*matched_style_ptr);
    }
    tls_matched_style_map.clear();
  }

  {
    auto& tls_matched_variable_map = matched_variable_map;
    if (tls_matched_variable_map.empty()) {
      return;
    }
    for (auto variable_ptr : tls_matched_variable_map) {
      for (const auto& [key, value] : *variable_ptr) {
        holder->UpdateCSSVariable(key, value, changed_css_vars);
      }
    }
    tls_matched_variable_map.clear();
  }
}

void CSSPatching::HandleCSSVariables(StyleMap& styles) {
  if (element_ == nullptr || element_->data_model() == nullptr) {
    LOGE(
        "CSSPatching::HandleCSSVariables failed since element or data_model is "
        "nullptr.");
    return;
  }
  if (element_->is_fiber_element() && element_->is_parallel_flush()) {
    if (css_var_handler_.HasCSSVariableInStyleMap(styles)) {
      // mark need refresh style in parallel flush with css variables in
      // StyleMap
      static_cast<FiberElement*>(element_)->MarkRefreshCSSStyles();
    }
  } else {
    css_var_handler_.HandleCSSVariables(styles, element_->data_model(),
                                        GetCSSParserConfigs());
  }
}

void CSSPatching::MergeHigherPriorityCSSStyle(const StyleMap& matched) {
  if (matched.empty()) {
    return;
  }
  matched_style_map.emplace_back(&matched);
}

void CSSPatching::SetCSSVariableToNode(const CSSVariableMap& matched) {
  if (matched.empty()) {
    return;
  }
  matched_variable_map.emplace_back(&matched);
}

void CSSPatching::GetCSSStyleCompatible(Element* element,
                                        CSSFragment* style_sheet) {
  auto* node = element->data_model();
  // absort the selector styles in :not(), then store the correspond selector
  // and scope the InitPseudoNotStyle function only judge once
  style_sheet->InitPseudoNotStyle();
  // If has_pseudo_not_style means the pseudo_not_style is not empty
  const bool has_pseudo_not_style = style_sheet->HasPseudoNotStyle();

  GetCSSByRule(CSSSheet::ALL_SELECT, style_sheet, node, "*");

  // first, handle the tag selector
  const base::String& tag_node = node->tag();
  if (!tag_node.empty()) {
    const std::string& rule_tag_selector = tag_node.str();
    if (has_pseudo_not_style) {
      PreSetGlobalPseudoNotCSS(
          CSSSheet::NAME_SELECT, rule_tag_selector,
          style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
          node);
    }

    GetCSSByRule(CSSSheet::NAME_SELECT, style_sheet, node, rule_tag_selector);

    if (has_pseudo_not_style) {
      report::GlobalFeatureCounter::Count(
          report::LynxFeature::CPP_ENABLE_PSEUDO_NOT_CSS,
          manager_->GetInstanceId());
      ApplyPseudoNotCSSStyle(node,
                             style_sheet->pseudo_not_style().pseudo_not_for_tag,
                             style_sheet, rule_tag_selector);
    }
    ApplyPseudoClassChildSelectorStyle(element, style_sheet, rule_tag_selector);
  }
  // Class selector
  if (has_pseudo_not_style) {
    // if the node doesnt contains andy class selectors, then apply the class
    // selectors from global :not() selector
    PreSetGlobalPseudoNotCSS(
        CSSSheet::CLASS_SELECT, "",
        style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
        node);
  }

  for (const auto& cls : node->classes()) {
    const auto rule_class_selector = GetClassSelectorRule(cls);
    GetCSSByRule(CSSSheet::CLASS_SELECT, style_sheet, node,
                 rule_class_selector);

    if (has_pseudo_not_style) {
      report::GlobalFeatureCounter::Count(
          report::LynxFeature::CPP_ENABLE_PSEUDO_NOT_CSS,
          manager_->GetInstanceId());
      ApplyPseudoNotCSSStyle(
          node, style_sheet->pseudo_not_style().pseudo_not_for_class,
          style_sheet, rule_class_selector);
    }
    ApplyPseudoClassChildSelectorStyle(element, style_sheet,
                                       rule_class_selector);
  }

  if (node->HasPseudoState(kPseudoStateFocus)) {
    GetPseudoClassStyle(PseudoClassType::kFocus, style_sheet, node);
  }

  if (node->HasPseudoState(kPseudoStateHover)) {
    GetPseudoClassStyle(PseudoClassType::kHover, style_sheet, node);
  }

  if (node->HasPseudoState(kPseudoStateActive)) {
    GetPseudoClassStyle(PseudoClassType::kActive, style_sheet, node);
  }

  // ID selector
  const base::String& id_node = node->idSelector();
  if (!id_node.empty()) {
    const std::string rule_id_selector = GetIDSelectorRule(id_node);
    if (has_pseudo_not_style) {
      PreSetGlobalPseudoNotCSS(
          CSSSheet::ID_SELECT, rule_id_selector,
          style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
          node);
    }

    GetCSSByRule(CSSSheet::ID_SELECT, style_sheet, node, rule_id_selector);

    if (has_pseudo_not_style) {
      report::GlobalFeatureCounter::Count(
          report::LynxFeature::CPP_ENABLE_PSEUDO_NOT_CSS,
          manager_->GetInstanceId());
      ApplyPseudoNotCSSStyle(node,
                             style_sheet->pseudo_not_style().pseudo_not_for_id,
                             style_sheet, rule_id_selector);
    }
    ApplyPseudoClassChildSelectorStyle(element, style_sheet, rule_id_selector);
  } else if (has_pseudo_not_style) {
    // if the node doesnt contains the id selector, then try to apply the id
    // selecotr form global :not() selector
    PreSetGlobalPseudoNotCSS(
        CSSSheet::ID_SELECT, "",
        style_sheet->pseudo_not_style().pseudo_not_global_map, style_sheet,
        node);
  }
}

static bool CompareRules(const css::MatchedRule& matched_rule1,
                         const css::MatchedRule& matched_rule2) {
  unsigned specificity1 = matched_rule1.Specificity();
  unsigned specificity2 = matched_rule2.Specificity();
  if (specificity1 != specificity2) return specificity1 < specificity2;

  return matched_rule1.Position() < matched_rule2.Position();
}

CSSPatching::MatchedVector<css::MatchedRule> CSSPatching::GetCSSMatchedRule(
    AttributeHolder* node, CSSFragment* style_sheet) {
  MatchedVector<css::MatchedRule> matched_rules;
  if (style_sheet && style_sheet->rule_set()) {
    unsigned level = 0;
    style_sheet->rule_set()->MatchStyles(node, level, matched_rules);
  }
  base::InsertionSort(matched_rules.data(), matched_rules.size(), CompareRules);
  return matched_rules;
}

void CSSPatching::GetCSSStyleNew(AttributeHolder* node,
                                 CSSFragment* style_sheet) {
  auto matched_rules = GetCSSMatchedRule(node, style_sheet);

  for (const auto& matched : matched_rules) {
    if (matched.Data()->Rule()->Token() != nullptr) {
      MergeHigherPriorityCSSStyle(
          matched.Data()->Rule()->Token().get()->GetAttributes());
      SetCSSVariableToNode(
          matched.Data()->Rule()->Token().get()->GetStyleVariables());
    }
  }
}

/**
   Preset Global :not() Styles
 */
void CSSPatching::PreSetGlobalPseudoNotCSS(
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
          MergeHigherPriorityCSSStyle(
              it_pseudo_not->second.get()->GetAttributes());
          SetCSSVariableToNode(
              it_pseudo_not->second.get()->GetStyleVariables());
        }
      }
    }
  }
}

void CSSPatching::ApplyPseudoNotCSSStyle(
    AttributeHolder* node, const PseudoClassStyleMap& pseudo_not_map,
    CSSFragment* style_sheet, const std::string& selector_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSPatching::ApplyPseudoNotCSSStyle");
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
          MergeHigherPriorityCSSStyle(
              it_pseudo_not->second.get()->GetAttributes());
          SetCSSVariableToNode(
              it_pseudo_not->second.get()->GetStyleVariables());
        }
      }
    }
  }
}

void CSSPatching::ApplyPseudoClassChildSelectorStyle(
    Element* current_node, CSSFragment* style_sheet,
    const std::string& selector_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "CSSPatching::ApplyPseudoClassChildSelectorStyle");
  if (current_node->IsFiberArch()) {
    // child selector is only supported in RadonArch.
    return;
  }
  const CSSParserTokenMap& child_pseudo = style_sheet->child_pseudo_map();
  if (child_pseudo.empty()) {
    return;
  }
  Element* parent = nullptr;
  if (current_node->is_fiber_element()) {
    parent = current_node->parent();
  } else {
    auto* radon_node = current_node->data_model()->radon_node_ptr();
    if (!radon_node) {
      return;
    }
    auto* radon_parent = radon_node->NodeParent();
    if (!radon_parent) {
      return;
    }
    parent = radon_parent->element();
  }
  if (!parent) {
    return;
  }
  for (const auto& it : child_pseudo) {
    if (it.second && it.second->IsPseudoStyleToken() &&
        it.first.compare(0, selector_key.size(), selector_key) == 0) {
      if (it.first.find(kCSSSelectorFirstChild) != std::string::npos) {
        if (current_node == parent->first_child()) {
          report::GlobalFeatureCounter::Count(
              report::LynxFeature::CPP_ENABLE_PSEUDO_CHILD_CSS,
              manager_->GetInstanceId());
          MergeHigherPriorityCSSStyle(it.second.get()->GetAttributes());
          SetCSSVariableToNode(it.second.get()->GetStyleVariables());
        }
      }
      if (it.first.find(kCSSSelectorLastChild) != std::string::npos) {
        if (current_node == parent->last_child()) {
          report::GlobalFeatureCounter::Count(
              report::LynxFeature::CPP_ENABLE_PSEUDO_CHILD_CSS,
              manager_->GetInstanceId());
          MergeHigherPriorityCSSStyle(it.second.get()->GetAttributes());
          SetCSSVariableToNode(it.second.get()->GetStyleVariables());
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
void CSSPatching::GetCSSByRule(CSSSheet::SheetType type,
                               CSSFragment* style_sheet, AttributeHolder* node,
                               const std::string& rule) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSPatching::GetCSSByRule",
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

  if (token != nullptr) {
    MergeHigherPriorityCSSStyle(token->GetAttributes());
    SetCSSVariableToNode(token->GetStyleVariables());
  }

  if ((type == CSSSheet::CLASS_SELECT || type == CSSSheet::ID_SELECT) &&
      style_sheet->HasCascadeStyle()) {
    ApplyCascadeStyles(style_sheet, node, rule);
  }
}

void CSSPatching::MergeHigherCascadeStyles(const std::string& current_selector,
                                           const std::string& parent_selector,
                                           AttributeHolder* node,
                                           CSSFragment* style_sheet) {
  std::string integrated_selector =
      MergeCSSSelector(current_selector, parent_selector);
  CSSParseToken* token_parent =
      style_sheet->GetCascadeStyle(integrated_selector);
  if (token_parent != nullptr) {
    MergeHigherPriorityCSSStyle(token_parent->GetAttributes());
    SetCSSVariableToNode(token_parent->GetStyleVariables());
  }
}

void CSSPatching::ApplyCascadeStyles(CSSFragment* style_sheet,
                                     AttributeHolder* node,
                                     const std::string& rule) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSPatching::ApplyCascadeStyles");
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

void CSSPatching::GetPseudoClassStyle(PseudoClassType pseudo_type,
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

void CSSPatching::ResolvePseudoSelectors() {
  if (element_ == nullptr || element_->data_model() == nullptr) {
    LOGE(
        "CSSPatching::ResolvePseudoSelectors failed since element or "
        "data_model "
        "is nullptr.");
    return;
  }

  if (element_->is_fiber_element()) {
    return;
  }

  CSSFragment* fragment = element_->GetRelatedCSSFragment();
  if (fragment == nullptr) {
    LOGE(
        "CSSPatching::ResolvePseudoSelectors failed since fragment is "
        "nullptr.");
    return;
  }

  auto* radon_element = static_cast<RadonElement*>(element_);

  if (fragment->enable_css_selector()) {
    if (element_->GetTag() == "text") {  // only text can support selection
      AttributeHolder selection;
      selection.AddPseudoState(kPseudoStateSelection);
      selection.SetPseudoElementOwner(element_->data_model());
      GetCSSStyleNew(&selection, fragment);

      StyleMap result;
      DidCollectMatchedRules(element_->data_model(), result, nullptr);

      if (result.empty()) {
        return;
      }
      // ::selection
      auto pseudo_node = CreatePseudoNode(CSSSheet::SELECTION_SELECT);
      if (!pseudo_node) {
        return;
      }
      report::GlobalFeatureCounter::Count(
          report::LynxFeature::CPP_ENABLE_PSEUDO_SELECTOR,
          manager_->GetInstanceId());
      pseudo_node->SetIsPseudoNode();
      pseudo_node->ConsumeStyle(result);
      pseudo_node->FlushProps();
      radon_element->InsertNode(pseudo_node);
    }
    return;
  }

  if (!fragment->HasPseudoStyle()) {
    return;
  }

  if (radon_element->GetTag() == "text") {  // only text can support selection
    // ::selection
    UpdateSelectionPseudo(ParsePseudoCSSTokens(radon_element->data_model(),
                                               kCSSSelectorSelection),
                          radon_element);
  }
}

void CSSPatching::ResolvePlaceHolder() {
  if (element_ == nullptr || element_->data_model() == nullptr) {
    LOGE(
        "CSSPatching::ResolvePlaceHolder failed since element or data_model is "
        "nullptr.");
    return;
  }

  CSSFragment* fragment = element_->GetRelatedCSSFragment();
  if (fragment == nullptr) {
    LOGE("CSSPatching::ResolvePlaceHolder failed since fragment is nullptr.");
    return;
  }

  if (element_->is_fiber_element()) {
    return;
  }

  if (!element_->HasPlaceHolder()) {
    return;
  }

  auto* radon_element = static_cast<RadonElement*>(element_);

  // Resolve other pseudo selectors
  //::placeholder styles
  if (fragment->enable_css_selector()) {
    AttributeHolder placeholder;
    placeholder.AddPseudoState(kPseudoStatePlaceHolder);
    placeholder.SetPseudoElementOwner(radon_element->data_model());
    GetCSSStyleNew(&placeholder, fragment);

    StyleMap result;
    DidCollectMatchedRules(element_->data_model(), result, nullptr);

    PseudoPlaceHolderStyles style;
    ParsePlaceHolderTokens(style, result);
    radon_element->SetPlaceHolderStyles(style);
  } else {
    auto tokens = ParsePseudoCSSTokens(radon_element->data_model(),
                                       kCSSSelectorPlaceholder);
    // process ::placeholder tokens
    const auto& placeholder_value = ParsePlaceHolderTokens(tokens);
    radon_element->SetPlaceHolderStyles(placeholder_value);
  }
}

void CSSPatching::GetCSSStyleForFiber(FiberElement* node,
                                      CSSFragment* style_sheet) {
  style_sheet->InitPseudoNotStyle();
  // If has_pseudo_not_style means the pseudo_not_style is not empty
  const auto has_pseudo_not_style = style_sheet->HasPseudoNotStyle();
  auto* holder = node->data_model();
  if (style_sheet->HasCSSStyle()) {
    // process "*" first
    CSSParseToken* token = style_sheet->GetCSSStyle("*");
    if (token) {
      MergeHigherPriorityCSSStyle(token->GetAttributes());
      SetCSSVariableToNode(token->GetStyleVariables());
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
        MergeHigherPriorityCSSStyle(token->GetAttributes());
        SetCSSVariableToNode(token->GetStyleVariables());
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
        MergeHigherPriorityCSSStyle(token->GetAttributes());
        SetCSSVariableToNode(token->GetStyleVariables());
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
        MergeHigherPriorityCSSStyle(token->GetAttributes());
        SetCSSVariableToNode(token->GetStyleVariables());
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

void CSSPatching::ApplyCascadeStylesForFiber(CSSFragment* style_sheet,
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

void CSSPatching::MergeHigherCascadeStylesForFiber(
    const std::string& current_selector, const std::string& parent_selector,
    AttributeHolder* node, CSSFragment* style_sheet) {
  std::string integrated_selector =
      MergeCSSSelector(current_selector, parent_selector);
  CSSParseToken* token_parent =
      style_sheet->GetCascadeStyle(integrated_selector);
  if (token_parent != nullptr) {
    MergeHigherPriorityCSSStyle(token_parent->GetAttributes());
    SetCSSVariableToNode(token_parent->GetStyleVariables());
  }
}

const tasm::CSSParserConfigs& CSSPatching::GetCSSParserConfigs() {
  if (manager_) {
    return manager_->GetCSSParserConfigs();
  }
  static base::NoDestructor<tasm::CSSParserConfigs> kDefaultCSSConfigs;
  return *kDefaultCSSConfigs;
}

RadonElement* CSSPatching::CreatePseudoNode(int style_type) {
  RadonElement* element = nullptr;
  if (style_type & CSSSheet::SELECTION_SELECT) {
    BASE_STATIC_STRING_DECL(kTextSelection, "text-selection");
    element = new RadonElement(kTextSelection, nullptr, manager_);
  }
  if (element) {
    EXEC_EXPR_FOR_INSPECTOR({ manager_->PrepareNodeForInspector(element); });
    element->ResetPseudoType(style_type);
  }
  return element;
}

void CSSPatching::UpdateContentNode(const StyleMap& attrs,
                                    RadonElement* element) {
  if (!element->IsPseudoNode() || !element->content_data()) return;

  ContentData* data = element->content_data();
  while (data) {
    RadonElement* node = nullptr;

    if (data->isText()) {
      BASE_STATIC_STRING_DECL(kRawText, "raw-text");
      BASE_STATIC_STRING_DECL(kText, "text");
      BASE_STATIC_STRING_DECL(kPseudo, "pseudo");
      node = new RadonElement(kRawText, nullptr, manager_);
      TextContentData* text_data = static_cast<TextContentData*>(data);
      node->SetAttribute(kText, lepus::Value(text_data->text()));
      // For reason: lepus string c++ do not support unicode convert now
      // so we pass a flag to RawTextShadowNode
      node->SetAttribute(kPseudo, lepus::Value(true));
    } else if (data->isImage()) {
      BASE_STATIC_STRING_DECL(kInlineImage, "inline-image");
      node = new RadonElement(kInlineImage, nullptr, manager_);
      ImageContentData* image_data = static_cast<ImageContentData*>(data);
      BASE_STATIC_STRING_DECL(kSrc, "src");
      node->SetAttribute(kSrc, lepus::Value(image_data->url()));
    } else if (data->isAttr()) {
      AttrContentData* content_data = static_cast<AttrContentData*>(data);
      auto content = content_data->attr_content();
      if (content.IsString()) {
        BASE_STATIC_STRING_DECL(kRawText, "raw-text");
        BASE_STATIC_STRING_DECL(kText, "text");
        node = new RadonElement(kRawText, nullptr, manager_);
        node->SetAttribute(kText, content);
      }
    }

    if (node) {
      EXEC_EXPR_FOR_INSPECTOR({ manager_->PrepareNodeForInspector(node); });
      node->SetIsPseudoNode();
      node->ConsumeStyle(attrs);
      node->FlushProps();
      element->InsertNode(node);
    }

    data = data->next();
  }
}

void CSSPatching::ParsePlaceHolderTokens(PseudoPlaceHolderStyles& result,
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

PseudoPlaceHolderStyles CSSPatching::ParsePlaceHolderTokens(
    const InlineTokenVector& tokens) {
  PseudoPlaceHolderStyles result;

  for (const auto& token : tokens) {
    auto& map = token->GetAttributes();
    ParsePlaceHolderTokens(result, map);
  }
  return result;
}

CSSPatching::InlineTokenVector CSSPatching::ParsePseudoCSSTokens(
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

void CSSPatching::ParsePseudoCSSTokensForFiber(FiberElement* element,
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

void CSSPatching::UpdateSelectionPseudo(const InlineTokenVector& token_list,
                                        RadonElement* self) {
  if (token_list.empty()) {
    return;
  }

  // TODO support more selection style, and handle multi selection style merge
  // currently only the last element is meaningful
  auto token = token_list.back();
  auto sheet = token->TargetSheet();

  if (!sheet) {
    return;
  }

  auto pseudo_node = CreatePseudoNode(sheet->GetType());

  if (!pseudo_node) {
    return;
  }

  pseudo_node->SetIsPseudoNode();
  pseudo_node->ConsumeStyle(token->GetAttributes());
  pseudo_node->FlushProps();

  self->InsertNode(pseudo_node);
}

void CSSPatching::GenerateContentData(const lepus::Value& value,
                                      const AttributeHolder* vnode,
                                      RadonElement* node) {
  struct Content {
    enum ContentType {
      TEXT = 0,
      URL,
      ATTR,
    };

    ContentType type;
    std::string content;
  };

  if (!value.IsString()) return;

  std::string_view quote = "\"";
  std::string_view right_brackets = ")";
  std::string_view url_key = "url(";
  std::string_view attr_key = "attr(";

  base::InlineVector<Content, 8> pseudo_contents;
  std::string tmp = value.StdString();
  bool left_match = false;
  bool right_match = false;
  size_t left_pos = 0;
  size_t right_pos = 0;

  bool invalidate_str = false;

  while (!tmp.empty() && !invalidate_str) {
    size_t quote_pos = tmp.find(quote);
    size_t url_pos = tmp.find(url_key);
    size_t attr_pos = tmp.find(attr_key);
    size_t blank_pos = tmp.find(" ");
    if (quote_pos == 0) {
      left_pos = quote_pos;
      left_match = (left_pos != std::string::npos);
      right_pos = tmp.find(quote, left_pos + 1);
      right_match = (right_pos != std::string::npos);

      if (left_match && right_match) {
        std::string sub_str =
            tmp.substr(left_pos + 1, right_pos - left_pos - 1);
        Content curr;
        curr.type = Content::TEXT;
        curr.content = sub_str;
        pseudo_contents.push_back(curr);
        tmp.erase(left_pos, right_pos - left_pos + 1);
        left_match = right_match = false;
      } else {
        invalidate_str = true;
      }
    } else if (url_pos == 0) {
      left_pos = url_pos;
      right_pos = tmp.find(right_brackets, left_pos + 4);
      CSSStringParser parser(tmp.c_str(), static_cast<int>(tmp.size()),
                             GetCSSParserConfigs());
      std::string sub_str = parser.ParseUrl();
      if (!sub_str.empty()) {
        Content curr;
        curr.type = Content::URL;
        curr.content = sub_str;
        pseudo_contents.push_back(curr);
        tmp.erase(left_pos, right_pos - left_pos + 1);
      } else {
        invalidate_str = true;
      }
    } else if (attr_pos == 0) {
      left_pos = attr_pos;
      left_match = (left_pos != std::string::npos);
      right_pos = tmp.find(right_brackets, left_pos + 5);
      right_match = (right_pos != std::string::npos);

      if (left_match && right_match) {
        std::string sub_str =
            tmp.substr(left_pos + 5, right_pos - left_pos - 5);
        Content curr;
        curr.type = Content::ATTR;
        curr.content = sub_str;
        pseudo_contents.push_back(curr);
        tmp.erase(left_pos, right_pos - left_pos + 1);
        left_match = right_match = false;
      } else {
        invalidate_str = true;
      }
    } else if (blank_pos == 0) {
      do {
        tmp.erase(0, 1);
        blank_pos = tmp.find(" ");
      } while (blank_pos == 0);
    } else {
      std::string result;
      for (auto c : tmp) {
        if (c != '\'') {
          result.push_back(c);
        }
      }
      Content curr;
      curr.type = Content::TEXT;
      curr.content = result;
      pseudo_contents.push_back(curr);
      tmp.clear();
    }
  }

  if (invalidate_str) pseudo_contents.clear();

  ContentData* pre = nullptr;
  for (const auto& pseudo_content : pseudo_contents) {
    ContentData* cur = nullptr;
    if (pseudo_content.type == Content::TEXT)
      cur =
          ContentData::createTextContent(base::String(pseudo_content.content));
    else if (pseudo_content.type == Content::URL)
      cur = ContentData::createImageContent(pseudo_content.content);
    else
      cur = ContentData::createAttrContent(vnode, pseudo_content.content);
    if (pre)
      pre->set_next(cur);
    else
      node->SetContentData(cur);
    pre = cur;
  }
}

}  // namespace tasm
}  // namespace lynx
