// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/selector/matching/attribute_selector_matching.h"

#include <string>
#include <tuple>
#include <utility>

#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {
bool AttributeSelectorMatching::Matches(const std::string_view& selector,
                                        const AttributeHolder& holder) {
  auto selector_with_brackets_removed = RemoveSelectorBrackets(selector);
  if (!selector_with_brackets_removed) {
    return false;
  }

  auto [key, bin_op, value] =
      GetSelectorAttrKeyOpValue(*selector_with_brackets_removed);
  if (key.substr(0, 5) == "data-") {
    std::string dataset_key_str{key.substr(5)};
    const auto& dataset = holder.dataset();
    const auto iter = dataset.find(dataset_key_str);
    return iter != dataset.end() && bin_op(iter->second.ToString(), value);
  } else {
    const auto& attributes = holder.attributes();
    const auto iter = attributes.find(std::string(key));
    return iter != attributes.end() && bin_op(iter->second.ToString(), value);
  }
}

std::optional<std::string_view>
AttributeSelectorMatching::RemoveSelectorBrackets(
    const std::string_view& selector) {
  if (selector.front() != '[' || selector.back() != ']') {
    return std::nullopt;
  }
  return selector.substr(1, selector.size() - 2);
}

std::tuple<std::string_view, AttributeSelectorMatching::BinOp, std::string_view>
AttributeSelectorMatching::GetSelectorAttrKeyOpValue(
    const std::string_view& selector) {
  auto equal_sign_pos = selector.find('=');
  if (equal_sign_pos == selector.npos) {
    BinOp op = [](std::string_view, std::string_view) { return true; };
    return {selector, op, std::string_view()};
  }
  if (equal_sign_pos == 0) {
    return {std::string_view(), EQ, selector};
  }

  bool exact_match;
  BinOp op;
  switch (selector[equal_sign_pos - 1]) {
    case '*': {
      exact_match = false;
      op = CONTAINS;
      break;
    }
    case '^': {
      exact_match = false;
      op = base::BeginsWith;
      break;
    }
    case '$': {
      exact_match = false;
      op = base::EndsWith;
      break;
    }
    default: {
      exact_match = true;
      op = EQ;
      break;
    }
  }

  std::string_view key = selector.substr(0, equal_sign_pos - (!exact_match));
  std::string_view value =
      selector.substr(equal_sign_pos + 1, selector.size() - equal_sign_pos - 1);

  return {std::move(key), op, std::move(value)};
}
}  // namespace tasm
}  // namespace lynx
