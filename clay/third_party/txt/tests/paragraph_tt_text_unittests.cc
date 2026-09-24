// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"

#include <textra/paragraph.h>
#include <textra/style.h>

#include "clay/third_party/txt/src/txt/placeholder_run.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace txt {

TEST(ParagraphTTTextTest, ConvertsBetweenTTTextAndUTF16Positions) {
  TTTextIndexMapper mapper;
  mapper.AppendText(u"A\U0001F600");
  mapper.AppendText(u"B\U0001F601C");

  EXPECT_EQ(mapper.GetUTF16Size(), 7u);
  const size_t character_positions[] = {0, 1, 1, 2, 3, 3, 4, 5};
  const size_t range_ends[] = {0, 1, 2, 2, 3, 4, 4, 5};
  for (size_t offset = 0; offset <= mapper.GetUTF16Size(); ++offset) {
    EXPECT_EQ(mapper.ToTTTextPosition(offset), character_positions[offset]);
    EXPECT_EQ(mapper.ToTTTextRangeEnd(offset), range_ends[offset]);
  }
  const size_t utf16_positions[] = {0, 1, 3, 4, 6, 7};
  for (size_t position = 0; position < 6; ++position) {
    EXPECT_EQ(mapper.ToUTF16Position(position), utf16_positions[position]);
  }
}

TEST(ParagraphTTTextTest, KeepsIndexesInSyncAcrossTextRunsAndPlaceholders) {
  tttext::ParagraphStyle paragraph_style;
#if defined(ENABLE_SKITY)
  ParagraphTTText paragraph(nullptr, paragraph_style, nullptr);
#else
  ParagraphTTText paragraph(nullptr, paragraph_style);
#endif
  tttext::Style style;
  paragraph.AddTextRun(style, u"A\U0001F600");
  PlaceholderRun placeholder(10, 10, PlaceholderAlignment::kBaseline,
                             TextBaseline::kAlphabetic, 10);
  paragraph.AddPlaceholder(style, placeholder, false);
  paragraph.AddTextRun(style, std::string("\xF0\x9F\x98\x81"
                                          "B"));

  EXPECT_EQ(paragraph.GetTextSize(), 7u);
  const auto boundary = paragraph.GetGraphemeBoundary(5);
  EXPECT_EQ(boundary.start, 4u);
  EXPECT_EQ(boundary.end, 6u);
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
  paragraph.AddTextRun(style, std::string("\xF0\x9F\x98\x80\0B", 6));

  EXPECT_EQ(paragraph.GetTextSize(), 3u);
  const auto boundary = paragraph.GetGraphemeBoundary(2);
  EXPECT_EQ(boundary.start, 1u);
  EXPECT_EQ(boundary.end, 3u);
}

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

}  // namespace txt

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
