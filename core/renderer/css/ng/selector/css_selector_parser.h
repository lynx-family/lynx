// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_RENDERER_CSS_NG_SELECTOR_CSS_SELECTOR_PARSER_H_
#define CORE_RENDERER_CSS_NG_SELECTOR_CSS_SELECTOR_PARSER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/selector/lynx_css_selector.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {
namespace css {

class CSSParserContext;
class CSSParserTokenStream;
class LynxCSSSelectorList;

struct LynxCSSParserSelector {
  LynxCSSParserSelector() : selector(std::make_unique<LynxCSSSelector>()) {}
  explicit LynxCSSParserSelector(const std::string& tag_name)
      : selector(std::make_unique<LynxCSSSelector>(tag_name)) {}
  std::unique_ptr<LynxCSSSelector> selector;
  std::unique_ptr<LynxCSSParserSelector> tag_history;
};

// SelectorVector is the list of CSS selectors as it is parsed,
// where each selector can contain others (in a tree). Typically,
// before actual use, you would convert it into a flattened list using
// LynxCSSParserSelector::AdoptSelectorVector(), but it can be useful to have
// this temporary form to find out e.g. how many bytes it will occupy (e.g. in
// StyleRule::Create) before you actually make that allocation.
using LynxCSSSelectorVector =
    std::vector<std::unique_ptr<LynxCSSParserSelector>>;

// FIXME: We should consider building CSSSelectors directly instead of using
// the intermediate LynxCSSParserSelector.
class CSSSelectorParser {
 public:
  // Both ParseSelector() and ConsumeSelector() return an empty list
  // on error.
  static LynxCSSSelectorVector ParseSelector(CSSParserTokenRange,
                                             const CSSParserContext*);

  static bool ConsumeANPlusB(CSSParserTokenRange&, std::pair<int, int>&);

  static LynxCSSSelector::PseudoType ParsePseudoType(const std::u16string&,
                                                     bool has_arguments);
  // Finds out how many elements one would need to allocate for
  // AdoptSelectorVector(), ie., storing the selector tree as a flattened list.
  // The returned count is in LynxCSSSelector elements, not bytes.
  static size_t FlattenedSize(const LynxCSSSelectorVector& selector_vector);
  static LynxCSSSelectorList AdoptSelectorVector(
      LynxCSSSelectorVector& selector_vector);
  static void AdoptSelectorVector(LynxCSSSelectorVector& selector_vector,
                                  LynxCSSSelector* selector_array,
                                  size_t flattened_size);

 private:
  CSSSelectorParser(const CSSParserContext*);

  // These will all consume trailing comments if successful

  LynxCSSSelectorVector ConsumeComplexSelectorList(CSSParserTokenRange&);
  // Consumes a complex selector list if inside_compound_pseudo_ is false,
  // otherwise consumes a compound selector list.
  LynxCSSSelectorList ConsumeNestedSelectorList(CSSParserTokenRange&);

  std::unique_ptr<LynxCSSParserSelector> ConsumeComplexSelector(
      CSSParserTokenRange&);

  // ConsumePartialComplexSelector() method provides the common logic of
  // consuming a complex selector and consuming a relative selector.
  //
  // After consuming the left-most combinator of a relative selector, we can
  // consume the remaining selectors with the common logic.
  // For example, after consuming the left-most combinator '~' of the relative
  // selector '~ .a ~ .b', we can consume remaining selectors '.a ~ .b'
  // with this method.
  //
  // After consuming the left-most compound selector and a combinator of a
  // complex selector, we can also use this method to consume the remaining
  // selectors of the complex selector.
  std::unique_ptr<LynxCSSParserSelector> ConsumePartialComplexSelector(
      CSSParserTokenRange&,
      LynxCSSSelector::RelationType& /* current combinator */,
      std::unique_ptr<LynxCSSParserSelector> /* previous compound selector */,
      unsigned& /* previous compound flags */);

  std::unique_ptr<LynxCSSParserSelector> ConsumeCompoundSelector(
      CSSParserTokenRange&);
  // This doesn't include element names, since they're handled specially
  std::unique_ptr<LynxCSSParserSelector> ConsumeSimpleSelector(
      CSSParserTokenRange&);

  bool ConsumeName(CSSParserTokenRange&, std::u16string& name);

  // These will return nullptr when the selector is invalid
  std::unique_ptr<LynxCSSParserSelector> ConsumeId(CSSParserTokenRange&);
  std::unique_ptr<LynxCSSParserSelector> ConsumeClass(CSSParserTokenRange&);
  std::unique_ptr<LynxCSSParserSelector> ConsumePseudo(CSSParserTokenRange&);
  std::unique_ptr<LynxCSSParserSelector> ConsumeAttribute(CSSParserTokenRange&);

  LynxCSSSelector::RelationType ConsumeCombinator(CSSParserTokenRange&);
  LynxCSSSelector::MatchType ConsumeAttributeMatch(CSSParserTokenRange&);
  LynxCSSSelector::AttributeMatchType ConsumeAttributeFlags(
      CSSParserTokenRange&);

  void PrependTypeSelectorIfNeeded(bool has_element_name,
                                   const std::u16string& element_name,
                                   LynxCSSParserSelector*);
  static std::unique_ptr<LynxCSSParserSelector> AddSimpleSelectorToCompound(
      std::unique_ptr<LynxCSSParserSelector> compound_selector,
      std::unique_ptr<LynxCSSParserSelector> simple_selector);
  static std::unique_ptr<LynxCSSParserSelector>
  SplitCompoundAtImplicitShadowCrossingCombinator(
      std::unique_ptr<LynxCSSParserSelector> compound_selector);

  bool failed_parsing_ = false;
  bool disallow_pseudo_elements_ = false;
  // When parsing a compound which includes a pseudo-element, the simple
  // selectors permitted to follow that pseudo-element may be restricted.
  // If this is the case, then restricting_pseudo_element_ will be set to the
  // PseudoType of the pseudo-element causing the restriction.
  LynxCSSSelector::PseudoType restricting_pseudo_element_ =
      LynxCSSSelector::kPseudoUnknown;

  class DisallowPseudoElementsScope {
   public:
    explicit DisallowPseudoElementsScope(CSSSelectorParser* parser)
        : parser_(parser), was_disallowed_(parser_->disallow_pseudo_elements_) {
      parser_->disallow_pseudo_elements_ = true;
    }
    DisallowPseudoElementsScope(const DisallowPseudoElementsScope&) = delete;
    DisallowPseudoElementsScope& operator=(const DisallowPseudoElementsScope&) =
        delete;

    ~DisallowPseudoElementsScope() {
      parser_->disallow_pseudo_elements_ = was_disallowed_;
    }

   private:
    CSSSelectorParser* parser_;
    bool was_disallowed_;
  };
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_SELECTOR_CSS_SELECTOR_PARSER_H_
