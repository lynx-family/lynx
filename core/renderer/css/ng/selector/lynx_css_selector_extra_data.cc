// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/selector/lynx_css_selector_extra_data.h"

#include <limits>

#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {
namespace css {

LynxCSSSelectorExtraData::LynxCSSSelectorExtraData(const std::string& value)
    : value_(value),
      match_type_(LynxCSSSelectorExtraData::MatchType::kUnknown),
      bits_(),
      attribute_(CSSGlobalStarString()),
      argument_(CSSGlobalEmptyString()) {}

LynxCSSSelectorExtraData::~LynxCSSSelectorExtraData() = default;

bool LynxCSSSelectorExtraData::MatchNth(unsigned unsigned_count) const {
  int max_value = std::numeric_limits<int>::max() / 2;
  int min_value = std::numeric_limits<int>::min() / 2;
  if (UNLIKELY(unsigned_count > static_cast<unsigned>(max_value) ||
               NthAValue() > max_value || NthAValue() < min_value ||
               NthBValue() > max_value || NthBValue() < min_value))
    return false;

  int count = static_cast<int>(unsigned_count);
  if (!NthAValue()) return count == NthBValue();
  if (NthAValue() > 0) {
    if (count < NthBValue()) return false;
    return (count - NthBValue()) % NthAValue() == 0;
  }
  if (count > NthBValue()) return false;
  return (NthBValue() - count) % (-NthAValue()) == 0;
}

}  // namespace css
}  // namespace lynx
