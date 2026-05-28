// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/style/rule_set.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/invalidation/rule_invalidation_set.h"
#include "core/renderer/css/ng/matcher/selector_matcher.h"
#include "core/renderer/css/ng/media_query/media_query_evaluator.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/style/condition_rule.h"
#include "core/renderer/css/shared_css_fragment.h"

namespace lynx {
namespace css {

RuleSet::RuleSet(tasm::SharedCSSFragment* fragment) : fragment_(fragment) {}

RuleSet::~RuleSet() = default;

void RuleSet::Merge(const RuleSet& rule_set) { deps_.push_back(&rule_set); }

static void MatchKey(StyleNode* node, const CompactRuleDataVector& list,
                     unsigned level, base::Vector<MatchedRule>& matched) {
  for (const auto& rule : list) {
    SelectorMatcher matcher;
    SelectorMatcher::SelectorMatchingContext context(node);
    context.selector = &rule.Selector();
    auto ret = matcher.Match(context);
    if (ret) {
      // Avoid copying the RuleData, only used in the local scope
      matched.emplace_back(&rule, level);
    }
  }
}

static void MatchKey(
    StyleNode* node, const std::string& key,
    const std::unordered_map<std::string, CompactRuleDataVector>& map,
    unsigned level, base::Vector<MatchedRule>& matched) {
  if (key.empty()) return;

  auto list = map.find(key);
  if (list != map.end()) {
    MatchKey(node, list->second, level, matched);
  }
}

void RuleSet::AddToRuleSet(const std::string& text,
                           const fml::RefPtr<tasm::CSSParseToken>& token) {
  auto selector_array = std::make_unique<LynxCSSSelector[]>(1);
  selector_array[0].SetValue(text);
  selector_array[0].SetMatch(LynxCSSSelector::MatchType::kClass);
  selector_array[0].SetLastInTagHistory(true);
  selector_array[0].SetLastInSelectorList(true);
  AddStyleRule(
      fml::MakeRefCounted<StyleRule>(std::move(selector_array), token));
}

void RuleSet::MatchStyles(StyleNode* node, unsigned& level,
                          base::Vector<MatchedRule>& output,
                          const MediaQueryEvaluator* evaluator) const {
  for (const auto* dep : deps_) {
    dep->MatchStyles(node, level, output, evaluator);
  }
  ++level;
  MatchOwnStyles(node, level, output);
  for (const auto& condition_rule : condition_rules_) {
    // When no evaluator is provided, preserve the legacy "always match"
    // behavior so call sites that have not opted into media evaluation keep
    // working. When an evaluator is provided, skip condition rules whose
    // MediaQuerySet does not match the current environment; rules without a
    // structured MediaQuerySet still fall through to MatchOwnStyles.
    if (evaluator && condition_rule->HasStructuredMediaQuery() &&
        !evaluator->Eval(condition_rule->MediaQueries().get())) {
      continue;
    }
    condition_rule->GetRuleSet().MatchOwnStyles(node, level, output);
  }
}

void RuleSet::MatchOwnStyles(StyleNode* node, unsigned level,
                             base::Vector<MatchedRule>& output) const {
  MatchKey(node, universal_rules_, level, output);
  if (node->GetPseudoState() != tasm::kPseudoStateNone) {
    MatchKey(node, pseudo_rules_, level, output);
  }
  MatchKey(node, node->tag().str(), tag_rules_, level, output);
  for (const auto& c : node->classes()) {
    MatchKey(node, c.str(), class_rules_, level, output);
  }
  MatchKey(node, node->idSelector().str(), id_rules_, level, output);
}

void RuleSet::AddStyleRule(fml::RefPtr<StyleRule> rule) {
  if (rule == nullptr) return;

  uint32_t position = rule_count_;
  if (rule->Position() > 0) {
    position = rule->Position();
  }

  for (unsigned selector_index = 0; selector_index != UINT_MAX;
       selector_index = rule->IndexOfNextSelectorAfter(selector_index)) {
    RuleData rule_data(rule, selector_index, position);
    ++rule_count_;
    AddToRuleSetInternal(rule->SelectorAt(selector_index), rule_data);
    if (!fragment_) continue;
    RuleInvalidationSet* set = fragment_->GetRuleInvalidationSet();
    if (!set) continue;
    set->AddSelector(rule->SelectorAt(selector_index));
  }
}

void RuleSet::AddConditionRule(fml::RefPtr<ConditionRule> rule) {
  if (rule == nullptr) return;
  if (rule->HasStructuredMediaQuery()) {
    has_media_query_rules_ = true;
  }
  condition_rules_.emplace_back(std::move(rule));
}

void RuleSet::AddToRuleSet(
    const std::string& key,
    std::unordered_map<std::string, CompactRuleDataVector>& map,
    const RuleData& rule) {
  map[key].push_back(rule);
}

static void ExtractSelector(const LynxCSSSelector* selector, std::string& id,
                            std::string& class_name, std::string& attr_name,
                            std::string& attr_value, std::string& tag_name,
                            LynxCSSSelector::PseudoType& pseudo_type) {
  switch (selector->Match()) {
    case LynxCSSSelector::kId:
      id = selector->Value();
      break;
    case LynxCSSSelector::kClass:
      class_name = selector->Value();
      break;
    case LynxCSSSelector::kTag:
      if (selector->Value() != CSSGlobalStarString())
        tag_name = selector->Value();
      break;
    case LynxCSSSelector::kPseudoClass:
    case LynxCSSSelector::kPseudoElement:
      switch (selector->GetPseudoType()) {
        case LynxCSSSelector::kPseudoActive:
        case LynxCSSSelector::kPseudoFocus:
        case LynxCSSSelector::kPseudoHover:
        case LynxCSSSelector::kPseudoPlaceholder:
        case LynxCSSSelector::kPseudoSelection:
          pseudo_type = selector->GetPseudoType();
          break;
        default:
          break;
      }
      break;
    case LynxCSSSelector::kAttributeExact:
    case LynxCSSSelector::kAttributeSet:
    case LynxCSSSelector::kAttributeHyphen:
    case LynxCSSSelector::kAttributeList:
    case LynxCSSSelector::kAttributeContain:
    case LynxCSSSelector::kAttributeBegin:
    case LynxCSSSelector::kAttributeEnd:
      attr_name = selector->Attribute();
      attr_value = selector->Value();
      break;
    default:
      break;
  }
}

static const LynxCSSSelector* ExtractBestSelector(
    const LynxCSSSelector& selector, std::string& id, std::string& class_name,
    std::string& attr_name, std::string& attr_value, std::string& tag_name,
    LynxCSSSelector::PseudoType& pseudo_type) {
  const LynxCSSSelector* it = &selector;
  for (; it && it->Relation() == LynxCSSSelector::kSubSelector;
       it = it->TagHistory()) {
    ExtractSelector(it, id, class_name, attr_name, attr_value, tag_name,
                    pseudo_type);
  }
  if (it) {
    ExtractSelector(it, id, class_name, attr_name, attr_value, tag_name,
                    pseudo_type);
  }
  return it;
}

bool RuleSet::AddToRuleSetInternal(const LynxCSSSelector& selector,
                                   const RuleData& rule_data) {
  std::string id;
  std::string class_name;
  std::string attr_name;
  std::string attr_value;
  std::string tag_name;
  LynxCSSSelector::PseudoType pseudo_type = LynxCSSSelector::kPseudoUnknown;

  const LynxCSSSelector* best = ExtractBestSelector(
      selector, id, class_name, attr_name, attr_value, tag_name, pseudo_type);
  if (best && best->Relation() == LynxCSSSelector::kDirectAdjacent) {
    has_adjacent_sibling_rules_ = true;
  }

  // Prefer rule sets in order of most likely to apply infrequently.
  if (!id.empty()) {
    AddToRuleSet(id, id_rules_, rule_data);
    return true;
  }

  if (!class_name.empty()) {
    AddToRuleSet(class_name, class_rules_, rule_data);
    return true;
  }

  if (!attr_name.empty()) {
    AddToRuleSet(attr_name, attr_rules_, rule_data);
    return true;
  }

  if (pseudo_type != LynxCSSSelector::kPseudoUnknown) {
    pseudo_rules_.push_back(rule_data);
    return true;
  }

  if (!tag_name.empty()) {
    AddToRuleSet(tag_name, tag_rules_, rule_data);
    return true;
  }

  universal_rules_.push_back(rule_data);
  return false;
}

fml::RefPtr<tasm::CSSParseToken> RuleSet::GetRootToken() {
  auto iter = std::find_if(universal_rules_.begin(), universal_rules_.end(),
                           [](const RuleData& rd) {
                             return rd.Selector().GetPseudoType() ==
                                    LynxCSSSelector::PseudoType::kPseudoRoot;
                           });
  if (iter != universal_rules_.end()) {
    return iter->Rule()->Token();
  }
  return nullptr;
}

}  // namespace css
}  // namespace lynx
