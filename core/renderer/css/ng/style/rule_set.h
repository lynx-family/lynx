// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_RULE_SET_H_
#define CORE_RENDERER_CSS_NG_STYLE_RULE_SET_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/css/ng/style/rule_data.h"
#include "core/renderer/css/style_node.h"

namespace lynx {

namespace tasm {
class CSSParseToken;
class SharedCSSFragment;
}  // namespace tasm

namespace css {

class RuleInvalidationSet;

struct MatchedRule {
  MatchedRule(const RuleData* rule_data, unsigned index)
      : rule_data_(rule_data) {
    position_ = (static_cast<uint64_t>(index) << RuleData::kPositionBits) +
                rule_data_->Position();
  }

  const RuleData* Data() const { return rule_data_; }
  uint64_t Position() const { return position_; }
  unsigned Specificity() const { return rule_data_->Specificity(); }

 private:
  const RuleData* rule_data_;
  uint64_t position_;
};

using CompactRuleDataVector = base::InlineVector<RuleData, 2>;

class RuleSet {
 public:
  explicit RuleSet(tasm::SharedCSSFragment* fragment) : fragment_(fragment) {}

  void MatchStyles(StyleNode* node, unsigned& level,
                   base::Vector<MatchedRule>& output) const;

  void AddToRuleSet(const std::string& text,
                    const std::shared_ptr<lynx::tasm::CSSParseToken>& token);

  void Merge(const RuleSet& rule_set) { deps_.push_back(rule_set); }

  void AddStyleRule(const std::shared_ptr<StyleRule>& r);

  std::shared_ptr<tasm::CSSParseToken> GetRootToken();

  const auto& id_rules(const std::string& key) { return id_rules_[key]; }

  const auto& class_rules(const std::string& key) { return class_rules_[key]; }

  const auto& attr_rules(const std::string& key) { return attr_rules_[key]; }

  const auto& tag_rules(const std::string& key) { return tag_rules_[key]; }

  const auto& pseudo_rules() { return pseudo_rules_; }

  const auto& universal_rules() { return universal_rules_; }

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

  std::vector<RuleSet> deps_;
  tasm::SharedCSSFragment* fragment_ = nullptr;
  unsigned rule_count_ = 0;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_RULE_SET_H_
