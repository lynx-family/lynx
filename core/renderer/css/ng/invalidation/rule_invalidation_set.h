// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_CSS_NG_INVALIDATION_RULE_INVALIDATION_SET_H_
#define CORE_RENDERER_CSS_NG_INVALIDATION_RULE_INVALIDATION_SET_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/css/ng/invalidation/invalidation_set.h"
#include "core/renderer/css/ng/invalidation/invalidation_set_feature.h"
#include "core/renderer/css/ng/selector/lynx_css_selector.h"

namespace lynx {
namespace css {

struct InvalidationLists;

class RuleInvalidationSet {
 public:
  RuleInvalidationSet() = default;
  RuleInvalidationSet(const RuleInvalidationSet&) = delete;
  RuleInvalidationSet& operator=(const RuleInvalidationSet&) = delete;

  void Merge(const RuleInvalidationSet& other);

  void Clear();

  void AddSelector(const LynxCSSSelector&);

  void CollectClass(InvalidationLists&, const std::string& class_name) const;
  void CollectId(InvalidationLists&, const std::string& id) const;
  void CollectPseudoClass(InvalidationLists&,
                          const LynxCSSSelector::PseudoType&) const;

 private:
  enum PositionType { kSubject, kAncestor };
  InvalidationSet* GetInvalidationSetForSimpleSelector(const LynxCSSSelector&,
                                                       PositionType);

  using InvalidationSetMap =
      std::unordered_map<std::string, InvalidationSetPtr>;
  using PseudoTypeInvalidationSetMap =
      std::unordered_map<LynxCSSSelector::PseudoType, InvalidationSetPtr>;

  void UpdateInvalidationSets(const LynxCSSSelector&, InvalidationSetFeature&,
                              PositionType);

  static void ExtractSimpleSelector(const LynxCSSSelector&,
                                    InvalidationSetFeature&);
  const LynxCSSSelector* ExtractCompound(const LynxCSSSelector&,
                                         InvalidationSetFeature&, PositionType);
  void ExtractSelectorList(const LynxCSSSelector&, PositionType);
  void AddFeatureToInvalidationSet(InvalidationSet&,
                                   const InvalidationSetFeature&);
  void AddSelectorToInvalidationSets(
      const LynxCSSSelector&, InvalidationSetFeature& descendant_feature);
  const LynxCSSSelector* AddCompoundSelectorToInvalidationSets(
      const LynxCSSSelector&, InvalidationSetFeature& descendant_feature);
  void AddSimpleSelectorToInvalidationSets(
      const LynxCSSSelector& simple_selector,
      InvalidationSetFeature& descendant_feature);

  static InvalidationSet& GetInvalidationSet(
      PositionType position, InvalidationSetPtr& invalidation_set);

  static InvalidationSet& GetInvalidationSet(InvalidationSetMap&,
                                             const std::string& key,
                                             PositionType);
  static InvalidationSet& GetInvalidationSet(PseudoTypeInvalidationSetMap&,
                                             LynxCSSSelector::PseudoType key,
                                             PositionType);

  static void CombineInvalidationSet(InvalidationSetMap&,
                                     const std::string& key, InvalidationSet*);
  static void CombineInvalidationSet(PseudoTypeInvalidationSetMap&,
                                     LynxCSSSelector::PseudoType key,
                                     InvalidationSet*);

  InvalidationSetMap class_invalidation_sets_;
  InvalidationSetMap id_invalidation_sets_;
  PseudoTypeInvalidationSetMap pseudo_invalidation_sets_;

  friend class RuleInvalidationSetTest;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_INVALIDATION_RULE_INVALIDATION_SET_H_
