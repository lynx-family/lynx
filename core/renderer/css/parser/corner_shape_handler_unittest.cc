// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore squircle superellipse squrcle infinitypx

#include <array>
#include <cmath>
#include <limits>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
namespace {

constexpr std::array<CSSPropertyID, 4> kCorners = {
    kPropertyIDCornerTopLeftShape, kPropertyIDCornerTopRightShape,
    kPropertyIDCornerBottomRightShape, kPropertyIDCornerBottomLeftShape};
constexpr double kInfinity = std::numeric_limits<double>::infinity();

struct ShapeCase {
  const char* input;
  double k;
};

const ShapeCase kShapes[] = {
    {"round", 1},
    {"squircle", 2},
    {"square", kInfinity},
    {"bevel", 0},
    {"scoop", -1},
    {"notch", -kInfinity},
    {"superellipse(0)", 0},
    {"superellipse(1)", 1},
    {"superellipse(2)", 2},
    {"superellipse(-1)", -1},
    {"superellipse(3)", 3},
    {"superellipse(+3)", 3},
    {"superellipse(0.25)", 0.25},
    {"superellipse(.5)", 0.5},
    {"superellipse(+.5)", 0.5},
    {"superellipse(-.75)", -0.75},
    {"superellipse(+1.25)", 1.25},
    {"superellipse(-1.25)", -1.25},
    {"superellipse(1e2)", 100},
    {"superellipse(1E2)", 100},
    {"superellipse(1e+2)", 100},
    {"superellipse(1E+2)", 100},
    {"superellipse(1e-2)", 0.01},
    {"superellipse(1E-2)", 0.01},
    {"superellipse(-2.5e+1)", -25},
    {"superellipse(+.5E-1)", 0.05},
    {"superellipse(infinity)", kInfinity},
    {"superellipse(-infinity)", -kInfinity},
    {" \tRoUnD\r\n\f", 1},
    {"SQUIRCLE", 2},
    {"Square", kInfinity},
    {"BEVEL", 0},
    {"Scoop", -1},
    {"NoTcH", -kInfinity},
    {" \tSuPeReLlIpSe( \n+.5E+1\r\f) \t", 5},
    {"SUPERELLIPSE( INFINITY )", kInfinity},
    {"Superellipse( -InFiNiTy )", -kInfinity},
};

StyleMap ExistingStyles() {
  StyleMap output;
  output[kPropertyIDWidth] = CSSValue(17, CSSValuePattern::PX);
  output[kPropertyIDCornerShape] =
      CSSValue("old-shorthand", CSSValuePattern::STRING, CSSValueType::DEFAULT);
  for (size_t i = 0; i < kCorners.size(); ++i) {
    output[kCorners[i]] = CSSValue(10.0 + i, CSSValuePattern::NUMBER);
  }
  return output;
}

void ExpectNumber(const StyleMap& output, CSSPropertyID id, double expected) {
  auto it = output.find(id);
  ASSERT_NE(it, output.end());
  ASSERT_EQ(it->second.GetPattern(), CSSValuePattern::NUMBER);
  ASSERT_TRUE(it->second.GetValue().IsNumber());
  double actual = it->second.GetNumber();
  if (std::isinf(expected)) {
    EXPECT_TRUE(std::isinf(actual));
    EXPECT_EQ(std::signbit(actual), std::signbit(expected));
  } else {
    EXPECT_DOUBLE_EQ(actual, expected);
  }
}

void ExpectUnchanged(const StyleMap& output, const StyleMap& before) {
  ASSERT_EQ(output.size(), before.size());
  for (const auto& entry : before) {
    auto it = output.find(entry.first);
    ASSERT_NE(it, output.end());
    EXPECT_EQ(it->second, entry.second);
  }
}

void ExpectRejected(CSSPropertyID id, const lepus::Value& input) {
  SCOPED_TRACE(CSSProperty::GetPropertyNameCStr(id));
  CSSParserConfigs configs;
  StyleMap output;
  EXPECT_FALSE(UnitHandler::Process(id, input, output, configs));
  EXPECT_TRUE(output.empty());

  output = ExistingStyles();
  const auto before = output;
  EXPECT_FALSE(UnitHandler::Process(id, input, output, configs));
  ExpectUnchanged(output, before);
}

void ExpectShorthand(const char* input, const std::array<double, 4>& expected,
                     StyleMap output = {}) {
  SCOPED_TRACE(input);
  CSSParserConfigs configs;
  const bool has_width = output.find(kPropertyIDWidth) != output.end();
  ASSERT_TRUE(UnitHandler::Process(kPropertyIDCornerShape, lepus::Value(input),
                                   output, configs));
  EXPECT_EQ(output.find(kPropertyIDCornerShape), output.end());
  EXPECT_EQ(output.size(), has_width ? 5u : 4u);
  for (size_t i = 0; i < kCorners.size(); ++i) {
    ExpectNumber(output, kCorners[i], expected[i]);
  }
  if (has_width) {
    auto it = output.find(kPropertyIDWidth);
    ASSERT_NE(it, output.end());
    EXPECT_EQ(it->second, CSSValue(17, CSSValuePattern::PX));
  }
}

}  // namespace

