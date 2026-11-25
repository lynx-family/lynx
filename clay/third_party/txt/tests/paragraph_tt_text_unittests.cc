// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_builder_tt_text.h"
#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#include "clay/third_party/txt/src/tttext/tttext_index_mapper.h"

#if defined(ENABLE_SKITY)
#include <cmath>
#include <limits>
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

TEST(ParagraphTTTextTest, GraphemeBoundariesUseUTF16Positions) {
  tttext::ParagraphStyle paragraph_style;
#if defined(ENABLE_SKITY)
  ParagraphTTText paragraph(nullptr, paragraph_style, nullptr);
#else
  ParagraphTTText paragraph(nullptr, paragraph_style);
#endif
  tttext::Style style;
  paragraph.AddTextRun(style,
                       u"A\U0001F44D\U0001F3FB"
                       u"e\u0301B");

  EXPECT_EQ(paragraph.GetTextSize(), 8u);
  for (size_t offset : {1u, 2u, 3u, 4u}) {
    const auto boundary = paragraph.GetGraphemeBoundary(offset);
    EXPECT_EQ(boundary.start, 1u);
    EXPECT_EQ(boundary.end, 5u);
  }
  for (size_t offset : {5u, 6u}) {
    const auto boundary = paragraph.GetGraphemeBoundary(offset);
    EXPECT_EQ(boundary.start, 5u);
    EXPECT_EQ(boundary.end, 7u);
  }
}

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

std::optional<tttext::FontInfo> GetFixtureFontInfo(
    const std::shared_ptr<FontCollection>& font_collection,
    const tttext::Style& style) {
  auto tt_font_collection = font_collection->GetIFontCollection(nullptr);
  auto typefaces = tt_font_collection.findTypefaces(style.GetFontDescriptor());
  if (typefaces.empty() || typefaces.front() == nullptr) {
    return std::nullopt;
  }
  return typefaces.front()->GetFontInfo(style.GetTextSize());
}

