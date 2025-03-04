// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_MATCHER_SELECTOR_MATCHER_H_
#define CORE_RENDERER_CSS_NG_MATCHER_SELECTOR_MATCHER_H_

#include <limits>

#include "core/renderer/css/ng/selector/lynx_css_selector.h"
#include "core/renderer/css/style_node.h"

namespace lynx {
namespace css {

class SelectorMatcher {
 public:
  SelectorMatcher() = default;

  SelectorMatcher(const SelectorMatcher&) = delete;
  SelectorMatcher& operator=(const SelectorMatcher&) = delete;

  struct SelectorMatchingContext {
   public:
    explicit SelectorMatchingContext(StyleNode* holder) : holder(holder) {}
    const LynxCSSSelector* selector = nullptr;
    StyleNode* holder = nullptr;
  };

  bool Match(const SelectorMatchingContext& context) const;

 private:
  bool MatchSimple(const SelectorMatchingContext&) const;

  enum MatchResult {
    kMatches,
    kFailsLocally,
    kFailsAllSiblings,
    kFailsCompletely
  };

  MatchResult MatchSelector(const SelectorMatchingContext&) const;
  MatchResult MatchForSubSelector(const SelectorMatchingContext&) const;
  MatchResult MatchForRelation(const SelectorMatchingContext&) const;
  bool MatchPseudoClass(const SelectorMatchingContext&) const;
  bool MatchPseudoElement(const SelectorMatchingContext&) const;
  bool MatchPseudoNot(const SelectorMatchingContext&) const;

  mutable bool in_match_ = false;
};
}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MATCHER_SELECTOR_MATCHER_H_
