// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#include "clay/third_party/txt/src/tttext/tttext_index_mapper.h"
#include "clay/third_party/txt/src/txt/font_collection_skity.h"

#include <textra/paragraph.h>
#include <textra/style.h>
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace txt {

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

TEST(ParagraphTTTextTest, HitTestsLeadingInsideTheOwningLineBox) {
  auto font_collection = std::make_shared<FontCollection>();
  font_collection->SetupDefaultFontManager(0);
  tttext::ParagraphStyle paragraph_style;
  paragraph_style.SetLineHeightOverride(true);
  paragraph_style.SetLineHeightInPxExact(54.f);
  ParagraphTTText paragraph(font_collection, paragraph_style);
  tttext::Style style;
  style.SetTextSize(18.f);

  paragraph.AddTextRun(
      style, u"alpha beta gamma delta epsilon zeta eta theta iota kappa");
  paragraph.Layout(180.f);

  const auto& metrics = paragraph.GetLineMetrics();
  ASSERT_GT(metrics.size(), 1u);
  const auto first_line_tight = paragraph.GetRectsForRange(
      metrics[0].start_index, metrics[0].end_index,
      Paragraph::RectHeightStyle::kTight, Paragraph::RectWidthStyle::kTight);
  const auto first_line_box = paragraph.GetRectsForRange(
      metrics[0].start_index, metrics[0].end_index,
      Paragraph::RectHeightStyle::kLineBox, Paragraph::RectWidthStyle::kTight);
  ASSERT_EQ(first_line_tight.size(), 1u);
  ASSERT_EQ(first_line_box.size(), 1u);
  const auto line_box = metrics[0].GetLineBox();
  const float line_top = line_box.Top();
  const float line_bottom = line_box.Bottom();
  EXPECT_NEAR(first_line_box[0].rect.Top(), line_top, 0.01f);
  EXPECT_NEAR(first_line_box[0].rect.Bottom(), line_bottom, 0.01f);
  EXPECT_TRUE(metrics[0].ContainsInLineBox(metrics[0].left, line_top));
  ASSERT_LT(first_line_tight[0].rect.Bottom(), line_bottom);
  const float leading_y =
      (first_line_tight[0].rect.Bottom() + line_bottom) / 2.f;

  const auto left = paragraph.GetGlyphPositionAtCoordinate(
      metrics[0].width * 0.25f, leading_y);
  const auto right = paragraph.GetGlyphPositionAtCoordinate(
      metrics[0].width * 0.75f, leading_y);

  EXPECT_LT(left.position, right.position);
  EXPECT_LE(right.position, metrics[0].end_index);
}

TEST(ParagraphTTTextTest, KeepsLineBoxStableAcrossBaselineOffsets) {
  auto font_collection = std::make_shared<FontCollection>();
  font_collection->SetupDefaultFontManager(0);
  tttext::ParagraphStyle paragraph_style;
  paragraph_style.SetLineHeightOverride(true);
  paragraph_style.SetLineHeightInPxExact(64.f);
  ParagraphTTText paragraph(font_collection, paragraph_style);
  tttext::Style normal;
  normal.SetTextSize(18.f);
  tttext::Style raised = normal;
  raised.SetBaselineOffset(20.f);
  tttext::Style lowered = normal;
  lowered.SetBaselineOffset(-20.f);

  paragraph.AddTextRun(normal, u"before ");
  paragraph.AddTextRun(raised, u"UP");
  paragraph.AddTextRun(normal, u" middle ");
  paragraph.AddTextRun(lowered, u"DOWN");
  paragraph.AddTextRun(normal, u" after");
  paragraph.Layout(1000.f);

  const auto normal_line_box =
      paragraph.GetRectsForRange(0, 6, Paragraph::RectHeightStyle::kLineBox,
                                 Paragraph::RectWidthStyle::kTight);
  const auto raised_line_box =
      paragraph.GetRectsForRange(7, 9, Paragraph::RectHeightStyle::kLineBox,
                                 Paragraph::RectWidthStyle::kTight);
  const auto lowered_line_box =
      paragraph.GetRectsForRange(17, 21, Paragraph::RectHeightStyle::kLineBox,
                                 Paragraph::RectWidthStyle::kTight);
  const auto whole_line_box =
      paragraph.GetRectsForRange(0, 27, Paragraph::RectHeightStyle::kLineBox,
                                 Paragraph::RectWidthStyle::kTight);

  const auto& metrics = paragraph.GetLineMetrics();
  ASSERT_EQ(metrics.size(), 1u);
  ASSERT_EQ(normal_line_box.size(), 1u);
  ASSERT_EQ(raised_line_box.size(), 1u);
  ASSERT_EQ(lowered_line_box.size(), 1u);
  ASSERT_EQ(whole_line_box.size(), 1u);
  const auto expected_line_box = metrics.front().GetLineBox();
  for (const auto* boxes : {&normal_line_box, &raised_line_box,
                            &lowered_line_box, &whole_line_box}) {
    EXPECT_NEAR(boxes->front().rect.Top(), expected_line_box.Top(), 0.01f);
    EXPECT_NEAR(boxes->front().rect.Bottom(), expected_line_box.Bottom(),
                0.01f);
  }
}

}  // namespace txt

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
