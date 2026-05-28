// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_CONDITION_RULE_H_
#define CORE_RENDERER_CSS_NG_STYLE_CONDITION_RULE_H_

#include <string>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/style/rule_set.h"

namespace lynx {
namespace css {

class ConditionRule : public fml::RefCountedThreadSafeStorage {
 public:
  explicit ConditionRule(std::string condition,
                         tasm::SharedCSSFragment* fragment)
      : condition_(std::move(condition)), rule_set_(fragment) {}

  void ReleaseSelf() const override { delete this; }

  const std::string& Condition() const { return condition_; }

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

 private:
  std::string condition_;
  RuleSet rule_set_;
  fml::RefPtr<const MediaQuerySet> media_queries_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_CONDITION_RULE_H_
