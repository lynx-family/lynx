// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_RULE_SET_H_
#define CORE_RENDERER_CSS_NG_STYLE_RULE_SET_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "core/base/lynx_export.h"
#include "core/renderer/css/ng/style/rule_data.h"
#include "core/renderer/css/style_node.h"

namespace lynx {

namespace tasm {
class CSSParseToken;
class SharedCSSFragment;
}  // namespace tasm

namespace css {

class ConditionRule;
class MediaQueryEvaluator;
class RuleInvalidationSet;
class SupportsEvaluator;

struct MatchedRule {
  MatchedRule(const RuleData* rule_data, unsigned index)
      : rule_data_(rule_data) {
    position_ = (static_cast<uint64_t>(index) << RuleData::kPositionBits) +
                rule_data_->Position();
  }

  const RuleData* Data() const { return rule_data_; }
  uint64_t Position() const { return position_; }
  unsigned Specificity() const { return rule_data_->Specificity(); }
  uint16_t LayerOrder() const { return layer_order_; }
  void SetLayerOrder(uint16_t order) { layer_order_ = order; }

 private:
  const RuleData* rule_data_;
  uint64_t position_;
  uint16_t layer_order_ = CascadeLayer::kImplicitOuterLayerOrder;
};

using CompactRuleDataVector = base::InlineVector<RuleData, 2>;

class RuleSet {
 public:
  explicit RuleSet(tasm::SharedCSSFragment* fragment);
  ~RuleSet();

  void MatchStyles(StyleNode* node, unsigned& level,
                   base::Vector<MatchedRule>& output,
                   const MediaQueryEvaluator* media_query_evaluator,
                   const SupportsEvaluator* supports_evaluator) const;

  LYNX_EXPORT_FOR_DEVTOOL void MatchOwnStyles(
      StyleNode* node, unsigned level, base::Vector<MatchedRule>& output) const;

  void AddToRuleSet(const std::string& text,
                    const fml::RefPtr<tasm::CSSParseToken>& token);

  void Merge(const RuleSet& rule_set);

  void AddStyleRule(fml::RefPtr<StyleRule> rule, CascadeLayer* layer = nullptr);

  void AddConditionRule(fml::RefPtr<ConditionRule> rule);

  fml::RefPtr<tasm::CSSParseToken> GetRootToken();

  const auto& id_rules(const std::string& key) { return id_rules_[key]; }

  const auto& class_rules(const std::string& key) { return class_rules_[key]; }

  const auto& attr_rules(const std::string& key) { return attr_rules_[key]; }

  const auto& tag_rules(const std::string& key) { return tag_rules_[key]; }

  const auto& pseudo_rules() { return pseudo_rules_; }

  const auto& universal_rules() { return universal_rules_; }

  bool HasAdjacentSiblingRules() const {
    if (has_adjacent_sibling_rules_) return true;
    for (const auto* dep : deps_) {
      if (dep->HasAdjacentSiblingRules()) return true;
    }
    return false;
  }

  bool HasCascadeLayers() const {
    return GetFeatureFlags() & kHasCascadeLayers;
  }

  // True when this rule set (or any merged dep) contains at least one
  // ConditionRule carrying a parsed @media query set. Callers can use this
  // as an O(1) gate to decide whether constructing a MediaQueryEvaluator is
  // worthwhile before invoking MatchStyles.
  bool HasMediaQueryRules() const { return GetFeatureFlags() & kHasMediaQuery; }

  bool HasSupportsRules() const { return GetFeatureFlags() & kHasSupports; }

  enum FeatureFlags : uint8_t {
    kNoFeatures = 0,
    kHasCascadeLayers = 1 << 0,
    kHasMediaQuery = 1 << 1,
    kHasSupports = 1 << 2,
  };

  void AddFeatureFlag(FeatureFlags feature_flag) {
    feature_flags_ |= feature_flag;
  }

  uint8_t GetFeatureFlags() const { return feature_flags_; }

  template <typename ForEachConditionalRuleVisitor>
  void ForEachConditionRule(ForEachConditionalRuleVisitor&& visitor) const {
    for (const auto* dep : deps_) {
      dep->ForEachConditionRule(visitor);
    }
    for (const auto& rule : condition_rules_) {
      if (rule) {
        visitor(*rule);
      }
    }
  }

  const std::vector<const RuleSet*>& deps() const { return deps_; }
  tasm::SharedCSSFragment* fragment() const { return fragment_; }

 private:
  bool AddToRuleSetInternal(const LynxCSSSelector& component,
                            const RuleData& rule);

  static void AddToRuleSet(
      const std::string& key,
      std::unordered_map<std::string, CompactRuleDataVector>& map,
      const RuleData& rule);

  std::unordered_map<std::string, CompactRuleDataVector> id_rules_;
  std::unordered_map<std::string, CompactRuleDataVector> class_rules_;
  std::unordered_map<std::string, CompactRuleDataVector> attr_rules_;
  std::unordered_map<std::string, CompactRuleDataVector> tag_rules_;
  CompactRuleDataVector pseudo_rules_;
  CompactRuleDataVector universal_rules_;

  std::vector<const RuleSet*> deps_;
  base::Vector<fml::RefPtr<ConditionRule>> condition_rules_;
  tasm::SharedCSSFragment* fragment_ = nullptr;
  unsigned rule_count_ = 0;
  bool has_adjacent_sibling_rules_ = false;
  uint8_t feature_flags_ = kNoFeatures;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_RULE_SET_H_
