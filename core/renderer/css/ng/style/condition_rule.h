// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_CONDITION_RULE_H_
#define CORE_RENDERER_CSS_NG_STYLE_CONDITION_RULE_H_

#include <atomic>
#include <cstdint>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/style/rule_set.h"
#include "core/renderer/css/ng/supports/supports_condition.h"

namespace lynx {
namespace css {

class SupportsEvaluator;

class ConditionRule : public fml::RefCountedThreadSafeStorage {
 public:
  explicit ConditionRule(tasm::SharedCSSFragment* fragment)
      : rule_set_(fragment) {}

  void ReleaseSelf() const override { delete this; }

  RuleSet& GetRuleSet() { return rule_set_; }
  const RuleSet& GetRuleSet() const { return rule_set_; }

  void AddStyleRule(fml::RefPtr<StyleRule> rule) {
    rule_set_.AddStyleRule(std::move(rule));
  }

  void SetMediaQueries(fml::RefPtr<const MediaQuerySet> queries) {
    media_queries_ = std::move(queries);
  }

  const fml::RefPtr<const MediaQuerySet>& MediaQueries() const {
    return media_queries_;
  }

  bool HasStructuredMediaQuery() const {
    return media_queries_ && !media_queries_->IsEmpty();
  }

  void SetSupportsCondition(
      fml::RefPtr<const SupportsConditionNode> condition) {
    supports_condition_ = std::move(condition);
    supports_condition_result_.store(kSupportsConditionUnknown,
                                     std::memory_order_relaxed);
  }

  const fml::RefPtr<const SupportsConditionNode>& SupportsCondition() const {
    return supports_condition_;
  }

  bool HasStructuredSupportsRules() const {
    return supports_condition_ != nullptr;
  }

  bool MatchesSupportsCondition(
      const SupportsEvaluator* supports_evaluator) const;

 private:
  static constexpr int8_t kSupportsConditionUnknown = -1;
  static constexpr int8_t kSupportsConditionFalse = 0;
  static constexpr int8_t kSupportsConditionTrue = 1;

  RuleSet rule_set_;
  fml::RefPtr<const MediaQuerySet> media_queries_;
  fml::RefPtr<const SupportsConditionNode> supports_condition_;
  mutable std::atomic<int8_t> supports_condition_result_{
      kSupportsConditionUnknown};
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_CONDITION_RULE_H_
