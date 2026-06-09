// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/style/condition_rule.h"

#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/ng/supports/supports_evaluator.h"

namespace lynx {
namespace css {

bool ConditionRule::MatchesSupportsCondition(
    const SupportsEvaluator* supports_evaluator) const {
  if (!HasStructuredSupportsRules()) {
    return true;
  }
  if (!supports_evaluator) {
    return false;
  }

  const int8_t cached =
      supports_condition_result_.load(std::memory_order_relaxed);
  if (cached != kSupportsConditionUnknown) {
    return cached == kSupportsConditionTrue;
  }

  const bool result = supports_evaluator->Eval(supports_condition_.get());
  supports_condition_result_.store(
      result ? kSupportsConditionTrue : kSupportsConditionFalse,
      std::memory_order_relaxed);
  return result;
}

}  // namespace css
}  // namespace lynx
