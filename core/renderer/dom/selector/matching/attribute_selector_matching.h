// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_SELECTOR_MATCHING_ATTRIBUTE_SELECTOR_MATCHING_H_
#define CORE_RENDERER_DOM_SELECTOR_MATCHING_ATTRIBUTE_SELECTOR_MATCHING_H_

#include <optional>
#include <string_view>
#include <tuple>

namespace lynx {
namespace tasm {
class AttributeHolder;

class AttributeSelectorMatching {
 public:
  /**
   * Checks whether the given attribute holder satisfies the attribute selector.
   * Supported selector:
   * - [attr]
   * - [attr=value]
   * - [attr*=value]
   * - [attr^=value]
   * - [attr$=value]
   *
   * @param selector Attribute selector.
   * @param holder Attribute holder.
   * @return True if the given attribute holder satisfies the selector.
   */
  static bool Matches(const std::string_view& selector,
                      const AttributeHolder& holder);

 private:
  using BinOp = bool (*)(std::string_view, std::string_view);

  /**
   * Remove the square brackets at the beginning and end of the selector.
   * @return The result if the original selector is surrounded by brackets.
   */
  static std::optional<std::string_view> RemoveSelectorBrackets(
      const std::string_view& selector);
  /**
   * Parse attribute key, comparison op and value from the selector.
   * @return A tuple containing the attribute key, comparison BinOp and value.
   */
  static std::tuple<std::string_view, BinOp, std::string_view>
  GetSelectorAttrKeyOpValue(const std::string_view& selector);

  static constexpr BinOp EQ = [](std::string_view attr,
                                 std::string_view expected) {
    return expected == attr;
  };

  static constexpr BinOp CONTAINS = [](std::string_view attr,
                                       std::string_view expected) {
    return attr.find(expected) != attr.npos;
  };
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_SELECTOR_MATCHING_ATTRIBUTE_SELECTOR_MATCHING_H_
