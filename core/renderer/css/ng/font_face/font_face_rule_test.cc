// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/font_face/font_face_rule.h"

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

TEST(FontFaceRuleTest, RoundTripBasic) {
  auto rule = fml::AdoptRef(new FontFaceRule(
      "MyFont",
      {{false, "https://cdn.com/font.woff2"}, {true, "PingFangSC-Regular"}},
      100, 900, 75, 125, {FontFaceStyleKind::kOblique, 1000, 2000},
      {{"wght", 700.0f}, {"wdth", 80.5f}},
      {{0x0000, 0x00FF}, {0x4E00, 0x9FFF}}));

  auto lepus = rule->ToLepus();
  auto restored = FontFaceRule::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);

  EXPECT_EQ(restored->Family(), "MyFont");
  ASSERT_EQ(restored->Sources().size(), 2u);
  EXPECT_FALSE(restored->Sources()[0].is_local);
  EXPECT_EQ(restored->Sources()[0].uri, "https://cdn.com/font.woff2");
  EXPECT_TRUE(restored->Sources()[1].is_local);
  EXPECT_EQ(restored->Sources()[1].uri, "PingFangSC-Regular");
  EXPECT_EQ(restored->WeightMin(), 100);
  EXPECT_EQ(restored->WeightMax(), 900);
  EXPECT_EQ(restored->StretchMin(), 75);
  EXPECT_EQ(restored->StretchMax(), 125);
  EXPECT_EQ(restored->StyleKind(), FontFaceStyleKind::kOblique);
  EXPECT_EQ(restored->ObliqueAngleMin(), 1000);
  EXPECT_EQ(restored->ObliqueAngleMax(), 2000);
  ASSERT_EQ(restored->VariationSettings().size(), 2u);
  EXPECT_EQ(restored->VariationSettings()[0].tag, "wght");
  EXPECT_FLOAT_EQ(restored->VariationSettings()[0].value, 700.0f);
  EXPECT_EQ(restored->VariationSettings()[1].tag, "wdth");
  EXPECT_FLOAT_EQ(restored->VariationSettings()[1].value, 80.5f);
  ASSERT_EQ(restored->GetUnicodeRange().size(), 2u);
  EXPECT_EQ(restored->GetUnicodeRange()[0].from, 0x0000u);
  EXPECT_EQ(restored->GetUnicodeRange()[0].to, 0x00FFu);
  EXPECT_EQ(restored->GetUnicodeRange()[1].from, 0x4E00u);
  EXPECT_EQ(restored->GetUnicodeRange()[1].to, 0x9FFFu);
}

TEST(FontFaceRuleTest, RoundTripDefaults) {
  auto rule = fml::AdoptRef(
      new FontFaceRule("Default", {{false, "https://example.com/font.ttf"}},
                       400, 400, 100, 100, {}, {}, {}));

  auto restored = FontFaceRule::FromLepus(rule->ToLepus());
  ASSERT_NE(restored, nullptr);

  EXPECT_EQ(restored->Family(), "Default");
  EXPECT_EQ(restored->WeightMin(), 400);
  EXPECT_EQ(restored->WeightMax(), 400);
  EXPECT_EQ(restored->StretchMin(), 100);
  EXPECT_EQ(restored->StretchMax(), 100);
  EXPECT_EQ(restored->StyleKind(), FontFaceStyleKind::kNormal);
  EXPECT_EQ(restored->ObliqueAngleMin(), 0);
  EXPECT_EQ(restored->ObliqueAngleMax(), 0);
  EXPECT_TRUE(restored->VariationSettings().empty());
  EXPECT_TRUE(restored->GetUnicodeRange().empty());
}

TEST(FontFaceRuleTest, FromLepusInvalidInput) {
  EXPECT_EQ(FontFaceRule::FromLepus(lepus_value(false)), nullptr);
  EXPECT_EQ(FontFaceRule::FromLepus(lepus_value(lepus::CArray::Create())),
            nullptr);

  auto short_arr = lepus::CArray::Create();
  for (int i = 0; i < 6; ++i) short_arr->emplace_back(0u);
  EXPECT_EQ(FontFaceRule::FromLepus(lepus_value(std::move(short_arr))),
            nullptr);
}

TEST(FontFaceRuleTest, WeightPackingBoundary) {
  auto rule = fml::AdoptRef(
      new FontFaceRule("W", {{false, "x"}}, 1, 1000, 100, 100, {}, {}, {}));

  auto restored = FontFaceRule::FromLepus(rule->ToLepus());
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->WeightMin(), 1);
  EXPECT_EQ(restored->WeightMax(), 1000);
}

TEST(FontFaceRuleTest, StretchPackingBoundary) {
  auto rule = fml::AdoptRef(
      new FontFaceRule("S", {{false, "x"}}, 400, 400, 50, 200, {}, {}, {}));

  auto restored = FontFaceRule::FromLepus(rule->ToLepus());
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->StretchMin(), 50);
  EXPECT_EQ(restored->StretchMax(), 200);
}

TEST(FontFaceRuleTest, EmptySources) {
  auto rule = fml::AdoptRef(
      new FontFaceRule("NoSrc", {}, 400, 400, 100, 100, {}, {}, {}));

  auto restored = FontFaceRule::FromLepus(rule->ToLepus());
  ASSERT_NE(restored, nullptr);
  EXPECT_TRUE(restored->Sources().empty());
}

}  // namespace css
}  // namespace lynx
