// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_builder_tt_text.h"
#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"

#if defined(ENABLE_SKITY)
#include <cmath>
#include <limits>
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

std::shared_ptr<FontCollection> CreateFixtureFontCollection(
    const char* file_name = "Roboto-Bold.ttf",
    const char* family = kFixtureFontFamily) {
  const std::string font_path =
      fml::paths::JoinPaths({clay::testing::GetFixturesPath(), file_name});
  auto font_provider = std::make_unique<TypefaceFontAssetProvider>();
  auto typeface = skity::Typeface::MakeFromFile(font_path.c_str());
  if (typeface == nullptr) {
    return nullptr;
  }
  font_provider->RegisterTypeface(std::move(typeface), family);
  auto font_manager =
      std::make_shared<AssetFontManager>(std::move(font_provider));
  auto font_collection = std::make_shared<FontCollection>();
  font_collection->SetDefaultFontManager(std::move(font_manager));
  return font_collection;
}

}  // namespace
#endif

#if defined(ENABLE_SKITY)
class ParagraphTTTextReuseTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fonts_ = CreateFixtureFontCollection();
    ASSERT_NE(fonts_, nullptr);
    style_.font_family = kFixtureFontFamily;
    style_.font_weight = FontWeight::w700;
    style_.text_align = TextAlign::left;
  }

  std::unique_ptr<Paragraph> Build(const std::u16string& text) {
    ParagraphBuilderTTText builder(style_, fonts_);
    auto text_style = style_.GetTextStyle();
    text_style.word_break = word_break_;
    builder.PushStyle(text_style);
    builder.AddText(text);
    builder.Pop();
    return builder.Build();
  }

  static const char* Reject(Paragraph& paragraph, double width) {
    return static_cast<ParagraphTTText&>(paragraph)
        .GetSingleLineGeometryRejectionReason(width);
  }

  void ExpectShrinkingPreservesGeometry(const std::u16string& text) {
    for (double font_size : {12.0, 17.5, 32.0}) {
      SCOPED_TRACE(::testing::Message()
                   << "font_size=" << font_size << "text_size=" << text.size());
      style_.font_size = font_size;
      auto original = Build(text);
      original->Layout(1024);
      const double target = std::ceil(original->GetMaxIntrinsicWidth());
      ASSERT_GT(target, 0);
      ASSERT_LT(target, 1023);
      ASSERT_EQ(Reject(*original, target), nullptr);
      auto rebuilt = Build(text);
      rebuilt->Layout(target);

      EXPECT_DOUBLE_EQ(original->GetHeight(), rebuilt->GetHeight());
      EXPECT_DOUBLE_EQ(original->GetMaxIntrinsicWidth(),
                       rebuilt->GetMaxIntrinsicWidth());
      EXPECT_DOUBLE_EQ(original->GetAlphabeticBaseline(),
                       rebuilt->GetAlphabeticBaseline());
      EXPECT_DOUBLE_EQ(original->GetIdeographicBaseline(),
                       rebuilt->GetIdeographicBaseline());
      const auto& before = original->GetLineMetrics();
      const auto& after = rebuilt->GetLineMetrics();
      ASSERT_EQ(before.size(), 1u);
      ASSERT_EQ(after.size(), 1u);
      EXPECT_EQ(before[0].start_index, after[0].start_index);
      EXPECT_EQ(before[0].end_index, after[0].end_index);
      EXPECT_EQ(before[0].hard_break, after[0].hard_break);
      EXPECT_DOUBLE_EQ(before[0].width, after[0].width);
      EXPECT_DOUBLE_EQ(before[0].height, after[0].height);
      EXPECT_DOUBLE_EQ(before[0].left, after[0].left);
      EXPECT_DOUBLE_EQ(before[0].baseline, after[0].baseline);
      EXPECT_DOUBLE_EQ(before[0].ascent, after[0].ascent);
      EXPECT_DOUBLE_EQ(before[0].descent, after[0].descent);
      for (size_t i = 0; i < text.size(); ++i) {
        const auto a = original->GetRectsForRange(
            i, i + 1, Paragraph::RectHeightStyle::kTight,
            Paragraph::RectWidthStyle::kTight);
        const auto b = rebuilt->GetRectsForRange(
            i, i + 1, Paragraph::RectHeightStyle::kTight,
            Paragraph::RectWidthStyle::kTight);
        ASSERT_EQ(a.size(), b.size());
        for (size_t j = 0; j < a.size(); ++j) {
          EXPECT_EQ(a[j].direction, b[j].direction);
          EXPECT_FLOAT_EQ(a[j].rect.Left(), b[j].rect.Left());
          EXPECT_FLOAT_EQ(a[j].rect.Right(), b[j].rect.Right());
          EXPECT_FLOAT_EQ(a[j].rect.Top(), b[j].rect.Top());
          EXPECT_FLOAT_EQ(a[j].rect.Bottom(), b[j].rect.Bottom());
        }
      }
      for (double x = -1; x <= target + 1; x += 0.5) {
        const auto a = original->GetGlyphPositionAtCoordinate(x, 0);
        const auto b = rebuilt->GetGlyphPositionAtCoordinate(x, 0);
        EXPECT_EQ(a.position, b.position);
        EXPECT_EQ(a.affinity, b.affinity);
      }
    }
  }

  std::shared_ptr<FontCollection> fonts_;
  ParagraphStyle style_;
  WordBreak word_break_ = WordBreak::kNormal;
};

