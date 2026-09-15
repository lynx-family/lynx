// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_segment.h"

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

TEST(DisplayListSegmentTest, RestoreOnlyActiveAncestorState) {
  RoundedRectangle clip;
  clip.SetWidth(40.f);
  clip.SetHeight(30.f);
  clip.SetRadiusXTopLeft(5.f);
  clip.SetRadiusYTopLeft(5.f);
  std::vector<RoundedRectangle> boxes;
  DisplayListBuilder builder;
  builder.Begin(1, PlatformRendererType::kView, 100, 200, 100, 100)
      .ClipRect(clip)
      .Begin(2, PlatformRendererType::kView, 10, 20, 50, 50)
      .ClipRect(clip)
      .DrawView(3, 10, 20)
      .Fill(0xff00ff00)
      .End()
      .Begin(4, PlatformRendererType::kView, 30, 40, 50, 50)
      .DrawView(5, 30, 40)
      .End()
      .End();
  const auto segments = SegmentDisplayList(builder.Build(), boxes);
  ASSERT_EQ(segments.size(), 3u);
  EXPECT_EQ(segments[0].end_item_index, 4u);
  EXPECT_EQ(segments[1].start_item_index, 5u);
  EXPECT_EQ(segments[1].end_item_index, 8u);
  EXPECT_EQ(segments[2].start_item_index, 9u);
  const auto& first_state = segments[1].state_item_indices;
  EXPECT_EQ((std::vector<size_t>{0, 1, 2, 3}),
            std::vector<size_t>(first_state.begin(), first_state.end()));
  const auto& second_state = segments[2].state_item_indices;
  EXPECT_EQ((std::vector<size_t>{0, 1, 7}),
            std::vector<size_t>(second_state.begin(), second_state.end()));
  EXPECT_FALSE(segments[0].has_drawing);
  EXPECT_TRUE(segments[1].has_drawing);
  // A tail containing only End operations needs no drawing node.
  EXPECT_FALSE(segments[2].has_drawing);
}

TEST(DisplayListSegmentTest, SplitNativeViewsAndCollectBoxes) {
  RoundedRectangle rounded;
  rounded.SetX(1.5f);
  rounded.SetY(2.5f);
  rounded.SetWidth(30.f);
  rounded.SetHeight(40.f);
  rounded.SetRadiusXTopLeft(1.f);
  rounded.SetRadiusYTopLeft(2.f);
  RoundedRectangle plain;
  plain.SetWidth(60.f);
  plain.SetHeight(70.f);
  DisplayListBuilder builder;
  int32_t rounded_index, plain_index;
  builder.RecordBoxModel(rounded, rounded_index)
      .DrawView(3, 10, 20)
      .DrawView(2, 30, 40)
      .RecordBoxModel(plain, plain_index)
      .DrawView(4, 0, 0);
  std::vector<RoundedRectangle> boxes(1);
  auto segments = SegmentDisplayList(builder.Build(), boxes);
  ASSERT_EQ(segments.size(), 4u);
  EXPECT_EQ(segments[1].preceding_view_id, 3);
  EXPECT_EQ(segments[2].preceding_view_id, 2);
  EXPECT_EQ(segments[3].preceding_view_id, 4);
  EXPECT_TRUE(segments[1].IsEmpty());
  EXPECT_TRUE(segments[3].IsEmpty());
  EXPECT_EQ(segments[2].ItemCount(), 1u);
  ASSERT_EQ(boxes.size(), 2u);
  EXPECT_EQ(rounded_index, 0);
  EXPECT_EQ(plain_index, 1);
  EXPECT_EQ(boxes[0].GetRect(), rounded.GetRect());
  EXPECT_FLOAT_EQ(boxes[0].GetRadiusXTopLeft(), 1.f);
  EXPECT_FLOAT_EQ(boxes[0].GetRadiusYTopLeft(), 2.f);
  EXPECT_EQ(boxes[1].GetRect(), plain.GetRect());
  EXPECT_FALSE(boxes[1].HasRadius());
  EXPECT_FALSE(segments[0].has_drawing);
  EXPECT_FALSE(segments[2].has_drawing);
  DisplayListBuilder empty;
  EXPECT_TRUE(SegmentDisplayList(empty.Build(), boxes).empty());
  EXPECT_TRUE(boxes.empty());
}

}  // namespace tasm
}  // namespace lynx
