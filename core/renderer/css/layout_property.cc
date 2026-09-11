// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/layout_property.h"

#include <array>

namespace lynx {
namespace tasm {

ConsumptionStatus LayoutProperty::ConsumptionTest(CSSPropertyID id) {
  static constexpr auto kWantedProperty = [] {
    std::array<ConsumptionStatus, kPropertyEnd> arr{};
    for (auto& status : arr) {
      status = ConsumptionStatus::SKIP;
    }

#define DECLARE_WANTED_PROPERTY(name, type) arr[kPropertyID##name] = type;
    FOREACH_LAYOUT_PROPERTY(DECLARE_WANTED_PROPERTY)
#undef DECLARE_WANTED_PROPERTY

    return arr;
  }();

  return kWantedProperty[id];
}

}  // namespace tasm
}  // namespace lynx
