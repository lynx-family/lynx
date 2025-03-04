// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_SELECT_ELEMENT_TOKEN_H_
#define CORE_RENDERER_CSS_SELECT_ELEMENT_TOKEN_H_

#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace tasm {
class SelectElementToken {
 public:
  enum class Combinator {
    LAST = 0,                          // no next selector
    CHILD = 1,                         // "a > b"
    DESCENDANT = 2,                    // "a b"
    DESCENDANT_ACROSS_COMPONENTS = 3,  // "a >>> b"
  };

  enum class Type {
    CSS_SELECTOR = 0,
    REF_ID = 1,
    ELEMENT_ID = 2,
  };

  // does not support ','
  static std::pair<std::vector<SelectElementToken>, bool> ParseCssSelector(
      const std::string &selector_string);

  SelectElementToken(const std::string &selector_string, Type type,
                     const Combinator &combinator_to_next)
      : selector_string(selector_string),
        type(type),
        combinator_to_next(combinator_to_next) {}

  SelectElementToken(const SelectElementToken &other) = default;
  SelectElementToken &operator=(const SelectElementToken &other) = default;

  SelectElementToken(SelectElementToken &&other) = default;
  SelectElementToken &operator=(SelectElementToken &&other) = default;

  bool OnlyCurrentComponent() const {
    return combinator_to_next != Combinator::DESCENDANT_ACROSS_COMPONENTS;
  }

  bool NoDescendant() const {
    return combinator_to_next == Combinator::LAST ||
           combinator_to_next == Combinator::CHILD;
  }

  std::string selector_string;
  Type type;
  Combinator combinator_to_next;

 private:
  static std::pair<SelectElementToken, bool> ParseSingleCssSelector(
      std::string::const_iterator &begin,
      const std::string::const_iterator &end);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_SELECT_ELEMENT_TOKEN_H_
