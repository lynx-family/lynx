// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/layout_property.h"

namespace lynx {
namespace tasm {

ConsumptionStatus LayoutProperty::ConsumptionTest(CSSPropertyID id) {
  static const auto& kWantedProperty = []() -> const int(&)[kPropertyEnd] {
    static int arr[kPropertyEnd];
    std::fill(std::begin(arr), std::end(arr), ConsumptionStatus::SKIP);

#define DECLARE_WANTED_PROPERTY(name, type) arr[kPropertyID##name] = type;
    FOREACH_LAYOUT_PROPERTY(DECLARE_WANTED_PROPERTY)
#undef DECLARE_WANTED_PROPERTY

    return arr;
  }();

  return static_cast<ConsumptionStatus>(kWantedProperty[id]);
}

}  // namespace tasm
}  // namespace lynx
