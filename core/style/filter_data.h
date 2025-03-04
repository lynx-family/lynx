// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_FILTER_DATA_H_
#define CORE_STYLE_FILTER_DATA_H_

#include <tuple>

#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {
struct FilterData {
  static constexpr int kIndexType = 0;
  static constexpr int kIndexAmount = 1;
  static constexpr int kIndexUnit = 2;
  FilterType type;
  NLength amount;

  FilterData();
  ~FilterData() = default;

  bool operator==(const FilterData& rhs) const {
    return std::tie(type, amount) == std::tie(rhs.type, rhs.amount);
  };

  bool operator!=(const FilterData& rhs) const { return !(*this == rhs); }

  void Reset();
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_FILTER_DATA_H_
