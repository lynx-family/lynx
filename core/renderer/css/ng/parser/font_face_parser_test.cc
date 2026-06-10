// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/font_face_parser.h"

#include <unordered_map>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {
namespace {

using DescriptorMap = std::unordered_map<std::string, std::string>;

}  // namespace

TEST(FontFaceParserTest, ParseRequiredDescriptorsAndRanges) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "\"Bitstream Vera Serif Bold\""},
      {"src",
       "local(\"PingFang SC\"), url(\"https://example.com/font.woff2\") "
       "format(\"woff2\") tech(variations)"},
      {"font-weight", "100 900"},
      {"font-stretch", "75% 125%"},
      {"font-style", "oblique 10deg 20deg"},
      {"font-variation-settings", "\"wght\" 700, \"wdth\" 80.5"},
      {"unicode-range", "U+0025-00FF, U+4??"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->Family(), "Bitstream Vera Serif Bold");
  ASSERT_EQ(rule->Sources().size(), 2u);
  EXPECT_TRUE(rule->Sources()[0].is_local);
  EXPECT_EQ(rule->Sources()[0].uri, "PingFang SC");
  EXPECT_FALSE(rule->Sources()[1].is_local);
  EXPECT_EQ(rule->Sources()[1].uri, "https://example.com/font.woff2");
  EXPECT_EQ(rule->WeightMin(), 100);
  EXPECT_EQ(rule->WeightMax(), 900);
  EXPECT_EQ(rule->StretchMin(), 75);
  EXPECT_EQ(rule->StretchMax(), 125);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kOblique);
  EXPECT_EQ(rule->ObliqueAngleMin(), 1000);
  EXPECT_EQ(rule->ObliqueAngleMax(), 2000);
  ASSERT_EQ(rule->VariationSettings().size(), 2u);
  EXPECT_EQ(rule->VariationSettings()[0].tag, "wght");
  EXPECT_FLOAT_EQ(rule->VariationSettings()[0].value, 700.0f);
  EXPECT_EQ(rule->VariationSettings()[1].tag, "wdth");
  EXPECT_FLOAT_EQ(rule->VariationSettings()[1].value, 80.5f);
  ASSERT_EQ(rule->GetUnicodeRange().size(), 2u);
  EXPECT_EQ(rule->GetUnicodeRange()[0].from, 0x0025u);
  EXPECT_EQ(rule->GetUnicodeRange()[0].to, 0x00FFu);
  EXPECT_EQ(rule->GetUnicodeRange()[1].from, 0x0400u);
  EXPECT_EQ(rule->GetUnicodeRange()[1].to, 0x04FFu);
}

TEST(FontFaceParserTest, ParseDefaults) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "My Font"},
      {"src", "url(https://example.com/font.ttf)"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->Family(), "My Font");
  ASSERT_EQ(rule->Sources().size(), 1u);
  EXPECT_FALSE(rule->Sources()[0].is_local);
  EXPECT_EQ(rule->Sources()[0].uri, "https://example.com/font.ttf");
  EXPECT_EQ(rule->WeightMin(), 400);
  EXPECT_EQ(rule->WeightMax(), 400);
  EXPECT_EQ(rule->StretchMin(), 100);
  EXPECT_EQ(rule->StretchMax(), 100);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kNormal);
  EXPECT_EQ(rule->ObliqueAngleMin(), 0);
  EXPECT_EQ(rule->ObliqueAngleMax(), 0);
  EXPECT_TRUE(rule->VariationSettings().empty());
  EXPECT_TRUE(rule->GetUnicodeRange().empty());
}

TEST(FontFaceParserTest, ParseSrcHintsSeparatedByWhitespace) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "HintedFont"},
      {"src",
       "url(\"https://example.com/font.woff2\") format(\"woff2\") "
       "tech(variations)"},
  });

  ASSERT_NE(rule, nullptr);
  ASSERT_EQ(rule->Sources().size(), 1u);
  EXPECT_FALSE(rule->Sources()[0].is_local);
  EXPECT_EQ(rule->Sources()[0].uri, "https://example.com/font.woff2");
}

