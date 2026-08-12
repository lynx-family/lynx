// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_segment.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

DisplayListItem MakeItem(DisplayListOpType type) {
  DisplayListItem item{};
  item.type = type;
  return item;
}

DisplayListItem MakeDrawViewItem(int32_t view_id) {
  auto item = MakeItem(DisplayListOpType::kDrawView);
  item.payload.draw_view.view_id = view_id;
  return item;
}

}  // namespace

TEST(DisplayListSegmentTest, EmptyListHasNoSegments) {
  DisplayList display_list;
  EXPECT_TRUE(SegmentDisplayList(display_list).empty());
}

TEST(DisplayListSegmentTest, ListWithoutViewHasOneSegment) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeItem(DisplayListOpType::kFill));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto segments = SegmentDisplayList(display_list);
  ASSERT_EQ(segments.size(), 1U);
  EXPECT_EQ(segments[0].start_item_index, 0U);
  EXPECT_EQ(segments[0].end_item_index, 3U);
  EXPECT_EQ(segments[0].preceding_view_id, -1);
}

TEST(DisplayListSegmentTest, SplitsBeforeAndAfterViews) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeDrawViewItem(11));
  display_list.AppendItem(MakeItem(DisplayListOpType::kFill));
  display_list.AppendItem(MakeDrawViewItem(22));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto segments = SegmentDisplayList(display_list);
  ASSERT_EQ(segments.size(), 3U);
  EXPECT_EQ(segments[0].start_item_index, 0U);
  EXPECT_EQ(segments[0].end_item_index, 1U);
  EXPECT_EQ(segments[0].preceding_view_id, -1);
  EXPECT_EQ(segments[1].start_item_index, 2U);
  EXPECT_EQ(segments[1].end_item_index, 3U);
  EXPECT_EQ(segments[1].preceding_view_id, 11);
  EXPECT_EQ(segments[2].start_item_index, 4U);
  EXPECT_EQ(segments[2].end_item_index, 5U);
  EXPECT_EQ(segments[2].preceding_view_id, 22);
}

TEST(DisplayListSegmentTest, PreservesEmptySegmentsForConsecutiveViews) {
  DisplayList display_list;
  display_list.AppendItem(MakeDrawViewItem(11));
  display_list.AppendItem(MakeDrawViewItem(22));

  const auto segments = SegmentDisplayList(display_list);
  ASSERT_EQ(segments.size(), 3U);
  EXPECT_TRUE(segments[0].IsEmpty());
  EXPECT_TRUE(segments[1].IsEmpty());
  EXPECT_TRUE(segments[2].IsEmpty());
  EXPECT_EQ(segments[1].preceding_view_id, 11);
  EXPECT_EQ(segments[2].preceding_view_id, 22);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
