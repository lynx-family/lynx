// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_RULE_DATA_H_
#define CORE_RENDERER_CSS_NG_STYLE_RULE_DATA_H_

#include <memory>

#include "core/renderer/css/ng/style/style_rule.h"

namespace lynx {
namespace css {

struct RuleData {
  static constexpr size_t kSelectorIndexBits = 13;

  static constexpr size_t kPositionBits = 19;

  RuleData(const std::shared_ptr<StyleRule>& rule, unsigned selector_index,
           unsigned position)
      : rule_(rule),
        selector_index_(selector_index),
        position_(position),
        specificity_(Selector().Specificity()) {}

  const LynxCSSSelector& Selector() const {
    return rule_->SelectorAt(selector_index_);
  }

  StyleRule* Rule() const { return rule_.get(); }

  unsigned Position() const { return position_; }

  unsigned SelectorIndex() const { return selector_index_; }

  unsigned Specificity() const { return specificity_; }

 private:
  std::shared_ptr<StyleRule> rule_;
  unsigned selector_index_ : kSelectorIndexBits;
  unsigned position_ : kPositionBits;
  unsigned specificity_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_RULE_DATA_H_
