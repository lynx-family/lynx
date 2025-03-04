// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/matcher/selector_matcher.h"

#include <string>

#include "base/include/auto_reset.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {
namespace css {

static StyleNode* Parent(
    const SelectorMatcher::SelectorMatchingContext& context) {
  return context.holder->SelectorMatchingParent();
}

bool SelectorMatcher::Match(const SelectorMatchingContext& context) const {
  base::AutoReset<bool> reset_in_match(&in_match_, true);

  return MatchSelector(context) == kMatches;
}

SelectorMatcher::MatchResult SelectorMatcher::MatchSelector(
    const SelectorMatchingContext& context) const {
  if (!MatchSimple(context)) {
    return kFailsLocally;
  }
  if (context.selector->IsLastInTagHistory()) {
    return kMatches;
  }

  MatchResult match;
  if (context.selector->Relation() != LynxCSSSelector::kSubSelector) {
    match = MatchForRelation(context);
  } else {
    match = MatchForSubSelector(context);
  }
  return match;
}

static inline SelectorMatcher::SelectorMatchingContext NextContext(
    const SelectorMatcher::SelectorMatchingContext& context) {
  SelectorMatcher::SelectorMatchingContext next_context(context);
  DCHECK(context.selector->TagHistory());
  next_context.selector = context.selector->TagHistory();
  return next_context;
}

SelectorMatcher::MatchResult SelectorMatcher::MatchForSubSelector(
    const SelectorMatchingContext& context) const {
  SelectorMatchingContext next_context = NextContext(context);
  return MatchSelector(next_context);
}

SelectorMatcher::MatchResult SelectorMatcher::MatchForRelation(
    const SelectorMatchingContext& context) const {
  SelectorMatchingContext next_context = NextContext(context);
  LynxCSSSelector::RelationType relation = context.selector->Relation();

  switch (relation) {
    case LynxCSSSelector::kDescendant: {
      for (next_context.holder = Parent(next_context); next_context.holder;
           next_context.holder = Parent(next_context)) {
        MatchResult match = MatchSelector(next_context);
        if (match == kMatches || match == kFailsCompletely) {
          return match;
        }
      }
      return kFailsCompletely;
    }
    case LynxCSSSelector::kChild: {
      next_context.holder = Parent(next_context);
      if (!next_context.holder) {
        return kFailsCompletely;
      }
      return MatchSelector(next_context);
    }
    case LynxCSSSelector::kDirectAdjacent: {
      next_context.holder = context.holder->PreviousSibling();
      if (!next_context.holder) {
        return kFailsAllSiblings;
      }
      return MatchSelector(next_context);
    }
    case LynxCSSSelector::kIndirectAdjacent: {
      next_context.holder = context.holder->PreviousSibling();
      for (; next_context.holder;
           next_context.holder = next_context.holder->PreviousSibling()) {
        MatchResult match = MatchSelector(next_context);
        if (match == kMatches || match == kFailsAllSiblings ||
            match == kFailsCompletely) {
          return match;
        }
      }
      return kFailsAllSiblings;
    }
    case LynxCSSSelector::kUAShadow: {
      next_context.holder = context.holder->PseudoElementOwner();
      return MatchSelector(next_context);
    }
    case LynxCSSSelector::kSubSelector:
    default:
      break;
  }
  return kFailsCompletely;
}

bool SelectorMatcher::MatchSimple(
    const SelectorMatchingContext& context) const {
  DCHECK(context.holder);
  auto& element = *context.holder;
  DCHECK(context.selector);
  const LynxCSSSelector& selector = *context.selector;

  switch (selector.Match()) {
    case LynxCSSSelector::kTag:
      return selector.Value() == CSSGlobalStarString() ||
             element.ContainsTagSelector(selector.Value());
    case LynxCSSSelector::kClass:
      return element.ContainsClassSelector(selector.Value());
    case LynxCSSSelector::kId:
      return element.ContainsIdSelector(selector.Value());
    case LynxCSSSelector::kPseudoClass:
      return MatchPseudoClass(context);
    case LynxCSSSelector::kPseudoElement:
      return MatchPseudoElement(context);
    default:
      return false;
  }
}

bool SelectorMatcher::MatchPseudoNot(
    const SelectorMatchingContext& context) const {
  const LynxCSSSelector& selector = *context.selector;
  DCHECK(selector.SelectorList());
  SelectorMatchingContext sub_context(context);
  for (sub_context.selector = selector.SelectorList()->First();
       sub_context.selector; sub_context.selector = LynxCSSSelectorList::Next(
                                 *sub_context.selector)) {
    if (MatchSelector(sub_context) == kMatches) {
      return false;
    }
  }
  return true;
}

bool SelectorMatcher::MatchPseudoClass(
    const SelectorMatchingContext& context) const {
  auto& element = *context.holder;
  const LynxCSSSelector& selector = *context.selector;

  switch (selector.GetPseudoType()) {
    case LynxCSSSelector::kPseudoNot:
      return MatchPseudoNot(context);
    case LynxCSSSelector::kPseudoHover:
      return element.HasPseudoState(tasm::kPseudoStateHover);
    case LynxCSSSelector::kPseudoActive:
      return element.HasPseudoState(tasm::kPseudoStateActive);
    case LynxCSSSelector::kPseudoFocus:
      return element.HasPseudoState(tasm::kPseudoStateFocus);
    case LynxCSSSelector::kPseudoRoot:
      return element.tag().str() == "page";
    case LynxCSSSelector::kPseudoUnknown:
    default:
      break;
  }
  return false;
}

bool SelectorMatcher::MatchPseudoElement(
    const SelectorMatchingContext& context) const {
  auto& holder = *context.holder;
  const LynxCSSSelector& selector = *context.selector;

  switch (selector.GetPseudoType()) {
    case LynxCSSSelector::PseudoType::kPseudoPlaceholder:
      return holder.HasPseudoState(tasm::kPseudoStatePlaceHolder);
    case LynxCSSSelector::PseudoType::kPseudoSelection:
      return holder.HasPseudoState(tasm::kPseudoStateSelection);
    default:
      break;
  }
  return false;
}
}  // namespace css
}  // namespace lynx