TEST(CornerShapeHandler, AllLonghandsKeywordsAndFunctions) {
  CSSParserConfigs configs;
  for (auto id : kCorners) {
    SCOPED_TRACE(CSSProperty::GetPropertyNameCStr(id));
    for (const auto& shape : kShapes) {
      SCOPED_TRACE(shape.input);
      StyleMap output;
      EXPECT_TRUE(
          UnitHandler::Process(id, lepus::Value(shape.input), output, configs));
      EXPECT_EQ(output.size(), 1u);
      ExpectNumber(output, id, shape.k);
    }
  }
}

TEST(CornerShapeHandler, ShorthandKeywordsAndFunctions) {
  for (const auto& shape : kShapes) {
    ExpectShorthand(shape.input, {shape.k, shape.k, shape.k, shape.k});
  }
}

TEST(CornerShapeHandler, ShorthandOneToFourValues) {
  const struct {
    const char* input;
    std::array<double, 4> expected;
  } cases[] = {
      {"round", {1, 1, 1, 1}},
      {"round squircle", {1, 2, 1, 2}},
      {"round squircle bevel", {1, 2, 0, 2}},
      {"round squircle bevel scoop", {1, 2, 0, -1}},
      {"superellipse(.5) notch", {0.5, -kInfinity, 0.5, -kInfinity}},
      {"square superellipse(-.75) scoop", {kInfinity, -0.75, -1, -0.75}},
      {" \tRoUnD\nSuPeReLlIpSe( +.5E+1 )\r\nSQUARE\fNoTcH ",
       {1, 5, kInfinity, -kInfinity}},
      {"superellipse(1) superellipse(2) superellipse(3) superellipse(4)",
       {1, 2, 3, 4}},
  };
  for (const auto& entry : cases) {
    ExpectShorthand(entry.input, entry.expected);
    // Success replaces all corners, erases the shorthand, and preserves width.
    ExpectShorthand(entry.input, entry.expected, ExistingStyles());
  }
}

TEST(CornerShapeHandler, ShorthandAdjacentFunctions) {
  ExpectShorthand("superellipse(1)round", {1, 1, 1, 1});
  ExpectShorthand("superellipse(1)superellipse(2)", {1, 2, 1, 2});
  ExpectShorthand("superellipse(1)superellipse(2)superellipse(3)scoop",
                  {1, 2, 3, -1});
}

TEST(CornerShapeHandler, LonghandOnlyReplacesItsOwnValue) {
  CSSParserConfigs configs;
  for (auto id : kCorners) {
    SCOPED_TRACE(CSSProperty::GetPropertyNameCStr(id));
    auto output = ExistingStyles();
    auto expected = output;
    expected[id] = CSSValue(0.5, CSSValuePattern::NUMBER);
    EXPECT_TRUE(UnitHandler::Process(id, lepus::Value("superellipse(.5)"),
                                     output, configs));
    ExpectUnchanged(output, expected);
  }
}

