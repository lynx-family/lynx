// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/renderer/css/ng/selector/css_selector_parser.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/ng/parser/css_parser_token_stream.h"
#include "core/renderer/css/ng/parser/string_to_number.h"

namespace lynx {
namespace css {

namespace {

void AppendTagHistory(LynxCSSParserSelector* target,
                      LynxCSSSelector::RelationType relation,
                      std::unique_ptr<LynxCSSParserSelector> selector) {
  LynxCSSParserSelector* end = target;
  while (end->tag_history) {
    end = end->tag_history.get();
  }
  end->selector->SetRelation(relation);
  end->tag_history = std::move(selector);
}

LynxCSSSelector::RelationType GetImplicitShadowCombinatorForMatching(
    LynxCSSParserSelector* selector) {
  switch (selector->selector->GetPseudoType()) {
    case LynxCSSSelector::PseudoType::kPseudoPlaceholder:
    case LynxCSSSelector::PseudoType::kPseudoSelection:
      return LynxCSSSelector::RelationType::kUAShadow;
    default:
      return LynxCSSSelector::RelationType::kSubSelector;
  }
}

bool NeedsImplicitShadowCombinatorForMatching(LynxCSSParserSelector* selector) {
  return GetImplicitShadowCombinatorForMatching(selector) !=
         LynxCSSSelector::RelationType::kSubSelector;
}

}  // namespace

// static
LynxCSSSelectorVector CSSSelectorParser::ParseSelector(
    CSSParserTokenRange range, const CSSParserContext* context) {
  CSSSelectorParser parser(context);
  range.ConsumeWhitespace();
  LynxCSSSelectorVector result = parser.ConsumeComplexSelectorList(range);
  if (!range.AtEnd()) return {};

  return result;
}

CSSSelectorParser::CSSSelectorParser(const CSSParserContext* context) {}

LynxCSSSelectorVector CSSSelectorParser::ConsumeComplexSelectorList(
    CSSParserTokenRange& range) {
  LynxCSSSelectorVector selector_list;
  std::unique_ptr<LynxCSSParserSelector> selector =
      ConsumeComplexSelector(range);
  if (!selector) return {};
  selector_list.push_back(std::move(selector));
  while (!range.AtEnd() && range.Peek().GetType() == kCommaToken) {
    range.ConsumeIncludingWhitespace();
    selector = ConsumeComplexSelector(range);
    if (!selector) return {};
    selector_list.push_back(std::move(selector));
  }

  if (failed_parsing_) return {};

  return selector_list;
}

LynxCSSSelectorList CSSSelectorParser::ConsumeNestedSelectorList(
    CSSParserTokenRange& range) {
  LynxCSSSelectorVector result = ConsumeComplexSelectorList(range);
  if (result.empty()) return {};
  return AdoptSelectorVector(result);
}

namespace {

enum CompoundSelectorFlags {
  kHasPseudoElementForRightmostCompound = 1 << 0,
};

unsigned ExtractCompoundFlags(const LynxCSSParserSelector& simple_selector) {
  if (simple_selector.selector->Match() != LynxCSSSelector::kPseudoElement)
    return 0;
  // We don't restrict what follows custom ::-webkit-* pseudo elements in UA
  // sheets. We currently use selectors in mediaControls.css like this:
  return kHasPseudoElementForRightmostCompound;
}

}  // namespace

std::unique_ptr<LynxCSSParserSelector>
CSSSelectorParser::ConsumeComplexSelector(CSSParserTokenRange& range) {
  std::unique_ptr<LynxCSSParserSelector> selector =
      ConsumeCompoundSelector(range);
  if (!selector) return nullptr;

  unsigned previous_compound_flags = 0;

  for (LynxCSSParserSelector* simple = selector.get();
       simple && !previous_compound_flags; simple = simple->tag_history.get())
    previous_compound_flags |= ExtractCompoundFlags(*simple);

  if (LynxCSSSelector::RelationType combinator = ConsumeCombinator(range)) {
    return ConsumePartialComplexSelector(range, combinator, std::move(selector),
                                         previous_compound_flags);
  }

  return selector;
}

std::unique_ptr<LynxCSSParserSelector>
CSSSelectorParser::ConsumePartialComplexSelector(
    CSSParserTokenRange& range, LynxCSSSelector::RelationType& combinator,
    std::unique_ptr<LynxCSSParserSelector> selector,
    unsigned& previous_compound_flags) {
  do {
    std::unique_ptr<LynxCSSParserSelector> next_selector =
        ConsumeCompoundSelector(range);
    if (!next_selector)
      return combinator == LynxCSSSelector::kDescendant ? std::move(selector)
                                                        : nullptr;
    if (previous_compound_flags & kHasPseudoElementForRightmostCompound)
      return nullptr;
    LynxCSSParserSelector* end = next_selector.get();
    unsigned compound_flags = ExtractCompoundFlags(*end);
    while (end->tag_history) {
      end = end->tag_history.get();
      compound_flags |= ExtractCompoundFlags(*end);
    }
    end->selector->SetRelation(combinator);
    previous_compound_flags = compound_flags;
    end->tag_history = std::move(selector);

    selector = std::move(next_selector);
  } while ((combinator = ConsumeCombinator(range)));

  return selector;
}

// Could be made smaller and faster by replacing pointer with an
// offset into a string buffer and making the bit fields smaller but
// that could not be maintained by hand.
struct NameToPseudoStruct {
  const char* string;
  unsigned type : 8;
};

// These tables should be kept sorted.
const static NameToPseudoStruct kPseudoTypeWithoutArgumentsMap[] = {
    {"active", LynxCSSSelector::kPseudoActive},
    {"after", LynxCSSSelector::kPseudoAfter},
    {"backdrop", LynxCSSSelector::kPseudoBackdrop},
    {"before", LynxCSSSelector::kPseudoBefore},
    {"checked", LynxCSSSelector::kPseudoChecked},
    {"default", LynxCSSSelector::kPseudoDefault},
    {"disabled", LynxCSSSelector::kPseudoDisabled},
    {"empty", LynxCSSSelector::kPseudoEmpty},
    {"enabled", LynxCSSSelector::kPseudoEnabled},
    {"first-child", LynxCSSSelector::kPseudoFirstChild},
    {"first-letter", LynxCSSSelector::kPseudoFirstLetter},
    {"first-line", LynxCSSSelector::kPseudoFirstLine},
    {"first-of-type", LynxCSSSelector::kPseudoFirstOfType},
    {"focus", LynxCSSSelector::kPseudoFocus},
    {"hover", LynxCSSSelector::kPseudoHover},
    {"last-child", LynxCSSSelector::kPseudoLastChild},
    {"last-of-type", LynxCSSSelector::kPseudoLastOfType},
    {"link", LynxCSSSelector::kPseudoLink},
    {"only-child", LynxCSSSelector::kPseudoOnlyChild},
    {"only-of-type", LynxCSSSelector::kPseudoOnlyOfType},
    {"placeholder", LynxCSSSelector::kPseudoPlaceholder},
    {"root", LynxCSSSelector::kPseudoRoot},
    {"selection", LynxCSSSelector::kPseudoSelection},
};

const static NameToPseudoStruct kPseudoTypeWithArgumentsMap[] = {
    {"dir", LynxCSSSelector::kPseudoDir},
    {"has", LynxCSSSelector::kPseudoHas},
    {"is", LynxCSSSelector::kPseudoIs},
    {"lang", LynxCSSSelector::kPseudoLang},
    {"not", LynxCSSSelector::kPseudoNot},
    {"nth-child", LynxCSSSelector::kPseudoNthChild},
    {"nth-last-child", LynxCSSSelector::kPseudoNthLastChild},
    {"nth-last-of-type", LynxCSSSelector::kPseudoNthLastOfType},
    {"nth-of-type", LynxCSSSelector::kPseudoNthOfType},
    {"where", LynxCSSSelector::kPseudoWhere},
};

static LynxCSSSelector::PseudoType NameToPseudoType(const std::string& name,
                                                    bool has_arguments) {
  if (name.empty()) return LynxCSSSelector::kPseudoUnknown;

  const NameToPseudoStruct* pseudo_type_map;
  const NameToPseudoStruct* pseudo_type_map_end;
  if (has_arguments) {
    pseudo_type_map = kPseudoTypeWithArgumentsMap;
    pseudo_type_map_end =
        kPseudoTypeWithArgumentsMap + std::size(kPseudoTypeWithArgumentsMap);
  } else {
    pseudo_type_map = kPseudoTypeWithoutArgumentsMap;
    pseudo_type_map_end = kPseudoTypeWithoutArgumentsMap +
                          std::size(kPseudoTypeWithoutArgumentsMap);
  }
  const NameToPseudoStruct* match = std::lower_bound(
      pseudo_type_map, pseudo_type_map_end, name,
      [](const NameToPseudoStruct& entry, const std::string& name) -> bool {
        DCHECK(entry.string);
        // If strncmp returns 0, then either the keys are equal, or |name| sorts
        // before |entry|.
        return strncmp(entry.string, reinterpret_cast<const char*>(name.data()),
                       name.length()) < 0;
      });
  if (match == pseudo_type_map_end || match->string != name)
    return LynxCSSSelector::kPseudoUnknown;

  return static_cast<LynxCSSSelector::PseudoType>(match->type);
}

LynxCSSSelector::PseudoType CSSSelectorParser::ParsePseudoType(
    const std::u16string& name, bool has_arguments) {
  auto key = ustring_helper::to_string(name);
  LynxCSSSelector::PseudoType pseudo_type =
      NameToPseudoType(key, has_arguments);
  if (pseudo_type != LynxCSSSelector::PseudoType::kPseudoUnknown)
    return pseudo_type;

  return LynxCSSSelector::PseudoType::kPseudoUnknown;
}

namespace {

bool IsSimpleSelectorValidAfterPseudoElement(
    const LynxCSSParserSelector& simple_selector,
    LynxCSSSelector::PseudoType compound_pseudo_element) {
  switch (compound_pseudo_element) {
    case LynxCSSSelector::kPseudoUnknown:
      return true;
    case LynxCSSSelector::kPseudoAfter:
    case LynxCSSSelector::kPseudoBefore:
      break;
    default:
      break;
  }
  if (simple_selector.selector->Match() != LynxCSSSelector::kPseudoClass)
    return false;
  LynxCSSSelector::PseudoType pseudo =
      simple_selector.selector->GetPseudoType();
  switch (pseudo) {
    case LynxCSSSelector::kPseudoNot:
      // These pseudo-classes are themselves always valid.
      // CSSSelectorParser::restricting_pseudo_element_ ensures that invalid
      // nested selectors will be dropped if they are invalid according to
      // this function.
      return true;
    default:
      break;
  }
  return false;
}

}  // namespace

std::unique_ptr<LynxCSSParserSelector>
CSSSelectorParser::ConsumeCompoundSelector(CSSParserTokenRange& range) {
  base::AutoReset<LynxCSSSelector::PseudoType> reset_restricting(
      &restricting_pseudo_element_, restricting_pseudo_element_);

  std::unique_ptr<LynxCSSParserSelector> compound_selector;
  std::u16string element_name;
  const bool has_tag_name = ConsumeName(range, element_name);
  if (!has_tag_name) {
    compound_selector = ConsumeSimpleSelector(range);
    if (!compound_selector) return nullptr;
    if (compound_selector->selector->Match() == LynxCSSSelector::kPseudoElement)
      restricting_pseudo_element_ =
          compound_selector->selector->GetPseudoType();
  }
  element_name = ToLower(element_name);

  while (std::unique_ptr<LynxCSSParserSelector> simple_selector =
             ConsumeSimpleSelector(range)) {
    if (simple_selector->selector->Match() == LynxCSSSelector::kPseudoElement)
      restricting_pseudo_element_ = simple_selector->selector->GetPseudoType();

    if (compound_selector) {
      compound_selector = AddSimpleSelectorToCompound(
          std::move(compound_selector), std::move(simple_selector));
    } else {
      compound_selector = std::move(simple_selector);
    }
  }

  if (!compound_selector) {
    return std::make_unique<LynxCSSParserSelector>(
        ustring_helper::to_string(element_name));
  }

  // Prepending a type selector to the compound is
  // unnecessary if this compound is an argument to a pseudo selector like
  // :not(), since a type selector will be prepended at the top level of the
  // selector if necessary. We need to propagate that context information here
  // to tell if we are at the top level.
  PrependTypeSelectorIfNeeded(has_tag_name, element_name,
                              compound_selector.get());
  return SplitCompoundAtImplicitShadowCrossingCombinator(
      std::move(compound_selector));
}

std::unique_ptr<LynxCSSParserSelector> CSSSelectorParser::ConsumeSimpleSelector(
    CSSParserTokenRange& range) {
  const CSSParserToken& token = range.Peek();
  std::unique_ptr<LynxCSSParserSelector> selector;
  if (token.GetType() == kHashToken)
    selector = ConsumeId(range);
  else if (token.GetType() == kDelimiterToken && token.Delimiter() == '.')
    selector = ConsumeClass(range);
  else if (token.GetType() == kLeftBracketToken)
    selector = ConsumeAttribute(range);
  else if (token.GetType() == kColonToken)
    selector = ConsumePseudo(range);
  else
    return nullptr;
  // TODO(futhark@chromium.org): crbug.com/578131
  // The UASheetMode check is a work-around to allow this selector in
  // mediaControls(New).css:
  // video::-webkit-media-text-track-region-container.scrolling
  if (!selector || (!IsSimpleSelectorValidAfterPseudoElement(
                       *selector.get(), restricting_pseudo_element_))) {
    failed_parsing_ = true;
  }
  return selector;
}

bool CSSSelectorParser::ConsumeName(CSSParserTokenRange& range,
                                    std::u16string& name) {
  name = CSSGlobalEmptyU16String();

  const CSSParserToken& first_token = range.Peek();
  if (first_token.GetType() == kIdentToken) {
    name = first_token.Value();
    range.Consume();
  } else if (first_token.GetType() == kDelimiterToken &&
             first_token.Delimiter() == '*') {
    name = CSSGlobalStarU16String();
    range.Consume();
  } else if (first_token.GetType() == kDelimiterToken &&
             first_token.Delimiter() == '|') {
    // This is an empty namespace, which'll get assigned this value below
    name = CSSGlobalEmptyU16String();
  } else {
    return false;
  }

  if (range.Peek().GetType() != kDelimiterToken ||
      range.Peek().Delimiter() != '|')
    return true;

  if (range.Peek(1).GetType() == kIdentToken) {
    range.Consume();
    name = range.Consume().Value();
  } else if (range.Peek(1).GetType() == kDelimiterToken &&
             range.Peek(1).Delimiter() == '*') {
    range.Consume();
    range.Consume();
    name = CSSGlobalStarU16String();
  } else {
    name = CSSGlobalEmptyU16String();
    return false;
  }

  return true;
}

std::unique_ptr<LynxCSSParserSelector> CSSSelectorParser::ConsumeId(
    CSSParserTokenRange& range) {
  DCHECK_EQ(range.Peek().GetType(), kHashToken);
  if (range.Peek().GetHashTokenType() != kHashTokenId) return nullptr;
  std::unique_ptr<LynxCSSParserSelector> selector =
      std::make_unique<LynxCSSParserSelector>();
  selector->selector->SetMatch(LynxCSSSelector::kId);
  selector->selector->SetValue(
      ustring_helper::to_string(range.Consume().Value()));
  return selector;
}

std::unique_ptr<LynxCSSParserSelector> CSSSelectorParser::ConsumeClass(
    CSSParserTokenRange& range) {
  DCHECK_EQ(range.Peek().GetType(), kDelimiterToken);
  DCHECK_EQ(range.Peek().Delimiter(), '.');
  range.Consume();
  if (range.Peek().GetType() != kIdentToken) return nullptr;
  std::unique_ptr<LynxCSSParserSelector> selector =
      std::make_unique<LynxCSSParserSelector>();
  selector->selector->SetMatch(LynxCSSSelector::kClass);
  std::u16string value = range.Consume().Value();
  selector->selector->SetValue(ustring_helper::to_string(value));
  return selector;
}

std::unique_ptr<LynxCSSParserSelector> CSSSelectorParser::ConsumeAttribute(
    CSSParserTokenRange& range) {
  DCHECK_EQ(range.Peek().GetType(), kLeftBracketToken);
  CSSParserTokenRange block = range.ConsumeBlock();
  block.ConsumeWhitespace();

  std::u16string attribute_name;
  if (!ConsumeName(block, attribute_name)) return nullptr;
  if (attribute_name == CSSGlobalStarU16String() || attribute_name.empty())
    return nullptr;
  block.ConsumeWhitespace();

  attribute_name = ToLower(attribute_name);
  std::unique_ptr<LynxCSSParserSelector> selector =
      std::make_unique<LynxCSSParserSelector>();

  if (block.AtEnd()) {
    selector->selector->SetAttribute(
        ustring_helper::to_string(attribute_name),
        LynxCSSSelector::AttributeMatchType::kCaseSensitive);
    selector->selector->SetMatch(LynxCSSSelector::kAttributeSet);
    return selector;
  }

  selector->selector->SetMatch(ConsumeAttributeMatch(block));

  const CSSParserToken& attribute_value = block.ConsumeIncludingWhitespace();
  if (attribute_value.GetType() != kIdentToken &&
      attribute_value.GetType() != kStringToken)
    return nullptr;
  selector->selector->SetValue(
      ustring_helper::to_string(attribute_value.Value()));
  selector->selector->SetAttribute(ustring_helper::to_string(attribute_name),
                                   ConsumeAttributeFlags(block));

  if (!block.AtEnd()) return nullptr;
  return selector;
}

std::unique_ptr<LynxCSSParserSelector> CSSSelectorParser::ConsumePseudo(
    CSSParserTokenRange& range) {
  DCHECK_EQ(range.Peek().GetType(), kColonToken);
  range.Consume();

  int colons = 1;
  if (range.Peek().GetType() == kColonToken) {
    range.Consume();
    colons++;
  }

  const CSSParserToken& token = range.Peek();
  if (token.GetType() != kIdentToken && token.GetType() != kFunctionToken)
    return nullptr;

  std::unique_ptr<LynxCSSParserSelector> selector =
      std::make_unique<LynxCSSParserSelector>();
  selector->selector->SetMatch(colons == 1 ? LynxCSSSelector::kPseudoClass
                                           : LynxCSSSelector::kPseudoElement);

  const std::u16string& value = token.Value();
  bool has_arguments = token.GetType() == kFunctionToken;
  std::string lower_value = ToLowerASCII(value);
  LynxCSSSelector::PseudoType pseudo_type =
      CSSSelectorParser::ParsePseudoType(value, has_arguments);
  selector->selector->SetValue(pseudo_type == LynxCSSSelector::kPseudoState
                                   ? ustring_helper::to_string(value)
                                   : lower_value);
  selector->selector->UpdatePseudoType(pseudo_type);

  if (selector->selector->Match() == LynxCSSSelector::kPseudoElement) {
    switch (selector->selector->GetPseudoType()) {
      case LynxCSSSelector::kPseudoBefore:
      case LynxCSSSelector::kPseudoAfter:
        break;
      default:
        break;
    }
  }

  if (selector->selector->Match() == LynxCSSSelector::kPseudoElement &&
      disallow_pseudo_elements_)
    return nullptr;

  if (token.GetType() == kIdentToken) {
    range.Consume();
    if (selector->selector->GetPseudoType() == LynxCSSSelector::kPseudoUnknown)
      return nullptr;
    return selector;
  }

  CSSParserTokenRange block = range.ConsumeBlock();
  block.ConsumeWhitespace();
  if (selector->selector->GetPseudoType() == LynxCSSSelector::kPseudoUnknown)
    return nullptr;

  switch (selector->selector->GetPseudoType()) {
    case LynxCSSSelector::kPseudoNot: {
      DisallowPseudoElementsScope scope(this);

      std::unique_ptr<LynxCSSSelectorList> selector_list =
          std::make_unique<LynxCSSSelectorList>();
      *selector_list = ConsumeNestedSelectorList(block);
      if (!selector_list->IsValid() || !block.AtEnd()) return nullptr;

      selector->selector->SetSelectorList(std::move(selector_list));
      return selector;
    }
    case LynxCSSSelector::kPseudoDir: {
      const CSSParserToken& ident = block.ConsumeIncludingWhitespace();
      if (ident.GetType() != kIdentToken || !block.AtEnd()) return nullptr;
      selector->selector->SetArgument(ustring_helper::to_string(ident.Value()));
      return selector;
    }
    case LynxCSSSelector::kPseudoLang: {
      // FIXME: CSS Selectors Level 4 allows :lang(*-foo)
      const CSSParserToken& ident = block.ConsumeIncludingWhitespace();
      if (ident.GetType() != kIdentToken || !block.AtEnd()) return nullptr;
      selector->selector->SetArgument(ustring_helper::to_string(ident.Value()));
      return selector;
    }
    case LynxCSSSelector::kPseudoNthChild:
    case LynxCSSSelector::kPseudoNthLastChild:
    case LynxCSSSelector::kPseudoNthOfType:
    case LynxCSSSelector::kPseudoNthLastOfType: {
      std::pair<int, int> ab;
      if (!ConsumeANPlusB(block, ab)) return nullptr;
      block.ConsumeWhitespace();
      if (!block.AtEnd()) return nullptr;
      selector->selector->SetNth(ab.first, ab.second);
      return selector;
    }
    default:
      break;
  }

  return nullptr;
}

LynxCSSSelector::RelationType CSSSelectorParser::ConsumeCombinator(
    CSSParserTokenRange& range) {
  LynxCSSSelector::RelationType fallback_result = LynxCSSSelector::kSubSelector;
  while (range.Peek().GetType() == kWhitespaceToken) {
    range.Consume();
    fallback_result = LynxCSSSelector::kDescendant;
  }

  if (range.Peek().GetType() != kDelimiterToken) return fallback_result;

  switch (range.Peek().Delimiter()) {
    case '+':
      range.ConsumeIncludingWhitespace();
      return LynxCSSSelector::kDirectAdjacent;

    case '~':
      range.ConsumeIncludingWhitespace();
      return LynxCSSSelector::kIndirectAdjacent;

    case '>':
      range.ConsumeIncludingWhitespace();
      return LynxCSSSelector::kChild;

    default:
      break;
  }
  return fallback_result;
}

LynxCSSSelector::MatchType CSSSelectorParser::ConsumeAttributeMatch(
    CSSParserTokenRange& range) {
  const CSSParserToken& token = range.ConsumeIncludingWhitespace();
  switch (token.GetType()) {
    case kIncludeMatchToken:
      return LynxCSSSelector::kAttributeList;
    case kDashMatchToken:
      return LynxCSSSelector::kAttributeHyphen;
    case kPrefixMatchToken:
      return LynxCSSSelector::kAttributeBegin;
    case kSuffixMatchToken:
      return LynxCSSSelector::kAttributeEnd;
    case kSubstringMatchToken:
      return LynxCSSSelector::kAttributeContain;
    case kDelimiterToken:
      if (token.Delimiter() == '=') return LynxCSSSelector::kAttributeExact;
      [[fallthrough]];
    default:
      failed_parsing_ = true;
      return LynxCSSSelector::kAttributeExact;
  }
}

LynxCSSSelector::AttributeMatchType CSSSelectorParser::ConsumeAttributeFlags(
    CSSParserTokenRange& range) {
  if (range.Peek().GetType() != kIdentToken)
    return LynxCSSSelector::AttributeMatchType::kCaseSensitive;
  const CSSParserToken& flag = range.ConsumeIncludingWhitespace();
  if (EqualIgnoringASCIICase(flag.Value(), u"i"))
    return LynxCSSSelector::AttributeMatchType::kCaseInsensitive;
  else if (EqualIgnoringASCIICase(flag.Value(), u"s")/* &&
           RuntimeEnabledFeatures::CSSCaseSensitiveSelectorEnabled()*/)
    return LynxCSSSelector::AttributeMatchType::kCaseSensitiveAlways;
  failed_parsing_ = true;
  return LynxCSSSelector::AttributeMatchType::kCaseSensitive;
}

bool CSSSelectorParser::ConsumeANPlusB(CSSParserTokenRange& range,
                                       std::pair<int, int>& result) {
  const CSSParserToken& token = range.Consume();
  if (token.GetType() == kNumberToken &&
      token.GetNumericValueType() == kIntegerValueType) {
    result = std::make_pair(0, token.NumericValue());
    return true;
  }
  if (token.GetType() == kIdentToken) {
    if (EqualIgnoringASCIICase(token.Value(), u"odd")) {
      result = std::make_pair(2, 1);
      return true;
    }
    if (EqualIgnoringASCIICase(token.Value(), u"even")) {
      result = std::make_pair(2, 0);
      return true;
    }
  }

  // The 'n' will end up as part of an ident or dimension. For a valid <an+b>,
  // this will store a string of the form 'n', 'n-', or 'n-123'.
  std::u16string n_string;

  if (token.GetType() == kDelimiterToken && token.Delimiter() == '+' &&
      range.Peek().GetType() == kIdentToken) {
    result.first = 1;
    n_string = range.Consume().Value();
  } else if (token.GetType() == kDimensionToken &&
             token.GetNumericValueType() == kIntegerValueType) {
    result.first = std::clamp<int64_t>(
        static_cast<int64_t>(token.NumericValue()),
        std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    n_string = token.Value();
  } else if (token.GetType() == kIdentToken) {
    if (token.Value()[0] == '-') {
      result.first = -1;
      n_string = token.Value().substr(1);
    } else {
      result.first = 1;
      n_string = token.Value();
    }
  }

  range.ConsumeWhitespace();

  if (n_string.empty() || !base::IsASCIIAlphaCaselessEqual(n_string[0], 'n'))
    return false;
  if (n_string.length() > 1 && n_string[1] != '-') return false;

  if (n_string.length() > 2) {
    bool valid;
    std::u16string s = n_string.substr(1);
    result.second = CharactersToInt(s.data(), s.length(),
                                    NumberParsingOptions::kStrict, &valid);
    return valid;
  }

  NumericSign sign = n_string.length() == 1 ? kNoSign : kMinusSign;
  if (sign == kNoSign && range.Peek().GetType() == kDelimiterToken) {
    char delimiter_sign = range.ConsumeIncludingWhitespace().Delimiter();
    if (delimiter_sign == '+')
      sign = kPlusSign;
    else if (delimiter_sign == '-')
      sign = kMinusSign;
    else
      return false;
  }

  if (sign == kNoSign && range.Peek().GetType() != kNumberToken) {
    result.second = 0;
    return true;
  }

  const CSSParserToken& b = range.Consume();
  if (b.GetType() != kNumberToken ||
      b.GetNumericValueType() != kIntegerValueType)
    return false;
  if ((b.GetNumericSign() == kNoSign) == (sign == kNoSign)) return false;
  result.second = std::clamp<int64_t>(static_cast<int64_t>(b.NumericValue()),
                                      std::numeric_limits<int>::lowest(),
                                      std::numeric_limits<int>::max());
  if (sign == kMinusSign) {
    // Negating minimum integer returns itself, instead return max integer.
    if (UNLIKELY(result.second == std::numeric_limits<int>::min()))
      result.second = std::numeric_limits<int>::max();
    else
      result.second = -result.second;
  }
  return true;
}

void CSSSelectorParser::PrependTypeSelectorIfNeeded(
    bool has_q_name, const std::u16string& element_name,
    LynxCSSParserSelector* compound_selector) {
  if (!has_q_name &&
      !NeedsImplicitShadowCombinatorForMatching(compound_selector))
    return;

  std::u16string determined_element_name =
      !has_q_name ? CSSGlobalStarU16String() : element_name;
  std::u16string tag = determined_element_name;

  if (tag != CSSGlobalStarU16String() ||
      NeedsImplicitShadowCombinatorForMatching(compound_selector)) {
    std::unique_ptr<LynxCSSParserSelector> second =
        std::make_unique<LynxCSSParserSelector>();
    second->selector = std::move(compound_selector->selector);
    second->tag_history = std::move(compound_selector->tag_history);
    compound_selector->tag_history = std::move(second);
    compound_selector->selector = std::make_unique<LynxCSSSelector>(
        ustring_helper::to_string(tag),
        determined_element_name == CSSGlobalStarU16String());
  }
}

std::unique_ptr<LynxCSSParserSelector>
CSSSelectorParser::AddSimpleSelectorToCompound(
    std::unique_ptr<LynxCSSParserSelector> compound_selector,
    std::unique_ptr<LynxCSSParserSelector> simple_selector) {
  AppendTagHistory(compound_selector.get(), LynxCSSSelector::kSubSelector,
                   std::move(simple_selector));
  return compound_selector;
}

std::unique_ptr<LynxCSSParserSelector>
CSSSelectorParser::SplitCompoundAtImplicitShadowCrossingCombinator(
    std::unique_ptr<LynxCSSParserSelector> compound_selector) {
  // The tagHistory is a linked list that stores combinator separated compound
  // selectors from right-to-left. Yet, within a single compound selector,
  // stores the simple selectors from left-to-right.
  //
  // ".a.b > div#id" is stored in a tagHistory as [div, #id, .a, .b], each
  // element in the list stored with an associated relation (combinator or
  // SubSelector).
  //
  // ::cue, ::shadow, and custom pseudo elements have an implicit ShadowPseudo
  // combinator to their left, which really makes for a new compound selector,
  // yet it's consumed by the selector parser as a single compound selector.
  //
  // Example:
  //
  // input#x::-webkit-clear-button -> [ ::-webkit-clear-button, input, #x ]
  //
  // Likewise, ::slotted() pseudo element has an implicit ShadowSlot combinator
  // to its left for finding matching slot element in other TreeScope.
  //
  // ::part has a implicit ShadowPart combinator to it's left finding the host
  // element in the scope of the style rule.
  //
  // Example:
  //
  // slot[name=foo]::slotted(div) -> [ ::slotted(div), slot, [name=foo] ]
  LynxCSSParserSelector* split_after = compound_selector.get();

  while (split_after->tag_history && !NeedsImplicitShadowCombinatorForMatching(
                                         split_after->tag_history.get())) {
    split_after = split_after->tag_history.get();
  }

  if (!split_after->tag_history.get()) {
    return compound_selector;
  }

  std::unique_ptr<LynxCSSParserSelector> remaining =
      std::move(split_after->tag_history);
  LynxCSSSelector::RelationType relation =
      GetImplicitShadowCombinatorForMatching(remaining.get());
  // We might need to split the compound twice since ::placeholder is allowed
  // after ::slotted and they both need an implicit combinator for matching.
  remaining =
      SplitCompoundAtImplicitShadowCrossingCombinator(std::move(remaining));
  AppendTagHistory(remaining.get(), relation, std::move(compound_selector));
  return remaining;
}

size_t CSSSelectorParser::FlattenedSize(
    const LynxCSSSelectorVector& selector_vector) {
  size_t flattened_size = 0;
  for (const std::unique_ptr<LynxCSSParserSelector>& selector_ptr :
       selector_vector) {
    for (LynxCSSParserSelector* selector = selector_ptr.get(); selector;
         selector = selector->tag_history.get())
      ++flattened_size;
  }
  // DCHECK(flattened_size);
  return flattened_size;
}

static size_t SelectorIndex(LynxCSSSelector* selector_array,
                            const LynxCSSSelector& selector) {
  return static_cast<size_t>(&selector - selector_array);
}

static const LynxCSSSelector& SelectorAt(LynxCSSSelector* selector_array,
                                         size_t index) {
  return selector_array[index];
}

static size_t IndexOfNextSelectorAfter(LynxCSSSelector* selector_array,
                                       size_t index) {
  const LynxCSSSelector& current = SelectorAt(selector_array, index);
  const LynxCSSSelector* next = LynxCSSSelectorList::Next(current);
  if (!next) return UINT_MAX;
  return SelectorIndex(selector_array, *next);
}

void CSSSelectorParser::AdoptSelectorVector(
    LynxCSSSelectorVector& selector_vector, LynxCSSSelector* selector_array,
    size_t flattened_size) {
  DCHECK_EQ(flattened_size, FlattenedSize(selector_vector));
  size_t array_index = 0;
  for (const std::unique_ptr<LynxCSSParserSelector>& selector_ptr :
       selector_vector) {
    LynxCSSParserSelector* current = selector_ptr.get();
    while (current) {
      selector_array[array_index] = std::move(*current->selector);
      current = current->tag_history.get();
      DCHECK(!selector_array[array_index].IsLastInSelectorList());
      if (current) {
        selector_array[array_index].SetLastInTagHistory(false);
      }
      ++array_index;
    }
    DCHECK(selector_array[array_index - 1].IsLastInTagHistory());
  }
  DCHECK_EQ(flattened_size, array_index);
  selector_array[array_index - 1].SetLastInSelectorList(true);
  for (size_t selector_index = 0; selector_index != UINT_MAX;
       selector_index =
           IndexOfNextSelectorAfter(selector_array, selector_index)) {
    selector_array[selector_index].UpdateSpecificity(
        selector_array[selector_index].CalcSpecificity());
  }

  selector_vector.clear();
}

LynxCSSSelectorList CSSSelectorParser::AdoptSelectorVector(
    LynxCSSSelectorVector& selector_vector) {
  if (selector_vector.empty()) {
    return {};
  }

  size_t flattened_size = FlattenedSize(selector_vector);

  auto selector_array = std::make_unique<LynxCSSSelector[]>(flattened_size);
  AdoptSelectorVector(selector_vector, selector_array.get(), flattened_size);
  return {std::move(selector_array)};
}

}  // namespace css
}  // namespace lynx
