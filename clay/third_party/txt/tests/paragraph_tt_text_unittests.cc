// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#include "clay/third_party/txt/src/tttext/tttext_index_mapper.h"

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

}  // namespace txt

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
