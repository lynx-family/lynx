// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/auto_font_size_line_ranges_handler.h"

#include <limits>

#include "base/include/value/array.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(XAutoFontSizeLineRangesHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDXAutoFontSizeLineRanges;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  EXPECT_EQ(output[id].GetArray()->size(), 0u);

  output.clear();
  impl = lepus::Value(
      "line-range(1, 18px, 22px), line-range(2 to 3, 16px, 18px), "
      "line-range(4 to infinity, 14px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(output[id].IsArray());
  auto ranges = output[id].GetArray();
  EXPECT_EQ(ranges->size(), 3u);

  auto r0 = ranges->get(0).Array();
  ASSERT_TRUE(r0);
  EXPECT_EQ(r0->size(), 6u);
  EXPECT_EQ(r0->get(0).Int32(), 1);
  EXPECT_EQ(r0->get(1).Int32(), 1);
  EXPECT_FLOAT_EQ(r0->get(2).Number(), 18.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(r0->get(3).Number()),
            CSSValuePattern::PX);
  EXPECT_FLOAT_EQ(r0->get(4).Number(), 22.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(r0->get(5).Number()),
            CSSValuePattern::PX);

  auto r1 = ranges->get(1).Array();
  ASSERT_TRUE(r1);
  EXPECT_EQ(r1->size(), 6u);
  EXPECT_EQ(r1->get(0).Int32(), 2);
  EXPECT_EQ(r1->get(1).Int32(), 3);
  EXPECT_FLOAT_EQ(r1->get(2).Number(), 16.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(r1->get(3).Number()),
            CSSValuePattern::PX);
  EXPECT_FLOAT_EQ(r1->get(4).Number(), 18.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(r1->get(5).Number()),
            CSSValuePattern::PX);

  auto r2 = ranges->get(2).Array();
  ASSERT_TRUE(r2);
  EXPECT_EQ(r2->size(), 6u);
  EXPECT_EQ(r2->get(0).Int32(), 4);
  EXPECT_EQ(r2->get(1).Int32(), std::numeric_limits<int>::max());
  EXPECT_FLOAT_EQ(r2->get(2).Number(), 14.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(r2->get(3).Number()),
            CSSValuePattern::PX);
  EXPECT_FLOAT_EQ(r2->get(4).Number(), 14.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(r2->get(5).Number()),
            CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("line-range(0, 12px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("line-range(2 to 1, 12px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("line-range(1, px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("line-range(3+, 14px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("line-range(2-3, 14px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("line_range(1, 12px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
