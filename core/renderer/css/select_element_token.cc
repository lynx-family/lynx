// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/select_element_token.h"

#include <algorithm>
#include <iterator>
namespace lynx {
namespace tasm {

namespace {
inline bool isblank(char c) { return c == ' ' || c == '\t'; }
}  // namespace

/**
 * @returns a std::pair.
 * pair.first is a vector of selectors parsed.
 * pair.second indicates if the selector is legal.
 */
std::pair<std::vector<SelectElementToken>, bool>
SelectElementToken::ParseCssSelector(const std::string& selector_string) {
  std::vector<SelectElementToken> res;
  for (auto begin = selector_string.cbegin();
       begin != selector_string.cend();) {
    auto pair = ParseSingleCssSelector(begin, selector_string.cend());
    if (!pair.second) {
      return {std::vector<SelectElementToken>(), false};
    }
    res.push_back(pair.first);
    // skip spaces
    begin = std::find_if_not(begin, selector_string.cend(), &isblank);
  }
  if (!res.empty() && res.back().combinator_to_next != Combinator::LAST) {
    return {std::vector<SelectElementToken>(), false};
  }
  return {res, true};
}

// parse a single selector. move begin iterator to the beginning of next single
// selector.
//
// current supported css selectors:
// id: "#id"
// class: ".class"
// tag: "tag"
// attribute: "[attribute=value]"
// child: "#a>#b"
// descendant: "#a #b"
// descendant_cross_components: "#a>>>#b"
std::pair<SelectElementToken, bool> SelectElementToken::ParseSingleCssSelector(
    std::string::const_iterator& begin,
    const std::string::const_iterator& end) {
  // skip spaces
  begin = std::find_if_not(begin, end, &isblank);
  if (begin == end) {
    return {SelectElementToken("", Type::CSS_SELECTOR, Combinator::LAST),
            false};
  }

  // check first character to filter not support selectors.
  if (*begin != '#'     // "#id"
      && *begin != '.'  // ".class"
      && *begin != '['  // "[attribute=value]"
      && !std::isalpha(static_cast<unsigned int>(*begin))  // "tag"
  ) {
    return {SelectElementToken("", Type::CSS_SELECTOR, Combinator::LAST),
            false};
  }

  // read selector string until space or '>' is met.
  std::string selector_string;
  auto combinator_begin =
      std::find_if(begin, end, [](auto c) { return isblank(c) || c == '>'; });
  std::copy(begin, combinator_begin, std::back_inserter(selector_string));

  // check selector string
  bool empty = selector_string.empty();
  bool character_only =
      selector_string.size() == 1 && !std::isalpha(selector_string.front());
  if (empty || character_only) {
    return {SelectElementToken("", Type::CSS_SELECTOR, Combinator::LAST),
            false};
  }

  // read combinator. only supports selectors like "a b" , "a>b" and "a>>>b".
  begin = std::find_if_not(combinator_begin, end, &isblank);
  if (begin == end) {
    return {SelectElementToken(selector_string, Type::CSS_SELECTOR,
                               Combinator::LAST),
            true};
  }
  if (*begin == '>') {
    std::advance(begin, 1);
    if (std::distance(begin, end) >= 2 && *begin == '>' &&
        *std::next(begin) == '>') {
      std::advance(begin, 2);
      return {SelectElementToken(selector_string, Type::CSS_SELECTOR,
                                 Combinator::DESCENDANT_ACROSS_COMPONENTS),
              true};
    } else {
      return {SelectElementToken(selector_string, Type::CSS_SELECTOR,
                                 Combinator::CHILD),
              true};
    }
  } else {
    // is descendant combinator
    return {SelectElementToken(selector_string, Type::CSS_SELECTOR,
                               Combinator::DESCENDANT),
            true};
  }
}

}  // namespace tasm
}  // namespace lynx
