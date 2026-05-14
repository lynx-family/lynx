// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/mask_composite_handler.h"

#include <array>

#include "base/include/value/array.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(MaskCompositeHandler, Valid) {
  constexpr auto id = CSSPropertyID::kPropertyIDMaskComposite;
  StyleMap output;
  CSSParserConfigs configs;

  EXPECT_TRUE(UnitHandler::Process(
      id, lepus::Value("add, subtract, intersect, exclude"), output, configs));
  ASSERT_TRUE(output.find(id) != output.end());

  auto composite = output[id];
  ASSERT_TRUE(composite.IsArray());
  auto array = composite.GetArray();
  ASSERT_EQ(array->size(), static_cast<size_t>(4));
  EXPECT_EQ(array->get(0).Number(),
            static_cast<int>(starlight::MaskCompositeType::kAdd));
  EXPECT_EQ(array->get(1).Number(),
            static_cast<int>(starlight::MaskCompositeType::kSubtract));
  EXPECT_EQ(array->get(2).Number(),
            static_cast<int>(starlight::MaskCompositeType::kIntersect));
  EXPECT_EQ(array->get(3).Number(),
            static_cast<int>(starlight::MaskCompositeType::kExclude));
}

TEST(MaskCompositeHandler, Invalid) {
  constexpr auto id = CSSPropertyID::kPropertyIDMaskComposite;
  CSSParserConfigs configs;
  for (const auto* value : {"", "none", "xor", "add subtract", "add,"}) {
    StyleMap output;
    EXPECT_FALSE(
        UnitHandler::Process(id, lepus::Value(value), output, configs));
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
