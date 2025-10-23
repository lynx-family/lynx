// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <unordered_map>

#include "clay/flow/embedded_views.h"
#include "clay/flow/view_slicer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

namespace {
void AddSliceOfSize(
    std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>>& slices,
    int64_t id, skity::Rect rect) {
  slices[id] = std::make_unique<SkPictureEmbedderViewSlice>(rect);
  SkPaint paint;
  paint.setColor(SkColors::kBlack);
  slices[id]->canvas()->drawRect(clay::ConvertSkityRectToSkRect(rect), paint);
}
}  // namespace

TEST(ViewSlicerTest, CanSlicerNonOverlappingViews) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  std::vector<int64_t> composition_order = {1};
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices;
  AddSliceOfSize(slices, 1, skity::Rect::MakeLTRB(99, 99, 100, 100));

  std::unordered_map<int64_t, skity::Rect> view_rects = {
      {1, skity::Rect::MakeLTRB(50, 50, 60, 60)}};

  auto computed_overlays =
      SliceViews(canvas, composition_order, slices, view_rects);

  EXPECT_TRUE(computed_overlays.empty());
}

TEST(ViewSlicerTest, IgnoresFractionalOverlaps) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  std::vector<int64_t> composition_order = {1};
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices;
  AddSliceOfSize(slices, 1, skity::Rect::MakeLTRB(0, 0, 50.49, 50.49));

  std::unordered_map<int64_t, skity::Rect> view_rects = {
      {1, skity::Rect::MakeLTRB(50.5, 50.5, 100, 100)}};

  auto computed_overlays =
      SliceViews(canvas, composition_order, slices, view_rects);

  EXPECT_TRUE(computed_overlays.empty());
}

TEST(ViewSlicerTest, ComputesOverlapWith1PV) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  std::vector<int64_t> composition_order = {1};
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices;
  AddSliceOfSize(slices, 1, skity::Rect::MakeLTRB(0, 0, 50, 50));

  std::unordered_map<int64_t, skity::Rect> view_rects = {
      {1, skity::Rect::MakeLTRB(0, 0, 100, 100)}};

  auto computed_overlays =
      SliceViews(canvas, composition_order, slices, view_rects);

  EXPECT_EQ(computed_overlays.size(), 1u);
  auto overlay = computed_overlays.find(1);
  ASSERT_NE(overlay, computed_overlays.end());

  EXPECT_EQ(overlay->second, skity::Rect::MakeLTRB(0, 0, 50, 50));
}

TEST(ViewSlicerTest, ComputesOverlapWith2PV) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  std::vector<int64_t> composition_order = {1, 2};
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices;
  AddSliceOfSize(slices, 1, skity::Rect::MakeLTRB(0, 0, 50, 50));
  AddSliceOfSize(slices, 2, skity::Rect::MakeLTRB(50, 50, 100, 100));

  std::unordered_map<int64_t, skity::Rect> view_rects = {
      {1, skity::Rect::MakeLTRB(0, 0, 50, 50)},      //
      {2, skity::Rect::MakeLTRB(50, 50, 100, 100)},  //
  };

  auto computed_overlays =
      SliceViews(canvas, composition_order, slices, view_rects);

  EXPECT_EQ(computed_overlays.size(), 2u);

  auto overlay = computed_overlays.find(1);
  ASSERT_NE(overlay, computed_overlays.end());

  EXPECT_EQ(overlay->second, skity::Rect::MakeLTRB(0, 0, 50, 50));

  overlay = computed_overlays.find(2);
  ASSERT_NE(overlay, computed_overlays.end());
  EXPECT_EQ(overlay->second, skity::Rect::MakeLTRB(50, 50, 100, 100));
}

TEST(ViewSlicerTest, OverlappingTwoPVs) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  std::vector<int64_t> composition_order = {1, 2};
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices;
  // This embeded view overlaps both platform views:
  //
  //   [  A  [   ]]
  //   [_____[ C ]]
  //   [  B  [   ]]
  //   [          ]
  AddSliceOfSize(slices, 1, skity::Rect::MakeLTRB(0, 0, 0, 0));
  AddSliceOfSize(slices, 2, skity::Rect::MakeLTRB(0, 0, 100, 100));

  std::unordered_map<int64_t, skity::Rect> view_rects = {
      {1, skity::Rect::MakeLTRB(0, 0, 50, 50)},      //
      {2, skity::Rect::MakeLTRB(50, 50, 100, 100)},  //
  };

  auto computed_overlays =
      SliceViews(canvas, composition_order, slices, view_rects);

  EXPECT_EQ(computed_overlays.size(), 1u);

  auto overlay = computed_overlays.find(2);
  ASSERT_NE(overlay, computed_overlays.end());

  // We create a single overlay for both overlapping sections.
  EXPECT_EQ(overlay->second, skity::Rect::MakeLTRB(0, 0, 100, 100));
}

}  // namespace testing
}  // namespace clay
