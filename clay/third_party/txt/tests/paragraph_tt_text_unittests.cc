// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_builder_tt_text.h"
#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#include "clay/third_party/txt/src/tttext/tttext_index_mapper.h"

#if defined(ENABLE_SKITY)
#include <cmath>
#include <optional>
#include <utility>
#endif

#include <textra/layout_region.h>
#include <textra/paragraph.h>
#include <textra/style.h>
#if defined(ENABLE_SKITY)
#include "clay/fml/paths.h"
#include "clay/testing/testing.h"
#include "clay/third_party/txt/src/txt/asset_font_manager_skity.h"
#include "clay/third_party/txt/src/txt/typeface_font_asset_provider_skity.h"
#endif
#include "clay/third_party/txt/src/txt/placeholder_run.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace txt {
#if defined(ENABLE_SKITY)
namespace {

constexpr char kFixtureFontFamily[] = "FixtureRoboto";

std::shared_ptr<FontCollection> CreateFixtureFontCollection() {
  const std::string font_path = fml::paths::JoinPaths(
      {clay::testing::GetFixturesPath(), "Roboto-Bold.ttf"});
  auto font_provider = std::make_unique<TypefaceFontAssetProvider>();
  auto typeface = skity::Typeface::MakeFromFile(font_path.c_str());
  if (typeface == nullptr) {
    return nullptr;
  }
  font_provider->RegisterTypeface(std::move(typeface), kFixtureFontFamily);
  auto font_manager =
      std::make_shared<AssetFontManager>(std::move(font_provider));
  auto font_collection = std::make_shared<FontCollection>();
  font_collection->SetDefaultFontManager(std::move(font_manager));
  return font_collection;
}

std::optional<tttext::FontInfo> GetFixtureFontInfo(
    const std::shared_ptr<FontCollection>& font_collection,
    const tttext::Style& style) {
  auto tt_font_collection = font_collection->GetIFontCollection();
  auto typefaces = tt_font_collection.findTypefaces(style.GetFontDescriptor());
  if (typefaces.empty() || typefaces.front() == nullptr) {
    return std::nullopt;
  }
  return typefaces.front()->GetFontInfo(style.GetTextSize());
}

}  // namespace
#endif

TEST(ParagraphTTTextTest, ConvertsBetweenTTTextAndUTF16Positions) {
  TTTextIndexMapper mapper;
  mapper.AppendText(std::u16string(u"A\U0001F600"));
  mapper.AppendText(std::u16string(u"B\U0001F601C"));

  EXPECT_EQ(mapper.GetUTF16Size(), 7u);

  EXPECT_EQ(mapper.ToTTTextPosition(0), 0u);
  EXPECT_EQ(mapper.ToTTTextPosition(1), 1u);
  EXPECT_EQ(mapper.ToTTTextPosition(2), 1u);
  EXPECT_EQ(mapper.ToTTTextPosition(3), 2u);
  EXPECT_EQ(mapper.ToTTTextPosition(4), 3u);
  EXPECT_EQ(mapper.ToTTTextPosition(5), 3u);
  EXPECT_EQ(mapper.ToTTTextPosition(6), 4u);
  EXPECT_EQ(mapper.ToTTTextPosition(7), 5u);
  EXPECT_EQ(mapper.ToTTTextPosition(99), 5u);

  EXPECT_EQ(mapper.ToTTTextRangeEnd(0), 0u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(1), 1u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(2), 2u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(3), 2u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(4), 3u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(5), 4u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(6), 4u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(7), 5u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(99), 5u);

  EXPECT_EQ(mapper.ToUTF16Position(0), 0u);
  EXPECT_EQ(mapper.ToUTF16Position(1), 1u);
  EXPECT_EQ(mapper.ToUTF16Position(2), 3u);
  EXPECT_EQ(mapper.ToUTF16Position(3), 4u);
  EXPECT_EQ(mapper.ToUTF16Position(4), 6u);
  EXPECT_EQ(mapper.ToUTF16Position(5), 7u);
  EXPECT_EQ(mapper.ToUTF16Position(99), 7u);
}

TEST(ParagraphTTTextTest, HandlesEmptyText) {
  TTTextIndexMapper mapper;

  EXPECT_EQ(mapper.GetUTF16Size(), 0u);
  EXPECT_EQ(mapper.ToTTTextPosition(1), 0u);
  EXPECT_EQ(mapper.ToTTTextRangeEnd(1), 0u);
  EXPECT_EQ(mapper.ToUTF16Position(1), 0u);
}

TEST(ParagraphTTTextTest, KeepsIndexesInSyncForEmbeddedNull) {
  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(nullptr, paragraph_style);
  tttext::Style style;

  paragraph.AddTextRun(style, std::u16string(u"A\0B", 3));
  paragraph.AddTextRun(style, std::string("A\0B", 3));

  EXPECT_EQ(paragraph.GetTextSize(), 2u);
  ASSERT_EQ(paragraph.paragraph_->GetCharCount(), 2u);
  EXPECT_EQ(paragraph.paragraph_->GetContentString(0, 2), "AA");
}

#if defined(ENABLE_SKITY)
TEST(ParagraphTTTextTest, MiddleUsesResolvedFontMetrics) {
  constexpr float kFontSize = 40.f;
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  tttext::Style style;
  tttext::FontDescriptor font_descriptor;
  font_descriptor.font_family_list_ = {kFixtureFontFamily};
  font_descriptor.font_style_ = tttext::FontStyle::Bold();
  style.SetFontDescriptor(font_descriptor);
  style.SetTextSize(kFontSize);

  const auto font_info = GetFixtureFontInfo(font_collection, style);
  ASSERT_TRUE(font_info.has_value());
  const float expected_center_from_baseline =
      (font_info->GetAscent() + font_info->GetDescent()) / 2.f;
  const float heuristic_center_from_baseline =
      (kFontSize * 0.25f - kFontSize * 0.75f) / 2.f;
  EXPECT_GT(
      std::abs(expected_center_from_baseline - heuristic_center_from_baseline),
      0.5f);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style);
  PlaceholderRun placeholder(20.f, 40.f, PlaceholderAlignment::kMiddle,
                             TextBaseline::kAlphabetic, 0.f);
  paragraph.AddPlaceholder(style, placeholder, false);
  paragraph.Layout(100.f);

  const auto boxes = paragraph.GetRectsForPlaceholders();
  ASSERT_EQ(boxes.size(), 1u);
  const auto& rect = boxes.front().rect;
  const float actual_center_from_baseline =
      (rect.Top() + rect.Bottom()) / 2.f - paragraph.GetAlphabeticBaseline();
  EXPECT_NEAR(actual_center_from_baseline, expected_center_from_baseline,
              0.01f);
}
#endif

TEST(ParagraphTTTextTest, DefaultPlaceholderUsesAlphabeticBaseline) {
  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(nullptr, paragraph_style);
  tttext::Style style;
  PlaceholderRun placeholder;
  placeholder.width = 10;
  placeholder.height = 1;

  EXPECT_EQ(placeholder.alignment, PlaceholderAlignment::kBaseline);
  EXPECT_EQ(placeholder.baseline, TextBaseline::kAlphabetic);
  paragraph.AddPlaceholder(style, placeholder, false);

  EXPECT_EQ(paragraph.GetTextSize(), 1u);
  EXPECT_EQ(paragraph.paragraph_->GetCharCount(), 1u);
}

}  // namespace txt

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