TEST_F(ParagraphTTTextReuseTest, ShrinkingPreservesRealLayoutGeometry) {
  // Compare independent real layouts with a fixture font, not mocked metrics.
  for (const std::u16string text :
       {u"Text", u"123456", u"Hello world", u"office", u"Cafe\u0301"}) {
    ExpectShrinkingPreservesGeometry(text);
  }
}

TEST_F(ParagraphTTTextReuseTest, ShrinkingPreservesRealCjkGeometry) {
  constexpr char kCjkFixture[] = "NotoSansCJK-VF-subset.otf.ttc";
  constexpr char kCjkFamily[] = "FixtureCJK";
  const auto path =
      fml::paths::JoinPaths({clay::testing::GetFixturesPath(), kCjkFixture});
  auto typeface = skity::Typeface::MakeFromFile(path.c_str());
  ASSERT_NE(typeface, nullptr);
  // The existing subset contains U+662F. Require a real Han glyph, not tofu.
  ASSERT_NE(typeface->UnicharToGlyph(0x662F), 0u);
  fonts_ = CreateFixtureFontCollection(kCjkFixture, kCjkFamily);
  ASSERT_NE(fonts_, nullptr);
  style_.font_family = kCjkFamily;
  style_.font_weight = FontWeight::w400;
  for (auto word_break :
       {WordBreak::kNormal, WordBreak::kBreakAll, WordBreak::kKeepAll}) {
    SCOPED_TRACE(static_cast<int>(word_break));
    word_break_ = word_break;
    ExpectShrinkingPreservesGeometry(u"\u662f\u662f\u662f");
  }
}

TEST_F(ParagraphTTTextReuseTest, WidthBoundaryAndInvalidValues) {
  auto paragraph = Build(u"Text");
  EXPECT_STREQ(Reject(*paragraph, 100), "no_layout");
  paragraph->Layout(1000);
  const double width = paragraph->GetMaxIntrinsicWidth();
  ASSERT_GT(width, 0);
  EXPECT_EQ(Reject(*paragraph, width), nullptr);
  EXPECT_STREQ(Reject(*paragraph, std::nextafter(width, 0.0)),
               "target_too_narrow");
  for (double invalid : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
    EXPECT_STREQ(Reject(*paragraph, invalid), "invalid_target_width");
  }
}

TEST_F(ParagraphTTTextReuseTest, RejectsWidthDependentGeometry) {
  for (auto alignment :
       {TextAlign::right, TextAlign::center, TextAlign::justify}) {
    style_.text_align = alignment;
    auto paragraph = Build(u"Text");
    paragraph->Layout(1000);
    EXPECT_STREQ(Reject(*paragraph, 1000), "non_left_alignment");
  }
  style_.text_align = TextAlign::left;
  auto multiline = Build(u"first\nsecond");
  multiline->Layout(1000);
  EXPECT_STREQ(Reject(*multiline, 1000), "not_single_line");

  for (const auto* text : {u"Text ", u"Text\u3000"}) {
    auto paragraph = Build(text);
    paragraph->Layout(1000);
    EXPECT_STREQ(Reject(*paragraph, 1000), "trailing_space_or_control");
  }

  ParagraphBuilderTTText builder(style_, fonts_);
  builder.AddText(u"Text");
  PlaceholderRun placeholder(10, 10, PlaceholderAlignment::kBaseline,
                             TextBaseline::kAlphabetic, 0);
  builder.AddPlaceholder(placeholder);
  auto with_placeholder = builder.Build();
  with_placeholder->Layout(1000);
  EXPECT_STREQ(Reject(*with_placeholder, 1000), "has_placeholder");
}
#endif

}  // namespace txt

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
