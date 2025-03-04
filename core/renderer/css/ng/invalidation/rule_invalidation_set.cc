// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/invalidation/rule_invalidation_set.h"

#include <algorithm>
#include <utility>

#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/selector/lynx_css_selector.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {
namespace css {

static inline bool SupportedRelation(LynxCSSSelector::RelationType relation) {
  return relation == LynxCSSSelector::kSubSelector ||
         relation == LynxCSSSelector::kDescendant ||
         relation == LynxCSSSelector::kChild ||
         relation == LynxCSSSelector::kUAShadow;
}

InvalidationSet& RuleInvalidationSet::GetInvalidationSet(
    PositionType position, InvalidationSetPtr& invalidation_set) {
  if (!invalidation_set) {
    invalidation_set =
        position == kSubject
            ? InvalidationSetPtr(InvalidationSet::SelfInvalidationSet())
            : DescendantInvalidationSet::Create();
    return *invalidation_set;
  }

  if (invalidation_set->IsSelfInvalidationSet() && position == kSubject) {
    return *invalidation_set;
  }

  // For example, '. a' creates a SelfInvalidationSet, then '.a .b' needs to
  // change the invalidation_set
  if (invalidation_set->IsSelfInvalidationSet()) {
    invalidation_set = DescendantInvalidationSet::Create();
    invalidation_set->SetInvalidatesSelf();
  }

  return *invalidation_set;
}

InvalidationSet& RuleInvalidationSet::GetInvalidationSet(
    InvalidationSetMap& map, const std::string& key, PositionType position) {
  InvalidationSetPtr& invalidation_set = map[key];
  return GetInvalidationSet(position, invalidation_set);
}

InvalidationSet& RuleInvalidationSet::GetInvalidationSet(
    PseudoTypeInvalidationSetMap& map, LynxCSSSelector::PseudoType key,
    PositionType position) {
  InvalidationSetPtr& invalidation_set = map[key];
  return GetInvalidationSet(position, invalidation_set);
}

// For example, '.a.b' will return '.a', '.a#b' will return '#b', 'div.a' will
// return '.a'
void RuleInvalidationSet::ExtractSimpleSelector(
    const LynxCSSSelector& selector, InvalidationSetFeature& feature) {
  if (selector.Match() == LynxCSSSelector::kTag &&
      selector.Value() != CSSGlobalStarString()) {
    feature.SetTag(selector.Value());
    return;
  }
  if (selector.Match() == LynxCSSSelector::kId) {
    feature.SetId(selector.Value());
    return;
  }
  if (selector.Match() == LynxCSSSelector::kClass) {
    feature.SetClass(selector.Value());
  }
}

InvalidationSet* RuleInvalidationSet::GetInvalidationSetForSimpleSelector(
    const LynxCSSSelector& selector, PositionType position) {
  if (selector.Match() == LynxCSSSelector::kClass) {
    return &GetInvalidationSet(class_invalidation_sets_, selector.Value(),
                               position);
  }
  if (selector.Match() == LynxCSSSelector::kId) {
    return &GetInvalidationSet(id_invalidation_sets_, selector.Value(),
                               position);
  }
  if (selector.Match() == LynxCSSSelector::kPseudoClass) {
    switch (selector.GetPseudoType()) {
      case LynxCSSSelector::kPseudoHover:
      case LynxCSSSelector::kPseudoFocus:
      case LynxCSSSelector::kPseudoActive:
        return &GetInvalidationSet(pseudo_invalidation_sets_,
                                   selector.GetPseudoType(), position);
      default:
        break;
    }
  }
  return nullptr;
}

void RuleInvalidationSet::UpdateInvalidationSets(
    const LynxCSSSelector& complex, InvalidationSetFeature& feature,
    PositionType position) {
  // For example, '.a' will return a 'feature' object with classes containing
  // 'a', and the last_in_compound is '.a' too.
  // Another example, '.a#b' will return a 'feature' object with ids containing
  // 'b' because id is more important than class, and the last_in_compound is
  // '.b' too.
  const LynxCSSSelector* last_in_compound =
      ExtractCompound(complex, feature, position);

  bool was_full_invalid = feature.FullInvalid();

  if (!feature.HasFeature()) {
    feature.SetFullInvalid(true);
  }

  const LynxCSSSelector* next_compound = last_in_compound->TagHistory();
  // For example, '.a .b', only support descendants
  if (next_compound && SupportedRelation(last_in_compound->Relation())) {
    // Add next_compound in *_invalidation_sets_
    AddSelectorToInvalidationSets(*next_compound, feature);
  }

  if (!next_compound) {
    return;
  }

  feature.SetFullInvalid(was_full_invalid);
}

void RuleInvalidationSet::ExtractSelectorList(
    const LynxCSSSelector& simple_selector, PositionType position) {
  const LynxCSSSelector* selector_list = simple_selector.SelectorListSelector();
  if (!selector_list) {
    return;
  }

  for (; selector_list;
       selector_list = LynxCSSSelectorList::Next(*selector_list)) {
    InvalidationSetFeature complex_feature;
    UpdateInvalidationSets(*selector_list, complex_feature, position);
  }
}

const LynxCSSSelector* RuleInvalidationSet::ExtractCompound(
    const LynxCSSSelector& compound, InvalidationSetFeature& feature,
    PositionType position) {
  // NOTE: This loop stops once we are at the end of the compound, i.e., we see
  // a relation that is not a sub-selector. So for e.g. .a .b.c#d, we will see
  // .b, .c, #d and then stop, returning a pointer to #d.
  const LynxCSSSelector* simple_selector = &compound;
  for (;; simple_selector = simple_selector->TagHistory()) {
    ExtractSimpleSelector(*simple_selector, feature);
    // Create and add invalidation-set to *_invalidation_sets_
    if (InvalidationSet* invalidation_set =
            GetInvalidationSetForSimpleSelector(*simple_selector, position)) {
      if (position == kSubject) {
        invalidation_set->SetInvalidatesSelf();
      }
    }

    // For the :not pseudo class
    ExtractSelectorList(*simple_selector, position);

    // Next should be another compound selector or null
    if (!simple_selector->TagHistory() ||
        simple_selector->Relation() != LynxCSSSelector::kSubSelector) {
      return simple_selector;
    }
  }
}

void RuleInvalidationSet::AddFeatureToInvalidationSet(
    InvalidationSet& invalidation_set, const InvalidationSetFeature& feature) {
  if (feature.FullInvalid()) {
    invalidation_set.SetWholeSubtreeInvalid();
    return;
  }

  for (const auto& id : feature.ids) {
    invalidation_set.AddId(id);
  }
  for (const auto& tag_name : feature.tag_names) {
    invalidation_set.AddTagName(tag_name);
  }
  for (const auto& class_name : feature.classes) {
    invalidation_set.AddClass(class_name);
  }
}

void RuleInvalidationSet::AddSimpleSelectorToInvalidationSets(
    const LynxCSSSelector& simple_selector,
    InvalidationSetFeature& descendant_feature) {
  // Add invalidation-set to *_invalidation_sets_ with type kAncestor
  if (InvalidationSet* invalidation_set =
          GetInvalidationSetForSimpleSelector(simple_selector, kAncestor)) {
    // For example, if we have a selector, that is '.m .p', for class m we
    // only have a descendant containing class p
    AddFeatureToInvalidationSet(*invalidation_set, descendant_feature);
  }
}

const LynxCSSSelector*
RuleInvalidationSet::AddCompoundSelectorToInvalidationSets(
    const LynxCSSSelector& compound,
    InvalidationSetFeature& descendant_feature) {
  // For example, for selector '.m .n.x .p' we will add feature to
  // InvalidationSets, the result is '.m .p', '.n .p', '.x .p',
  // '.p'(SelfInvalidationSet).
  const LynxCSSSelector* simple_selector = &compound;
  for (; simple_selector; simple_selector = simple_selector->TagHistory()) {
    AddSimpleSelectorToInvalidationSets(*simple_selector, descendant_feature);
    if (simple_selector->Relation() != LynxCSSSelector::kSubSelector) {
      break;
    }
    if (!simple_selector->TagHistory()) {
      break;
    }
  }

  return simple_selector;
}

void RuleInvalidationSet::AddSelectorToInvalidationSets(
    const LynxCSSSelector& selector,
    InvalidationSetFeature& descendant_feature) {
  // The 'selector' is the selector immediately to the left of the rightmost
  // combinator. descendant_feature has the feature of the rightmost compound
  // selector.
  const LynxCSSSelector* compound = &selector;
  while (compound) {
    // NOTE: We only support descendants
    if (!SupportedRelation(compound->Relation())) {
      return;
    }

    // For example, for selector '.m .n.x .p' the loop is '.n' and '.m'
    const LynxCSSSelector* last_in_compound =
        AddCompoundSelectorToInvalidationSets(*compound, descendant_feature);
    DCHECK(last_in_compound);
    compound = last_in_compound->TagHistory();
  }
}

void RuleInvalidationSet::AddSelector(const LynxCSSSelector& selector) {
  InvalidationSetFeature feature;
  UpdateInvalidationSets(selector, feature, kSubject);
}

void RuleInvalidationSet::CombineInvalidationSet(
    InvalidationSetMap& map, const std::string& key,
    InvalidationSet* invalidation_set) {
  DCHECK(invalidation_set);
  InvalidationSetPtr& value = map[key];
  GetInvalidationSet(
      invalidation_set->IsSelfInvalidationSet() ? kSubject : kAncestor, value)
      .Combine(*invalidation_set);
}

void RuleInvalidationSet::CombineInvalidationSet(
    PseudoTypeInvalidationSetMap& map, LynxCSSSelector::PseudoType key,
    InvalidationSet* invalidation_set) {
  DCHECK(invalidation_set);
  InvalidationSetPtr& value = map[key];
  GetInvalidationSet(
      invalidation_set->IsSelfInvalidationSet() ? kSubject : kAncestor, value)
      .Combine(*invalidation_set);
}

void RuleInvalidationSet::Merge(const RuleInvalidationSet& other) {
  for (const auto& entry : other.class_invalidation_sets_)
    CombineInvalidationSet(class_invalidation_sets_, entry.first,
                           entry.second.get());
  for (const auto& entry : other.id_invalidation_sets_)
    CombineInvalidationSet(id_invalidation_sets_, entry.first,
                           entry.second.get());
  for (const auto& entry : other.pseudo_invalidation_sets_) {
    auto key = static_cast<LynxCSSSelector::PseudoType>(entry.first);
    CombineInvalidationSet(pseudo_invalidation_sets_, key, entry.second.get());
  }
}

void RuleInvalidationSet::Clear() {
  class_invalidation_sets_.clear();
  id_invalidation_sets_.clear();
  pseudo_invalidation_sets_.clear();
}

#define COLLECT_INVALIDATION_SETS(field, name, key_type)                  \
  void RuleInvalidationSet::Collect##name(                                \
      InvalidationLists& invalidation_lists, const key_type& key) const { \
    auto it = field.find(key);                                            \
    if (it == field.end()) {                                              \
      return;                                                             \
    }                                                                     \
    if (!it->second->IsAlive()) {                                         \
      return;                                                             \
    }                                                                     \
    DescendantInvalidationSet* descendants =                              \
        static_cast<DescendantInvalidationSet*>(it->second.get());        \
    if (descendants) {                                                    \
      invalidation_lists.descendants.push_back(descendants);              \
    }                                                                     \
  }

COLLECT_INVALIDATION_SETS(id_invalidation_sets_, Id, std::string)
COLLECT_INVALIDATION_SETS(class_invalidation_sets_, Class, std::string)
COLLECT_INVALIDATION_SETS(pseudo_invalidation_sets_, PseudoClass,
                          LynxCSSSelector::PseudoType)
#undef COLLECT_INVALIDATION_SETS

}  // namespace css
}  // namespace lynx
