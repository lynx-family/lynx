// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_segment.h"

#include <algorithm>

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

DisplayListItem MakeDrawViewItem(int32_t view_id, float offset_x = 0.f,
                                 float offset_y = 0.f) {
  auto item = MakeItem(DisplayListOpType::kDrawView);
  item.payload.draw_view.view_id = view_id;
  item.payload.draw_view.offset_x = offset_x;
  item.payload.draw_view.offset_y = offset_y;
  return item;
}

base::Vector<size_t> CollectStateItems(const DisplayListSegmentResult& result,
                                       size_t state_index) {
  base::Vector<size_t> state_items;
  while (state_index != kInvalidDisplayListIndex) {
    const auto& state = result.states[state_index];
    state_items.emplace_back(state.item_index);
    state_index = state.parent_state_index;
  }
  std::reverse(state_items.begin(), state_items.end());
  return state_items;
}

}  // namespace

TEST(DisplayListSegmentTest, EmptyListHasNoSegments) {
  DisplayList display_list;
  const auto result = SegmentDisplayList(display_list);
  EXPECT_TRUE(result.segments.empty());
  EXPECT_TRUE(result.states.empty());
}

TEST(DisplayListSegmentTest, ListWithoutViewHasHostSegment) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeItem(DisplayListOpType::kClipRect));
  display_list.AppendItem(MakeItem(DisplayListOpType::kFill));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.segments.size(), 1U);
  EXPECT_TRUE(result.states.empty());
  EXPECT_EQ(result.segments[0].start_item_index, 0U);
  EXPECT_EQ(result.segments[0].end_item_index, 4U);
  EXPECT_EQ(result.segments[0].preceding_view_id, -1);
  EXPECT_TRUE(result.segments[0].has_drawable_content);
}

TEST(DisplayListSegmentTest, KeepsViewDataOnFollowingSegment) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kFill));
  display_list.AppendItem(MakeDrawViewItem(11, 10.f, 20.f));
  display_list.AppendItem(MakeItem(DisplayListOpType::kBorder));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.segments.size(), 2U);
  EXPECT_EQ(result.segments[0].preceding_view_id, -1);
  EXPECT_TRUE(result.segments[0].has_drawable_content);
  EXPECT_EQ(result.segments[1].preceding_view_id, 11);
  EXPECT_FLOAT_EQ(result.segments[1].preceding_view_offset_x, 10.f);
  EXPECT_FLOAT_EQ(result.segments[1].preceding_view_offset_y, 20.f);
  EXPECT_TRUE(result.segments[1].has_drawable_content);
}

TEST(DisplayListSegmentTest, ConsecutiveViewsKeepEmptyFollowingSegments) {
  DisplayList display_list;
  display_list.AppendItem(MakeDrawViewItem(11, 1.f, 2.f));
  display_list.AppendItem(MakeDrawViewItem(22, 3.f, 4.f));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.segments.size(), 3U);
  EXPECT_FALSE(result.segments[0].has_drawable_content);
  EXPECT_EQ(result.segments[1].preceding_view_id, 11);
  EXPECT_FALSE(result.segments[1].has_drawable_content);
  EXPECT_EQ(result.segments[2].preceding_view_id, 22);
  EXPECT_FALSE(result.segments[2].has_drawable_content);
}

TEST(DisplayListSegmentTest, StructuralTailIsNotDrawable) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeDrawViewItem(11));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.segments.size(), 2U);
  EXPECT_FALSE(result.segments[0].has_drawable_content);
  EXPECT_FALSE(result.segments[1].has_drawable_content);
}

TEST(DisplayListSegmentTest, SharesActiveBeginAndClipStateAfterView) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeItem(DisplayListOpType::kClipRect));
  display_list.AppendItem(MakeDrawViewItem(11));
  display_list.AppendItem(MakeItem(DisplayListOpType::kFill));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.states.size(), 3U);
  ASSERT_EQ(result.segments.size(), 2U);
  EXPECT_EQ(CollectStateItems(result, result.segments[1].initial_state_index),
            (base::Vector<size_t>{0U, 1U, 2U}));
}

TEST(DisplayListSegmentTest, ClosedChildStateIsNotReplayed) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeItem(DisplayListOpType::kClipRect));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));
  display_list.AppendItem(MakeDrawViewItem(11));
  display_list.AppendItem(MakeItem(DisplayListOpType::kFill));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.segments.size(), 2U);
  EXPECT_EQ(CollectStateItems(result, result.segments[1].initial_state_index),
            (base::Vector<size_t>{0U}));
}

TEST(DisplayListSegmentTest, SegmentsInSameScopeSharePersistentState) {
  DisplayList display_list;
  display_list.AppendItem(MakeItem(DisplayListOpType::kBegin));
  display_list.AppendItem(MakeDrawViewItem(11));
  display_list.AppendItem(MakeDrawViewItem(22));
  display_list.AppendItem(MakeItem(DisplayListOpType::kEnd));

  const auto result = SegmentDisplayList(display_list);
  ASSERT_EQ(result.states.size(), 1U);
  ASSERT_EQ(result.segments.size(), 3U);
  EXPECT_EQ(result.segments[1].initial_state_index,
            result.segments[2].initial_state_index);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