TEST(FontFaceParserTest, ParseWeightRangeNormalizesEndpoints) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "WeightFont"},
      {"src", "url(font.woff2)"},
      {"font-weight", "900 100"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->WeightMin(), 100);
  EXPECT_EQ(rule->WeightMax(), 900);
}

TEST(FontFaceParserTest, ParseWeightRangeKeywordEndpoints) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "WeightFont"},
      {"src", "url(font.woff2)"},
      {"font-weight", "bold normal"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->WeightMin(), 400);
  EXPECT_EQ(rule->WeightMax(), 700);
}

TEST(FontFaceParserTest, ParseFontStyleItalic) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "StyleFont"},
      {"src", "url(font.woff2)"},
      {"font-style", "italic"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kItalic);
  EXPECT_EQ(rule->ObliqueAngleMin(), 0);
  EXPECT_EQ(rule->ObliqueAngleMax(), 0);
}

TEST(FontFaceParserTest, ParseFontStyleObliqueDefaultAngle) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "StyleFont"},
      {"src", "url(font.woff2)"},
      {"font-style", "oblique"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kOblique);
  EXPECT_EQ(rule->ObliqueAngleMin(), 1400);  // 14.00°
  EXPECT_EQ(rule->ObliqueAngleMax(), 1400);  // 14.00°
}

TEST(FontFaceParserTest, ParseFontStyleObliqueConvertsAngleUnits) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "StyleFont"},
      {"src", "url(font.woff2)"},
      {"font-style", "oblique -0.25turn 1.5707963267948966rad"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kOblique);
  EXPECT_EQ(rule->ObliqueAngleMin(), -9000);
  EXPECT_EQ(rule->ObliqueAngleMax(), 9000);
}

TEST(FontFaceParserTest, RejectsMissingFamilyOrSrc) {
  EXPECT_EQ(FontFaceParser::Parse(DescriptorMap{{"font-family", "MyFont"}}),
            nullptr);
  EXPECT_EQ(FontFaceParser::Parse(DescriptorMap{{"src", "url(font.woff2)"}}),
            nullptr);
}

TEST(FontFaceParserTest, RejectsInvalidSrcList) {
  EXPECT_EQ(FontFaceParser::Parse(DescriptorMap{
                {"font-family", "MyFont"},
                {"src", "url()"},
            }),
            nullptr);
  EXPECT_EQ(FontFaceParser::Parse(DescriptorMap{
                {"font-family", "MyFont"},
                {"src", "local(\"A\"), invalid(\"font.woff2\")"},
            }),
            nullptr);
}

TEST(FontFaceParserTest, InvalidOptionalDescriptorsUseDefaults) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "MyFont"},
      {"src", "url(font.woff2)"},
      {"font-weight", "100 200 300"},
      {"font-stretch", "bogus"},
      {"font-style", "italic oblique"},
      {"font-variation-settings", "\"bad\""},
      {"unicode-range", "not-a-range"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->WeightMin(), 400);
  EXPECT_EQ(rule->WeightMax(), 400);
  EXPECT_EQ(rule->StretchMin(), 100);
  EXPECT_EQ(rule->StretchMax(), 100);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kNormal);
  EXPECT_TRUE(rule->VariationSettings().empty());
  EXPECT_TRUE(rule->GetUnicodeRange().empty());
}

TEST(FontFaceParserTest, SubPercentStretchUsesDefault) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "MyFont"},
      {"src", "url(font.woff2)"},
      {"font-stretch", "0.4%"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->StretchMin(), 100);
  EXPECT_EQ(rule->StretchMax(), 100);
}

TEST(FontFaceParserTest, InvalidFontStyleObliqueAngleUsesDefault) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "MyFont"},
      {"src", "url(font.woff2)"},
      {"font-style", "oblique 91deg"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kNormal);
}

TEST(FontFaceParserTest, ExtremeFontStyleObliqueAngleUsesDefault) {
  auto rule = FontFaceParser::Parse(DescriptorMap{
      {"font-family", "MyFont"},
      {"src", "url(font.woff2)"},
      {"font-style", "oblique 1e100turn"},
  });

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->StyleKind(), FontFaceStyleKind::kNormal);
}

}  // namespace css
}  // namespace lynx