TEST(CornerShapeHandler, InvalidShapesAreTransactional) {
  const char* invalid[] = {
      "",
      " \t\n\r\f",
      "auto",
      "-x-smooth",
      "squrcle",
      "smooth(60)",
      "unknown",
      "roundish",
      "0",
      "1",
      "-1",
      ".5",
      "infinity",
      "-infinity",
      "superellipse",
      "superellipse()",
      "superellipse( )",
      "superellipse(1",
      "superellipse(1))",
      "superellipse((1))",
      "superellipse (1)",
      "superellipse(1 2)",
      "superellipse(1,2)",
      "superellipse(1,)",
      "superellipse(1px)",
      "superellipse(1%)",
      "superellipse(1deg)",
      "superellipse(1em)",
      "superellipse(1s)",
      "superellipse(NaN)",
      "superellipse(inf)",
      "superellipse(+infinity)",
      "superellipse(--infinity)",
      "superellipse(infinitypx)",
      "superellipse(round)",
      "superellipse(calc(1 + 2))",
      "superellipse(0x10)",
      "superellipse(.)",
      "superellipse(1.)",
      "superellipse(1.e2)",
      "superellipse(1e9999)",
      "superellipse(-1e9999)",
      "superellipse(+)",
      "superellipse(-)",
      "superellipse(--1)",
      "superellipse(+-1)",
      "superellipse(+ 1)",
      "superellipse(1.2.3)",
      "superellipse(.5.6)",
      "superellipse(+.5.6)",
      "superellipse(1e)",
      "superellipse(1E)",
      "superellipse(1e+)",
      "superellipse(1E-)",
      "superellipse(1e 2)",
      "superellipse(1e++2)",
      "superellipse(1)junk",
      "round;",
      "round!",
      "round !important",
      "round, squircle",
      "round / squircle",
      "'round'",
      "\"round\"",
      "super-ellipse(2)",
  };
  for (const auto* input : invalid) {
    SCOPED_TRACE(input);
    ExpectRejected(kPropertyIDCornerShape, lepus::Value(input));
    for (auto id : kCorners) {
      ExpectRejected(id, lepus::Value(input));
    }
  }
}

TEST(CornerShapeHandler, LonghandsRejectMultipleShapes) {
  const char* invalid[] = {"round squircle",
                           "round squircle bevel",
                           "round squircle bevel scoop",
                           "superellipse(1) superellipse(2)",
                           "superellipse(1)round",
                           "superellipse(1)superellipse(2)"};
  for (auto id : kCorners) {
    for (const auto* input : invalid) {
      SCOPED_TRACE(input);
      ExpectRejected(id, lepus::Value(input));
    }
  }
}

TEST(CornerShapeHandler, ShorthandFailureAfterValidPrefixIsTransactional) {
  const char* invalid[] = {
      "round invalid",
      "round squircle invalid",
      "round squircle bevel invalid",
      "round squircle bevel scoop notch",
      "round squircle bevel scoop superellipse(2)",
      "round squircle bevel scoop invalid",
      "round superellipse(1px)",
      "round squircle superellipse(1 2)",
      "round squircle bevel superellipse(1",
      "round squircle bevel scoop;",
      "round auto",
      "round -x-smooth",
  };
  for (const auto* input : invalid) {
    SCOPED_TRACE(input);
    ExpectRejected(kPropertyIDCornerShape, lepus::Value(input));
  }
}

TEST(CornerShapeHandler, EmbeddedNullIsTransactional) {
  for (const auto& input : {std::string("round\0scoop", 11),
                            std::string("superellipse(1)\0round", 21),
                            std::string("superellipse(1\0)", 16)}) {
    ExpectRejected(kPropertyIDCornerShape, lepus::Value(input));
    for (auto id : kCorners) {
      ExpectRejected(id, lepus::Value(input));
    }
  }
}

TEST(CornerShapeHandler, NonStringInputIsTransactional) {
  const lepus::Value invalid[] = {
      lepus::Value(),
      lepus::Value(true),
      lepus::Value(false),
      lepus::Value(1),
      lepus::Value(0.5),
      lepus::Value(kInfinity),
      lepus::Value(lepus::CArray::Create()),
      lepus::Value(lepus::Dictionary::Create()),
  };
  for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
    SCOPED_TRACE(i);
    ExpectRejected(kPropertyIDCornerShape, invalid[i]);
    for (auto id : kCorners) {
      ExpectRejected(id, invalid[i]);
    }
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
