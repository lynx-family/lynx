// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_EXTRA_DATA_H_
#define CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_EXTRA_DATA_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace lynx {
namespace css {

class LynxCSSSelectorList;

struct LynxCSSSelectorExtraData {
  enum class AttributeMatchType : int {
    kCaseSensitive,
    kCaseInsensitive,
    kCaseSensitiveAlways,
  };

  enum class MatchType : uint8_t {
    kUnknown,
    kNth,
    kAttr,
    kHas,
  };

  explicit LynxCSSSelectorExtraData(const std::string& value);
  ~LynxCSSSelectorExtraData();

  bool MatchNth(unsigned count) const;
  int NthAValue() const { return bits_.nth_.a_; }
  int NthBValue() const { return bits_.nth_.b_; }

  std::string value_;
  MatchType match_type_;
  union {
    struct {
      int a_;  // Used for :nth-*
      int b_;  // Used for :nth-*
    } nth_;

    struct {
      AttributeMatchType
          attribute_match_;  // used for attribute selector (with value)
      bool is_case_sensitive_attribute_;
    } attr_;

    struct {
      bool contains_pseudo_;

      bool contains_complex_logical_combinations_;
    } has_;
  } bits_;

  std::string attribute_;
  std::string argument_;
  std::unique_ptr<LynxCSSSelectorList> selector_list_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_EXTRA_DATA_H_