tttext::Style MakeFixtureStyle(float text_size) {
  tttext::Style style;
  tttext::FontDescriptor font_descriptor;
  font_descriptor.font_family_list_ = {kFixtureFontFamily};
  font_descriptor.font_style_ = tttext::FontStyle::Bold();
  style.SetFontDescriptor(font_descriptor);
  style.SetTextSize(text_size);
  return style;
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
#if defined(ENABLE_SKITY)
  ParagraphTTText paragraph(nullptr, paragraph_style, nullptr);
#else
  ParagraphTTText paragraph(nullptr, paragraph_style);
#endif
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
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
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

TEST(ParagraphTTTextTest, NormalAndTightRectsShareParagraphCoordinates) {
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  constexpr float kFontSize = 40.f;
  tttext::Style style;
  tttext::FontDescriptor font_descriptor;
  font_descriptor.font_family_list_ = {kFixtureFontFamily};
  font_descriptor.font_style_ = tttext::FontStyle::Bold();
  style.SetFontDescriptor(font_descriptor);
  style.SetTextSize(kFontSize);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
  paragraph.AddTextRun(style, std::u16string(u"Hello"));
  paragraph.Layout(1024);

  ASSERT_EQ(paragraph.region_->GetLineCount(), 1u);
  auto* line = paragraph.region_->GetLine(0);
  const uint32_t start = line->GetStartCharPos();
  const uint32_t end = line->GetEndCharPos();
  const float baseline = line->GetLineBaseLine();

  float normal[4] = {0};
  line->GetBoundingRectByCharRange(normal, start, end);
  float tight[4] = {0};
  line->GetTightBoundingRectByCharRange(tight, start, end);

  const auto normal_boxes = paragraph.GetRectsForRange(
      start, end, Paragraph::RectHeightStyle::kMax,
      Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(normal_boxes.size(), 1u);
  EXPECT_FLOAT_EQ(normal_boxes[0].rect.Left(), normal[0]);
  EXPECT_FLOAT_EQ(normal_boxes[0].rect.Top(), normal[1]);
  EXPECT_FLOAT_EQ(normal_boxes[0].rect.Right(), normal[0] + normal[2]);
  EXPECT_FLOAT_EQ(normal_boxes[0].rect.Bottom(), normal[1] + normal[3]);

  const auto tight_boxes = paragraph.GetRectsForRange(
      start, end, Paragraph::RectHeightStyle::kTight,
      Paragraph::RectWidthStyle::kTight);
  ASSERT_EQ(tight_boxes.size(), 1u);
  EXPECT_FLOAT_EQ(tight_boxes[0].rect.Left(), tight[0]);
  EXPECT_FLOAT_EQ(tight_boxes[0].rect.Right(), tight[2]);
  EXPECT_FLOAT_EQ(tight_boxes[0].rect.Top(), baseline + tight[1]);
  EXPECT_FLOAT_EQ(tight_boxes[0].rect.Bottom(), baseline + tight[3]);

  // Tight glyph bounds stay inside the normal line-metric bounds.
  EXPECT_GE(tight_boxes[0].rect.Top(), normal_boxes[0].rect.Top() - 0.01f);
  EXPECT_LE(tight_boxes[0].rect.Bottom(),
            normal_boxes[0].rect.Bottom() + 0.01f);
  EXPECT_GE(tight_boxes[0].rect.Left(), normal_boxes[0].rect.Left() - 0.01f);
  EXPECT_LE(tight_boxes[0].rect.Right(), normal_boxes[0].rect.Right() + 0.01f);
}

// Multi-line range: one box per overlapping line, all Y coordinates are
// paragraph-absolute (line baseline based), and the second line sits below
// the first.
TEST(ParagraphTTTextTest, MultiLineRangeRectsShareParagraphCoordinates) {
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
  auto style28 = MakeFixtureStyle(28.f);
  paragraph.AddTextRun(style28, std::u16string(u"First"));
  paragraph.AddTextRun(style28, std::u16string(u"\nSecond"));
  paragraph.Layout(300.f);

  const auto& metrics = paragraph.GetLineMetrics();
  ASSERT_EQ(metrics.size(), 2u);

  const size_t total = paragraph.GetTextSize();
  const auto normal = paragraph.GetRectsForRange(
      0, total, Paragraph::RectHeightStyle::kMax,
      Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(normal.size(), 2u);
  const auto tight = paragraph.GetRectsForRange(
      0, total, Paragraph::RectHeightStyle::kTight,
      Paragraph::RectWidthStyle::kTight);
  ASSERT_EQ(tight.size(), 2u);

  for (size_t k = 0; k < 2u; ++k) {
    // Normal line bounds are paragraph-absolute: top = baseline - ascent,
    // bottom = baseline + descent.
    EXPECT_NEAR(normal[k].rect.Top(),
                static_cast<float>(metrics[k].baseline - metrics[k].ascent),
                0.5f);
    EXPECT_NEAR(normal[k].rect.Bottom(),
                static_cast<float>(metrics[k].baseline + metrics[k].descent),
                0.5f);
    EXPECT_GE(tight[k].rect.Top(), normal[k].rect.Top() - 0.01f);
    EXPECT_LE(tight[k].rect.Bottom(), normal[k].rect.Bottom() + 0.01f);
  }
  EXPECT_GE(normal[1].rect.Top(), normal[0].rect.Bottom() - 0.01f);
}

// Mixed run sizes on one line still return one line box per style; tight
// glyph bounds stay inside the normal line-metric bounds.
TEST(ParagraphTTTextTest, MixedRunSizesReturnPerRunBounds) {
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
  auto style20 = MakeFixtureStyle(20.f);
  auto style40 = MakeFixtureStyle(40.f);
  paragraph.AddTextRun(style20, std::u16string(u"Hi"));
  paragraph.AddTextRun(style40, std::u16string(u"World"));
  paragraph.Layout(1024);

  const size_t total = paragraph.GetTextSize();
  ASSERT_EQ(total, 7u);

  const auto normal = paragraph.GetRectsForRange(
      0, total, Paragraph::RectHeightStyle::kMax,
      Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(normal.size(), 1u);
  const auto tight = paragraph.GetRectsForRange(
      0, total, Paragraph::RectHeightStyle::kTight,
      Paragraph::RectWidthStyle::kTight);
  ASSERT_EQ(tight.size(), 1u);
  EXPECT_GT(normal[0].rect.Bottom(), normal[0].rect.Top());
  EXPECT_GE(tight[0].rect.Top(), normal[0].rect.Top() - 0.01f);
  EXPECT_LE(tight[0].rect.Bottom(), normal[0].rect.Bottom() + 0.01f);

  // Second (larger) run range returns a valid, non-empty box.
  const auto second_run = paragraph.GetRectsForRange(
      2, total, Paragraph::RectHeightStyle::kMax,
      Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(second_run.size(), 1u);
  EXPECT_GT(second_run[0].rect.Bottom(), second_run[0].rect.Top());
}

// Limited-support contract: the pinned dependency has one combined tight
// API, so a hybrid tight/normal style falls back to normal line-metric
// bounds instead of mixing a tight axis with a normal axis.
TEST(ParagraphTTTextTest, HybridTightStylesFallBackToNormalBounds) {
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
  auto style40 = MakeFixtureStyle(40.f);
  paragraph.AddTextRun(style40, std::u16string(u"Hello"));
  paragraph.Layout(1024);

  const auto normal = paragraph.GetRectsForRange(
      0, 5, Paragraph::RectHeightStyle::kMax, Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(normal.size(), 1u);

  const auto tight_height_only = paragraph.GetRectsForRange(
      0, 5, Paragraph::RectHeightStyle::kTight,
      Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(tight_height_only.size(), 1u);
  const auto tight_width_only = paragraph.GetRectsForRange(
      0, 5, Paragraph::RectHeightStyle::kMax,
      Paragraph::RectWidthStyle::kTight);
  ASSERT_EQ(tight_width_only.size(), 1u);

  for (const auto* hybrid : {&tight_height_only[0], &tight_width_only[0]}) {
    EXPECT_FLOAT_EQ(hybrid->rect.Left(), normal[0].rect.Left());
    EXPECT_FLOAT_EQ(hybrid->rect.Top(), normal[0].rect.Top());
    EXPECT_FLOAT_EQ(hybrid->rect.Right(), normal[0].rect.Right());
    EXPECT_FLOAT_EQ(hybrid->rect.Bottom(), normal[0].rect.Bottom());
  }

  // The combined tight path still produces a no-taller glyph box when both
  // axes request tight bounds.
  const auto tight = paragraph.GetRectsForRange(
      0, 5, Paragraph::RectHeightStyle::kTight,
      Paragraph::RectWidthStyle::kTight);
  ASSERT_EQ(tight.size(), 1u);
  EXPECT_LE(tight[0].rect.Bottom() - tight[0].rect.Top(),
            normal[0].rect.Bottom() - normal[0].rect.Top() + 0.02f);
}

// A range spanning an inline placeholder is reported through normal line
// bounds; the placeholder itself is still returned by GetRectsForPlaceholders.
TEST(ParagraphTTTextTest, RangeAcrossPlaceholderUsesLineNormalBounds) {
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
  auto style = MakeFixtureStyle(40.f);
  paragraph.AddTextRun(style, std::u16string(u"A"));
  PlaceholderRun placeholder(20.f, 40.f, PlaceholderAlignment::kMiddle,
                             TextBaseline::kAlphabetic, 0.f);
  paragraph.AddPlaceholder(style, placeholder, false);
  paragraph.AddTextRun(style, std::u16string(u"B"));
  paragraph.Layout(100.f);

  EXPECT_EQ(paragraph.GetTextSize(), 3u);
  const auto placeholders = paragraph.GetRectsForPlaceholders();
  ASSERT_EQ(placeholders.size(), 1u);

  const auto normal = paragraph.GetRectsForRange(
      0, 3, Paragraph::RectHeightStyle::kMax, Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(normal.size(), 1u);
  EXPECT_GT(normal[0].rect.Bottom(), normal[0].rect.Top());
  EXPECT_LE(normal[0].rect.Top(), placeholders[0].rect.Top() + 0.01f);
  EXPECT_GE(normal[0].rect.Bottom(), placeholders[0].rect.Bottom() - 0.01f);
}

// Range rects and hit-testing share the same paragraph coordinate space: a
// point inside a range box maps back into that range.
TEST(ParagraphTTTextTest, HitTestCoordinateMatchesRangeRect) {
  auto font_collection = CreateFixtureFontCollection();
  ASSERT_NE(font_collection, nullptr);

  tttext::ParagraphStyle paragraph_style;
  ParagraphTTText paragraph(font_collection, paragraph_style, nullptr);
  auto style40 = MakeFixtureStyle(40.f);
  paragraph.AddTextRun(style40, std::u16string(u"Hello"));
  paragraph.Layout(1024);

  const auto boxes = paragraph.GetRectsForRange(
      0, 5, Paragraph::RectHeightStyle::kMax, Paragraph::RectWidthStyle::kMax);
  ASSERT_EQ(boxes.size(), 1u);
  const float cx = (boxes[0].rect.Left() + boxes[0].rect.Right()) / 2.f;
  const float cy = (boxes[0].rect.Top() + boxes[0].rect.Bottom()) / 2.f;
  const auto position = paragraph.GetGlyphPositionAtCoordinate(cx, cy);
  EXPECT_GE(position.position, 0u);
  EXPECT_LE(position.position, 5u);
}
#endif

TEST(ParagraphTTTextTest, DefaultPlaceholderUsesAlphabeticBaseline) {
  tttext::ParagraphStyle paragraph_style;
#if defined(ENABLE_SKITY)
  ParagraphTTText paragraph(nullptr, paragraph_style, nullptr);
#else
  ParagraphTTText paragraph(nullptr, paragraph_style);
#endif
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
  PlaceholderRun placeholder;
  placeholder.width = 10;
  placeholder.height = 10;
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
